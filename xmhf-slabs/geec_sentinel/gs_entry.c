/*
 * @XMHF_LICENSE_HEADER_START@
 *
 * eXtensible, Modular Hypervisor Framework (XMHF)
 * Copyright (c) 2009-2012 Carnegie Mellon University
 * Copyright (c) 2010-2012 VDG Inc.
 * All Rights Reserved.
 *
 * Developed by: XMHF Team
 *               Carnegie Mellon University / CyLab
 *               VDG Inc.
 *               http://xmhf.org
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the names of Carnegie Mellon or VDG Inc, nor the names of
 * its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @XMHF_LICENSE_HEADER_END@
 */

/*
 * HIC trampoline and stubs
 *
 * author: amit vasudevan (amitvasudevan@acm.org)
 */

#include <xmhf.h>
#include <xmhf-debug.h>

#include <xmhfgeec.h>

#include <xc.h>
#include <geec_sentinel.h>




/////
// sentinel hub
/////

static inline void _geec_sentinel_checkandhalt_callcaps(u32 src_slabid, u32 dst_slabid, u32 dst_uapifn){
    //check call capabilities
    if( !(_xmhfhic_common_slab_info_table[dst_slabid].slab_callcaps & XMHFGEEC_SLAB_CALLCAP_MASK(src_slabid)) ){
        _XDPRINTF_("GEEC_SENTINEL: Halt!. callcap check failed for src(%u)-->dst(%u), dst caps=0x%x\n",
                   src_slabid, dst_slabid, _xmhfhic_common_slab_info_table[dst_slabid].slab_callcaps);
        HALT();
    }

    //check uapi capabilities
    if( _xmhfhic_common_slab_info_table[dst_slabid].slab_uapisupported){
        if(!(_xmhfhic_common_slab_info_table[src_slabid].slab_uapicaps[dst_slabid] & XMHFGEEC_SLAB_UAPICAP_MASK(dst_uapifn)))
        {
            _XDPRINTF_("GEEC_SENTINEL: Halt!. uapicap check failed for src(%u)-->dst(%u), dst_uapifn=%u, dst_uapimask=0x%08x\n",
                       src_slabid, dst_slabid, dst_uapifn,
                       (u32)_xmhfhic_common_slab_info_table[src_slabid].slab_uapicaps[dst_slabid]);
            HALT();
        }
    }
}



void geec_sentinel_main(slab_params_t *sp, void *caller_stack_frame){



    switch(sp->slab_ctype){
        case XMHFGEEC_SENTINEL_CALL_FROM_VfT_PROG:{

            switch (_xmhfhic_common_slab_info_table[sp->dst_slabid].slabtype){

                case XMHFGEEC_SLABTYPE_VfT_PROG:{
                    _geec_sentinel_checkandhalt_callcaps(sp->src_slabid, sp->dst_slabid, sp->dst_uapifn);
                    CASM_FUNCCALL(_geec_sentinel_xfer_vft_prog_to_vft_prog,
                                  _xmhfhic_common_slab_info_table[sp->dst_slabid].entrystub,
                                  caller_stack_frame);
                    _XDPRINTF_("GEEC_SENTINEL[ln:%u]: halting. should never be here!\n",
                               __LINE__);
                    HALT();

                }
                break;


                case XMHFGEEC_SLABTYPE_uVT_PROG:
                case XMHFGEEC_SLABTYPE_uVU_PROG:{
                    _geec_sentinel_checkandhalt_callcaps(sp->src_slabid, sp->dst_slabid, sp->dst_uapifn);
                    sp->slab_ctype = XMHFGEEC_SENTINEL_CALL_VfT_PROG_TO_uVT_uVU_PROG;
                    _geec_sentinel_transition_vft_prog_to_uvt_uvu_prog(sp, caller_stack_frame);
                    _XDPRINTF_("GEEC_SENTINEL[ln:%u]: halting. should never be here!\n",
                               __LINE__);
                    HALT();

                }
                break;


                case XMHFGEEC_SLABTYPE_uVT_PROG_GUEST:
                case XMHFGEEC_SLABTYPE_uVU_PROG_GUEST:
                case XMHFGEEC_SLABTYPE_uVU_PROG_RICHGUEST:{
                    u32 errorcode;
                    _geec_sentinel_checkandhalt_callcaps(sp->src_slabid, sp->dst_slabid, sp->dst_uapifn);
                    sp->slab_ctype = XMHFGEEC_SENTINEL_CALL_VfT_PROG_TO_uVT_uVU_PROG_GUEST;
                    CASM_FUNCCALL(xmhfhw_cpu_x86vmx_vmwrite,VMCS_CONTROL_VPID, sp->dst_slabid );
                    CASM_FUNCCALL(xmhfhw_cpu_x86vmx_vmwrite,VMCS_CONTROL_EPT_POINTER_FULL, (_xmhfhic_common_slab_info_table[sp->dst_slabid].mempgtbl_cr3  | 0x1E) );
                    CASM_FUNCCALL(xmhfhw_cpu_x86vmx_vmwrite,VMCS_CONTROL_EPT_POINTER_HIGH, 0);
                    CASM_FUNCCALL(xmhfhw_cpu_x86vmx_vmwrite,VMCS_CONTROL_IO_BITMAPA_ADDRESS_FULL, _xmhfhic_common_slab_info_table[sp->dst_slabid].iotbl_base);
                    CASM_FUNCCALL(xmhfhw_cpu_x86vmx_vmwrite,VMCS_CONTROL_IO_BITMAPA_ADDRESS_HIGH, 0);
                    CASM_FUNCCALL(xmhfhw_cpu_x86vmx_vmwrite,VMCS_CONTROL_IO_BITMAPB_ADDRESS_FULL, (_xmhfhic_common_slab_info_table[sp->dst_slabid].iotbl_base + PAGE_SIZE_4K));
                    CASM_FUNCCALL(xmhfhw_cpu_x86vmx_vmwrite,VMCS_CONTROL_IO_BITMAPB_ADDRESS_HIGH, 0);


                    CASM_FUNCCALL(xmhfhw_cpu_x86vmx_vmwrite,VMCS_GUEST_RSP, _xmhfhic_common_slab_info_table[sp->dst_slabid].slabtos[(u16)sp->cpuid]);
                    CASM_FUNCCALL(xmhfhw_cpu_x86vmx_vmwrite,VMCS_GUEST_RIP, _xmhfhic_common_slab_info_table[sp->dst_slabid].entrystub);

                    errorcode = CASM_FUNCCALL(_geec_sentinel_xfer_vft_prog_to_uvt_uvu_prog_guest, CASM_NOPARAM);

                    switch(errorcode){
                        case 0:	//no error code, VMCS pointer is invalid
                            _XDPRINTF_("GEEC_SENTINEL: VMLAUNCH error; VMCS pointer invalid?\n");
                            break;
                        case 1:{//error code available, so dump it
                            u32 code=xmhfhw_cpu_x86vmx_vmread(VMCS_INFO_VMINSTR_ERROR);
                            _XDPRINTF_("GEEC_SENTINEL: VMLAUNCH error; code=%x\n", code);
                            break;
                        }
                    }

                    HALT();

                }
                break;

                default:
                    _XDPRINTF_("GEEC_SENTINEL(ln:%u): Unrecognized transition. Halting!\n", __LINE__);
                    HALT();
            }


        }
        break;




        case XMHFGEEC_SENTINEL_RET_VfT_PROG_TO_uVT_uVU_PROG:{
                    _geec_sentinel_transition_ret_vft_prog_to_uvt_uvu_prog(sp, caller_stack_frame);
                    _XDPRINTF_("GEEC_SENTINEL[ln:%u]: halting. should never be here!\n",
                               __LINE__);
                    HALT();
        }
        break;





        case XMHFGEEC_SENTINEL_CALL_uVT_uVU_PROG_TO_VfT_PROG:{
            _geec_sentinel_checkandhalt_callcaps(sp->src_slabid, sp->dst_slabid, sp->dst_uapifn);
            _geec_sentinel_transition_call_uvt_uvu_prog_to_vft_prog(sp, caller_stack_frame);
            _XDPRINTF_("GEEC_SENTINEL[ln:%u]: halting. should never be here!\n", __LINE__);
            HALT();
        }
        break;




        case XMHFGEEC_SENTINEL_RET_uVT_uVU_PROG_TO_VfT_PROG:{
            _geec_sentinel_transition_ret_uvt_uvu_prog_to_vft_prog(sp, caller_stack_frame);
            _XDPRINTF_("GEEC_SENTINEL[ln:%u]: halting. should never be here!\n", __LINE__);
            HALT();
        }
        break;



        case XMHFGEEC_SENTINEL_CALL_EXCEPTION:{
            if(!(_xmhfhic_common_slab_info_table[sp->dst_slabid].slabtype == XMHFGEEC_SLABTYPE_VfT_PROG)){
                _XDPRINTF_("GEEC_SENTINEL(ln:%u): exception target slab not VfT_PROG. Halting!\n", __LINE__);
                HALT();
            }

            CASM_FUNCCALL(_geec_sentinel_xfer_exception_to_vft_prog,
              _xmhfhic_common_slab_info_table[sp->dst_slabid].entrystub,
              caller_stack_frame);
            _XDPRINTF_("GEEC_SENTINEL[ln:%u]: halting. should never be here!\n",
                       __LINE__);
            HALT();

        }
        break;






        case XMHFGEEC_SENTINEL_RET_EXCEPTION:{
            if(!
               (_xmhfhic_common_slab_info_table[sp->src_slabid].slabtype == XMHFGEEC_SLABTYPE_VfT_PROG &&
                sp->dst_slabid == XMHFGEEC_SLAB_GEEC_SENTINEL)){
                _XDPRINTF_("GEEC_SENTINEL(ln:%u): exception ret source slab not VfT_PROG_EXCEPTION. Halting!\n", __LINE__);
                HALT();
            }

            //_geec_sentinel_dump_exframe(sp->in_out_params);

            CASM_FUNCCALL(_geec_sentinel_xfer_ret_from_exception,
                sp->in_out_params);
            _XDPRINTF_("GEEC_SENTINEL[ln:%u]: halting. should never be here!\n",
                       __LINE__);
            HALT();

        }
        break;







        case XMHFGEEC_SENTINEL_CALL_INTERCEPT:{
            if(!(_xmhfhic_common_slab_info_table[sp->dst_slabid].slabtype == XMHFGEEC_SLABTYPE_VfT_PROG)){
                _XDPRINTF_("GEEC_SENTINEL(ln:%u): intercept target slab not VfT_PROG_INTERCEPT. Halting!\n", __LINE__);
                HALT();
            }

            CASM_FUNCCALL(_geec_sentinel_xfer_intercept_to_vft_prog,
              _xmhfhic_common_slab_info_table[sp->dst_slabid].entrystub,
              caller_stack_frame);
            _XDPRINTF_("GEEC_SENTINEL[ln:%u]: halting. should never be here!\n",
                       __LINE__);
            HALT();

        }
        break;






        case XMHFGEEC_SENTINEL_RET_INTERCEPT:{
            if(!
               (_xmhfhic_common_slab_info_table[sp->src_slabid].slabtype == XMHFGEEC_SLABTYPE_VfT_PROG &&
                (_xmhfhic_common_slab_info_table[sp->dst_slabid].slabtype == XMHFGEEC_SLABTYPE_uVT_PROG_GUEST ||
                 _xmhfhic_common_slab_info_table[sp->dst_slabid].slabtype == XMHFGEEC_SLABTYPE_uVU_PROG_GUEST ||
                 _xmhfhic_common_slab_info_table[sp->dst_slabid].slabtype == XMHFGEEC_SLABTYPE_uVU_PROG_RICHGUEST
                )
               )){
                _XDPRINTF_("GEEC_SENTINEL(ln:%u): intercept ret source slab not VfT_PROG_INTERCEPT. Halting!\n", __LINE__);
                HALT();
            }

            CASM_FUNCCALL(_geec_sentinel_xfer_ret_from_intercept, sp->in_out_params);
            _XDPRINTF_("GEEC_SENTINEL[ln:%u]: halting. should never be here!\n",
                       __LINE__);
            HALT();

        }
        break;






        default:
            _XDPRINTF_("GEEC_SENTINEL: unkown call type %x. Halting!\n", sp->slab_ctype);
            HALT();

    }

}



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

#include <xmhf.h>
#include <xmhf-debug.h>

#include <xmhfgeec.h>

#include <geec_sentinel.h>
#include <xc.h>
#include <uapi_gcpustate.h>
#include <uapi_slabmemacc.h>
#include <xg_richguest.h>
#include <xc_testslab.h>

#include <xc_init.h>

extern x_slab_info_t _x_xmhfhic_common_slab_info_table[XMHF_HIC_MAX_SLABS];


//////
//XMHF_SLABNEW(xcinit)

/*
 * slab code
 *
 * author: amit vasudevan (amitvasudevan@acm.org)
 */


__attribute__(( aligned(16) )) static u64 _xcguestslab_init_gdt[]  = {
	0x0000000000000000ULL,	//NULL descriptor
	0x00cf9b000000ffffULL,	//CPL-0 32-bit code descriptor (CS32)
	0x00cf93000000ffffULL,	//CPL-0 32-bit data descriptor (DS/SS/ES/FS/GS)
	0x0000000000000000ULL,	//NULL descriptor
};




void slab_main(slab_params_t *sp){

    bool isbsp = (sp->cpuid & 0x80000000UL) ? true : false;
    u64 inputval, outputval;
    static u64 cpucount=0;
    static u32 __xcinit_smplock = 1;

	_XDPRINTF_("%s[%u]: Got control: ESP=%08x\n", __func__, (u16)sp->cpuid, CASM_FUNCCALL(read_esp,CASM_NOPARAM));

    if(!isbsp){
        _XDPRINTF_("%s[%u]: AP Halting!\n", __func__, (u16)sp->cpuid);

        CASM_FUNCCALL(spin_lock,&__xcinit_smplock);
        cpucount++;
        CASM_FUNCCALL(spin_unlock,&__xcinit_smplock);

        HALT();
    }else{
        //BSP
        _XDPRINTF_("%s[%u]: BSP waiting to rally APs...\n",
                __func__, (u16)sp->cpuid);

        while(cpucount < (xcbootinfo->cpuinfo_numentries-1));

        _XDPRINTF_("%s[%u]: BSP, APs halted. Proceeding...\n",
                __func__, (u16)sp->cpuid);
    }



    // call test slab
    {
        slab_params_t spl;
        spl.src_slabid = XMHF_HYP_SLAB_XCINIT;
        spl.dst_slabid = XMHF_HYP_SLAB_XC_TESTSLAB;
        spl.cpuid = 0;
        spl.in_out_params[0] = 0xF00DDEAD;
        XMHF_SLAB_CALLNEW(&spl);
        _XDPRINTF_("XC_INIT[%u]: called test slab, return value=%x\n",
                   (u16)sp->cpuid, spl.in_out_params[1]);
    }



    {
        u32 guest_slab_header_paddr = _xmhfhic_common_slab_info_table[XMHF_GUEST_SLAB_XCGUESTSLAB].slab_physmem_extents[1].addr_start;
        u32 guest_slab_gdt_paddr = guest_slab_header_paddr + offsetof(guest_slab_header_t, gdt);
        u32 guest_slab_magic_paddr = guest_slab_header_paddr + offsetof(guest_slab_header_t, magic);
        u32 guest_slab_magic;


        //get and dump slab header magic
        {
            slab_params_t spl;
            //xmhf_hic_uapi_physmem_desc_t *pdesc = (xmhf_hic_uapi_physmem_desc_t *)&spl.in_out_params[2];
            xmhf_uapi_slabmemacc_params_t *smemaccp = (xmhf_uapi_slabmemacc_params_t *)spl.in_out_params;


            smemaccp->dst_slabid = XMHF_GUEST_SLAB_XCGUESTSLAB;
            smemaccp->addr_to = &guest_slab_magic;
            smemaccp->addr_from = guest_slab_magic_paddr;
            smemaccp->numbytes = sizeof(guest_slab_magic);

            //spl.in_out_params[0] = XMHF_HIC_UAPI_PHYSMEM;
            smemaccp->uapiphdr.uapifn = XMHF_HIC_UAPI_PHYSMEM_PEEK;
            spl.cpuid = sp->cpuid;
            spl.src_slabid = XMHF_HYP_SLAB_XCINIT;
            spl.dst_slabid = XMHF_HYP_SLAB_UAPI_SLABMEMACC;

            XMHF_SLAB_CALLNEW(&spl);
            _XDPRINTF_("%s[%u]: guest slab header at=%x\n", __func__, (u16)sp->cpuid, guest_slab_header_paddr);
            _XDPRINTF_("%s[%u]: guest slab header magic=%x\n", __func__, (u16)sp->cpuid, guest_slab_magic);
        }


        //initialize guest slab gdt
        {
            slab_params_t spl;
            //xmhf_hic_uapi_physmem_desc_t *pdesc = (xmhf_hic_uapi_physmem_desc_t *)&spl.in_out_params[2];
            xmhf_uapi_slabmemacc_params_t *smemaccp = (xmhf_uapi_slabmemacc_params_t *)spl.in_out_params;

            smemaccp->dst_slabid = XMHF_GUEST_SLAB_XCGUESTSLAB;
            smemaccp->addr_to = guest_slab_gdt_paddr;
            smemaccp->addr_from = &_xcguestslab_init_gdt;
            smemaccp->numbytes = sizeof(_xcguestslab_init_gdt);

            //spl.in_out_params[0] = XMHF_HIC_UAPI_PHYSMEM;
            smemaccp->uapiphdr.uapifn = XMHF_HIC_UAPI_PHYSMEM_POKE;
            spl.cpuid = sp->cpuid;
            spl.src_slabid = XMHF_HYP_SLAB_XCINIT;
            spl.dst_slabid = XMHF_HYP_SLAB_UAPI_SLABMEMACC;

            XMHF_SLAB_CALLNEW(&spl);
        }

        //setup guest slab VMCS GDT base and limit
        {
            slab_params_t spl;
            xmhf_uapi_gcpustate_vmrw_params_t *gcpustate_vmrwp =
                (xmhf_uapi_gcpustate_vmrw_params_t *)spl.in_out_params;

            spl.cpuid = sp->cpuid;
            spl.src_slabid = XMHF_HYP_SLAB_XCINIT;
            spl.dst_slabid = XMHF_HYP_SLAB_UAPI_GCPUSTATE;

            //spl.in_out_params[0] = XMHF_HIC_UAPI_CPUSTATE;
            gcpustate_vmrwp->uapiphdr.uapifn = XMHF_HIC_UAPI_CPUSTATE_VMWRITE;
            gcpustate_vmrwp->encoding = VMCS_GUEST_GDTR_BASE;
            gcpustate_vmrwp->value = guest_slab_gdt_paddr;

            XMHF_SLAB_CALLNEW(&spl);

            gcpustate_vmrwp->encoding = VMCS_GUEST_GDTR_LIMIT;
            gcpustate_vmrwp->value =  (sizeof(_xcguestslab_init_gdt)-1);

            XMHF_SLAB_CALLNEW(&spl);

        }


    }


/*    //debug
    _XDPRINTF_("Halting!\n");
    _XDPRINTF_("XMHF Tester Finished!\n");
    HALT();
*/

    //invoke hypapp initialization callbacks
    xc_hcbinvoke(XMHF_HYP_SLAB_XCINIT,
                 sp->cpuid, XC_HYPAPPCB_INITIALIZE, 0, XMHF_GUEST_SLAB_XCGUESTSLAB);


    //call guestslab
    {
        slab_params_t spl;

        memset(&spl, 0, sizeof(spl));
        spl.cpuid = sp->cpuid;
        spl.src_slabid = XMHF_HYP_SLAB_XCINIT;
        spl.dst_slabid = XMHF_GUEST_SLAB_XCGUESTSLAB;

        _XDPRINTF_("%s[%u]: Proceeding to call xcguestslab; ESP=%08x\n", __func__, (u16)sp->cpuid, CASM_FUNCCALL(read_esp,CASM_NOPARAM));

        XMHF_SLAB_CALLNEW(&spl);
    }


    _XDPRINTF_("%s[%u]: Should  never get here.Halting!\n", __func__, (u16)sp->cpuid);
    HALT();

    return;
}








#if 0


    _xcinit_dotests(cpuid);

    _XDPRINTF_("%s[%u]: Should  never get here.Halting!\n",
        __func__, (u32)cpuid);

    HALT();


#endif // 0

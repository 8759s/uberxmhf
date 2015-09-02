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
 * libxmhfhw CASM functions verification driver
 * author: amit vasudevan (amitvasudevan@acm.org)
*/


#include <xmhf.h>
#include <xmhf-hwm.h>
#include <xmhfgeec.h>
#include <xmhf-debug.h>

u32 cpuid = 0;	//BSP cpu

//////
// frama-c non-determinism functions
//////

u32 Frama_C_entropy_source;

//@ assigns Frama_C_entropy_source \from Frama_C_entropy_source;
void Frama_C_update_entropy(void);

u32 framac_nondetu32(void){
  Frama_C_update_entropy();
  return (u32)Frama_C_entropy_source;
}

u32 framac_nondetu32interval(u32 min, u32 max)
{
  u32 r,aux;
  Frama_C_update_entropy();
  aux = Frama_C_entropy_source;
  if ((aux>=min) && (aux <=max))
    r = aux;
  else
    r = min;
  return r;
}


//////
u32 saved_cpu_gprs_ebx=0;
u32 saved_cpu_gprs_esi=0;
u32 saved_cpu_gprs_edi=0;
u32 saved_cpu_gprs_ebp=0;

void cabi_establish(void){
	xmhfhwm_cpu_gprs_ebx = 5UL;
	xmhfhwm_cpu_gprs_esi = 6UL;
	xmhfhwm_cpu_gprs_edi = 7UL;
	saved_cpu_gprs_ebx = xmhfhwm_cpu_gprs_ebx;
	saved_cpu_gprs_esi = xmhfhwm_cpu_gprs_esi;
	saved_cpu_gprs_edi = xmhfhwm_cpu_gprs_edi;
}

void cabi_check(void){
	//@ assert saved_cpu_gprs_ebx == xmhfhwm_cpu_gprs_ebx;
	//@ assert saved_cpu_gprs_esi == xmhfhwm_cpu_gprs_esi;
	//@ assert saved_cpu_gprs_edi == xmhfhwm_cpu_gprs_edi;
}



u32 check_esp, check_eip = CASM_RET_EIP;


//////
#if defined (DRV_SLAB_ENTRYSTUB)

slab_params_t drv_slab_entrystub_sp;

void geec_sentinel_main(slab_params_t *sp,
	void *caller_stack_frame){
	//Pre:
	//	[esp] = CASM_RET_EIP
	//  	[esp+4] = slab_params_t *sp
	//	[esp+8] = &[esp+8]
	//	[esp+12] = ebx
	//	[esp+16] = esi
	//	[esp+20] = edi
	//	[esp+24] = ebp
	//	[esp+28] = return address to resume the (verified,privileged) caller
	//	[esp+32] = slab_params_t *sp
	//@assert sp->src_slabid == 6UL;
	//@assert xmhfhwm_cpu_gprs_esp == (check_esp - (9 * sizeof(u32)));
	//@assert *((u32 *)xmhfhwm_cpu_gprs_esp) == CASM_RET_EIP;
	//@assert *((u32 *)((u32)xmhfhwm_cpu_gprs_esp+4)) == (unsigned int)&drv_slab_entrystub_sp;
	//@assert *((u32 *)((u32)xmhfhwm_cpu_gprs_esp+8)) == (u32)xmhfhwm_cpu_gprs_esp+12;
	//@assert *((u32 *)((u32)xmhfhwm_cpu_gprs_esp+12)) == saved_cpu_gprs_ebx;
	//@assert *((u32 *)((u32)xmhfhwm_cpu_gprs_esp+16)) == saved_cpu_gprs_esi;
	//@assert *((u32 *)((u32)xmhfhwm_cpu_gprs_esp+20)) == saved_cpu_gprs_edi;
	//@assert *((u32 *)((u32)xmhfhwm_cpu_gprs_esp+24)) == saved_cpu_gprs_ebp;
	//@assert *((u32 *)((u32)xmhfhwm_cpu_gprs_esp+28)) == CASM_RET_EIP;
	//@assert *((u32 *)((u32)xmhfhwm_cpu_gprs_esp+32)) == (unsigned int)&drv_slab_entrystub_sp;

	//@assert false;
}


void drv_slab_entrystub(void){
	xmhfhwm_cpu_gprs_ebx = 5UL;
	xmhfhwm_cpu_gprs_esi = 6UL;
	xmhfhwm_cpu_gprs_edi = 7UL;
	xmhfhwm_cpu_gprs_ebp = 8UL;
	saved_cpu_gprs_ebx = xmhfhwm_cpu_gprs_ebx;
	saved_cpu_gprs_esi = xmhfhwm_cpu_gprs_esi;
	saved_cpu_gprs_edi = xmhfhwm_cpu_gprs_edi;
	saved_cpu_gprs_ebp = xmhfhwm_cpu_gprs_ebp;


	drv_slab_entrystub_sp.cpuid = 0;
	drv_slab_entrystub_sp.src_slabid = 6UL;

	CASM_FUNCCALL(_slab_entrystub, &drv_slab_entrystub_sp);
	//@assert false;
}

#endif // DRV_SLAB_ENTRYSTUB

void main(void){
	//populate hardware model stack and program counter
	xmhfhwm_cpu_gprs_esp = _slab_tos[cpuid];
	xmhfhwm_cpu_gprs_eip = check_eip;
	check_esp = xmhfhwm_cpu_gprs_esp; // pointing to top-of-stack

	//execute harness: TODO
#if defined (DRV_SLAB_ENTRYSTUB)
	drv_slab_entrystub();
#endif //DRV_SLAB_ENTRYSTUB


	//@assert xmhfhwm_cpu_gprs_esp == check_esp;
	//@assert xmhfhwm_cpu_gprs_eip == check_eip;
}



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
 * x86 cpu hardware model implementation
 * author: amit vasudevan (amitvasudevan@acm.org)
*/


#include <xmhf.h>
#include <xmhf-hwm.h>

u32 xmhfhwm_cpu_gprs_eip = 0;
u32 xmhfhwm_cpu_gprs_esp = 0;

u32 xmhfhwm_cpu_gprs_eax = 0;
u32 xmhfhwm_cpu_gprs_edx = 0;
u32 xmhfhwm_cpu_eflags = 0;


void _impl_xmhfhwm_cpu_insn_hlt(void){
	//@assert 0;
	while(1);
}



void _impl_xmhfhwm_cpu_insn_pushl_mesp(int index){
	u32 value;
	value = *((u32 *)(xmhfhwm_cpu_gprs_esp + index));
	xmhfhwm_cpu_gprs_esp -= sizeof(u32);
	*((u32 *)xmhfhwm_cpu_gprs_esp) = value;
}

void _impl_xmhfhwm_cpu_insn_pushl_mem(u32 value){
	xmhfhwm_cpu_gprs_esp -= sizeof(u32);
	*((u32 *)xmhfhwm_cpu_gprs_esp) = value;
}


u32 _impl_xmhfhwm_cpu_insn_popl_mem(void){
	u32 value = *((u32 *)xmhfhwm_cpu_gprs_esp);
	xmhfhwm_cpu_gprs_esp += sizeof(u32);
	return value;
}

void _impl_xmhfhwm_cpu_insn_addl_imm_esp(u32 value){
	xmhfhwm_cpu_gprs_esp += value;
}


void _impl_xmhfhwm_cpu_insn_movl_mesp_eax(int index){
	u32 *value;
	value = (u32 *)((u32)((int)xmhfhwm_cpu_gprs_esp + (int)index));
	xmhfhwm_cpu_gprs_eax = *value;
}


void _impl_xmhfhwm_cpu_insn_cmpl_imm_meax(u32 value, int index){
	uint32_t value_meax;
	value_meax = *((uint32_t *)((uint32_t)((int32_t)xmhfhwm_cpu_gprs_eax + (int32_t)index)));

	//XXX: TODO propagation of CF, PF, AF, SF and OF
        if(value_meax - value == 0)
		xmhfhwm_cpu_eflags |= EFLAGS_ZF;

}

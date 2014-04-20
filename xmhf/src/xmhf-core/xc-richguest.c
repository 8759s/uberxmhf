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

// XMHF "rich" (SMP) guest component implementation
// takes care of initializing and booting up the "rich" guest
// author: amit vasudevan (amitvasudevan@acm.org)

#include <xmhf-core.h> 


//void xmhf_richguest_initialize(u32 index_cpudata_bsp){
//	xmhf_richguest_arch_initialize(index_cpudata_bsp);	
//}

void xmhf_richguest_initialize(xc_cpu_t *xc_cpu_bsp, xc_partition_t *xc_partition_richguest){
	xmhf_richguest_arch_initialize(xc_cpu_bsp, xc_partition_richguest);	
}


//initialize environment to boot "rich" guest
//void xmhf_smpguest_initialize(context_desc_t context_desc){
//void xmhf_richguest_addcpuandrun(u32 index_cpudata){
void xmhf_richguest_addcpuandrun(u32 index_cpudata, xc_partition_t *xc_partition_richguest){
		
	//initialize CPU
	//xmhf_baseplatform_cpuinitialize();

	//initialize partition monitor (i.e., hypervisor) for this CPU
	//xmhf_partition_initializemonitor(vcpu);
	//xmhf_partition_initializemonitor(context_desc);
	xmhf_partition_initializemonitor(index_cpudata);
	

	//setup guest OS state for partition
	//xmhf_partition_setupguestOSstate(vcpu);
	//xmhf_partition_setupguestOSstate(context_desc);
	xmhf_partition_setupguestOSstate(index_cpudata);

	//initialize memory protection for this core
	//xmhf_memprot_initialize(context_desc);		
	xmhf_memprot_initialize(index_cpudata, xc_partition_richguest);		

	//start partition (guest)
	printf("\n%s[%u]: starting partition...", __FUNCTION__, index_cpudata);
	//xmhf_partition_start(context_desc);
	xmhf_partition_start(index_cpudata);
}


/*//quiesce interface to switch all guest cores into hypervisor mode
static void xmhf_smpguest_quiesce(VCPU *vcpu) __attribute__((unused));
static void xmhf_smpguest_quiesce(VCPU *vcpu){
	xmhf_smpguest_arch_quiesce(vcpu);
}

//endquiesce interface to resume all guest cores after a quiesce
static void xmhf_smpguest_endquiesce(VCPU *vcpu) __attribute__((unused));
static void xmhf_smpguest_endquiesce(VCPU *vcpu){
	xmhf_smpguest_arch_endquiesce(vcpu);
}*/


//walk guest page tables; returns pointer to corresponding guest physical address
//note: returns 0xFFFFFFFF if there is no mapping
u8 * xmhf_smpguest_walk_pagetables(context_desc_t context_desc, u32 vaddr){
	u8 *retvalue;
	retvalue = xmhf_smpguest_arch_walk_pagetables(context_desc, vaddr);
	return retvalue;
}

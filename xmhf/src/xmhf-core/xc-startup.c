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

// runtime.c
// author: amit vasudevan (amitvasudevan@acm.org)

//---includes-------------------------------------------------------------------
#include <xmhf-core.h> 
#include <xc-x86.h>

//initialize global core cpu data structure, g_xc_cpu
//and g_xc_cpu_count;
static void _xc_startup_initialize_cpudata(XMHF_BOOTINFO *bootinfo){
	u32 i;
	
	printf("\nNo. of CPU entries = %u", bootinfo->cpuinfo_numentries);

	for(i=0; i < xcbootinfo->cpuinfo_numentries; i++){
		g_xc_cpu[i].cpuid = bootinfo->cpuinfo_buffer[i].lapic_id;
		g_xc_cpu[i].is_bsp = bootinfo->cpuinfo_buffer[i].isbsp;
		g_xc_cpu[i].is_quiesced = false;
		//g_xc_cpu[i].index_cpuarchdata = i;	//indexes into g_xc_cpuarchdata[i][] for arch. specific data buffer
		g_xc_cpu[i].cpuarchdata = (xc_cpuarchdata_t *)&g_xc_cpuarchdata[i][0];
		g_xc_cpu[i].index_partitiondata = XC_INDEX_INVALID;
		printf("\nCPU #%u: bsp=%u, lapic_id=0x%02x, cpuarchdata=%08x", i, bootinfo->cpuinfo_buffer[i].isbsp, bootinfo->cpuinfo_buffer[i].lapic_id, (u32)g_xc_cpu[i].cpuarchdata);
	}

	g_xc_cpu_count = bootinfo->cpuinfo_numentries;
}

//setup global partition data structures g_xc_primary_partition and g_xc_secondary_partition
static void _xc_startup_initialize_partitiondata(void){
		u32 i;
		
		//primary partitions
		for(i=0; i < MAX_PRIMARY_PARTITIONS; i++){
				g_xc_primary_partition[i].partitionid=i;
				g_xc_primary_partition[i].partitiontype = XC_PARTITION_PRIMARY;
				g_xc_primary_partition[i].hptdata = &g_xc_primary_partition_hptdata[i];
				g_xc_primary_partition[i].trapmaskdata = &g_xc_primary_partition_trapmaskdata[i];
		}

		//secondary partitions
		for(i=0; i < MAX_SECONDARY_PARTITIONS; i++){
				g_xc_secondary_partition[i].partitionid=i;
				g_xc_secondary_partition[i].partitiontype = XC_PARTITION_SECONDARY;
				g_xc_secondary_partition[i].hptdata = &g_xc_secondary_partition_hptdata[i];
				g_xc_secondary_partition[i].trapmaskdata = NULL;
		}
}

// initialize global cpu table (g_xc_cputable)
//static void _xc_startup_initialize_cputable(void){
//static u32 _xc_startup_initialize_cputable(void){
static xc_cpu_t * _xc_startup_initialize_cputable(void){
	u32 i;
	//u32 index_cpudata_bsp;
	xc_cpu_t *xc_cpu_bsp;

	for(i=0; i < g_xc_cpu_count; i++){
			g_xc_cputable[i].cpuid = g_xc_cpu[i].cpuid;
			g_xc_cputable[i].index_cpudata = i;
			if(g_xc_cpu[i].is_bsp)
				//index_cpudata_bsp = i;
				xc_cpu_bsp = &g_xc_cpu[i];
	}
	
	//return index_cpudata_bsp;
	return xc_cpu_bsp;
}

void xmhf_runtime_entry(void){
	//u32 index_cpudata_bsp;
	xc_cpu_t *xc_cpu_bsp;

	//setup debugging	
	xmhf_debug_init((char *)&xcbootinfo->debugcontrol_buffer);
	printf("\nxmhf-core: starting...");

    //[debug] dump E820
 	#ifndef __XMHF_VERIFICATION__
 	printf("\nNumber of E820 entries = %u", xcbootinfo->memmapinfo_numentries);
	{
		int i;
		for(i=0; i < (int)xcbootinfo->memmapinfo_numentries; i++){
			printf("\n0x%08x%08x, size=0x%08x%08x (%u)", 
			  xcbootinfo->memmapinfo_buffer[i].baseaddr_high, xcbootinfo->memmapinfo_buffer[i].baseaddr_low,
			  xcbootinfo->memmapinfo_buffer[i].length_high, xcbootinfo->memmapinfo_buffer[i].length_low,
			  xcbootinfo->memmapinfo_buffer[i].type);
		}
  	}
	#endif //__XMHF_VERIFICATION__

	//initialize global cpu data structure
	_xc_startup_initialize_cpudata(xcbootinfo);

	//initialize partition data structures
	_xc_startup_initialize_partitiondata();
	
	//initialize global cpu table
	//index_cpudata_bsp = _xc_startup_initialize_cputable();
	xc_cpu_bsp = _xc_startup_initialize_cputable();

	//setup Master-ID Table (MIDTABLE)
	{
		int i;
		#ifndef __XMHF_VERIFICATION__
			for(i=0; i < (int)xcbootinfo->cpuinfo_numentries; i++){
				g_midtable[g_midtable_numentries].cpu_lapic_id = xcbootinfo->cpuinfo_buffer[i].lapic_id;
				g_midtable[g_midtable_numentries].vcpu_vaddr_ptr = 0;
				g_midtable_numentries++;
			}
		#else
		//verification is always done in the context of a single core and vcpu/midtable is 
		//populated by the verification driver
		//TODO: incorporate some sort of BIOS data area within the verification harness that will
		//allow us to populate these tables during verification
		#endif
	}

  	//initialize basic platform elements
	xmhf_baseplatform_initialize();

	#ifndef __XMHF_VERIFICATION__
	//setup XMHF exception handler component
	xmhf_xcphandler_initialize();
	#endif

#if defined (__DMAP__)
	xmhf_dmaprot_reinitialize();
#endif

	//initialize richguest
	//xmhf_richguest_initialize(index_cpudata_bsp);
	xmhf_richguest_initialize(xc_cpu_bsp, xc_partition_richguest);

	//invoke XMHF api hub initialization function to initialize core API
	//interface layer
	xmhf_apihub_initialize();

	//call hypapp main function
	{
		hypapp_env_block_t hypappenvb;
		hypappenvb.runtimephysmembase = (u32)xcbootinfo->physmem_base;  
		hypappenvb.runtimesize = (u32)xcbootinfo->size;
	
		//call app main
		printf("\n%s: proceeding to call xmhfhypapp_main on BSP", __FUNCTION__);
		printf("\n%s: CR3 before going in=%08x", __FUNCTION__, read_cr3());
		xmhfhypapp_main(hypappenvb);
		printf("\n%s: CR3 after coming back=%08x", __FUNCTION__, read_cr3());
		printf("\n%s: came back into core", __FUNCTION__);

	}   	

	//initialize base platform with SMP 
	xmhf_baseplatform_smpinitialize();

	printf("\nRuntime: We should NEVER get here!");
	HALT_ON_ERRORCOND(0);
}


//we get control here in the context of *each* physical CPU core 
//void xmhf_runtime_main(context_desc_t context_desc){ 
void xmhf_runtime_main(u32 index_cpudata){ 
	//[debug]
	//printf("\n%s: partdesc.id=%u, cpudesc.id=%u, cpudesc.isbsp=%u", __FUNCTION__, context_desc.partition_desc.id, context_desc.cpu_desc.id, context_desc.cpu_desc.isbsp);
	printf("\n%s: index_cpudata=%u", __FUNCTION__, index_cpudata);

	//TODO: check if this CPU is allocated to the "rich" guest. if so, pass it on to
	//the rich guest initialization procedure. if the CPU is not allocated to the
	//rich guest, enter it into a CPU pool for use by other partitions
	
	//initialize and boot "rich" guest
	//xmhf_smpguest_initialize(context_desc);
	xmhf_richguest_addcpuandrun(index_cpudata, xc_partition_richguest);

	//TODO: implement CPU pooling for use by other partitions
	
	#ifndef __XMHF_VERIFICATION__
	printf("\n%s: index_cpudata=%u: FATAL, should not be here. HALTING!", __FUNCTION__, index_cpudata);
	HALT();
	#endif //__XMHF_VERIFICATION__
	
}

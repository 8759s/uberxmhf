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

/**
 * XMHF core global data module
 * author: amit vasudevan (amitvasudevan@acm.org)
 */

#include <xmhf-core.h>


//bplt-data.c

//system e820 map
//GRUBE820 g_e820map[MAX_E820_ENTRIES] __attribute__(( section(".data") ));

//SMP CPU map; lapic id, base, ver and bsp indication for each available core
//PCPU	g_cpumap[MAX_PCPU_ENTRIES] __attribute__(( section(".data") ));

//runtime stacks for individual cores
//u8 g_cpustacks[RUNTIME_STACK_SIZE * MAX_PCPU_ENTRIES] __attribute__(( section(".stack") ));

//master id table, contains core lapic id to VCPU mapping information
//MIDTAB g_midtable[MAX_MIDTAB_ENTRIES] __attribute__(( section(".data") ));

//number of entries in the master id table, in essence the number of 
//physical cores in the system
//u32 g_midtable_numentries __attribute__(( section(".data") )) = 0;

//variable that is incremented by 1 by all cores that boot up, this should
//be finally equal to g_midtable_numentries at runtime which signifies
//that all physical cores have been booted up and initialized by the runtime
//u32 g_cpus_active __attribute__(( section(".data") )) = 0;

//SMP lock for the above variable
//u32 g_lock_cpus_active __attribute__(( section(".data") )) = 1;
    
//variable that is set to 1 by the BSP after rallying all the other cores.
//this is used by the application cores to enter the "wait-for-SIPI" state    
//u32 g_ap_go_signal __attribute__(( section(".data") )) = 0;

//SMP lock for the above variable
//u32 g_lock_ap_go_signal __attribute__(( section(".data") )) = 1;


//rntm-data.c

//runtime parameter block pointer 
//RPB *rpb __attribute__(( section(".data") )); 
//RPB *rpb __attribute__(( section(".data") )); 
//XMHF_BOOTINFO *xcbootinfo;

//runtime stack
static u8 _init_stack[MAX_PLATFORM_CPUSTACK_SIZE] __attribute__(( section(".stack") ));

static XMHF_BOOTINFO xcbootinfo_store __attribute__(( section(".s_rpb") )) = {
	.magic= RUNTIME_PARAMETER_BLOCK_MAGIC,
	.entrypoint= (u32)xmhf_runtime_entry,
	.stack_base = (u32)_init_stack,
	.stack_size = MAX_PLATFORM_CPUSTACK_SIZE,
};

XMHF_BOOTINFO *xcbootinfo= &xcbootinfo_store;


//core DMA protection buffer (if DMA protections need to be re-initialized on the target platform)
u8 g_core_dmaprot_buffer[SIZE_CORE_DMAPROT_BUFFER] __attribute__(( section(".palign_data") ));

//variable that is incremented by 1 by all cores that cycle through appmain
//successfully, this should be finally equal to g_midtable_numentries at
//runtime which signifies that EMHF appmain executed successfully on all
//cores
//u32 g_appmain_success_counter __attribute__(( section(".data") )) = 0;

//SMP lock for the above variable
//u32 g_lock_appmain_success_counter __attribute__(( section(".data") )) = 1;


//xc-apihub.c

//----------------------------------------------------------------------
/*
 * 	apih-pbvph-data.c
 * 
 *  XMHF core API interface component pass-by-value parameter handling
 *  backend
 * 
 *  global data variables
 * 
 *  author: amit vasudevan (amitvasudevan@acm.org)
 */

XMHF_HYPAPP_PARAMETERBLOCK *paramcore = (XMHF_HYPAPP_PARAMETERBLOCK *)&paramcore_start;

XMHF_HYPAPP_PARAMETERBLOCK *paramhypapp = (XMHF_HYPAPP_PARAMETERBLOCK *)&paramhypapp_start;


//----------------------------------------------------------------------
XMHF_HYPAPP_HEADER *g_hypappheader=(XMHF_HYPAPP_HEADER *)__TARGET_BASE_XMHFHYPAPP;

//hypapp callback hub entry point and hypapp top of stack
u32 hypapp_cbhub_pc=0;
u32 hypapp_tos=0;

//----------------------------------------------------------------------
//variables
//XXX: move them into relevant component headers

// platform cpus
xc_cpu_t g_xc_cpu[MAX_PLATFORM_CPUS] __attribute__(( section(".data") ));

// count of platform cpus
u32 g_xc_cpu_count __attribute__(( section(".data") )) = 0;

// platform cpu arch. data buffer
//u8 g_xc_cpuarchdata[MAX_PLATFORM_CPUS][MAX_PLATFORM_CPUARCHDATA_SIZE] __attribute__(( section(".palign_data") ));
xc_cpuarchdata_t g_xc_cpuarchdata[MAX_PLATFORM_CPUS][MAX_PLATFORM_CPUARCHDATA_SIZE] __attribute__(( section(".data"), aligned(4096) ));

// platform cpu stacks
u8 g_xc_cpustack[MAX_PLATFORM_CPUS][MAX_PLATFORM_CPUSTACK_SIZE] __attribute__(( section(".stack") ));

// primary partitions
xc_partition_t g_xc_primary_partition[MAX_PRIMARY_PARTITIONS] __attribute__(( section(".data") ));

// secondary partitions
xc_partition_t g_xc_secondary_partition[MAX_SECONDARY_PARTITIONS] __attribute__(( section(".data") ));

// primary partition hpt data buffers
xc_partition_hptdata_t g_xc_primary_partition_hptdata[MAX_PRIMARY_PARTITIONS][MAX_PRIMARY_PARTITION_HPTDATA_SIZE] __attribute__(( section(".data"), aligned(4096) ));

// secondary partition hpt data buffers
xc_partition_hptdata_t g_xc_secondary_partition_hptdata[MAX_SECONDARY_PARTITIONS][MAX_SECONDARY_PARTITION_HPTDATA_SIZE] __attribute__(( section(".data"), aligned(4096) ));

// primary partition trap mask data buffers
xc_partition_trapmaskdata_t g_xc_primary_partition_trapmaskdata[MAX_PRIMARY_PARTITIONS][MAX_PRIMARY_PARTITION_TRAPMASKDATA_SIZE] __attribute__(( section(".data"), aligned(4096) ));

// partition data structure pointer for the richguest
xc_partition_t *xc_partition_richguest = (xc_partition_t *)&g_xc_primary_partition[0];

// cpu table
xc_cputable_t g_xc_cputable[MAX_PLATFORM_CPUS] __attribute__(( section(".data") ));


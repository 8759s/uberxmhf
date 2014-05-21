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

#include <xmhf-core.h>

#include <xc-x86.h>
#include <xc-x86vmx.h>

/*
 * 	xc-apihub x86 arch. backend implementation -- initbs portion
 *  author: amit vasudevan (amitvasudevan@acm.org)
 */

//----------------------------------------------------------------------
// local variables

//hypapp PAE page tables
static u64 hypapp_3level_pdpt[PAE_MAXPTRS_PER_PDPT] __attribute__(( section(".palign_data") ));
static u64 hypapp_3level_pdt[PAE_PTRS_PER_PDPT * PAE_PTRS_PER_PDT] __attribute__(( section(".palign_data") ));

//core PAE page tables
static u64 core_3level_pdpt[PAE_MAXPTRS_PER_PDPT] __attribute__(( section(".palign_data") ));
static u64 core_3level_pdt[PAE_PTRS_PER_PDPT * PAE_PTRS_PER_PDT] __attribute__(( section(".palign_data") ));

// initialization function for the core API interface
void xmhf_apihub_arch_initialize (void){

#ifndef __XMHF_VERIFICATION__

	printf("\n%s: starting...", __FUNCTION__);
	printf("\n%s: hypappheader at %08x", __FUNCTION__, g_hypappheader);
	printf("\n%s: hypappheader->magic is %08x", __FUNCTION__, g_hypappheader->magic);
	
	printf("\n%s: paramcore at 0x%08x", __FUNCTION__, (u32)paramcore);
	printf("\n%s: paramhypapp at 0x%08x", __FUNCTION__, (u32)paramhypapp);
	
	hypapp_cbhub_pc = (u32)g_hypappheader->addr_hypappfromcore;
	hypapp_tos = (u32)g_hypappheader->addr_tos;

	printf("\n%s: hypapp cbhub entry point=%x, TOS=%x", __FUNCTION__, hypapp_cbhub_pc, hypapp_tos);

	//cast hypapp header information into hypappheader 
	//(a data structure of type XMHF_HYPAPP_HEADER) and populate the
	//hypapp parameter block field
	{
		g_hypappheader->apb.bootsector_ptr = (u32)xcbootinfo->richguest_bootmodule_base;
		g_hypappheader->apb.bootsector_size = (u32)xcbootinfo->richguest_bootmodule_size;
		g_hypappheader->apb.runtimephysmembase = (u32)xcbootinfo->physmem_base;  
		strncpy(g_hypappheader->apb.cmdline, xcbootinfo->cmdline_buffer, sizeof(g_hypappheader->apb.cmdline));
		printf("\n%s: sizeof(XMHF_HYPAPP_HEADER)=%u", __FUNCTION__, sizeof(XMHF_HYPAPP_HEADER));
		printf("\n%s: sizeof(APP_PARAM_BLOCK)=%u", __FUNCTION__, sizeof(APP_PARAM_BLOCK));
			
	}

	//g_hypappheader->addr_hypapptocore = (u32)&xmhf_apihub_fromhypapp;

	//check for PCID support (if present)
	{
			u32 eax, ebx, ecx, edx;
			
			cpuid(0x1, &eax, &ebx, &ecx, &edx);
			
			if( ecx & (1UL << 17) )
				printf("\n%s: PCID supported", __FUNCTION__);
			else
				printf("\n%s: PCID not supported", __FUNCTION__);
	}
	
	//initialize core PAE page-tables
	{
		u32 i, hva=0;
		u64 default_flags = (u64)(_PAGE_PRESENT);
		
		//init pdpt
		for(i = 0; i < PAE_PTRS_PER_PDPT; i++) {
			u64 pdt_spa = hva2spa((void *)core_3level_pdt) + (i << PAGE_SHIFT_4K);
			core_3level_pdpt[i] = pae_make_pdpe(pdt_spa, default_flags);
		}

		//init pdts with unity mappings
		default_flags = (u64)(_PAGE_PRESENT | _PAGE_RW | _PAGE_PSE | _PAGE_USER);
		for(i = 0, hva = 0; i < (ADDR_4GB >> (PAE_PDT_SHIFT)); i++, hva += PAGE_SIZE_2M) {
			u64 spa = hva2spa((void*)hva);
			u64 flags = default_flags;

			//mark core code/data/stack pages supervisor
			if(spa >= 0x10000000 && spa < 0x1CC00000)
				flags &= ~(u64)(_PAGE_USER);
			
			//mark core parameter region as read-write, no-execute
			if(spa == 0x1CC00000){
				//flags |= (u64)(_PAGE_RW | _PAGE_NX);
				flags |= (u64)(_PAGE_RW);
				flags |= (u64)(_PAGE_NX);
			}	
			
			//mark hypapp parameter region as user-read-only, no-execute
			if(spa == 0x1CE00000){
				flags &= ~(u64)(_PAGE_RW);
				flags |= (u64)_PAGE_NX;
			}
			//mark hypapp code/data/stack pages as read-only, no-execute
			if(spa >= 0x1D000000 && spa < 0x20000000){
				flags &= ~(u64)(_PAGE_RW);
				flags |= (u64)_PAGE_NX;
			}
		
			if(spa == 0xfee00000 || spa == 0xfec00000) {
				//Unity-map some MMIO regions with Page Cache disabled 
				//0xfed00000 contains Intel TXT config regs & TPM MMIO 
				//0xfee00000 contains LAPIC base 
				flags |= (u64)(_PAGE_PCD);
			}

			core_3level_pdt[i] = pae_make_pde_big(spa, flags);
		}	
	}
	
	//initialize hypapp PAE page-tables
	{
		u32 i, hva=0;
		u64 default_flags = (u64)(_PAGE_PRESENT);
		
		//init pdpt
		for(i = 0; i < PAE_PTRS_PER_PDPT; i++) {
			u64 pdt_spa = hva2spa((void *)hypapp_3level_pdt) + (i << PAGE_SHIFT_4K);
			hypapp_3level_pdpt[i] = pae_make_pdpe(pdt_spa, default_flags);
		}

		//init pdts with unity mappings
		default_flags = (u64)(_PAGE_PRESENT | _PAGE_RW | _PAGE_PSE | _PAGE_USER);
		for(i = 0, hva = 0; i < (ADDR_4GB >> (PAE_PDT_SHIFT)); i++, hva += PAGE_SIZE_2M) {
			u64 spa = hva2spa((void*)hva);
			u64 flags = default_flags;

			//mark core code/data/stack pages supervisor
			if(spa >= 0x10000000 && spa < 0x1CC00000)
				flags &= ~(u64)(_PAGE_USER);
			
			//mark core parameter region as user-read-only, no-execute
			if(spa == 0x1CC00000){
				flags &= ~(u64)(_PAGE_RW);
				flags |= (u64)(_PAGE_NX);
			}
		
			//mark hypapp parameter region as user-read-write, no-execute
			if(spa == 0x1CE00000){
				flags |= (u64)(_PAGE_RW);
				flags |= (u64)(_PAGE_NX);
			}
			
			//mark hypapp code/data/stack pages as read-write
			if(spa >= 0x1D000000 && spa < 0x20000000)
				flags |= (u64)(_PAGE_RW);

			if(spa == 0xfee00000 || spa == 0xfec00000) {
				//Unity-map some MMIO regions with Page Cache disabled 
				//0xfed00000 contains Intel TXT config regs & TPM MMIO 
				//0xfee00000 contains LAPIC base 
				flags |= (u64)(_PAGE_PCD);
			}

			hypapp_3level_pdt[i] = pae_make_pde_big(spa, flags);
		}	
	}


	//setup core and hypapp page table base addresses and print them out
	{
		core_ptba = (u32)&core_3level_pdpt;
		hypapp_ptba = (u32)&hypapp_3level_pdpt;
		printf("\n%s: core_ptba=%08x, hypapp_ptba=%08x", __FUNCTION__, core_ptba, hypapp_ptba);
	}

		
	//initialize core paging
	xmhf_baseplatform_arch_x86_initialize_paging((u32)&core_3level_pdpt);

	//turn on WP bit in CR0 register for supervisor mode read-only permission support
	{
		u32 cr0;
		cr0=read_cr0();
		printf("\n%s: CR0=%08x", __FUNCTION__, cr0);
		cr0 |= CR0_WP;
		printf("\n%s: attempting to change CR0 to %08x", __FUNCTION__, cr0);
		write_cr0(cr0);
		cr0 = read_cr0();
		printf("\n%s: CR0 changed to %08x", __FUNCTION__, cr0);
	}

	//turn on NX protections
	{
		u32 eax, edx;
		rdmsr(MSR_EFER, &eax, &edx);
		eax |= (1 << EFER_NXE);
		wrmsr(MSR_EFER, eax, edx);
		printf("\n%s: NX protections enabled: MSR_EFER=%08x%08x", __FUNCTION__, edx, eax);
	}

#endif //__XMHF_VERIFICATION__
}


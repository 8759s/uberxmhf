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
 * XMHF core initbs (initialization-bootstrap) slab
 * x86 vmx arch. backend
 * author: amit vasudevan (amitvasudevan@acm.org)
 */

#include <xmhf-core.h>
#include <xc-x86.h>
#include <xc-x86vmx.h>

#define __XMHF_SLAB_CALLER_INDEX__ 	XMHF_SLAB_INITBS_INDEX
#include <xc-init.h>
#include <xcexhub.h>
#undef __XMHF_SLAB_CALLER_INDEX__

/* originally within xc-baseplatform-x86vmx.c */

//----------------------------------------------------------------------
// local variables

/*
 * XMHF base platform SMP real mode trampoline
 * this is where all AP CPUs start executing when woken up
 * 
 * author: amit vasudevan (amitvasudevan@acm.org)
 */

/*
	.code16
  .global _ap_bootstrap_start
  _ap_bootstrap_start:
    jmp ap_bootstrap_bypassdata
    .global _ap_cr3_value
    _ap_cr3_value:
      .long 0
    .global _ap_cr4_value
    _ap_cr4_value: 
      .long 0
    .global _ap_runtime_entrypoint
    _ap_runtime_entrypoint:
	  .long 0
    .align 16
    .global _mle_join_start
    _mle_join_start:
    .long _ap_gdt_end - _ap_gdt_start - 1 // gdt_limit
    .long _ap_gdt_start - _ap_bootstrap_start + 0x10000// gdt_base
    .long 0x00000008 // CS
    .long _ap_clear_pipe - _ap_bootstrap_start + 0x10000 // entry point
    _mle_join_end:
    
    _ap_gdtdesc:
      .word _ap_gdt_end - _ap_gdt_start - 1
      .long _ap_gdt_start - _ap_bootstrap_start + 0x10000  
    
    .align 16
    _ap_gdt_start:
      .quad 0x0000000000000000
      .quad 0x00cf9a000000ffff	
      .quad 0x00cf92000000ffff
    _ap_gdt_end:
      .word 0
    
    .align 16
  ap_bootstrap_bypassdata:
      movw $0x1000, %ax
    	movw %ax, %ds
    	movw %ax, %es
    	movw $0xFFFF, %sp
    	movw $0x4000, %ax
    	movw %ax, %ss
    	
    	movw $0x0020, %si

      lgdt (%si)

      movl %cr0, %eax
      orl $0x1, %eax
      movl %eax, %cr0

      jmpl $0x08, $(_ap_clear_pipe - _ap_bootstrap_start + (AP_BOOTSTRAP_CODE_SEG << 4))
    .code32
    _ap_clear_pipe:
      movw $0x10, %ax
      movw %ax, %ds
      movw %ax, %es
      movw %ax, %ss
      movw %ax, %fs
      movw %ax, %gs

             
      movl $(_ap_cr3_value - _ap_bootstrap_start + (AP_BOOTSTRAP_CODE_SEG << 4)), %esi
      movl (%esi), %ebx
      movl %ebx, %cr3
      movl $(_ap_cr4_value - _ap_bootstrap_start + (AP_BOOTSTRAP_CODE_SEG << 4)), %esi
      movl (%esi), %ebx
      movl %ebx, %cr4
      movl $(_ap_runtime_entrypoint - _ap_bootstrap_start + (AP_BOOTSTRAP_CODE_SEG << 4)), %esi
      movl (%esi), %ebx
      
      movl %cr0, %eax
      orl $0x80000000, %eax	
      movl %eax, %cr0

      jmpl *%ebx
      hlt
*/


/*static u8 _ap_bootstrap_blob[256] = {
															//0x00: _ap_bootstrap_start
		0xeb, 0x4e, 										//0x00: jmp ap_bootstrap_bypassdata
		0x00, 0x00, 0x00, 0x00,								//0x02: _ap_cr3_value
		0x00, 0x00, 0x00, 0x00,								//0x06: _ap_cr4_value
		0x00, 0x00, 0x00, 0x00, 							//0x0a: _ap_runtime_entrypoint
		0x90, 0x90,											//.align 16
															//0x10: _mle_join_start
		0x17, 0x00, 0x00, 0x00,								//0x10: .long _ap_gdt_end - _ap_gdt_start - 1 // gdt_limit
		0x30, 0x00, 0x01, 0x00,								//0x14: .long _ap_gdt_start - _ap_bootstrap_start + 0x10000// gdt_base
		0x08, 0x00, 0x00, 0x00,								//0x18: .long 0x00000008 // CS
		0x77, 0x00, 0x01, 0x00, 							//0x1C: .long _ap_clear_pipe - _ap_bootstrap_start + 0x10000 // entry point
															//0x20: _ap_gdtdesc:
		0x17, 0x00,											//0x20: .word _ap_gdt_end - _ap_gdt_start - 1
		0x30, 0x00, 0x01, 0x00,								//0x22: .long _ap_gdt_start - _ap_bootstrap_start + 0x10000  
															//0x26: .align 16
		0x90, 0x90, 0x90, 0x90, 							//0x26: .align 16
		0x90, 0x90, 0x90, 0x90,								//0x29: .align 16
		0x90, 0x90,											//0x2d: .align 16
															//0x30:_ap_gdt_start:
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 	//0x30: .quad 0x0000000000000000
		0xff, 0xff, 0x00, 0x00, 0x00, 0x9a, 0xcf, 0x00, 	//0x38: .quad 0x00cf9a000000ffff	
		0xff, 0xff, 0x00, 0x00, 0x00, 0x92, 0xcf, 0x00, 	//0x40: .quad 0x00cf92000000ffff
															//0x48: _ap_gdt_end:
		0x00, 0x00,											//0x48: .word 0
															//0x4a: .align 16
		0x90, 0x90, 0x90, 0x90, 0x90, 0x90,					//0x4a: .align 16
															//0x50: _ap_bootstrap_bypassdata
		0xb8, 0x00, 0x10,									//0x50: mov    $0x1000,%ax
		0x8e, 0xd8,                							//0x53: mov    %ax,%ds
		0x8e, 0xc0,                							//0x55: mov    %ax,%es
		0xbc, 0xff, 0xff,          							//0x57: mov    $0xffff,%sp
		0xb8, 0x00, 0x40,          							//0x5a: mov    $0x4000,%ax
		0x8e, 0xd0,                							//0x5d: mov    %ax,%ss
		0xbe, 0x20, 0x00,          							//0x5f: mov    $0x20,%si
		0x0f, 0x01, 0x14,          							//0x62: lgdtw  (%si)
		0x0f, 0x20, 0xc0,          							//0x65: mov    %cr0,%eax
		0x66, 0x83, 0xc8, 0x01,    							//0x68: or     $0x1,%eax
		0x0f, 0x22, 0xc0,          							//0x6c: mov    %eax,%cr0
		0x66, 0xea, 0x77, 0x00, 0x01, 0x00, 0x08, 0x00,		//0x6f: jmpl $0x08, $(_ap_clear_pipe - _ap_bootstrap_start + (AP_BOOTSTRAP_CODE_SEG << 4))
															//0x77: _ap_clear_pipe:
		0x66, 0xb8, 0x10, 0x00,    							//0x77: mov    $0x10,%ax
		0x8e, 0xd8,                							//0x7b: mov    %eax,%ds
		0x8e, 0xc0,                							//0x7d: mov    %eax,%es
		0x8e, 0xd0,                							//0x7f: mov    %eax,%ss
		0x8e, 0xe0,                							//0x81: mov    %eax,%fs
		0x8e, 0xe8,                							//0x83: mov    %eax,%gs
		0xbe, 0x02, 0x00, 0x01, 0x00,						//0x85: movl $(_ap_cr3_value - _ap_bootstrap_start + (AP_BOOTSTRAP_CODE_SEG << 4)), %esi
		0x8b, 0x1e,                							//0x8a: mov    (%esi),%ebx
		0x0f, 0x22, 0xdb,          							//0x8c: mov    %ebx,%cr3
		0xbe, 0x06, 0x00, 0x01, 0x00,						//0x8f: movl $(_ap_cr4_value - _ap_bootstrap_start + (AP_BOOTSTRAP_CODE_SEG << 4)), %esi
		0x8b, 0x1e,                							//0x94: mov    (%esi),%ebx
		0x0f, 0x22, 0xe3,         							//0x96: mov    %ebx,%cr4
		0xbe, 0x0a, 0x00, 0x01, 0x00,						//0x99: movl $(_ap_runtime_entrypoint - _ap_bootstrap_start + (AP_BOOTSTRAP_CODE_SEG << 4)), %esi
		0x8b, 0x1e,                							//0x9e: mov    (%esi),%ebx
		0x0f, 0x20, 0xc0,          							//0xa0: mov    %cr0,%eax
		0x0d, 0x00, 0x00, 0x00, 0x80,						//0xa3: or     $0x80000000,%eax
		0x0f, 0x22, 0xc0,         							//0xa8: mov    %eax,%cr0
		0xff, 0xe3,              							//0xab: jmp    *%ebx
		0xf4,	                  							//0xad: hlt    

};*/

extern u8 _ap_bootstrap_blob[256];

static u32 * _ap_bootstrap_blob_cr3 = (u32 *) & _ap_bootstrap_blob[0x02];

static u32 * _ap_bootstrap_blob_cr4 = (u32 *) &_ap_bootstrap_blob[0x06];

static u32 * _ap_bootstrap_blob_runtime_entrypoint = (u32 *) &_ap_bootstrap_blob[0x0a];

static u8 * _ap_bootstrap_blob_mle_join_start = (u8 *) &_ap_bootstrap_blob[0x10];

// platform cpu stacks
static u8 _cpustack[MAX_PLATFORM_CPUS][MAX_PLATFORM_CPUSTACK_SIZE] __attribute__(( section(".stack") ));


static void _ap_pmode_entry_with_paging(void) __attribute__((naked));
static void xmhf_baseplatform_arch_x86_smpinitialize_commonstart(void);

// cpu table
static xc_cputable_t _cputable[MAX_PLATFORM_CPUS];


// count of platform cpus
static u32 _cpucount = 0;


//XXX: TODO: get rid of these externs and bring them in here as static
//extern arch_x86_gdtdesc_t _gdt;
//static arch_x86_idtdesc_t _idt;

//----------------------------------------------------------------------
// functions



//wake up application processors (cores) in the system
void xmhf_baseplatform_arch_x86vmx_wakeupAPs(void){
	//step-1: setup AP boot-strap code at in the desired physical memory location 
	//note that we need an address < 1MB since the APs are woken up in real-mode
	//we choose 0x10000 physical or 0x1000:0x0000 logical
    {
		*_ap_bootstrap_blob_cr3 = read_cr3();
        *_ap_bootstrap_blob_cr4 = read_cr4();
        *_ap_bootstrap_blob_runtime_entrypoint = (u32)&_ap_pmode_entry_with_paging;
        #ifndef __XMHF_VERIFICATION__
        memcpy((void *)0x10000, (void *)_ap_bootstrap_blob, sizeof(_ap_bootstrap_blob));
        #endif
    }

#if defined (__DRT__)	
    //step-2: wake up the APs sending the INIT-SIPI-SIPI sequence as per the
    //MP protocol. Use the APIC for IPI purposes.
    if(!txt_is_launched()) { // XXX TODO: Do actual GETSEC[WAKEUP] in here?
        printf("\nBSP: Using APIC to awaken APs...");
        xmhf_baseplatform_arch_x86_wakeupAPs();
        printf("\nBSP: APs should be awake.");
    }else{
		//we ran SENTER, so do a GETSEC[WAKEUP]
        txt_heap_t *txt_heap;
        os_mle_data_t *os_mle_data;
        mle_join_t *mle_join;
        sinit_mle_data_t *sinit_mle_data;
        os_sinit_data_t *os_sinit_data;

        // sl.c unity-maps 0xfed00000 for 2M so these should work fine 
        #ifndef __XMHF_VERIFICATION__
        txt_heap = get_txt_heap();
        //printf("\ntxt_heap = 0x%08x", (u32)txt_heap);
        os_mle_data = get_os_mle_data_start(txt_heap);
        (void)os_mle_data;
        //printf("\nos_mle_data = 0x%08x", (u32)os_mle_data);
        sinit_mle_data = get_sinit_mle_data_start(txt_heap);
        //printf("\nsinit_mle_data = 0x%08x", (u32)sinit_mle_data);
        os_sinit_data = get_os_sinit_data_start(txt_heap);
        //printf("\nos_sinit_data = 0x%08x", (u32)os_sinit_data);
	#endif
            
        // Start APs.  Choose wakeup mechanism based on
        // capabilities used. MLE Dev Guide says MLEs should
        // support both types of Wakeup mechanism. 

        // We are jumping straight into the 32-bit portion of the
        // unity-mapped trampoline that starts at 64K
        // physical. Without SENTER, or with AMD, APs start in
        // 16-bit mode.  We get to skip that. 
        //printf("\nBSP: _mle_join_start = 0x%08x, _ap_bootstrap_start = 0x%08x",
		//	(u32)_mle_join_start, (u32)_ap_bootstrap_start);
        printf("\nBSP: _ap_bootstrap_blob_mle_join_start = 0x%08x, _ap_bootstrap_blob = 0x%08x",
			(u32)_ap_bootstrap_blob_mle_join_start, (u32)_ap_bootstrap_blob);

        // enable SMIs on BSP before waking APs (which will enable them on APs)
        // because some SMM may take immediate SMI and hang if AP gets in first 
        //printf("Enabling SMIs on BSP\n");
        //__getsec_smctrl();
                
        #ifndef __XMHF_VERIFICATION__
        mle_join = (mle_join_t*)((u32)_ap_bootstrap_blob_mle_join_start - (u32)_ap_bootstrap_blob + 0x10000); // XXX magic number
        #endif
        
        printf("\nBSP: mle_join.gdt_limit = %x", mle_join->gdt_limit);
        printf("\nBSP: mle_join.gdt_base = %x", mle_join->gdt_base);
        printf("\nBSP: mle_join.seg_sel = %x", mle_join->seg_sel);
        printf("\nBSP: mle_join.entry_point = %x", mle_join->entry_point);                

	#ifndef __XMHF_VERIFICATION__
        write_priv_config_reg(TXTCR_MLE_JOIN, (uint64_t)(unsigned long)mle_join);
		
        if (os_sinit_data->capabilities.rlp_wake_monitor) {
            printf("\nBSP: joining RLPs to MLE with MONITOR wakeup");
            printf("\nBSP: rlp_wakeup_addr = 0x%x", sinit_mle_data->rlp_wakeup_addr);
            *((uint32_t *)(unsigned long)(sinit_mle_data->rlp_wakeup_addr)) = 0x01;
        }else {
            printf("\nBSP: joining RLPs to MLE with GETSEC[WAKEUP]");
            __getsec_wakeup();
            printf("\nBSP: GETSEC[WAKEUP] completed");
        }
	#endif

		
	}
	
#else //!__DRT__
        printf("\nBSP: Using APIC to awaken APs...");
        xmhf_baseplatform_arch_x86_wakeupAPs();
        printf("\nBSP: APs should be awake.");

#endif 


}


/*//--debug: dumpVMCS dumps VMCS contents-----------------------------------------
void xmhf_baseplatform_arch_x86vmx_dumpVMCS(VCPU *vcpu){
  		printf("\nGuest State follows:");
		printf("\nguest_CS_selector=0x%04x", (unsigned short)vcpu->vmcs.guest_CS_selector);
		printf("\nguest_DS_selector=0x%04x", (unsigned short)vcpu->vmcs.guest_DS_selector);
		printf("\nguest_ES_selector=0x%04x", (unsigned short)vcpu->vmcs.guest_ES_selector);
		printf("\nguest_FS_selector=0x%04x", (unsigned short)vcpu->vmcs.guest_FS_selector);
		printf("\nguest_GS_selector=0x%04x", (unsigned short)vcpu->vmcs.guest_GS_selector);
		printf("\nguest_SS_selector=0x%04x", (unsigned short)vcpu->vmcs.guest_SS_selector);
		printf("\nguest_TR_selector=0x%04x", (unsigned short)vcpu->vmcs.guest_TR_selector);
		printf("\nguest_LDTR_selector=0x%04x", (unsigned short)vcpu->vmcs.guest_LDTR_selector);
		printf("\nguest_CS_access_rights=0x%08lx", 
			(unsigned long)vcpu->vmcs.guest_CS_access_rights);
		printf("\nguest_DS_access_rights=0x%08lx", 
			(unsigned long)vcpu->vmcs.guest_DS_access_rights);
		printf("\nguest_ES_access_rights=0x%08lx", 
			(unsigned long)vcpu->vmcs.guest_ES_access_rights);
		printf("\nguest_FS_access_rights=0x%08lx", 
			(unsigned long)vcpu->vmcs.guest_FS_access_rights);
		printf("\nguest_GS_access_rights=0x%08lx", 
			(unsigned long)vcpu->vmcs.guest_GS_access_rights);
		printf("\nguest_SS_access_rights=0x%08lx", 
			(unsigned long)vcpu->vmcs.guest_SS_access_rights);
		printf("\nguest_TR_access_rights=0x%08lx", 
			(unsigned long)vcpu->vmcs.guest_TR_access_rights);
		printf("\nguest_LDTR_access_rights=0x%08lx", 
			(unsigned long)vcpu->vmcs.guest_LDTR_access_rights);

		printf("\nguest_CS_base/limit=0x%08lx/0x%04x", 
			(unsigned long)vcpu->vmcs.guest_CS_base, (unsigned short)vcpu->vmcs.guest_CS_limit);
		printf("\nguest_DS_base/limit=0x%08lx/0x%04x", 
			(unsigned long)vcpu->vmcs.guest_DS_base, (unsigned short)vcpu->vmcs.guest_DS_limit);
		printf("\nguest_ES_base/limit=0x%08lx/0x%04x", 
			(unsigned long)vcpu->vmcs.guest_ES_base, (unsigned short)vcpu->vmcs.guest_ES_limit);
		printf("\nguest_FS_base/limit=0x%08lx/0x%04x", 
			(unsigned long)vcpu->vmcs.guest_FS_base, (unsigned short)vcpu->vmcs.guest_FS_limit);
		printf("\nguest_GS_base/limit=0x%08lx/0x%04x", 
			(unsigned long)vcpu->vmcs.guest_GS_base, (unsigned short)vcpu->vmcs.guest_GS_limit);
		printf("\nguest_SS_base/limit=0x%08lx/0x%04x", 
			(unsigned long)vcpu->vmcs.guest_SS_base, (unsigned short)vcpu->vmcs.guest_SS_limit);
		printf("\nguest_GDTR_base/limit=0x%08lx/0x%04x",
			(unsigned long)vcpu->vmcs.guest_GDTR_base, (unsigned short)vcpu->vmcs.guest_GDTR_limit);		
		printf("\nguest_IDTR_base/limit=0x%08lx/0x%04x",
			(unsigned long)vcpu->vmcs.guest_IDTR_base, (unsigned short)vcpu->vmcs.guest_IDTR_limit);		
		printf("\nguest_TR_base/limit=0x%08lx/0x%04x",
			(unsigned long)vcpu->vmcs.guest_TR_base, (unsigned short)vcpu->vmcs.guest_TR_limit);		
		printf("\nguest_LDTR_base/limit=0x%08lx/0x%04x",
			(unsigned long)vcpu->vmcs.guest_LDTR_base, (unsigned short)vcpu->vmcs.guest_LDTR_limit);		

		printf("\nguest_CR0=0x%08lx, guest_CR4=0x%08lx, guest_CR3=0x%08lx",
			(unsigned long)vcpu->vmcs.guest_CR0, (unsigned long)vcpu->vmcs.guest_CR4,
			(unsigned long)vcpu->vmcs.guest_CR3);
		printf("\nguest_RSP=0x%08lx", (unsigned long)vcpu->vmcs.guest_RSP);
		printf("\nguest_RIP=0x%08lx", (unsigned long)vcpu->vmcs.guest_RIP);
		printf("\nguest_RFLAGS=0x%08lx", (unsigned long)vcpu->vmcs.guest_RFLAGS);
}*/



void xcinitbs_arch_initialize_exception_handling(void){
	printf("%s: proceeding to invoke xcexhub_initialize...\n", __FUNCTION__);
	XMHF_SLAB_CALL(xcexhub_initialize());
	printf("%s: xcexhub_initialize completed successfully.\n", __FUNCTION__);
}

//initialize SMP
void xmhf_baseplatform_arch_smpinitialize(void){
	u32 i;
	
	_cpucount = xcbootinfo->cpuinfo_numentries;
	
	//initialize cpu table
	for(i=0; i < _cpucount; i++){
			_cputable[i].cpuid = xcbootinfo->cpuinfo_buffer[i].lapic_id;
			_cputable[i].cpu_index = i;
	}
  
	//save cpu MTRR state which we will later replicate on all APs
	xmhf_baseplatform_arch_x86_savecpumtrrstate();

	//signal that basic base platform data structure initialization is complete 
	//(used by the exception handler component)
	//g_bplt_initiatialized = true;

  //wake up APS
  if(_cpucount > 1){
	  xmhf_baseplatform_arch_x86vmx_wakeupAPs();
  }


  //fall through to common code  
  {
   printf("\nRelinquishing BSP thread and moving to common...");
   // Do some low-level init and then call allcpus_common_start() below
   _ap_pmode_entry_with_paging(); 
   printf("\nBSP must never get here. HALT!");
   HALT();
  }
}


static void _ap_pmode_entry_with_paging(void) __attribute__((naked)){

    asm volatile(	"lgdt %0\r\n"
					"lidt %1\r\n"
					"mov %2, %%ecx\r\n"
					"rdmsr\r\n"
					"andl $0xFFFFF000, %%eax\r\n"
					"addl $0x20, %%eax\r\n"
					"movl (%%eax), %%eax\r\n"
					"shr $24, %%eax\r\n"
					"movl %3, %%edx\r\n"
					"movl %4, %%ebx\r\n"
					"xorl %%ecx, %%ecx\r\n"
					"xorl %%edi, %%edi\r\n"
					"getidxloop:\r\n"
					"movl 0x0(%%ebx, %%edi), %%ebp\r\n"  	//ebp contains the lapic id
					"cmpl %%eax, %%ebp\r\n"
					"jz gotidx\r\n"
					"incl %%ecx\r\n"
					"addl %5, %%edi\r\n"
					"cmpl %%edx, %%ecx\r\n"
					"jb getidxloop\r\n"
					"hlt\r\n"								//we should never get here, if so just halt
					"gotidx:\r\n"							// ecx contains index into g_xc_cputable
					"movl 0x4(%%ebx, %%edi), %%eax\r\n"	 	// eax = g_xc_cputable[ecx].cpu_index
					"movl %6, %%edi \r\n"					// edi = &_cpustack
					"movl %7, %%ecx \r\n"					// ecx = sizeof(_cpustack[0])
					"mull %%ecx \r\n"						// eax = sizeof(_cpustack[0]) * eax
					"addl %%ecx, %%eax \r\n"				// eax = (sizeof(_cpustack[0]) * eax) + sizeof(_cpustack[0])
					"addl %%edi, %%eax \r\n"				// eax = &_cpustack + (sizeof(_cpustack[0]) * eax) + sizeof(_cpustack[0])
					"movl %%eax, %%esp \r\n"				// esp = top of stack for the cpu
					:
					: "m" (_gdt), "m" (_idt), "i" (MSR_APIC_BASE), "m" (_cpucount), "i" (&_cputable), "i" (sizeof(xc_cputable_t)), "i" (&_cpustack), "i" (sizeof(_cpustack[0]))
	);

	xmhf_baseplatform_arch_x86_smpinitialize_commonstart();
	
}

//common function which is entered by all CPUs upon SMP initialization
//note: this is specific to the x86 architecture backend
static void xmhf_baseplatform_arch_x86_smpinitialize_commonstart(void){
	u32 cpuid = xmhf_baseplatform_arch_x86_getcpulapicid();
	bool is_bsp = xmhf_baseplatform_arch_x86_isbsp();
	u32 bcr0;
		
	//initialize base CPU state
	xmhf_baseplatform_arch_x86_cpu_initialize();

	//replicate common MTRR state on this CPU
	xmhf_baseplatform_arch_x86_restorecpumtrrstate();
  	
	//set bit 5 (EM) of CR0 to be VMX compatible in case of Intel cores
	bcr0 = read_cr0();
	bcr0 |= 0x20;
	write_cr0(bcr0);

	//load TR 
	{
	  u32 gdtstart = (u32)xmhf_baseplatform_arch_x86_getgdtbase();
	  u16 trselector = 	__TRSEL;
	  #ifndef __XMHF_VERIFICATION__
	  asm volatile("movl %0, %%edi\r\n"
		"xorl %%eax, %%eax\r\n"
		"movw %1, %%ax\r\n"
		"addl %%eax, %%edi\r\n"		//%edi is pointer to TSS descriptor in GDT
		"addl $0x4, %%edi\r\n"		//%edi points to top 32-bits of 64-bit TSS desc.
		"lock andl $0xFFFF00FF, (%%edi)\r\n"
		"lock orl  $0x00008900, (%%edi)\r\n"
		"ltr %%ax\r\n"				//load TR
	     : 
	     : "m"(gdtstart), "m"(trselector)
	     : "edi", "eax"
	  );
	  #endif
	}


	//if(is_bsp){
		printf("\n%s: cpu %x, isbsp=%u, Proceeding to call init_entry...\n", __FUNCTION__, cpuid, is_bsp);
		XMHF_SLAB_CALL(init_entry(cpuid, is_bsp));
	//}else{
	//	printf("\n%s: cpu %x, isbsp=%u, Halting\n", __FUNCTION__, cpuid, is_bsp);
	//	HALT();
	//}
}



/* originally within xc-initbs-dmaprot-x86vmx.c */

//re-initialize DMA protections (if needed) for the runtime
bool xmhf_dmaprot_arch_reinitialize(void){
	//we don't need to reinitialize DMA protections since we setup
	//VT-d PMRs in the secure loader
	return true;
}





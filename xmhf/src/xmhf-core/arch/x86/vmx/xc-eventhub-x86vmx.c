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

// peh-x86vmx-main.c
// EMHF partition event-hub for Intel x86 vmx
// author: amit vasudevan (amitvasudevan@acm.org)
#include <xmhf-core.h> 
#include <xc-x86.h>
#include <xc-x86vmx.h>

void xmhf_parteventhub_arch_x86vmx_entry(void) __attribute__((naked)){
		//step-1: save all CPU GPRs
		asm volatile ("pushal\r\n");
		
		//step-2: grab xc_cpu_t *
		asm volatile ("movl 32(%esp), %esi\r\n");
			      
		//step-3: get hold of pointer to saved GPR on stack
		asm volatile ("movl %esp, %eax\r\n");

		//step-4: invoke "C" event handler
		//1st argument is xc_cpu_t * followed by pointer to saved GPRs
		asm volatile ("pushl %eax\r\n");
		asm volatile ("pushl %esi\r\n");
		asm volatile ("call xmhf_parteventhub_arch_x86vmx_intercept_handler\r\n");
		asm volatile ("addl $0x08, %esp\r\n");

		//step-5; restore all CPU GPRs
		asm volatile ("popal\r\n");

		//resume partition
		asm volatile ("vmresume\r\n");
              
		//if we get here then vm resume failed, just bail out with a BP exception 
		asm volatile ("int $0x03\r\n");
		asm volatile ("hlt\r\n");
}

/*//---VMX decode assist----------------------------------------------------------
//map a CPU register index into appropriate xc_cpu_t *xc_cpu or struct regs *r field 
//and return the address of the field
static u32 * _vmx_decode_reg(u32 gpr, xc_cpu_t *xc_cpu, struct regs *r){
  if ( ((int)gpr >=0) && ((int)gpr <= 7) ){
  
	  switch(gpr){
		case 0: return ( (u32 *)&r->eax );
		case 1: return ( (u32 *)&r->ecx );
		case 2: return ( (u32 *)&r->edx );
		case 3: return ( (u32 *)&r->ebx );
		case 4: return ( (u32 *)&xc_cpu->vmcs.guest_RSP);
		case 5: return ( (u32 *)&r->ebp );
		case 6: return ( (u32 *)&r->esi );
		case 7: return ( (u32 *)&r->edi );
	  }
   }else{
		printf("\n[%02x]%s: invalid gpr value (%u). halting!", xc_cpu->cpuid,
			__FUNCTION__, gpr);
		HALT();
   }

	//we will never get here, appease the compiler
	return (u32 *)&r->eax;
}*/


static u32 _vmx_getregval(u32 gpr, xc_cpu_t *xc_cpu, struct regs *r){

	  switch(gpr){
		case 0: return r->eax;
		case 1: return r->ecx;
		case 2: return r->edx;
		case 3: return r->ebx;
		case 4: return xmhfhw_cpu_x86vmx_vmread(VMCS_GUEST_RSP);
		case 5: return r->ebp;
		case 6: return r->esi;
		case 7: return r->edi;
		default:
			printf("\n%s: warning, invalid gpr value (%u): returning zero value", __FUNCTION__, gpr);
			return 0;
	}
}




//---intercept handler (CPUID)--------------------------------------------------
static void _vmx_handle_intercept_cpuid(xc_cpu_t *xc_cpu, struct regs *r){
	//printf("\nCPU(0x%02x): CPUID", xc_cpu->cpuid);
	asm volatile ("cpuid\r\n"
          :"=a"(r->eax), "=b"(r->ebx), "=c"(r->ecx), "=d"(r->edx)
          :"a"(r->eax), "c" (r->ecx));
	//xc_cpu->vmcs.guest_RIP += xc_cpu->vmcs.info_vmexit_instruction_length;
	xmhfhw_cpu_x86vmx_vmwrite(VMCS_GUEST_RIP, (xmhfhw_cpu_x86vmx_vmread(VMCS_GUEST_RIP)+xmhfhw_cpu_x86vmx_vmread(VMCS_INFO_VMEXIT_INSTRUCTION_LENGTH)) );
}






//------------------------------------------------------------------------------
// guest MSR r/w intercept handling
// HAL invokes NT kernel via SYSENTER if CPU supports it. However,
// regular apps using NTDLL will still use INT 2E if registry entry is not
// tweaked. So, we HAVE to emulate SYSENTER_CS/EIP/ESP to ensure that
// NT kernel doesnt panic with SESSION5_INITIALIZATION_FAILED!
//
// This took me nearly a month of disassembly into the HAL, 
// NTKERNEL and debugging to figure out..eh? 
//
// AMD SVM is neater, only
// when you ask for these MSR intercepts do they get stored and read from
// the VMCB. However, for Intel regardless they get stored and read from VMCS
// for the guest. So we need to have these intercepts bare minimum!!
// A line to this effect would have been much appreciated in the Intel manuals
// doh!!!
//------------------------------------------------------------------------------
  
//---intercept handler (WRMSR)--------------------------------------------------
static void _vmx_handle_intercept_wrmsr(xc_cpu_t *xc_cpu, struct regs *r){
	//printf("\nCPU(0x%02x): WRMSR 0x%08x", xc_cpu->cpuid, r->ecx);

	switch(r->ecx){
		case IA32_SYSENTER_CS_MSR:
			//xc_cpu->vmcs.guest_SYSENTER_CS = (unsigned int)r->eax;
			xmhfhw_cpu_x86vmx_vmwrite(VMCS_GUEST_SYSENTER_CS, r->eax);
			break;
		case IA32_SYSENTER_EIP_MSR:
			//xc_cpu->vmcs.guest_SYSENTER_EIP = (unsigned long long)r->eax;
			xmhfhw_cpu_x86vmx_vmwrite(VMCS_GUEST_SYSENTER_EIP, r->eax);
			break;
		case IA32_SYSENTER_ESP_MSR:
			//xc_cpu->vmcs.guest_SYSENTER_ESP = (unsigned long long)r->eax;
			xmhfhw_cpu_x86vmx_vmwrite(VMCS_GUEST_SYSENTER_ESP, r->eax);
			break;
		default:{
			asm volatile ("wrmsr\r\n"
          : //no outputs
          :"a"(r->eax), "c" (r->ecx), "d" (r->edx));	
			break;
		}
	}
	
	//xc_cpu->vmcs.guest_RIP += xc_cpu->vmcs.info_vmexit_instruction_length;
	xmhfhw_cpu_x86vmx_vmwrite(VMCS_GUEST_RIP, (xmhfhw_cpu_x86vmx_vmread(VMCS_GUEST_RIP)+xmhfhw_cpu_x86vmx_vmread(VMCS_INFO_VMEXIT_INSTRUCTION_LENGTH)) );
	//printf("\nCPU(0x%02x): WRMSR end", xc_cpu->cpuid);
}

//---intercept handler (RDMSR)--------------------------------------------------
static void _vmx_handle_intercept_rdmsr(xc_cpu_t *xc_cpu, struct regs *r){
	//printf("\nCPU(0x%02x): RDMSR 0x%08x", xc_cpu->cpuid, r->ecx);

	switch(r->ecx){
		case IA32_SYSENTER_CS_MSR:
			//r->eax = (u32)xc_cpu->vmcs.guest_SYSENTER_CS;
			r->eax = xmhfhw_cpu_x86vmx_vmread(VMCS_GUEST_SYSENTER_CS);
			r->edx = 0;
			break;
		case IA32_SYSENTER_EIP_MSR:
			//r->eax = (u32)xc_cpu->vmcs.guest_SYSENTER_EIP;
			r->eax = xmhfhw_cpu_x86vmx_vmread(VMCS_GUEST_SYSENTER_EIP);
			r->edx = 0;
			break;
		case IA32_SYSENTER_ESP_MSR:
			//r->eax = (u32)xc_cpu->vmcs.guest_SYSENTER_ESP;
			r->eax = xmhfhw_cpu_x86vmx_vmread(VMCS_GUEST_SYSENTER_ESP);
			r->edx = 0;
			break;
		default:{
			asm volatile ("rdmsr\r\n"
          : "=a"(r->eax), "=d"(r->edx)
          : "c" (r->ecx));
			break;
		}
	}
	//xc_cpu->vmcs.guest_RIP += xc_cpu->vmcs.info_vmexit_instruction_length;
	xmhfhw_cpu_x86vmx_vmwrite(VMCS_GUEST_RIP, (xmhfhw_cpu_x86vmx_vmread(VMCS_GUEST_RIP)+xmhfhw_cpu_x86vmx_vmread(VMCS_INFO_VMEXIT_INSTRUCTION_LENGTH)) );

	//printf("\nCPU(0x%02x): RDMSR (0x%08x)=0x%08x%08x", xc_cpu->cpuid, r->ecx, r->edx, r->eax);
}


//---intercept handler (EPT voilation)----------------------------------
static void _vmx_handle_intercept_eptviolation(xc_cpu_t *xc_cpu, struct regs *r __attribute__((unused))){
	u32 errorcode, gpa, gva;
	context_desc_t context_desc;
	
	context_desc.partition_desc.partitionid = 0;
	context_desc.cpu_desc.cpuid = xc_cpu->cpuid;
	context_desc.cpu_desc.isbsp = xc_cpu->is_bsp;
	context_desc.cpu_desc.xc_cpu = xc_cpu;

	//xc_cpu_t *xc_cpu = (xc_cpu_t *)&g_bplt_xc_cpu[context_desc.cpu_desc.cpuid];
	//errorcode = (u32)xc_cpu->vmcs.info_exit_qualification;
	errorcode = xmhfhw_cpu_x86vmx_vmread(VMCS_INFO_EXIT_QUALIFICATION);
	//gpa = (u32) xc_cpu->vmcs.guest_paddr_full;
	gpa = xmhfhw_cpu_x86vmx_vmread(VMCS_INFO_GUEST_PADDR_FULL);
	//gva = (u32) xc_cpu->vmcs.info_guest_linear_address;
	gva = xmhfhw_cpu_x86vmx_vmread(VMCS_INFO_GUEST_LINEAR_ADDRESS);

	//check if EPT violation is due to LAPIC interception
	//if(xc_cpu->is_bsp && (gpa >= g_vmx_lapic_base) && (gpa < (g_vmx_lapic_base + PAGE_SIZE_4K)) ){
	//	xmhf_smpguest_arch_x86vmx_eventhandler_hwpgtblviolation(context_desc, gpa, errorcode);
	//}else{ //no, pass it to hypapp 
		xmhf_smpguest_arch_x86vmx_quiesce(xc_cpu);
		xmhfhypapp_handleintercept_hwpgtblviolation(context_desc, gpa, gva,	(errorcode & 7));
		xmhf_smpguest_arch_x86vmx_endquiesce(xc_cpu);
	//}		
}


//---intercept handler (I/O port access)----------------------------------------
static void _vmx_handle_intercept_ioportaccess(xc_cpu_t *xc_cpu, struct regs *r __attribute__((unused))){
  //xc_cpu_t *xc_cpu = (xc_cpu_t *)&g_bplt_xc_cpu[context_desc.cpu_desc.cpuid];
    u32 access_size, access_type, portnum, stringio;
	u32 app_ret_status = APP_IOINTERCEPT_CHAIN;
	context_desc_t context_desc;

	context_desc.partition_desc.partitionid = 0;
	context_desc.cpu_desc.cpuid = xc_cpu->cpuid;
	context_desc.cpu_desc.isbsp = xc_cpu->is_bsp;
	context_desc.cpu_desc.xc_cpu = xc_cpu;
	
	//access_size = (u32)xc_cpu->vmcs.info_exit_qualification & 0x00000007UL;
	//access_type = ((u32)xc_cpu->vmcs.info_exit_qualification & 0x00000008UL) >> 3;
	//portnum =  ((u32)xc_cpu->vmcs.info_exit_qualification & 0xFFFF0000UL) >> 16;
	//stringio = ((u32)xc_cpu->vmcs.info_exit_qualification & 0x00000010UL) >> 4;
	
	access_size = xmhfhw_cpu_x86vmx_vmread(VMCS_INFO_EXIT_QUALIFICATION) & 0x00000007UL;
	access_type = ( xmhfhw_cpu_x86vmx_vmread(VMCS_INFO_EXIT_QUALIFICATION) & 0x00000008UL) >> 3;
	portnum =  ( xmhfhw_cpu_x86vmx_vmread(VMCS_INFO_EXIT_QUALIFICATION) & 0xFFFF0000UL) >> 16;
	stringio = ( xmhfhw_cpu_x86vmx_vmread(VMCS_INFO_EXIT_QUALIFICATION) & 0x00000010UL) >> 4;
	
  HALT_ON_ERRORCOND(!stringio);	//we dont handle string IO intercepts

  //call our app handler, TODO: it should be possible for an app to
  //NOT want a callback by setting up some parameters during appmain
	xmhf_smpguest_arch_x86vmx_quiesce(xc_cpu);
	app_ret_status=xmhfhypapp_handleintercept_portaccess(context_desc, portnum, access_type, access_size);
    xmhf_smpguest_arch_x86vmx_endquiesce(xc_cpu);

  if(app_ret_status == APP_IOINTERCEPT_CHAIN){
   	if(access_type == IO_TYPE_OUT){
  		if( access_size== IO_SIZE_BYTE)
  				outb((u8)r->eax, portnum);
  		else if (access_size == IO_SIZE_WORD)
  				outw((u16)r->eax, portnum);
  		else if (access_size == IO_SIZE_DWORD)
  				outl((u32)r->eax, portnum);	
  	}else{
  		if( access_size== IO_SIZE_BYTE){
  				r->eax &= 0xFFFFFF00UL;	//clear lower 8 bits
  				r->eax |= (u8)inb(portnum);
  		}else if (access_size == IO_SIZE_WORD){
  				r->eax &= 0xFFFF0000UL;	//clear lower 16 bits
  				r->eax |= (u16)inw(portnum);
  		}else if (access_size == IO_SIZE_DWORD){
  				r->eax = (u32)inl(portnum);	
  		}
  	}
  	//xc_cpu->vmcs.guest_RIP += xc_cpu->vmcs.info_vmexit_instruction_length;
  	xmhfhw_cpu_x86vmx_vmwrite(VMCS_GUEST_RIP, (xmhfhw_cpu_x86vmx_vmread(VMCS_GUEST_RIP)+xmhfhw_cpu_x86vmx_vmread(VMCS_INFO_VMEXIT_INSTRUCTION_LENGTH)) );


  }else{
    //skip the IO instruction, app has taken care of it
  	//xc_cpu->vmcs.guest_RIP += xc_cpu->vmcs.info_vmexit_instruction_length;
  	xmhfhw_cpu_x86vmx_vmwrite(VMCS_GUEST_RIP, (xmhfhw_cpu_x86vmx_vmread(VMCS_GUEST_RIP)+xmhfhw_cpu_x86vmx_vmread(VMCS_INFO_VMEXIT_INSTRUCTION_LENGTH)) );

  }

	return;
}


//---CR0 access handler-------------------------------------------------
static void vmx_handle_intercept_cr0access_ug(xc_cpu_t *xc_cpu, struct regs *r, u32 gpr, u32 tofrom){
	u32 cr0_value;
	
	HALT_ON_ERRORCOND(tofrom == VMX_CRX_ACCESS_TO);
	
	//cr0_value = *((u32 *)_vmx_decode_reg(gpr, xc_cpu, r));
	cr0_value = _vmx_getregval(gpr, xc_cpu, r);

	//printf("\n[cr0-%02x] MOV TO, current=0x%08x, proposed=0x%08x", xc_cpu->cpuid,
	//	(u32)xc_cpu->vmcs.guest_CR0, cr0_value);

	//xc_cpu->vmcs.control_CR0_shadow = cr0_value;
	//xc_cpu->vmcs.guest_CR0 = cr0_value & ~(CR0_CD | CR0_NW);
	xmhfhw_cpu_x86vmx_vmwrite(VMCS_CONTROL_CR0_SHADOW, cr0_value);
	xmhfhw_cpu_x86vmx_vmwrite(VMCS_GUEST_CR0, (cr0_value & ~(CR0_CD | CR0_NW)));
	
	//flush mappings
	xmhf_memprot_arch_x86vmx_flushmappings();
}

//---CR4 access handler---------------------------------------------------------
static void vmx_handle_intercept_cr4access_ug(xc_cpu_t *xc_cpu, struct regs *r, u32 gpr, u32 tofrom){
  if(tofrom == VMX_CRX_ACCESS_TO){
	u32 cr4_proposed_value;
	
	//cr4_proposed_value = *((u32 *)_vmx_decode_reg(gpr, xc_cpu, r));
	cr4_proposed_value = _vmx_getregval(gpr, xc_cpu, r);
	
	//printf("\nCPU(0x%02x): CS:EIP=0x%04x:0x%08x MOV CR4, xx", xc_cpu->cpuid,
	//	(u16)xc_cpu->vmcs.guest_CS_selector, (u32)xc_cpu->vmcs.guest_RIP);
  
	//printf("\nMOV TO CR4 (flush TLB?), current=0x%08x, proposed=0x%08x",
	//		(u32)xc_cpu->vmcs.guest_CR4, cr4_proposed_value);

	#if defined (__NESTED_PAGING__)
	//we need to flush EPT mappings as we emulated CR4 load above
	__vmx_invvpid(VMX_INVVPID_SINGLECONTEXT, 1, 0);
	#endif
  }

}

//---XSETBV intercept handler-------------------------------------------
static void _vmx_handle_intercept_xsetbv(xc_cpu_t *xc_cpu, struct regs *r){
	u64 xcr_value;
	
	xcr_value = ((u64)r->edx << 32) + (u64)r->eax;
	
	if(r->ecx != XCR_XFEATURE_ENABLED_MASK){
			printf("\n%s: unhandled XCR register %u", __FUNCTION__, r->ecx);
			HALT();
	}

	//XXX: TODO: check for invalid states and inject GP accordingly
	
	printf("\n%s: xcr_value=%llx", __FUNCTION__, xcr_value);
	
	//XXX: debug: dump CR4 contents
	{
		u32 t_cr4;
		t_cr4 = read_cr4();
		printf("\n%s: host cr4 value=%08x", __FUNCTION__, t_cr4);
	}
	
	//set XCR with supplied value
	xsetbv(XCR_XFEATURE_ENABLED_MASK, xcr_value);

	//skip the emulated XSETBV instruction
	//xc_cpu->vmcs.guest_RIP += xc_cpu->vmcs.info_vmexit_instruction_length;
  	xmhfhw_cpu_x86vmx_vmwrite(VMCS_GUEST_RIP, (xmhfhw_cpu_x86vmx_vmread(VMCS_GUEST_RIP)+xmhfhw_cpu_x86vmx_vmread(VMCS_INFO_VMEXIT_INSTRUCTION_LENGTH)) );
}						
			


//---hvm_intercept_handler------------------------------------------------------
u32 xmhf_parteventhub_arch_x86vmx_intercept_handler(xc_cpu_t *xc_cpu, struct regs *r){

	//sanity check for VM-entry errors
	if( xmhfhw_cpu_x86vmx_vmread(VMCS_INFO_VMEXIT_REASON) & 0x80000000UL ){
		printf("\nVM-ENTRY error: reason=0x%08x, qualification=0x%016llx", 
			xmhfhw_cpu_x86vmx_vmread(VMCS_INFO_VMEXIT_REASON), xmhfhw_cpu_x86vmx_vmread(VMCS_INFO_EXIT_QUALIFICATION));
		HALT();
	}
  
	//handle intercepts
	switch(xmhfhw_cpu_x86vmx_vmread(VMCS_INFO_VMEXIT_REASON)){
		//--------------------------------------------------------------
		//xmhf-core and hypapp intercepts
		//--------------------------------------------------------------
		
		case VMX_VMEXIT_VMCALL:{
			//if INT 15h E820 hypercall, then let the xmhf-core handle it
			if(xmhfhw_cpu_x86vmx_vmread(VMCS_GUEST_CS_BASE) == (VMX_UG_E820HOOK_CS << 4) &&
				xmhfhw_cpu_x86vmx_vmread(VMCS_GUEST_RIP) == VMX_UG_E820HOOK_IP){
					context_desc_t context_desc;
	
					context_desc.partition_desc.partitionid = 0;
					context_desc.cpu_desc.cpuid = xc_cpu->cpuid;
					context_desc.cpu_desc.isbsp = xc_cpu->is_bsp;
					context_desc.cpu_desc.xc_cpu = xc_cpu;
	
				//we need to be either in real-mode or in protected
				//mode with paging and EFLAGS.VM bit set (virtual-8086 mode)
				HALT_ON_ERRORCOND( !(xmhfhw_cpu_x86vmx_vmread(VMCS_GUEST_CR0) & CR0_PE)  ||
					( (xmhfhw_cpu_x86vmx_vmread(VMCS_GUEST_CR0) & CR0_PE) && (xmhfhw_cpu_x86vmx_vmread(VMCS_GUEST_CR0) & CR0_PG) &&
						(xmhfhw_cpu_x86vmx_vmread(VMCS_GUEST_RFLAGS) & EFLAGS_VM)  ) );
				xmhf_smpguest_arch_x86vmx_handle_guestmemoryreporting(context_desc, r);
				
			}else{	//if not E820 hook, give hypapp a chance to handle the hypercall
				xmhf_smpguest_arch_x86vmx_quiesce(xc_cpu);
				{
					u64 hypercall_id = (u64)r->eax;
					u64 hypercall_param = ((u64)r->edx << 32) | r->ecx;
					context_desc_t context_desc;
	
					context_desc.partition_desc.partitionid = 0;
					context_desc.cpu_desc.cpuid = xc_cpu->cpuid;
					context_desc.cpu_desc.isbsp = xc_cpu->is_bsp;
					context_desc.cpu_desc.xc_cpu = xc_cpu;
	
					if( xmhfhypapp_handlehypercall(context_desc, hypercall_id, hypercall_param) != APP_SUCCESS){
						printf("\nCPU(0x%02x): error(halt), unhandled hypercall 0x%08x!", xc_cpu->cpuid, r->eax);
						HALT();
					}
				}
				xmhf_smpguest_arch_x86vmx_endquiesce(xc_cpu);
				xmhfhw_cpu_x86vmx_vmwrite(VMCS_GUEST_RIP, (xmhfhw_cpu_x86vmx_vmread(VMCS_GUEST_RIP)+3) );
			}
		}
		break;

		case VMX_VMEXIT_IOIO:{
			_vmx_handle_intercept_ioportaccess(xc_cpu, r);
		}
		break;

		case VMX_VMEXIT_EPT_VIOLATION:{
			_vmx_handle_intercept_eptviolation(xc_cpu, r);
		}
		break;  

		case VMX_VMEXIT_INIT:{
			context_desc_t context_desc;
	
			context_desc.partition_desc.partitionid = 0;
			context_desc.cpu_desc.cpuid = xc_cpu->cpuid;
			context_desc.cpu_desc.isbsp = xc_cpu->is_bsp;
			context_desc.cpu_desc.xc_cpu = xc_cpu;

			printf("\n***** VMEXIT_INIT xmhfhypapp_handleshutdown\n");
			xmhfhypapp_handleshutdown(context_desc);      
			printf("\nCPU(0x%02x): Fatal, xmhfhypapp_handleshutdown returned. Halting!", xc_cpu->cpuid);
			HALT();
		}
		break;

		//--------------------------------------------------------------
		//xmhf-core only intercepts
		//--------------------------------------------------------------

 		case VMX_VMEXIT_EXCEPTION:{
			switch( ( xmhfhw_cpu_x86vmx_vmread(VMCS_INFO_VMEXIT_INTERRUPT_INFORMATION) & INTR_INFO_VECTOR_MASK) ){
	
				case 0x02:	//NMI
					#ifndef __XMHF_VERIFICATION__
					//we currently discharge quiescing via manual inspection
					xmhf_smpguest_arch_x86vmx_eventhandler_nmiexception(xc_cpu, r);
					#endif // __XMHF_VERIFICATION__
					break;
				
				default:
					printf("\nVMEXIT-EXCEPTION:");
					printf("\ncontrol_exception_bitmap=0x%08lx",
						xmhfhw_cpu_x86vmx_vmread(VMCS_CONTROL_EXCEPTION_BITMAP));
					printf("\ninterruption information=0x%08lx", 
						xmhfhw_cpu_x86vmx_vmread(VMCS_INFO_VMEXIT_INTERRUPT_INFORMATION));
					printf("\nerrorcode=0x%08lx", 
						xmhfhw_cpu_x86vmx_vmread(VMCS_INFO_VMEXIT_INTERRUPT_ERROR_CODE));
					HALT();
			}
		}
		break;

 		case VMX_VMEXIT_CRX_ACCESS:{
			u32 tofrom, gpr, crx; 
			crx=(u32) ((u64)xmhfhw_cpu_x86vmx_vmread(VMCS_INFO_EXIT_QUALIFICATION) & 0x000000000000000FULL);
			gpr=
			 (u32) (((u64)xmhfhw_cpu_x86vmx_vmread(VMCS_INFO_EXIT_QUALIFICATION) & 0x0000000000000F00ULL) >> (u64)8);
			tofrom = 
			(u32) (((u64)xmhfhw_cpu_x86vmx_vmread(VMCS_INFO_EXIT_QUALIFICATION) & 0x0000000000000030ULL) >> (u64)4); 

			if ( ((int)gpr >=0) && ((int)gpr <= 7) ){
				switch(crx){
					case 0x0: //CR0 access
						vmx_handle_intercept_cr0access_ug(xc_cpu, r, gpr, tofrom);	
						break;
					
					case 0x4: //CR4 access
						vmx_handle_intercept_cr4access_ug(xc_cpu, r, gpr, tofrom);	
						break;
				
					default:
						printf("\nunhandled crx, halting!");
						HALT();
				}
			  	xmhfhw_cpu_x86vmx_vmwrite(VMCS_GUEST_RIP, (xmhfhw_cpu_x86vmx_vmread(VMCS_GUEST_RIP)+xmhfhw_cpu_x86vmx_vmread(VMCS_INFO_VMEXIT_INSTRUCTION_LENGTH)) );

			}else{
				printf("\n[%02x]%s: invalid gpr value (%u). halting!", xc_cpu->cpuid,
					__FUNCTION__, gpr);
				HALT();
			}
		}
		break;	

 		case VMX_VMEXIT_RDMSR:
			_vmx_handle_intercept_rdmsr(xc_cpu, r);
			break;
			
		case VMX_VMEXIT_WRMSR:
			_vmx_handle_intercept_wrmsr(xc_cpu, r);
			break;
			
		case VMX_VMEXIT_CPUID:
			_vmx_handle_intercept_cpuid(xc_cpu, r);
			break;

		case VMX_VMEXIT_TASKSWITCH:{
			u32 idt_v = xmhfhw_cpu_x86vmx_vmread(VMCS_INFO_IDT_VECTORING_INFORMATION) & VECTORING_INFO_VALID_MASK;
			u32 type = xmhfhw_cpu_x86vmx_vmread(VMCS_INFO_IDT_VECTORING_INFORMATION) & VECTORING_INFO_TYPE_MASK;
			u32 reason = xmhfhw_cpu_x86vmx_vmread(VMCS_INFO_EXIT_QUALIFICATION) >> 30;
			u16 tss_selector = (u16)xmhfhw_cpu_x86vmx_vmread(VMCS_INFO_EXIT_QUALIFICATION);
			context_desc_t context_desc;
	
			context_desc.partition_desc.partitionid = 0;
			context_desc.cpu_desc.cpuid = xc_cpu->cpuid;
			context_desc.cpu_desc.isbsp = xc_cpu->is_bsp;
			context_desc.cpu_desc.xc_cpu = xc_cpu;
			
			if(reason == TASK_SWITCH_GATE && type == INTR_TYPE_NMI){
				printf("\nCPU(0x%02x): NMI received (MP guest shutdown?)", xc_cpu->cpuid);
				xmhfhypapp_handleshutdown(context_desc);      
				printf("\nCPU(0x%02x): warning, xmhfhypapp_handleshutdown returned!", xc_cpu->cpuid);
				printf("\nCPU(0x%02x): HALTING!", xc_cpu->cpuid);
				HALT();
			}else{
				printf("\nCPU(0x%02x): Unhandled Task Switch. Halt!", xc_cpu->cpuid);
				printf("\n	idt_v=0x%08x, type=0x%08x, reason=0x%08x, tsssel=0x%04x",
					idt_v, type, reason, tss_selector); 
			}
			HALT();
		}
		break;

		case VMX_VMEXIT_XSETBV:{
			_vmx_handle_intercept_xsetbv(xc_cpu, r);
		}
		break;

		case VMX_VMEXIT_SIPI:{
			u32 sipivector = (u8)xmhfhw_cpu_x86vmx_vmread(VMCS_INFO_EXIT_QUALIFICATION);
			printf("\nCPU(%02x): SIPI vector=0x%08x", xc_cpu->cpuid, sipivector);
			xmhfhw_cpu_x86vmx_vmwrite(VMCS_GUEST_CS_SELECTOR, ((sipivector * PAGE_SIZE_4K) >> 4));
			xmhfhw_cpu_x86vmx_vmwrite(VMCS_GUEST_CS_BASE, (sipivector * PAGE_SIZE_4K));
			xmhfhw_cpu_x86vmx_vmwrite(VMCS_GUEST_RIP, 0x0ULL);
			xmhfhw_cpu_x86vmx_vmwrite(VMCS_GUEST_ACTIVITY_STATE, 0);	//active
		}
		break;

    
		default:{
			printf("\nCPU(0x%02x): Unhandled intercept: 0x%08x", xc_cpu->cpuid, (u32)xmhfhw_cpu_x86vmx_vmread(VMCS_INFO_VMEXIT_REASON));
			printf("\n	CPU(0x%02x): EFLAGS=0x%08x", xc_cpu->cpuid, (u32)xmhfhw_cpu_x86vmx_vmread(VMCS_GUEST_RFLAGS));
			printf("\n	SS:ESP =0x%04x:0x%08x", (u16)xmhfhw_cpu_x86vmx_vmread(VMCS_GUEST_SS_SELECTOR), (u32)xmhfhw_cpu_x86vmx_vmread(VMCS_GUEST_RSP));
			printf("\n	CS:EIP =0x%04x:0x%08x", (u16)xmhfhw_cpu_x86vmx_vmread(VMCS_GUEST_CS_SELECTOR), (u32)xmhfhw_cpu_x86vmx_vmread(VMCS_GUEST_RIP));
			printf("\n	IDTR base:limit=0x%08x:0x%04x", (u32)xmhfhw_cpu_x86vmx_vmread(VMCS_GUEST_IDTR_BASE),
					(u16)xmhfhw_cpu_x86vmx_vmread(VMCS_GUEST_IDTR_LIMIT));
			printf("\n	GDTR base:limit=0x%08x:0x%04x", (u32)xmhfhw_cpu_x86vmx_vmread(VMCS_GUEST_GDTR_BASE),
					(u16)xmhfhw_cpu_x86vmx_vmread(VMCS_GUEST_GDTR_LIMIT));
			if(xmhfhw_cpu_x86vmx_vmread(VMCS_INFO_IDT_VECTORING_INFORMATION) & 0x80000000){
				printf("\nCPU(0x%02x): HALT; Nested events unhandled 0x%08x",
					xc_cpu->cpuid, xmhfhw_cpu_x86vmx_vmread(VMCS_INFO_IDT_VECTORING_INFORMATION));
			}
			HALT();
		}		
	} //end switch((u32)xc_cpu->vmcs.info_vmexit_reason)
	

 	//check and clear guest interruptibility state
	if(xmhfhw_cpu_x86vmx_vmread(VMCS_GUEST_INTERRUPTIBILITY) != 0){
		xmhfhw_cpu_x86vmx_vmwrite(VMCS_GUEST_INTERRUPTIBILITY, 0);
	}

	//make sure we have no nested events
	if(xmhfhw_cpu_x86vmx_vmread(VMCS_INFO_IDT_VECTORING_INFORMATION) & 0x80000000){
		printf("\nCPU(0x%02x): HALT; Nested events unhandled with hwp:0x%08x",
			xc_cpu->cpuid, xmhfhw_cpu_x86vmx_vmread(VMCS_INFO_IDT_VECTORING_INFORMATION));
		HALT();
	}

#ifdef __XMHF_VERIFICATION_DRIVEASSERTS__
	//ensure that whenever a partition is resumed on a xc_cpu, we have extended paging
	//enabled and that the base points to the extended page tables we have initialized
	assert( (xc_cpu->vmcs.control_VMX_seccpu_based & 0x2) );
	assert( (xc_cpu->vmcs.control_EPT_pointer_high == 0) && (xc_cpu->vmcs.control_EPT_pointer_full == (hva2spa((void*)xc_cpu->vmx_vaddr_ept_pml4_table) | 0x1E)) );
#endif	

	return 1;
}

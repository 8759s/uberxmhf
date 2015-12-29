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

#include <geec_prime.h>

/*@
	requires 0 <= slabid < XMHFGEEC_TOTAL_SLABS;
	requires (paddr_end >= paddr_start);
	requires (paddr_end < (0xFFFFFFFFUL - PAGE_SIZE_2M));
	requires (paddr_end - paddr_start) <= MAX_SLAB_DMADATA_SIZE;

@*/
static void gp_s2_sdasetupdevpgtbl_splintpdt(u32 slabid, u32 paddr_start, u32 paddr_end){
	u32 paddr;
	u32 pd_index=0;


    	/*@
		loop invariant a1: paddr_start <= paddr <= (paddr_end+PAGE_SIZE_2M);
		loop assigns paddr;
		loop assigns pd_index;
		loop variant (paddr_end+PAGE_SIZE_2M)-paddr;
	@*/
	for(paddr = paddr_start; paddr < paddr_end; paddr+= PAGE_SIZE_2M){
		if(pd_index >= VTD_PTRS_PER_PDT){
			CASM_FUNCCALL(xmhfhw_cpu_hlt, CASM_NOPARAM);
		}else{
			/*//grab index of pdpt, pdt this paddr
			u32 pdpt_index = pae_get_pdpt_index(paddr);
			u32 pdt_index = pae_get_pdt_index(paddr);

			//stick a pt for the pdt entry
			_slabdevpgtbl_pdt[slabid][pdpt_index][pdt_index] =
			    vtd_make_pdte((u64)_slabdevpgtbl_pt[slabid][pd_index], (VTD_PAGE_READ | VTD_PAGE_WRITE));

			//populate pt entries for this 2M range
			gp_s2_sdasetupdevpgtbl_setptentries(slabid, pd_index, paddr);
			*/

			pd_index++;
		}
	}


}

#if 0
void gp_s2_sdasetupdevpgtbl(u32 slabid){
	u32 i;

	if( (xmhfgeec_slab_info_table[slabid].slab_physmem_extents[3].addr_end - xmhfgeec_slab_info_table[slabid].slab_physmem_extents[3].addr_start) >
		MAX_SLAB_DMADATA_SIZE ){
		_XDPRINTF_("%s: Error: slab %u dmadata section over limit. bailing out!\n",
			   __func__, slabid);
		_slabdevpgtbl_infotable[slabid].devpgtbl_initialized = false;
		CASM_FUNCCALL(xmhfhw_cpu_hlt, CASM_NOPARAM);
	}else{

		//initialize lvl1 page table (pml4t)
		memset(&_slabdevpgtbl_pml4t[slabid], 0, sizeof(_slabdevpgtbl_pml4t[0]));
		_slabdevpgtbl_pml4t[slabid][0] =
		vtd_make_pml4te((u64)_slabdevpgtbl_pdpt[slabid], (VTD_PAGE_READ | VTD_PAGE_WRITE));

		//initialize lvl2 page table (pdpt)
		memset(&_slabdevpgtbl_pdpt[slabid], 0, sizeof(_slabdevpgtbl_pdpt[0]));
		for(i=0; i < VTD_PTRS_PER_PDPT; i++){
		_slabdevpgtbl_pdpt[slabid][i] =
		    vtd_make_pdpte((u64)_slabdevpgtbl_pdt[slabid][i], (VTD_PAGE_READ | VTD_PAGE_WRITE));
		}


		gp_s2_sdasetupdevpgtbl_splintpdt(slabid, xmhfgeec_slab_info_table[slabid].slab_physmem_extents[3].addr_start,
						xmhfgeec_slab_info_table[slabid].slab_physmem_extents[3].addr_end);
		_slabdevpgtbl_infotable[slabid].devpgtbl_initialized = true;
	}

}
#endif

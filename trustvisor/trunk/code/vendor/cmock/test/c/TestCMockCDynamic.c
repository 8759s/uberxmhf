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
 * This file is part of the EMHF historical reference
 * codebase, and is released under the terms of the
 * GNU General Public License (GPL) version 2.
 * Please see the LICENSE file for details.
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

/* ==========================================
    CMock Project - Automatic Mock Generation for C
    Copyright (c) 2007 Mike Karlesky, Mark VanderVoord, Greg Williams
    [Released under MIT License. Please refer to license.txt for details]
========================================== */

#include "unity.h"
#include "cmock.h"

#define TEST_MEM_INDEX_SIZE  (sizeof(CMOCK_MEM_INDEX_TYPE)) 
#define TEST_MEM_INDEX_PAD   ((sizeof(CMOCK_MEM_INDEX_TYPE) + 7) & ~7) //round up to nearest 4 byte boundary

unsigned int StartingSize;

void setUp(void)
{
  CMock_Guts_MemFreeAll();
  StartingSize = CMock_Guts_MemBytesFree();
  TEST_ASSERT_EQUAL(0, CMock_Guts_MemBytesUsed());
}

void tearDown(void)
{
}

void test_MemNewWillReturnNullIfGivenIllegalSizes(void)
{
  TEST_ASSERT_NULL( CMock_Guts_MemNew(0) );

  //verify we're cleared still
  TEST_ASSERT_EQUAL(0, CMock_Guts_MemBytesUsed());
  TEST_ASSERT_EQUAL(0, CMock_Guts_MemBytesFree());
}

void test_MemNewWillNowSupportSizesGreaterThanTheDefinesCMockSize(void)
{
    TEST_ASSERT_EQUAL(0, CMock_Guts_MemBytesFree());
	
    TEST_ASSERT_NOT_NULL(CMock_Guts_MemNew(CMOCK_MEM_SIZE - TEST_MEM_INDEX_SIZE + 1) );

    TEST_ASSERT_EQUAL(CMOCK_MEM_SIZE + TEST_MEM_INDEX_PAD, CMock_Guts_MemBytesUsed());
    TEST_ASSERT_EQUAL(CMOCK_MEM_SIZE, CMock_Guts_MemBytesFree());
}

void test_MemChainWillReturnNullAndDoNothingIfGivenIllegalInformation(void)
{
  unsigned int* next = CMock_Guts_MemNew(8);
  TEST_ASSERT_EQUAL(8 + TEST_MEM_INDEX_PAD, CMock_Guts_MemBytesUsed());
  TEST_ASSERT_EQUAL(StartingSize - 8 - TEST_MEM_INDEX_PAD, CMock_Guts_MemBytesFree());

  TEST_ASSERT_NULL( CMock_Guts_MemChain((void*)((unsigned int)next + CMOCK_MEM_SIZE), next) );
  TEST_ASSERT_NULL( CMock_Guts_MemChain(next, (void*)((unsigned int)next + CMOCK_MEM_SIZE)) );

  //verify we're still the same
  TEST_ASSERT_EQUAL(8 + TEST_MEM_INDEX_PAD, CMock_Guts_MemBytesUsed());
  TEST_ASSERT_EQUAL(StartingSize - 8 - TEST_MEM_INDEX_PAD, CMock_Guts_MemBytesFree());
}

void test_MemNextWillReturnNullIfGivenABadRoot(void)
{
  TEST_ASSERT_NULL( CMock_Guts_MemNext(NULL) );
  TEST_ASSERT_NULL( CMock_Guts_MemNext((void*)2) );
  TEST_ASSERT_NULL( CMock_Guts_MemNext((void*)0xFFFFFFFE) );

  //verify we're cleared still
  TEST_ASSERT_EQUAL(0, CMock_Guts_MemBytesUsed());
  TEST_ASSERT_EQUAL(StartingSize, CMock_Guts_MemBytesFree());
}

void test_ThatWeCanClaimAndChainAFewElementsTogether(void)
{
  unsigned int  i;
  unsigned int* first = NULL;
  unsigned int* next;
  unsigned int* element[4];

  //verify we're cleared first
  TEST_ASSERT_EQUAL(0, CMock_Guts_MemBytesUsed());
  TEST_ASSERT_EQUAL(StartingSize, CMock_Guts_MemBytesFree());

  //first element
  element[0] = CMock_Guts_MemNew(sizeof(unsigned int));
  TEST_ASSERT_NOT_NULL(element[0]);
  first = CMock_Guts_MemChain(first, element[0]);
  TEST_ASSERT_EQUAL_PTR(element[0], first);
  *element[0] = 0;

  //verify we're using the right amount of memory
  TEST_ASSERT_EQUAL(1 * (TEST_MEM_INDEX_PAD + 8), CMock_Guts_MemBytesUsed());
  TEST_ASSERT_EQUAL(StartingSize - 1 * (TEST_MEM_INDEX_PAD + 8), CMock_Guts_MemBytesFree());

  //second element
  element[1] = CMock_Guts_MemNew(sizeof(unsigned int));
  TEST_ASSERT_NOT_NULL(element[1]);
  TEST_ASSERT_NOT_EQUAL(element[0], element[1]);
  TEST_ASSERT_EQUAL_PTR(first, CMock_Guts_MemChain(first, element[1]));
  *element[1] = 1;

  //verify we're using the right amount of memory
  TEST_ASSERT_EQUAL(2 * (TEST_MEM_INDEX_PAD + 8), CMock_Guts_MemBytesUsed());
  TEST_ASSERT_EQUAL(StartingSize - 2 * (TEST_MEM_INDEX_PAD + 8), CMock_Guts_MemBytesFree());

  //third element
  element[2] = CMock_Guts_MemNew(sizeof(unsigned int));
  TEST_ASSERT_NOT_NULL(element[2]);
  TEST_ASSERT_NOT_EQUAL(element[0], element[2]);
  TEST_ASSERT_NOT_EQUAL(element[1], element[2]);
  TEST_ASSERT_EQUAL_PTR(first, CMock_Guts_MemChain(first, element[2]));
  *element[2] = 2;

  //verify we're using the right amount of memory
  TEST_ASSERT_EQUAL(3 * (TEST_MEM_INDEX_PAD + 8), CMock_Guts_MemBytesUsed());
  TEST_ASSERT_EQUAL(StartingSize - 3 * (TEST_MEM_INDEX_PAD + 8), CMock_Guts_MemBytesFree());

  //fourth element
  element[3] = CMock_Guts_MemNew(sizeof(unsigned int));
  TEST_ASSERT_NOT_NULL(element[3]);
  TEST_ASSERT_NOT_EQUAL(element[0], element[3]);
  TEST_ASSERT_NOT_EQUAL(element[1], element[3]);
  TEST_ASSERT_NOT_EQUAL(element[2], element[3]);
  TEST_ASSERT_EQUAL_PTR(first, CMock_Guts_MemChain(first, element[3]));
  *element[3] = 3;

  //verify we're using the right amount of memory
  TEST_ASSERT_EQUAL(4 * (TEST_MEM_INDEX_PAD + 8), CMock_Guts_MemBytesUsed());
  TEST_ASSERT_EQUAL(StartingSize - 4 * (TEST_MEM_INDEX_PAD + 8), CMock_Guts_MemBytesFree());

  //traverse list
  next = first;
  for (i = 0; i < 4; i++)
  {
    TEST_ASSERT_EQUAL_PTR(element[i], next);
    TEST_ASSERT_EQUAL(i, *next);
    next = CMock_Guts_MemNext(next);
  }

  //verify we get a null at the end of the list
  TEST_ASSERT_NULL(next);

  //verify we're using the right amount of memory
  TEST_ASSERT_EQUAL(4 * (TEST_MEM_INDEX_PAD + 8), CMock_Guts_MemBytesUsed());
  TEST_ASSERT_EQUAL(StartingSize - 4 * (TEST_MEM_INDEX_PAD + 8), CMock_Guts_MemBytesFree());

  //Free it all
  CMock_Guts_MemFreeAll();

  //verify we're cleared
  TEST_ASSERT_EQUAL(0, CMock_Guts_MemBytesUsed());
  TEST_ASSERT_EQUAL(StartingSize, CMock_Guts_MemBytesFree());
}

void test_ThatWeCanAskForAllSortsOfSizes(void)
{
  unsigned int  i;
  unsigned int* first = NULL;
  unsigned int* next;
  unsigned int  sizes[10]          = {3,  1,  80,  5,  8,  31, 7,  911, 2,  80};
  unsigned int  sizes_buffered[10] = {16, 16, 88,  16, 16, 40, 16, 920, 16, 88}; //includes counter
  unsigned int  sum = 0;
  unsigned int  cap;

  for (i = 0; i < 10; i++)
  {
    next = CMock_Guts_MemNew(sizes[i]);
    TEST_ASSERT_NOT_NULL(next);

    first = CMock_Guts_MemChain(first, next);
    TEST_ASSERT_NOT_NULL(first);

    sum += sizes_buffered[i];
    cap = (StartingSize > (sum + CMOCK_MEM_SIZE)) ? StartingSize : (sum + CMOCK_MEM_SIZE);
    TEST_ASSERT_EQUAL(sum, CMock_Guts_MemBytesUsed());
    TEST_ASSERT(cap >= CMock_Guts_MemBytesFree());
  }

  //verify we can still walk through the elements allocated
  next = first;
  for (i = 0; i < 10; i++)
  {
    TEST_ASSERT_NOT_NULL(next);
    next = CMock_Guts_MemNext(next);
  }

  //there aren't any after that
  TEST_ASSERT_NULL(next);
}

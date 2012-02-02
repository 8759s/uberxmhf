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

#ifndef EUCHK_H
#define EUCHK_H

#include <eulog.h>

#define EU_CHK_PRI(cond, priority, args...)             \
  do {                                                  \
    if (!(cond)) {                                      \
      EULOG(priority, "EU_CHK failed: %s", #cond);      \
      (void)0, ## args;                                 \
      goto out;                                         \
    }                                                   \
  } while(0)

#define EU_CHK_T(cond, args...) EU_CHK_PRI(cond, EUTRACE, ## args)
#define EU_CHK_W(cond, args...) EU_CHK_PRI(cond, EUWARN, ## args)
#define EU_CHK_E(cond, args...) EU_CHK_PRI(cond, EUERR, ## args)

#define EU_CHK EU_CHK_E

/* use like assert, but where arg will always be expanded and the
   check never disabled */
#define EU_VERIFY(cond)                         \
  do {                                          \
    if (!(cond)) {                              \
      euerr("EU_VERIFY failed: %s", #cond);     \
      abort();                                  \
    }                                           \
  }

#endif

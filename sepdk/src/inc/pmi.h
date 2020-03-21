/*COPYRIGHT**
    Copyright (C) 2005-2020 Intel Corporation.  All Rights Reserved.

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.






**COPYRIGHT*/






#ifndef _PMI_H_
#define _PMI_H_

#include "lwpmudrv_defines.h"
#include <linux/ptrace.h>
#include <linux/version.h>

#if defined(DRV_IA32)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,25)
#define REGS_xcs(regs)       regs->xcs
#define REGS_eip(regs)       regs->eip
#define REGS_eflags(regs)    regs->eflags
#else
#define REGS_xcs(regs)       regs->cs
#define REGS_eip(regs)       regs->ip
#define REGS_eflags(regs)    regs->flags
#endif
#endif

#if defined(DRV_EM64T)
#define REGS_cs(regs)        regs->cs

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,25)
#define REGS_rip(regs)       regs->rip
#define REGS_eflags(regs)    regs->eflags
#else
#define REGS_rip(regs)       regs->ip
#define REGS_eflags(regs)    regs->flags
#endif
#endif

asmlinkage VOID PMI_Interrupt_Handler(struct pt_regs *regs);

#endif  


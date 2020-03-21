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





#ifndef _LINUXOS_H_
#define _LINUXOS_H_

// defines for options parameter of samp_load_image_notify_routine()
#define LOPTS_1ST_MODREC     0x1
#define LOPTS_GLOBAL_MODULE  0x2
#define LOPTS_EXE            0x4

#define FOR_EACH_TASK        for_each_process
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,19,00)
#define DRV_F_DENTRY f_path.dentry
#else
#define DRV_F_DENTRY    f_dentry
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,25)
#define D_PATH(vm_file, name, maxlen)     \
    d_path((vm_file)->f_dentry, (vm_file)->f_vfsmnt, (name), (maxlen))
#else
#define D_PATH(vm_file, name, maxlen)     \
    d_path(&((vm_file)->f_path), (name), (maxlen))
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0)
#define DRV_VM_MOD_EXECUTABLE(vma)    \
    (vma->vm_flags & VM_EXECUTABLE)
#else
#define DRV_VM_MOD_EXECUTABLE(vma)    \
    (linuxos_Equal_VM_Exe_File(vma))
#define DRV_MM_EXE_FILE_PRESENT
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
#define DRV_ALLOW_VDSO
#endif

#if defined(DRV_IA32)
#define FIND_VMA(mm, data)   find_vma ((mm), (U32)(data));
#endif
#if defined(DRV_EM64T)
#define FIND_VMA(mm, data)   find_vma ((mm), (U64)(data));
#endif

extern VOID
LINUXOS_Install_Hooks (
    VOID
);

extern VOID
LINUXOS_Uninstall_Hooks (
    VOID
);

extern OS_STATUS
LINUXOS_Enum_Process_Modules (
    DRV_BOOL at_end
);

extern DRV_BOOL
LINUXOS_Check_KVM_Guest_Process (
    VOID
);
#if defined(DRV_CPU_HOTPLUG)
extern VOID
LINUXOS_Register_Hotplug(
    VOID
);

extern VOID
LINUXOS_Unregister_Hotplug(
    VOID
);
#endif
#endif


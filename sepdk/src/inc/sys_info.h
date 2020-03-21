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






#ifndef _SYS_INFO_H_
#define _SYS_INFO_H_

#include "lwpmudrv_defines.h"

#define  KNIGHTS_FAMILY     0x06
#define  KNL_MODEL          0x57
#define  KNM_MODEL          0x85

#define is_Knights_family(family, model)      ((family==KNIGHTS_FAMILY)&&((model==KNL_MODEL)||(model==KNM_MODEL)))

typedef struct __generic_ioctl {
    U32    size;
    S32    ret;
    U64    rsv[3];
} GENERIC_IOCTL;

#define GENERIC_IOCTL_size(gio)     (gio)->size
#define GENERIC_IOCTL_ret(gio)      (gio)->ret

//
// This one is unusual in that it's really a variable
// size. The system_info field is just a easy way
// to access the base information, but the actual size
// when used tends to be much larger that what is 
// shown here.
//
typedef struct __system_info {
    GENERIC_IOCTL gen;
    VTSA_SYS_INFO sys_info;
} IOCTL_SYS_INFO;

#define  IOCTL_SYS_INFO_gen(isi)         (isi)->gen
#define  IOCTL_SYS_INFO_sys_info(isi)    (isi)->sys_info

extern  U32   SYS_INFO_Build (void);
extern  void  SYS_INFO_Transfer (PVOID buf_usr_to_drv, unsigned long len_usr_to_drv);
extern  void  SYS_INFO_Destroy (void);
extern  void  SYS_INFO_Build_Cpu (PVOID param);

#endif


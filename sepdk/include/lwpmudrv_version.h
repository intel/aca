/****
    Copyright (C) 2019-2020 Intel Corporation.  All Rights Reserved.

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






****/

/*
 *  File  : lwpmudrv_version.h
 */

#ifndef _LWPMUDRV_VERSION_H_
#define _LWPMUDRV_VERSION_H_

#define _STRINGIFY(x)     #x
#define STRINGIFY(x)      _STRINGIFY(x)
#define _STRINGIFY_W(x)   L#x
#define STRINGIFY_W(x)    _STRINGIFY_W(x)

#define SEP_MAJOR_VERSION   5
#define SEP_MINOR_VERSION   17
#define SEP_API_VERSION     5 // API version is independent of major/minor and tracks driver version

#define SEP_PREV_MAJOR_VERSION   5
#define EMON_PREV_MAJOR_VERSION  11

#define SEP_RELEASE_STRING  "Beta"

#define EMON_MAJOR_VERSION          11
#define EMON_MINOR_VERSION          SEP_MINOR_VERSION
#define EMON_PRODUCT_RELEASE_STRING SEP_RELEASE_STRING

#if defined(SEP_ENABLE_PRIVATE_CPUS)
#define PRODUCT_TYPE      "private"
#define SEP_NAME          "sepint"
#define SEP_NAME_W        L"sepint"
#else
#if defined(SEP_ENABLE_NDA_CPUS)
#define PRODUCT_TYPE      "NDA"
#else
#define PRODUCT_TYPE      "public"
#endif
#define SEP_NAME          "sep"
#define SEP_NAME_W        L"sep"
#endif

#if !defined(PRODUCT_BUILDER)
#define PRODUCT_BUILDER unknown
#endif


#define TB_FILE_EXT       ".tb7"
#define TB_FILE_EXT_W     L".tb7"

#define SEP_PRODUCT_NAME  "Sampling Enabling Product"
#define EMON_PRODUCT_NAME "EMON"

#define PRODUCT_VERSION_DATE    __DATE__ " at " __TIME__

#define SEP_PRODUCT_COPYRIGHT  "Copyright(C) 2007-2019 Intel Corporation. All rights reserved."
#define EMON_PRODUCT_COPYRIGHT "Copyright(C) 1993-2019 Intel Corporation. All rights reserved."

#define PRODUCT_DISCLAIMER  "Warning: This computer program is protected under U.S. and international\ncopyright laws, and may only be used or copied in accordance with the terms\nof the license agreement.  Except as permitted by such license, no part\nof this computer program may be reproduced, stored in a retrieval system,\nor transmitted in any form or by any means without the express written consent\nof Intel Corporation."
#define PRODUCT_VERSION     STRINGIFY(SEP_MAJOR_VERSION) "." STRINGIFY(SEP_MINOR_VERSION)

#define SEP_MSG_PREFIX    SEP_NAME STRINGIFY(SEP_MAJOR_VERSION) "_" STRINGIFY(SEP_MINOR_VERSION) ":"
#define SEP_VERSION_STR   STRINGIFY(SEP_MAJOR_VERSION) "." STRINGIFY(SEP_MINOR_VERSION)

#if defined(DRV_OS_WINDOWS)

#define SEP_DRIVER_NAME   SEP_NAME "drv" STRINGIFY(SEP_MAJOR_VERSION)
#define SEP_DRIVER_NAME_W SEP_NAME_W L"drv" STRINGIFY_W(SEP_MAJOR_VERSION)
#define SEP_DEVICE_NAME   SEP_DRIVER_NAME

#define SEP_PREV_DRIVER_NAME   SEP_NAME "drv" STRINGIFY(SEP_PREV_MAJOR_VERSION)
#define SEP_PREV_DRIVER_NAME_W SEP_NAME_W L"drv" STRINGIFY_W(SEP_PREV_MAJOR_VERSION)
#define SEP_PREV_DEVICE_NAME   SEP_PREV_DRIVER_NAME

#endif

#if defined(DRV_OS_LINUX) || defined(DRV_OS_SOLARIS) || defined(DRV_OS_ANDROID) || defined(DRV_OS_FREEBSD)

#define SEP_DRIVER_NAME   SEP_NAME STRINGIFY(SEP_MAJOR_VERSION)
#define SEP_SAMPLES_NAME  SEP_DRIVER_NAME "_s"
#define SEP_UNCORE_NAME   SEP_DRIVER_NAME "_u"
#define SEP_SIDEBAND_NAME SEP_DRIVER_NAME "_b"
#define SEP_EMON_NAME     SEP_DRIVER_NAME "_e"
#define SEP_DEVICE_NAME   "/dev/" SEP_DRIVER_NAME

#define SEP_PREV_DRIVER_NAME   SEP_NAME STRINGIFY(SEP_PREV_MAJOR_VERSION)
#define SEP_PREV_SAMPLES_NAME  SEP_PREV_DRIVER_NAME "_s"
#define SEP_PREV_UNCORE_NAME   SEP_PREV_DRIVER_NAME "_u"
#define SEP_PREV_SIDEBAND_NAME SEP_PREV_DRIVER_NAME "_b"
#define SEP_PREV_DEVICE_NAME   "/dev/" SEP_PREV_DRIVER_NAME

#endif

#if defined(DRV_OS_MAC)

#define SEP_DRIVER_NAME   SEP_NAME STRINGIFY(SEP_MAJOR_VERSION)
#define SEP_SAMPLES_NAME  SEP_DRIVER_NAME "_s"
#define SEP_DEVICE_NAME   SEP_DRIVER_NAME

#define SEP_PREV_DRIVER_NAME   SEP_NAME STRINGIFY(SEP_PREV_MAJOR_VERSION)
#define SEP_PREV_SAMPLES_NAME  SEP_PREV_DRIVER_NAME "_s"
#define SEP_PREV_DEVICE_NAME   SEP_PREV_DRIVER_NAME

#endif

#endif

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
 *
 *    Description:  types and definitions shared between PAX kernel 
 *                  and user modes
 *
 *    NOTE: alignment on page boundaries is required on 64-bit platforms!
 *
*/


#ifndef _PAX_SHARED_H_
#define _PAX_SHARED_H_

#include "lwpmudrv_defines.h"
#include "lwpmudrv_types.h"

#define _STRINGIFY(x)     #x
#define STRINGIFY(x)      _STRINGIFY(x)

// PAX versioning

#define PAX_MAJOR_VERSION        1    // major version (increment only when PAX driver is incompatible with previous versions)
#define PAX_MINOR_VERSION        0    // minor version (increment only when new APIs are added, but driver remains backwards compatible)
#define PAX_BUGFIX_VERSION       2    // bugfix version (increment only for bug fixes that don't affect usermode/driver compatibility)

#define PAX_VERSION_STR          STRINGIFY(PAX_MAJOR_VERSION)"."STRINGIFY(PAX_MINOR_VERSION)"."STRINGIFY(PAX_BUGFIX_VERSION)

// PAX device name

#if defined(DRV_OS_WINDOWS)
#define PAX_NAME                 "sepdal"
#define PAX_NAME_W               L"sepdal"
#else
#define PAX_NAME                 "pax"
#endif

// PAX PMU reservation states

#define PAX_PMU_RESERVED         1
#define PAX_PMU_UNRESERVED       0

#define PAX_GUID_UNINITIALIZED   0

// PAX_IOCTL definitions

#if defined(DRV_OS_WINDOWS)

//
// The name of the device as seen by the driver
//
#define LSTRING(x)               L#x
#define PAX_OBJECT_DEVICE_NAME   L"\\Device\\sepdal" // LSTRING(PAX_NAME)
#define PAX_OBJECT_LINK_NAME     L"\\DosDevices\\sepdal" // LSTRING(PAX_NAME)

#define PAX_DEVICE_NAME          PAX_NAME // for CreateFile called by app

#define PAX_IOCTL_DEVICE_TYPE    0xA000   // values 0-32768 reserved for Microsoft
#define PAX_IOCTL_FUNCTION       0xA00    // values 0-2047  reserved for Microsoft

//
// Basic CTL CODE macro to reduce typographical errors
//
#define PAX_CTL_READ_CODE(x)     CTL_CODE(PAX_IOCTL_DEVICE_TYPE,  \
                                          PAX_IOCTL_FUNCTION+(x), \
                                          METHOD_BUFFERED,        \
                                          FILE_READ_ACCESS)

#define PAX_IOCTL_INFO           PAX_CTL_READ_CODE(1)
#define PAX_IOCTL_STATUS         PAX_CTL_READ_CODE(2)
#define PAX_IOCTL_RESERVE_ALL    PAX_CTL_READ_CODE(3)
#define PAX_IOCTL_UNRESERVE      PAX_CTL_READ_CODE(4)

#elif defined(DRV_OS_LINUX) || defined (DRV_OS_ANDROID) || defined (DRV_OS_SOLARIS)

#define PAX_DEVICE_NAME          "/dev/" PAX_NAME

#define PAX_IOC_MAGIC            100
#define PAX_IOCTL_INFO           _IOW(PAX_IOC_MAGIC, 1, IOCTL_ARGS)
#define PAX_IOCTL_STATUS         _IOW(PAX_IOC_MAGIC, 2, IOCTL_ARGS)
#define PAX_IOCTL_RESERVE_ALL    _IO (PAX_IOC_MAGIC, 3)
#define PAX_IOCTL_UNRESERVE      _IO (PAX_IOC_MAGIC, 4)

#if defined(HAVE_COMPAT_IOCTL) && defined(DRV_EM64T)
#define PAX_IOCTL_COMPAT_INFO           _IOW(PAX_IOC_MAGIC, 1, compat_uptr_t)
#define PAX_IOCTL_COMPAT_STATUS         _IOW(PAX_IOC_MAGIC, 2, compat_uptr_t)
#define PAX_IOCTL_COMPAT_RESERVE_ALL    _IO (PAX_IOC_MAGIC, 3)
#define PAX_IOCTL_COMPAT_UNRESERVE      _IO (PAX_IOC_MAGIC, 4)
#endif

#elif defined(DRV_OS_FREEBSD)

#define PAX_DEVICE_NAME          "/dev/" PAX_NAME

#define PAX_IOC_MAGIC            100
#define PAX_IOCTL_INFO           _IOW(PAX_IOC_MAGIC, 1, IOCTL_ARGS_NODE)
#define PAX_IOCTL_STATUS         _IOW(PAX_IOC_MAGIC, 2, IOCTL_ARGS_NODE)
#define PAX_IOCTL_RESERVE_ALL    _IO (PAX_IOC_MAGIC, 3)
#define PAX_IOCTL_UNRESERVE      _IO (PAX_IOC_MAGIC, 4)

#elif defined(DRV_OS_MAC)

// OSX driver names are always in reverse DNS form.
#define PAXDriverClassName       com_intel_driver_PAX
#define kPAXDriverClassName      "com_intel_driver_PAX"
#define PAX_DEVICE_NAME          "com.intel.driver.PAX"

// User client method dispatch selectors.
enum {
    kPAXUserClientOpen,
    kPAXUserClientClose,
    kPAXReserveAll,
    kPAXUnreserve,
    kPAXGetStatus,
    kPAXGetInfo,
    kPAXDataIO,
    kNumberOfMethods // Must be last 
};

#else
#warning "unknown OS in pax_shared.h"
#endif

// data for PAX_IOCTL_INFO call

struct PAX_INFO_NODE_S {
    volatile U64  managed_by;        // entity managing PAX
    volatile U32  version;           // PAX version number
    volatile U64  reserved1;         // force 8-byte alignment
    volatile U32  reserved2;         // unreserved
};

typedef struct PAX_INFO_NODE_S  PAX_INFO_NODE;
typedef        PAX_INFO_NODE   *PAX_INFO;

// data for PAX_IOCTL_STATUS call

struct PAX_STATUS_NODE_S {
    volatile U64            guid;              // reservation ID (globally unique identifier)
    volatile DRV_FILE_DESC  pid;               // pid of process that has the reservation
    volatile U64            start_time;        // reservation start time
    volatile U32            is_reserved;       // 1 if there is a reservation, 0 otherwise
};

typedef struct PAX_STATUS_NODE_S  PAX_STATUS_NODE;
typedef        PAX_STATUS_NODE   *PAX_STATUS;

struct PAX_VERSION_NODE_S {
    union {
        U32      version;
        struct {
            U32  major:8;
            U32  minor:8;
            U32  bugfix:16;
        } s1;
    } u1;
};

typedef struct PAX_VERSION_NODE_S  PAX_VERSION_NODE;
typedef        PAX_VERSION_NODE   *PAX_VERSION;

#define PAX_VERSION_NODE_version(v) (v)->u1.version
#define PAX_VERSION_NODE_major(v)   (v)->u1.s1.major
#define PAX_VERSION_NODE_minor(v)   (v)->u1.s1.minor
#define PAX_VERSION_NODE_bugfix(v)  (v)->u1.s1.bugfix

#endif


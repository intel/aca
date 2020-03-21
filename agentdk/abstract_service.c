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

#ifndef _ABSTRACT_SERVICE_C_INC_
#define _ABSTRACT_SERVICE_C_INC_

#include "lwpmudrv_defines.h"
#include "lwpmudrv_version.h"
#include "lwpmudrv_types.h"
#include "lwpmudrv_ioctl.h"
#include "lwpmudrv_ecb.h"
#include "lwpmudrv_struct.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#if defined(DRV_OS_LINUX) || defined(DRV_OS_ANDROID)
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/syscall.h>
#endif

#if defined(DRV_OS_SOLARIS) || defined(DRV_OS_FREEBSD)
#include <sys/ioccom.h>
#endif

#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

#include "log.h"
#include "abstract_service.h"
#include "communication.h"

#if defined(DRV_SOFIA) || defined(DRV_BUTTER) || defined(DRV_OS_ANDROID) || defined(DRV_OS_OPENWRT)
#define DRV_DEVICE_DELIMITER "_"
#elif defined(DRV_OS_LINUX) || defined(DRV_OS_SOLARIS) || defined(DRV_OS_FREEBSD)
#define DRV_DEVICE_DELIMITER "/"
#endif

static U32            prev_driver_loaded = 0;
static DRV_FILE_DESC  driver_handle = DRV_INVALID_FILE_DESC_VALUE;

static S32            abs_num_cpus = 0;
extern U32            data_transfer_mode;

pthread_cond_t        stop_received;
pthread_mutex_t       stop_lock;

#define DRV_OPERATION_PAX 0x40086401
/* ------------------------------------------------------------------------- */
/*!
 * @fn          U32 abstract_Sleep (milliseconds)
 *
 * @param       U32 milliseconds - time to sleep
 *
 * @brief       Have user code sleep for a period of time
 *
 * @return      U32 - Return Status Code
 *
 * <I>Special Notes:</I>
 *
 */
static U32
abstract_Sleep (
    U32  milliseconds
)
{
#if defined(DRV_OS_LINUX) || defined(DRV_OS_ANDROID)
    double           seconds = (double)(milliseconds/1000);
    int              ret_val = 0;
    struct timespec  ts;
    struct timeval   target_time;
    struct timeval   current_time;
    U64              target_usec;
    U64              current_usec;
    U64              usec_left;
    double           sec_left;

    gettimeofday(&target_time,NULL);
    target_usec = (target_time.tv_sec*1e+6) + target_time.tv_usec + (milliseconds*1000);

    ts.tv_sec  = (time_t)seconds;
    ts.tv_nsec = (long)((seconds - ts.tv_sec)*1e+9);

    do {
        ret_val = nanosleep(&ts, &ts);
        if ((ret_val != 0) && (errno != EINTR)) {
            return VT_SAM_ERROR;
        }

        gettimeofday(&current_time,NULL);
        current_usec = (current_time.tv_sec*1e+6) + current_time.tv_usec;
        if (current_usec > target_usec) {
            break;
        }
        usec_left  = target_usec - current_usec;
        sec_left   = (double)(usec_left/1e+6);
        ts.tv_sec  = (time_t)sec_left;
        ts.tv_nsec = (long)((sec_left - ts.tv_sec)*1e+9);
    } while (ts.tv_sec > 0 || ts.tv_nsec > 0);
#else
    usleep(milliseconds * 1000);
#endif
    return VT_SUCCESS;
}


/* ------------------------------------------------------------------------- */
/*!
 * @fn        abstract_Open_Device_Driver (driver_name)
 *
 * @param     char *driver_name - name of driver
 *
 * @brief     Return handle to the specified device-driver, which can
 *            subsequently be used for calls to DeviceIoControl.
 *
 * @return    DRV_FILE_DESC
 *
 */
static DRV_FILE_DESC
abstract_Open_Device_Driver (
    char*       driver_name
)
{
    DRV_FILE_DESC device_handle;
    char control_device[MAX_STRING_LENGTH];

    DRV_SNPRINTF(control_device, 100, 100, "%s%sc", driver_name, DRV_DEVICE_DELIMITER);
    device_handle = open(control_device, O_RDWR);

    if (device_handle == DRV_INVALID_FILE_DESC_VALUE) {
        SEPAGENT_PRINT_DEBUG("Could not open %s\n", control_device);
    }

    return device_handle;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn        abstract_Send_IOCTL_helper (cmd, ioctl_arg)
 *
 * @param     U32 cmd - ioctl cmd
 *            IOCTL_ARGs arg
 * @brief     Helper function to perform any prerequisite operation in addition to IOCTL handling.
 *
 * @return    None
 *
 */
static VOID
abstract_Send_IOCTL_helper(
    U32         cmd,
    IOCTL_ARGS  arg
)
{

    // Create pthreads for module/sample/sideband
    if (cmd == DRV_OPERATION_INIT_DRIVER) {
        abstract_Start_Threads(arg->buf_usr_to_drv);
    }

    // Create pthreads for uncore
    if (cmd == DRV_OPERATION_INIT_UNC) {
        abstract_Start_Threads_UNC(1); // Default number of packages = 1
    }

    // Set OSID
    if (cmd == DRV_OPERATION_SET_OSID) {
        abstract_Set_OSID(arg->buf_usr_to_drv);
    }

    if (cmd == DRV_OPERATION_STOP) {
        if (data_transfer_mode == DELAYED_TRANSFER) {
            pthread_mutex_lock(&stop_lock);
            pthread_cond_broadcast(&stop_received);
            pthread_mutex_unlock(&stop_lock);
        }
        abstract_Stop_Threads();
    }
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn        abstract_Do_IOCTL_RW (control_code, in_buf, in_buf_len, out_buf, out_buf_len)
 *
 * @param     U32   control_code    - code for the IOCTL to perform
 * @param     PVOID in_buf          - Pointer to a read buffer
 * @param     S32   in_buf_len      - length of the read buffer
 * @param     PVOID out_buf         - Pointer to a write buffer
 * @param     S32   out_buf_len     - length of the write buffer
 *
 * @brief     Performs the IOCTL specified by the control code and
 *            returns.  Expects to pass in an in buffer and get data back
 *            through the out buffer.
 *
 * @return    DRV_STATUS            - Status Error Code
 *
 */
DRV_DLLEXPORT DRV_STATUS
ABSTRACT_Send_IOCTL (
    U32                cmd,
    IOCTL_ARGS         arg
)
{
    S32             bytes_ret;
    DRV_STATUS      status;
    DRV_FILE_DESC   driver_handle = DRV_INVALID_FILE_DESC_VALUE;
    U32             command;

    if (cmd == DRV_OPERATION_SET_OSID || cmd == DRV_OPERATION_PAX) {
        return VT_SUCCESS;
    }
    driver_handle = abstract_Open_Device_Driver(SEP_DEVICE_NAME);

    if (driver_handle == DRV_INVALID_FILE_DESC_VALUE) {
        driver_handle = abstract_Open_Device_Driver(SEP_PREV_DEVICE_NAME);
        if (driver_handle == DRV_INVALID_FILE_DESC_VALUE) {
            free(arg);
            return VT_DRIVER_COMM_FAILED;
        }
    }

    if (arg->len_drv_to_usr == 0 && arg->len_usr_to_drv == 0) {
        command = LWPMUDRV_IOCTL_IO(cmd);
    }
    else if (arg->len_drv_to_usr != 0) {
        command = LWPMUDRV_IOCTL_IOR(cmd);
    }
    else {
        command = LWPMUDRV_IOCTL_IOW(cmd);
    }

    bytes_ret = ioctl(driver_handle, command, arg);
    status = !(bytes_ret < 0) ? VT_SUCCESS : VT_DRIVER_COMM_FAILED;
    if (driver_handle != DRV_INVALID_FILE_DESC_VALUE) {
        close(driver_handle);
    }

    abstract_Send_IOCTL_helper(cmd, arg);

    return status;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn        abstract_Do_IOCTL_RW (command, in_buf, in_buf_len, out_buf, out_buf_len)
 *
 * @param     U32   command         - code for the IOCTL to perform
 * @param     PVOID in_buf          - Pointer to a read buffer
 * @param     S32   in_buf_len      - length of the read buffer
 * @param     PVOID out_buf         - Pointer to a write buffer
 * @param     S32   out_buf_len     - length of the write buffer
 *
 * @brief     Performs the IOCTL specified by the control code and
 *            returns.  Expects to pass in an in buffer and get data back
 *            through the out buffer.
 *
 * @return    DRV_STATUS            - Status Error Code
 *
 */
static DRV_STATUS
abstract_Do_IOCTL_RW (
    U32      command,
    PVOID    buf_drv_to_usr,
    U32      len_drv_to_usr,
    PVOID    buf_usr_to_drv,
    U32      len_usr_to_drv
)
{
    IOCTL_ARGS           arg;
    S32             bytes_ret;
    DRV_STATUS      status;
    DRV_FILE_DESC   driver_handle = DRV_INVALID_FILE_DESC_VALUE;

    arg = (IOCTL_ARGS)calloc(1, sizeof(IOCTL_ARGS_NODE));
    if (!arg) {
        return VT_NO_MEMORY;
    }

    driver_handle = abstract_Open_Device_Driver(SEP_DEVICE_NAME);

    if (driver_handle == DRV_INVALID_FILE_DESC_VALUE) {
        driver_handle = abstract_Open_Device_Driver(SEP_PREV_DEVICE_NAME);
        if (driver_handle == DRV_INVALID_FILE_DESC_VALUE) {
            free(arg);
            return VT_DRIVER_COMM_FAILED;
        }
    }
    arg->command = command;
    arg->len_drv_to_usr = len_drv_to_usr;
    arg->buf_drv_to_usr = buf_drv_to_usr;
    arg->len_usr_to_drv = len_usr_to_drv;
    arg->buf_usr_to_drv = buf_usr_to_drv;

    bytes_ret = ioctl(driver_handle, LWPMUDRV_IOCTL_IORW(command), arg);
    status = !(bytes_ret < 0) ? VT_SUCCESS : VT_DRIVER_COMM_FAILED;
    if (driver_handle != DRV_INVALID_FILE_DESC_VALUE) {
        close(driver_handle);
    }

    free(arg);

    return status;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn        abstract_Do_IOCTL (command)
 *
 * @param     U32 signal_fn  - Code for the parameterless ioctl
 *
 * @brief     Performs the IOCTL specified by the signal code.
 *            No input data is sent to the driver and none is expected back.
 *
 * @return    DRV_STATUS - Status Error Code
 *
 */
static DRV_STATUS
abstract_Do_IOCTL (
    U32     command
)
{
    DRV_STATUS      status        = VT_SUCCESS;;
    DRV_FILE_DESC   driver_handle = DRV_INVALID_FILE_DESC_VALUE;
    IOCTL_ARGS           arg;

    arg = (IOCTL_ARGS) calloc(1, sizeof(IOCTL_ARGS_NODE));
    if (!arg) {
        return VT_NO_MEMORY;
    }
    driver_handle = abstract_Open_Device_Driver(SEP_DEVICE_NAME);

    if (driver_handle == DRV_INVALID_FILE_DESC_VALUE) {
        driver_handle = abstract_Open_Device_Driver(SEP_PREV_DEVICE_NAME);
        if (driver_handle == DRV_INVALID_FILE_DESC_VALUE) {
            free(arg);
            return VT_DRIVER_COMM_FAILED;
        }
    }

    if (ioctl(driver_handle, LWPMUDRV_IOCTL_IO(command)) < 0) {
        status = VT_DRIVER_COMM_FAILED;
    }

    if (driver_handle != DRV_INVALID_FILE_DESC_VALUE) {
        close(driver_handle);
    }

    free(arg);

    return status;
}

/*!
 * @fn        abstract_Do_IOCTL_R (read_fn, in_buf, in_buf_len)
 *
 * @param     U32   read_fn       - code of IOCTL to perform
 * @param     PVOID in_buf        - Pointer to a buffer of data to read
 * @param     S32   in_buf_len    - length of the read buffer
 *
 * @brief     Performs the IOCTL specified by the control code and
 *            returns.  Expects to pass in an in buffer.
 *            No data is expected to be sent back via the write buffer.
 *
 * @return    DRV_STATUS    - Status Error Code
 *
 */
static DRV_STATUS
abstract_Do_IOCTL_R (
    U32     command,
    PVOID   buf_drv_to_usr,
    U32     len_drv_to_usr
)
{
    IOCTL_ARGS           arg;
    U32             result;
    DRV_STATUS      status;
    DRV_FILE_DESC   driver_handle = DRV_INVALID_FILE_DESC_VALUE;

    arg = (IOCTL_ARGS)calloc(1, sizeof(IOCTL_ARGS_NODE));
    if (!arg) {
        return VT_NO_MEMORY;
    }
    driver_handle = abstract_Open_Device_Driver(SEP_DEVICE_NAME);

    if (driver_handle == DRV_INVALID_FILE_DESC_VALUE) {
       driver_handle = abstract_Open_Device_Driver(SEP_PREV_DEVICE_NAME);
       if (driver_handle == DRV_INVALID_FILE_DESC_VALUE) {
            free(arg);
            return VT_DRIVER_COMM_FAILED;
       }
    }

    arg->len_drv_to_usr = len_drv_to_usr;
    arg->buf_drv_to_usr = buf_drv_to_usr;
    arg->len_usr_to_drv = 0;
    arg->buf_usr_to_drv = NULL;
    arg->command = command;

    result = ioctl(driver_handle, LWPMUDRV_IOCTL_IOR(command), arg);
    status = (result == 0) ? VT_SUCCESS : VT_DRIVER_COMM_FAILED;
    if (driver_handle != DRV_INVALID_FILE_DESC_VALUE) {
        close(driver_handle);
    }

    free(arg);

    return status;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn        abstract_Do_IOCTL_W (command, out_buf, out_buf_len)
 *
 * @param     U32   command        - code of IOCTL to perform
 * @param     PVOID out_buf        - Pointer to a buffer of data to write
 * @param     S32   out_buf_len    - length of the write buffer
 *
 * @brief     Performs the IOCTL specified by write_fn on the args in out_buf.
 *
 * @return    DRV_STATUS     - Status Error Code
 *
 */
static DRV_STATUS
abstract_Do_IOCTL_W (
    U32     command,
    PVOID   buf_usr_to_drv,
    U32     len_usr_to_drv
)
{
    IOCTL_ARGS           arg;
    S32             bytes_ret;
    DRV_STATUS      status;
    DRV_FILE_DESC   driver_handle = DRV_INVALID_FILE_DESC_VALUE;

    arg = (IOCTL_ARGS)calloc(1, sizeof(IOCTL_ARGS_NODE));
    if (!arg) {
        return VT_NO_MEMORY;
    }
    driver_handle = abstract_Open_Device_Driver(SEP_DEVICE_NAME);

    if (driver_handle == DRV_INVALID_FILE_DESC_VALUE) {
        driver_handle = abstract_Open_Device_Driver(SEP_PREV_DEVICE_NAME);
        if (driver_handle == DRV_INVALID_FILE_DESC_VALUE) {
            free(arg);
            return VT_DRIVER_COMM_FAILED;
        }
    }
    arg->len_drv_to_usr = 0;
    arg->buf_drv_to_usr = NULL;
    arg->len_usr_to_drv = len_usr_to_drv;
    arg->buf_usr_to_drv = buf_usr_to_drv;
    arg->command = command;

    bytes_ret = ioctl(driver_handle, LWPMUDRV_IOCTL_IOW(command), arg);
    status = !(bytes_ret < 0) ? VT_SUCCESS : VT_DRIVER_COMM_FAILED;
    if (driver_handle != DRV_INVALID_FILE_DESC_VALUE) {
        close(driver_handle);
    }

    free(arg);

    return status;
}


/* ------------------------------------------------------------------------- */
/*!
 * @fn        ABSTRACT_Open_Driver (void)
 *
 * @param     void
 *
 * @brief     This function open the driver to perform work.
 *
 * @return    DRV_STATUS - Status Error Code
 *
 */
static DRV_STATUS
ABSTRACT_Open_Driver (
    void
)
{
    if (driver_handle == DRV_INVALID_FILE_DESC_VALUE) {
        SEPAGENT_PRINT_DEBUG("Attempt to open driver connection\n");
        driver_handle = abstract_Open_Device_Driver(SEP_DEVICE_NAME);
        if (driver_handle == DRV_INVALID_FILE_DESC_VALUE) {
            driver_handle = abstract_Open_Device_Driver(SEP_PREV_DEVICE_NAME);
            if (driver_handle == DRV_INVALID_FILE_DESC_VALUE) {
                return VT_DRIVER_OPEN_FAILED;
            }
            prev_driver_loaded = 1;
        }
    }

    return VT_SUCCESS;
}


/* ------------------------------------------------------------------------- */
/*!
 * @fn        ABSTRACT_Close_Driver (void)
 *
 * @param     void
 *
 * @brief     This function close the driver for the current invocation.
 *            The driver itself will not be unloaded.
 *
 *
 * @return    VOID
 *
 */
static VOID
ABSTRACT_Close_Driver (
    void
)
{
    S32 i;

    if (driver_handle != DRV_INVALID_FILE_DESC_VALUE) {
        close(driver_handle);
        SEPAGENT_PRINT_DEBUG("Driver closed\n");
        driver_handle = DRV_INVALID_FILE_DESC_VALUE;
    }

    return;
}


#endif

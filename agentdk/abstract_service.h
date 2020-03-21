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
 *  File  : abstract.h
 */

#ifndef _ABSTRACT_SERVICE_H_INC_
#define _ABSTRACT_SERVICE_H_INC_

#ifdef __cplusplus
extern "C" {
#endif

#define DRV_PAUSE_SIGNAL        0
#define DRV_RESUME_SIGNAL       1
#define DRV_STOP_SIGNAL         2
#define DRV_CANCEL_SIGNAL       3
#define DRV_FINISH_SIGNAL       4
#define DRV_CREATE_MARKER       5
#define DRV_MARK_SAMPLING       6
#define DRV_MARK_OFF_SAMPLING   7
#define DRV_SET_THREAD_NAME     8
#define DRV_FLUSH_SIGNAL        9
#define DRV_RELEASE_PMU         10
#define DRV_GET_MARKER_ID       11
#define DRV_READ_SIGNAL         12

#define PAX_CTRL                "pax_ctrl"

/*
 * ABSTRACT_Get_Pipe_Name
 *     Parameters
 *      void
 *
 *     Returns
 *          Name of the pipe
 *
 *     Description
 *          Returns the name of the pipe. The pipe is used for message
 *          passing during pause, resume, stop and cancel calls.
 */
extern void
ABSTRACT_Get_Pipe_Name (S8 *);

// Name of the pipe used for Inter-process communication
#define SEP_IPC_PIPE_NAME(arg1)     ABSTRACT_Get_Pipe_Name(arg1)

#define ABSTRACT_NHM_TSC_FREQ_MSR 0xce

/*
 * @fn        ABSTRACT_Send_IOCTL (control_code, args)
 *
 * @param     U32   control_code    - code for the IOCTL to perform
 * @param     IOCTL_ARGS args       - IOCTL args
 *
 * @brief     Performs the IOCTL specified by the control code and
 *            returns.  Expects to pass IOCTL_ARGS buffer
 *
 * @return    DRV_STATUS            - Status Error Code
 *
 */
DRV_DLLEXPORT DRV_STATUS
ABSTRACT_Send_IOCTL (
    U32                cmd,
    IOCTL_ARGS         arg
);


/*
 * ABSTRACT_Open_Driver
 *     Parameters
 *          NONE
 *     Returns
 *          TRUE - if successful or already open
 *
 *     Description
 *          This function open the driver to perform work.
 *
 */
static DRV_STATUS
ABSTRACT_Open_Driver(void);

/*
 * ABSTRACT_Close_Driver
 *     Parameters
 *          NONE
 *
 *     Returns
 *          NONE
 *
 *     Description
 *          This function close the driver for the current invocation.
 *          The driver itself will not be unloaded.
 *
 */
static VOID
ABSTRACT_Close_Driver(void);


#ifdef __cplusplus
}
#endif

#endif

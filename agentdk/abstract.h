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

#ifndef _ABSTRACT_H_INC_
#define _ABSTRACT_H_INC_

#ifdef __cplusplus
extern "C" {
#endif


#if defined(DRV_OS_ANDROID)
#define OUT_BUF_SIZE 16
#endif

#if defined(DRV_OS_LINUX) || defined(DRV_OS_SOLARIS) || defined(DRV_OS_MAC) || defined(DRV_OS_FREEBSD)
#define OUT_BUF_SIZE 17
#endif

typedef struct OSI_THREAD_NODE_S  OSI_THREAD_NODE;
typedef        OSI_THREAD_NODE   *OSI_THREAD;
struct OSI_THREAD_NODE_S {
    pthread_t           thread;
    pthread_attr_t      attr;
};

#define OSI_THREAD_thread(opt)    (opt)->thread
#define OSI_THREAD_attr(opt)      (opt)->attr


/*
 *  Structures used to manage Read Threads
 */
#define THREAD_ARG_SIZE MAXNAMELEN
typedef struct THREAD_ARG_NODE_S  THREAD_ARG_NODE;
typedef        THREAD_ARG_NODE   *THREAD_ARG;
struct THREAD_ARG_NODE_S {
    S32    me;
    char   dname[THREAD_ARG_SIZE];
    char   oname[THREAD_ARG_SIZE];
    int    dev_id;
    int    epd;
    U32    buf_size;
    void  *dma_buf;
    U32    conn_id;
    U32    conn_type;
};

#define THREAD_ARG_me(targ)              (targ)->me
#define THREAD_ARG_dname(targ)           (targ)->dname
#define THREAD_ARG_oname(targ)           (targ)->oname
#define THREAD_ARG_epd(targ)             (targ)->epd
#define THREAD_ARG_dma_buf(targ)         (targ)->dma_buf
#define THREAD_ARG_buf_size(targ)        (targ)->buf_size
#define THREAD_ARG_dev_id(targ)          (targ)->dev_id
#define THREAD_ARG_conn_id(targ)         (targ)->conn_id
#define THREAD_ARG_conn_type(targ)       (targ)->conn_type


typedef struct READ_THREAD_NODE_S  READ_THREAD_NODE;
typedef        READ_THREAD_NODE   *READ_THREAD;
struct READ_THREAD_NODE_S {
    OSI_THREAD_NODE     tn;
    THREAD_ARG_NODE     arg;
};

#define READ_THREAD_tn(rt)        (rt)->tn
#define READ_THREAD_arg(rt)       (rt)->arg

#define READ_THREAD_thread(rt)    OSI_THREAD_thread(&(READ_THREAD_tn((rt))))
#define READ_THREAD_attr(rt)      OSI_THREAD_attr(&(READ_THREAD_tn((rt))))

#define READ_THREAD_me(rt)        THREAD_ARG_me(&(READ_THREAD_arg((rt))))
#define READ_THREAD_dname(rt)     THREAD_ARG_dname(&(READ_THREAD_arg((rt))))
#define READ_THREAD_oname(rt)     THREAD_ARG_oname(&(READ_THREAD_arg((rt))))
#define READ_THREAD_epd(rt)       THREAD_ARG_epd(&(READ_THREAD_arg((rt))))
#define READ_THREAD_dma_buf(rt)   THREAD_ARG_dma_buf(&(READ_THREAD_arg((rt))))
#define READ_THREAD_buf_size(rt)  THREAD_ARG_buf_size(&(READ_THREAD_arg((rt))))
#define READ_THREAD_dev_id(rt)    THREAD_ARG_dev_id(&(READ_THREAD_arg((rt))))
#define READ_THREAD_conn_id(rt)   THREAD_ARG_conn_id(&(READ_THREAD_arg((rt))))
#define READ_THREAD_conn_type(rt) THREAD_ARG_conn_type(&(READ_THREAD_arg((rt))))

/*
 * @fn          abstract_Start_Threads()
 *
 * @brief       All core buffers and temp files are set up
 *              and the system will be prepared to start sampling.
 *              It must be called just before DRV_OPERATION_INIT_DRIVER.
 *              It must also be called just once per collector session.
 * @param       None
 *
 * @return      Status
 */
static DRV_STATUS
abstract_Start_Threads (
    S8  *pcfg_buf
);

/*
 * @fn          abstract_Start_Threads_UNC()
 *
 * @brief       All uncore buffers and temp files for uncore are set up
 *              and the system will be prepared to start sampling.
 *              It must be called just before DRV_OPERATION_INIT_UNC.
 *              This must also be called just once per sampling session.
 *
 * @param       None
 *
 * @return      Status
 */
static DRV_STATUS
abstract_Start_Threads_UNC (
    U32 num_packages
);

/*
 * @fn          abstract_Stop_Threads()
 *
  * @brief      Stop threads - core and uncore.
 *
 * @param       None
 *
 * @return      Status
 */
static DRV_STATUS
abstract_Stop_Threads (
);

/*
 * @fn          abstract_Set_OSID(S8 *buf)
 *
  * @brief      set osid
 *
 * @param       IN osid - pointer to osid
 *
 * @return      None
 */
static VOID
abstract_Set_OSID(
    S8 *buf
);


/*
 * @fn          ABSTRACT_Num_CPUs (num_cpus)
 *
 * @param       U32 *num_cpus      - number of CPUs
 *
 * @brief       Retrieve number of cores in system
 *
 * @return      DRV_STATUS         - VT_SUCCESS on success
 */
DRV_DLLEXPORT DRV_STATUS
ABSTRACT_Num_CPUs (
    U32 *num_cpus
);


/*
 * @fn          ABSTRACT_Version (void)
 *
 * @param       void
 *
 * @return      U32 - The version number of the kernel mode driver.
 *
 * @brief       return the version number of the driver
 */
DRV_DLLEXPORT U32
ABSTRACT_Version (
    void
);


/*
 * @fn          U32  ABSTRACT_Get_Agent_Mode ( U32 *agent_mode, U32 transfer_mode )
 *
 * @brief       Get sep driver mode (Native/Guest VM/Host VM)
 *
 * @param       OUT agent_mode  - agent mode
 *              IN data_transfer_mode   - transfer mode
 *
 * @return      Status
 *
 * <I>Special Notes:</I>
 *              <NONE>
 */
DRV_DLLEXPORT U32
ABSTRACT_Get_Agent_Mode (
    U32 *agent_mode,
    U32  transfer_mode
);


/*
 * @fn          ABSTRACT_Check_KVM_Enabled
 *
 * @brief       Check if the KVM is enabled
 *
 * @param       None
 *
 * @return      DRV_BOOL  KVM enable status
 *
 * <I>Special Notes:</I>
 *            < None >
 */
DRV_DLLEXPORT DRV_BOOL
ABSTRACT_Check_KVM_Enabled (
    void
);


/*
 * @fn          ABSTRACT_Get_Drv_Setup_Info()
 *
  * @brief      Get numerous information from driver.
 *
 * @param       DRV_SETUP_INFO     drv_setup_info
 *
 * @return      Status
 */
DRV_DLLEXPORT DRV_STATUS
ABSTRACT_Get_Drv_Setup_Info (
    DRV_SETUP_INFO     drv_setup_info
);

#ifdef __cplusplus
}

#endif

#endif

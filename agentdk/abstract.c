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

#include "lwpmudrv_defines.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/utsname.h>
#if defined(DRV_OS_LINUX) || defined(DRV_OS_ANDROID)
#include <linux/ioctl.h>
#if defined(DRV_IA32)
#include <errno.h>
#endif
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

#include <unistd.h>
#include <sys/mman.h>

#include "lwpmudrv_types.h"
#include "lwpmudrv_ioctl.h"
#include "lwpmudrv_ecb.h"
#include "lwpmudrv_struct.h"
#include "rise_errors.h"
#include "abstract.h"
#include "log.h"
#include "./abstract_service.c"


/*
 *  Start File gather implementation
 */
static  READ_THREAD_NODE       mod_r;
static  READ_THREAD            samp_r               = NULL;
static  READ_THREAD            uncsamp_r            = NULL;
static  DRV_BOOL               unc_threads_spawn    = FALSE;
static  READ_THREAD            sideband_r           = NULL;
static  S8                    *seed_name            = NULL;
static  DRV_BOOL               counting_mode        = FALSE;
static  U32                    abs_num_packages     = 0;
static  U32                    agent_mode           = NATIVE_AGENT;
static  U32                    sched_switch_enabled = FALSE;
static  U32                    agent_osid           = 0;

/* ------------------------------------------------------------------------- */
/*!
 * @fn          abstract_Send_Data_To_Host(args)
 *
 * @param       THREAD_ARG args - thread specific argument holding the arguments
 *
 * @brief       helper function to sends data form specified file to remote host
 *
 * @return      DRV_STATUS - 0 for success, otherwise for failure
 *
 */
static int
abstract_Send_Data_To_Host(
    THREAD_ARG  args
)
{
    DRV_STATUS   status         = VT_SUCCESS;
    U64         *output_buffer  = NULL;
    U64          out_buf_size   = 1 << OUT_BUF_SIZE;
    S32          bytecount      = 0;
    S32          total_sent     = 0;
    S32          send_size      = 0;
    U32          out_fd;

    output_buffer   = (U64 *)calloc((size_t)out_buf_size, sizeof(U64));
    if (!output_buffer) {
        SEPAGENT_PRINT_ERROR("Could not allocate output buffer\n");
        return VT_NO_MEMORY;
    }
    out_fd = open(THREAD_ARG_oname(args), O_RDONLY);
    if (out_fd == -1) {
        SEPAGENT_PRINT_ERROR("Could not open tmp file(%s)\n", THREAD_ARG_oname(args));
        free(output_buffer);
        return VT_FILE_OPEN_FAILED;
    }
    //read the tmp file and send to remote host
    while (TRUE) {
        memset(output_buffer, '\0', sizeof(output_buffer));
        bytecount = read(out_fd, output_buffer, (out_buf_size*sizeof(U64)));
        if (bytecount <= 0) {
            break;
        }
        COMM_Send_Data_On_Target(THREAD_ARG_conn_id(args), THREAD_ARG_conn_type(args), output_buffer, bytecount);
    }
    free(output_buffer);
    close(out_fd);
    return VT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn          abstract_Read_Records(args)
 *
 * @param       PVOID args - thread specific argument holding the arguments
 *
 * @brief       Reader thread for each device / temp file that needs to be created
 *
 * @return      DRV_STATUS - 0 for success, otherwise for failure
 *
 */
static PVOID
abstract_Read_Records (
    PVOID  args
)
{
    int                  dev_fd = -1;
    int                  out_fd = -1;;
    int                  status;
    U64                  out_buf_size     = 1 << OUT_BUF_SIZE;
    ssize_t              bytecount        = 0;
    S32                  bytecount_target = 0;
    int                  me;
    char                *device_name;
    char                *output_name;
    U64                 *output_buffer    = NULL;
    int                  write_return_int;
    U32                  conn_id          = 0;
    U32                  conn_type        = 0;

    me              = THREAD_ARG_me((THREAD_ARG)args);
    output_buffer   = (U64 *) calloc(out_buf_size, sizeof(U64));

    if (!output_buffer) {
        SEPAGENT_PRINT_ERROR(" Could not allocate output buffer\n");
        pthread_exit((PVOID)VT_NO_MEMORY);
    }

    device_name = THREAD_ARG_dname((THREAD_ARG)args);
    output_name = THREAD_ARG_oname((THREAD_ARG)args);
    conn_id     = THREAD_ARG_conn_id((THREAD_ARG)args);
    conn_type   = THREAD_ARG_conn_type((THREAD_ARG)args);

    SEPAGENT_PRINT_DEBUG("got device_name=%s, output_name=%s, me=%d, conn_id=%u\n", device_name, output_name, me, conn_id);

    dev_fd = open(device_name, O_RDONLY);
    if (dev_fd == -1) {
        SEPAGENT_PRINT_ERROR("Could not open device %s\n", device_name);
        pthread_exit((PVOID)VT_INVALID_DEVICE);
    }

    if (data_transfer_mode == DELAYED_TRANSFER) {
        pthread_cond_init(&stop_received, NULL);
        out_fd = open(output_name, O_CREAT|O_TRUNC|O_WRONLY, 0644);
        if (out_fd == -1) {
            SEPAGENT_PRINT_ERROR("Could not open %s (tmp file) on target\n", output_name);
            pthread_exit((PVOID)VT_FILE_OPEN_FAILED);
        }
    }

    SEPAGENT_PRINT_DEBUG("|output_buffer|[%d] is %llu\n", me, (out_buf_size * sizeof(U64)));

    do {
        bytecount = read(dev_fd, output_buffer, (out_buf_size * sizeof(U64)));

        SEPAGENT_PRINT_DEBUG("read %lu bytes from %s\n", (unsigned long)bytecount, device_name);

        if (bytecount != 0) {
            if (data_transfer_mode == IMMEDIATE_TRANSFER) {
                status = COMM_Send_Data_On_Target(conn_id, conn_type, output_buffer, bytecount);
                if (status != VT_SUCCESS) {
                    SEPAGENT_PRINT_WARNING("couldn't send data to host, conn_id=%u, conn_type=%u\n", conn_id, conn_type);
                }
            }
            else {
                if (out_fd >= 0) {
                    write_return_int = write(out_fd, output_buffer, bytecount);
                    if (write_return_int <= 0) {
                         SEPAGENT_PRINT_WARNING("couldn't write to file\n");
                    }
                }
            }
        }
    } while (bytecount != 0);

    SEPAGENT_PRINT_DEBUG("exited %s read loop with value %lu\n", device_name, (unsigned long)bytecount);

    status = close(dev_fd);
    if (status < 0) {
        perror("closing dev_fd");
    }

    SEPAGENT_PRINT_DEBUG("Closed device %s\n", device_name);

    // In delayed mode, wait for stop command and send data to remote host
    if (data_transfer_mode == DELAYED_TRANSFER) {
        status = close(out_fd);
        if (status < 0) {
            perror("closing out_fd");
        }
        else {
            SEPAGENT_PRINT_DEBUG("Closed tmp file %s\n", output_name);
            pthread_mutex_lock(&stop_lock);
            pthread_cond_wait(&stop_received, &stop_lock);
            pthread_mutex_unlock(&stop_lock);
            status = abstract_Send_Data_To_Host(args);
        }
    }
    free(output_buffer);
    pthread_exit((PVOID)&status);
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn          abstract_Initialize_Read_Thread(rt)
 *
 * @param       READ_THREAD rt - The read thread node to process
 *
 * @brief       Initialize the read thread attributes and create the thread
 *
 * @return      int            - return status of pthread_create
 *
 */
static int
abstract_Initialize_Read_Thread (
    READ_THREAD rt
)
{
    int status = 0;

    pthread_attr_init(&READ_THREAD_attr(rt));
    pthread_attr_setdetachstate(&READ_THREAD_attr(rt),PTHREAD_CREATE_JOINABLE);
    status = pthread_create(&READ_THREAD_thread(rt),
                            &READ_THREAD_attr(rt),
                            abstract_Read_Records,
                            &READ_THREAD_arg(rt));

    return status;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn          abstract_Spawn_Pthreads(num_cpus)
 *
 * @param       int        num_cpus - number of threads to spawn for reading samples
 *
 * @brief       Spawn n+2 threads to create the sample and module temp files
 *              Allocate and set up per-thread argument structures
 *              Allocate and set up pthread data structures,
 *              later used for joining in abstract_Join_Pthreads()
 *
 * @return      DRV_STATUS - 0 for success, otherwise for failure
 *
 */
static DRV_STATUS
abstract_Spawn_Pthreads (
    int         num_cpus,
    DRV_CONFIG  pcfg
)
{
    int  status;
    int  num_chars;
    int  i;
    char *device_name = NULL;

    if (prev_driver_loaded) {
        device_name = SEP_PREV_DEVICE_NAME;
    }
    else {
        device_name = SEP_DEVICE_NAME;
    }

    /*
     * Alloc space for sample readers. Exit if any problems
     */
    if (agent_mode == NATIVE_AGENT || agent_mode == HOST_VM_AGENT) {
        samp_r  = (READ_THREAD)calloc(num_cpus, sizeof(READ_THREAD_NODE));
        if (!samp_r) {
            printf("Error: Unable to allocate memory for threads.\n");
            return -1;
        }
    }
    /*
     * Alloc space for pebs process info readers. Exit if any problems
     */
    if (sched_switch_enabled) {
        sideband_r  = (READ_THREAD)calloc(num_cpus, sizeof(READ_THREAD_NODE));
        if (!sideband_r) {
            printf("Error: Unable to allocate memory for threads.\n");
            return -1;
        }
    }
    /*
     * Create a thread to read the module records and write them to a file
     */
    num_chars = DRV_SNPRINTF(READ_THREAD_dname(&mod_r),
                             THREAD_ARG_SIZE,
                             THREAD_ARG_SIZE,
                             "%s%sm",
                             device_name,  DRV_DEVICE_DELIMITER);
    if (num_chars < 0 || num_chars > THREAD_ARG_SIZE) {
        return VT_SAM_ERROR;
    }
    if (data_transfer_mode == DELAYED_TRANSFER) {
        num_chars = DRV_SNPRINTF(READ_THREAD_oname(&mod_r),
                                THREAD_ARG_SIZE,
                                THREAD_ARG_SIZE,
                                "%so%u_user.mod",
                                seed_name, agent_osid);
        if (num_chars < 0 || num_chars > THREAD_ARG_SIZE) {
            return VT_SAM_ERROR;
        }
    }

    READ_THREAD_me(&mod_r)       = 0;
    READ_THREAD_conn_id(&mod_r)    = COMM_MODULE_CONN_ID;
    READ_THREAD_conn_type(&mod_r)  = COMM_DATA_MODULE;
    status = abstract_Initialize_Read_Thread(&mod_r);
    if (status) {
        SEPAGENT_PRINT_ERROR("return code from pthread_create() is %d\n", status);
        exit(-1);
    }

    SEPAGENT_PRINT_DEBUG("Created module reader thread %s \n", READ_THREAD_dname(&mod_r));

    /*
     * Create a thread for each cpu to read the sampling records and write them to a file
     */
    if (agent_mode == NATIVE_AGENT || agent_mode == HOST_VM_AGENT) {
        for (i = 0; i < num_cpus; i++) {
            READ_THREAD  lt = &samp_r[i];
            num_chars = DRV_SNPRINTF(READ_THREAD_dname(lt),
                                     THREAD_ARG_SIZE,
                                     THREAD_ARG_SIZE,
                                     "%s%ss%d",
                                     device_name, DRV_DEVICE_DELIMITER, i);
            if (num_chars < 0 || num_chars > THREAD_ARG_SIZE) {
                return VT_SAM_ERROR;
            }
            if (data_transfer_mode == DELAYED_TRANSFER) {
                num_chars = DRV_SNPRINTF(READ_THREAD_oname(lt),
                                         THREAD_ARG_SIZE,
                                         THREAD_ARG_SIZE,
                                         "%s%d.txt",
                                         seed_name, i);
                if (num_chars < 0 || num_chars > THREAD_ARG_SIZE) {
                    return VT_SAM_ERROR;
                }
            }
            READ_THREAD_me(lt)      = i;
            READ_THREAD_conn_id(lt) = i;
            READ_THREAD_conn_type(lt) = COMM_DATA_CPU;
            status = abstract_Initialize_Read_Thread(lt);
            if (status) {
                SEPAGENT_PRINT_ERROR("while creating samp %d is %d\n", i, status);
                exit(-1);
            }
            SEPAGENT_PRINT_DEBUG("Created sample reader thread %s \n", READ_THREAD_dname(lt));
        }
    }
    /*
     * Create a thread for each cpu to read the sideband records and write them to a file
     */
    if (sched_switch_enabled) {
        for (i = 0; i < num_cpus; i++) {
            READ_THREAD  lt = &sideband_r[i];
            num_chars = DRV_SNPRINTF(READ_THREAD_dname(lt),
                                     THREAD_ARG_SIZE,
                                     THREAD_ARG_SIZE,
                                     "%s%sb%d",
                                     device_name, DRV_DEVICE_DELIMITER, i);
            if (num_chars < 0 || num_chars > THREAD_ARG_SIZE) {
                return VT_SAM_ERROR;
            }
            if (data_transfer_mode == DELAYED_TRANSFER) {
                num_chars = DRV_SNPRINTF(READ_THREAD_oname(lt),
                                        THREAD_ARG_SIZE,
                                        THREAD_ARG_SIZE,
                                        "%so%u_v%d_sideband.txt",
                                        seed_name, agent_osid, i);
                if (num_chars < 0 || num_chars > THREAD_ARG_SIZE) {
                    return VT_SAM_ERROR;
                }
            }
            READ_THREAD_me(lt)      = i;
            READ_THREAD_conn_id(lt)   = i;
            READ_THREAD_conn_type(lt) = COMM_DATA_SIDEBAND;
            status = abstract_Initialize_Read_Thread(lt);
            if (status) {
                SEPAGENT_PRINT_ERROR("while creating samp %d is %d\n", i, status);
                exit(-1);
            }
            SEPAGENT_PRINT_DEBUG("Created sideband reader thread %s\n", READ_THREAD_dname(lt));
        }
    }
    return VT_SUCCESS;
}


/* ------------------------------------------------------------------------- */
/*!
 * @fn          abstract_Spawn_Pthreads_UNC(num_packages)
 *
 * @param       U32        num_packages - number of threads to spawn for reading samples
 *
 * @brief       Spawn n+2 threads to create the uncore sample temp files
 *              Allocate and set up per-thread argument structures
 *              Allocate and set up pthread data structures,
 *              later used for joining in abstract_Join_Pthreads()
 *
 * @return      DRV_STATUS - 0 for success, otherwise for failure
 *
 */
static DRV_STATUS
abstract_Spawn_Pthreads_UNC (
    U32         num_packages
)
{
    int  status;
    int  num_chars;
    int  i;
    char *device_name = NULL;

    if (prev_driver_loaded) {
        device_name = SEP_PREV_DEVICE_NAME;
    }
    else {
        device_name = SEP_DEVICE_NAME;
    }

    /*
     * Alloc space for uncore sample readers. Exit if any problems
     */
    uncsamp_r  = (READ_THREAD)calloc(num_packages, sizeof(READ_THREAD_NODE));
    if (!uncsamp_r) {
        printf("Error: Unable to allocate memory for threads.\n");
        return -1;
    }

    /*
     * Create a thread for each package to read the uncore sampling records and write them to a file
     *
     */
    for (i = 0; i < num_packages; i++) {
        READ_THREAD  lt = &uncsamp_r[i];
        num_chars = DRV_SNPRINTF(READ_THREAD_dname(lt),
                                 THREAD_ARG_SIZE,
                                 THREAD_ARG_SIZE,
                                 "%s%su%d",
                                 device_name, DRV_DEVICE_DELIMITER, i);
        if (num_chars < 0 || num_chars > THREAD_ARG_SIZE) {
            return VT_SAM_ERROR;
        }
        if (data_transfer_mode == DELAYED_TRANSFER) {
            num_chars = DRV_SNPRINTF(READ_THREAD_oname(lt),
                                     THREAD_ARG_SIZE,
                                     THREAD_ARG_SIZE,
                                     "%s%d.unc",
                                     seed_name, i);
            if (num_chars < 0 || num_chars > THREAD_ARG_SIZE) {
                return VT_SAM_ERROR;
            }
        }
        READ_THREAD_me(lt)       = i;
        READ_THREAD_conn_id(lt)    = COMM_UNCORE_CONN_ID;
        READ_THREAD_conn_type(lt)  = COMM_DATA_UNCORE;
        status = abstract_Initialize_Read_Thread(lt);
        if (status) {
            SEPAGENT_PRINT_ERROR("while creating unc samp %d is %d\n", i, status);
            exit(-1);
        }
        SEPAGENT_PRINT_DEBUG("Created uncore sample reader thread %s\n", READ_THREAD_dname(lt));
    }
    return VT_SUCCESS;
}


/* ------------------------------------------------------------------------- */
/*!
 * @fn          abstract_Join_Pthreads(num_cpus)
 *
 * @param       int num_cpus - number of sample threads
 *
 * @brief       Wait for the n+1 threads spawned above to finish
 *              Free per-thread argument and pthread structures
 *
 * @return      DRV_STATUS   - 0 for success, otherwise for failure
 *
 */
static DRV_STATUS
abstract_Join_Pthreads (
    int num_cpus
)
{
    int    i, status;
    PVOID  join_status;

    SEPAGENT_PRINT_DEBUG("Start joining the pthreads\n");

    if (agent_mode == NATIVE_AGENT || agent_mode == HOST_VM_AGENT) {
        for (i = 0; i < num_cpus; i++) {
            READ_THREAD lt = &samp_r[i];
            if (READ_THREAD_thread(lt) == 0) {
                continue;
            }
            pthread_attr_destroy(&READ_THREAD_attr(lt));
            status = pthread_join(READ_THREAD_thread(lt), &join_status);
            if (status) {
                SEPAGENT_PRINT_ERROR("pthread_join()[%d] returns %d\n", i, status);
                exit(-1);
            }
            SEPAGENT_PRINT_DEBUG("Sample reading[%d] done\n", i);
        }
    }

    if (sched_switch_enabled) {
        for (i = 0; i < num_cpus; i++) {
            READ_THREAD lt = &sideband_r[i];
            if (READ_THREAD_thread(lt) == 0) {
                continue;
            }
            pthread_attr_destroy(&READ_THREAD_attr(lt));
            status = pthread_join(READ_THREAD_thread(lt), &join_status);
            if (status) {
                SEPAGENT_PRINT_ERROR("pthread_join()[%d] returns %d\n", i, status);
                exit(-1);
            }
            SEPAGENT_PRINT_DEBUG("Sideband info reading[%d] done\n", i);
        }
    }

    pthread_attr_destroy(&READ_THREAD_attr(&mod_r));
    status = pthread_join(READ_THREAD_thread(&mod_r), &join_status);
    if (status) {
        SEPAGENT_PRINT_ERROR("pthread_join() on module read returns %d\n", status);
        exit(-1);
    }
    SEPAGENT_PRINT_DEBUG("Module thread done\n");
    SEPAGENT_PRINT_DEBUG("Completed join with status=%ld\n", (long)join_status);
    return OS_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn          abstract_Join_Pthreads_UNC(num_packages)
 *
 * @param       int num_packages - number of uncore sample threads
 *
 * @brief       Wait for the n+1 threads spawned above to finish
 *              Free per-thread argument and pthread structures
 *
 * @return      DRV_STATUS   - 0 for success, otherwise for failure
 *
 */
static DRV_STATUS
abstract_Join_Pthreads_UNC (
    U32 num_packages
)
{
    int    i, status;
    PVOID  join_status = NULL;

    SEPAGENT_PRINT_DEBUG("Start joining the uncore pthreads\n");

    for (i = 0; i < num_packages; i++) {
        READ_THREAD lt = &uncsamp_r[i];
        if (READ_THREAD_thread(lt) == 0) {
            continue;
        }
        pthread_attr_destroy(&READ_THREAD_attr(lt));
        status = pthread_join(READ_THREAD_thread(lt), &join_status);
        if (status) {
            SEPAGENT_PRINT_ERROR("pthread_join()[%d] returns %d\n", i, status);
            exit(-1);
        }
        SEPAGENT_PRINT_DEBUG("Uncore sample reading[i] done\n");
    }
    if (join_status != NULL) {
        SEPAGENT_PRINT_DEBUG("Completed join with status=%ld\n", (long)join_status);
    }
    return OS_SUCCESS;
}

/*
 *  End File gather implementation
 */


#if 0
/* ------------------------------------------------------------------------- */
/*!
 * @fn          ABSTRACT_Start_Sampling (buffer_size)
 *
 * @param       U32 num_cpus   - Return the number of CPU's (cores) that are in the system
 *
 * @return      DRV_STATUS     - Status Error Code
 *
 * @brief       This function will start the actual collection (sampling or counting modes).
 *              It must be preceeded by call to ABSTRACT_Prepare_For_Start().
 *              NOTE: needed to add a parameter for windows.  Ignored for now in linux.
 *
 */
DRV_DLLEXPORT DRV_STATUS
ABSTRACT_Start_Sampling (
    U32 buffer_size
)
{
    DRV_STATUS status;
    U32        bufferSize;

    bufferSize = buffer_size;
    status = abstract_Do_IOCTL_W(DRV_OPERATION_INIT_PMU, (VOID *) &bufferSize, sizeof(U32));
    if (status == VT_SUCCESS) {
        status = abstract_Do_IOCTL(DRV_OPERATION_START);
    }

    return status;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn          ABSTRACT_Stop_Sampling (flag)
 *
 * @param       DRV_BOOL flag  - 0 if only stop sampling in driver side, 1 if stop sampling in driver side, and complete the sampling session
 *
 * @brief       Stop sampling.  Called during the normal scheme of sampling.
 *
 * @return      DRV_STATUS     - Status Error Code
 *
 */
DRV_DLLEXPORT DRV_STATUS
ABSTRACT_Stop_Sampling (
    DRV_BOOL flag
)
{
    U32 kernel_cs = 0;

    abstract_Do_IOCTL(DRV_OPERATION_STOP);

    if (abs_num_cpus == 0) {
        return VT_SUCCESS;
    }
    abstract_Join_Pthreads(abs_num_cpus);
    if (unc_threads_spawn) {
        abstract_Join_Pthreads_UNC(abs_num_packages);
    }

    if (!remote_target) {
        abstract_Do_IOCTL_R(DRV_OPERATION_KERNEL_CS, (U32*)&kernel_cs, sizeof(U32));
    }

    return VT_SUCCESS;
}


/* ------------------------------------------------------------------------- */
/*!
 * @fn          ABSTRACT_Prepare_For_Start
 *
 * @param       DRV_CONFIG        pcfg          - Driver configuration node
 * @param       EVENT_CONIFIG     ec            - The event configuration node
 * @param       ECB              *ecb           - The sets of event groups and the register values that need to be programmed
 * @param       U32               size_of_alloc - size of the event blocks
 * @param       LBR               lbr           - the lbr configuration
 * @param       RO                ro            - the read-only registers configuration
 * @param       S32              *num_cpus      - number of cores that are reported to be in the system
 * @param       U32               num_descriptors
 * @param       EVENT_DESC       *desc
 * @param       DRV_TOPOLOGY_INFO drv_topology
 * @param       U32               size_of_tp
 * @param       U32               num_event_groups - number of event groups for this PMU
 * @param       U32               gm_enabled_arg   - grandmesa enable option
 * @param       U32               client_app
 *
 * @brief       This function will initialize the driver.  All buffers and temp files
 *              are set up and the system will be prepared to start collection (either
 *              sampling or counting modes).
 *              It must be called just before ABSTRACT_Start_Sampling().
 *              It must also be called just once per collector session.
 *
 * @return      DRV_STATUS                      - Status Error Code
 *
 */
DRV_DLLEXPORT DRV_STATUS
ABSTRACT_Prepare_For_Start (
    DRV_CONFIG          pcfg,
    EVENT_CONFIG        ec,
    ECB                 *ecb,
    U32                 size_of_alloc,
    LBR                 lbr,
    RO                  ro,
    S32                 *num_cpus,
    U32                 num_descriptors,
    EVENT_DESC          *desc,
    DRV_TOPOLOGY_INFO   drv_topology,
    U32                 size_of_tp,
    U32                 num_event_groups,
    U32                 gm_enabled_arg,
    U32                 client_app
)
{
    DRV_STATUS status;
#if defined(MYDEBUG)
    printf("Entered ABSTRACT_Prepare_For_Start()\n");
#endif
    g_pcfg                   = pcfg;
    counting_mode            = DRV_CONFIG_counting_mode(pcfg);
    seed_name                = DRV_CONFIG_seed_name(pcfg);

    if (status == VT_SUCCESS && !DRV_CONFIG_counting_mode(pcfg) ) {
        status = abstract_Spawn_Pthreads(*num_cpus, pcfg);
        abs_num_cpus = *num_cpus;
    }

    if (status != VT_SUCCESS) {
        printf("Error: Unable to prepare the driver to start sampling\n");
    }

    return status;
}


#if defined(DRV_IA32) || defined(DRV_EM64T)/* ------------------------------------------------------------------------- */
/*!
 * @fn          ABSTRACT_Prepare_For_Start_UNC(pcfg_unc, ec, *ecb_unc, size_of_alloc, num_units, num_event_groups)
 *
 * @param       pcfg_unc               - Driver configuration node
 * @param       ec                     - The event configuration node
 * @param       ecb_unc                - The sets of event groups and the register values
 *                                       that need to be programmed
 * @param       size_of_alloc          - size of the event blocks
 * @param       num_units              - number of units for this PMU
 * @param       num_event_groups       - number of event groups for this PMU
 *
 * @brief       This function will initialize the driver.  All buffers and temp files
 *              are set up and the system will be prepared to start sampling.
 *              It must be called just before ABSTRACT_Start_Sampling().
 *              This must also be called just once per sampling session.
 *
 * @return      DRV_STATUS - 0 for success, otherwise for failure
 *
 */
DRV_DLLEXPORT DRV_STATUS
ABSTRACT_Prepare_For_Start_UNC (
    DRV_CONFIG        pcfg_unc,
    EVENT_CONFIG      ec,
    ECB              *ecb_unc,
    U32               size_of_alloc,
    U32               num_units,
    U32               num_event_groups,
    U32               num_packages
)
{
    DRV_STATUS status;

    printf("Entered lwpmu_prepare_start_unc()\n");

    status = abstract_Initialize_Driver_UNC(pcfg_unc);
    if (status == VT_SUCCESS) {
        status = abstract_Program_PMU_UNC(ec, ecb_unc, size_of_alloc, num_event_groups);
    }
    if (status == VT_SUCCESS) {
        status = abstract_Set_Units_For_Device(num_units);
    }
    if (status == VT_SUCCESS && !counting_mode && !unc_threads_spawn) {
        status = abstract_Spawn_Pthreads_UNC(num_packages);
        abs_num_packages = num_packages;
        unc_threads_spawn = TRUE;
    }
    if (status != VT_SUCCESS) {
        printf("Error: Unable to prepare the driver to start sampling.\n");
        fflush(stderr);
    }

    return status;
}
#endif
#endif


/******************************************************************************
 * @fn          abstract_Start_Threads()
 *
 * @brief       All buffers and temp files
 *              are set up and the system will be prepared to start collection (either
 *              sampling or counting modes).
 *              It must be called just before ABSTRACT_Start_Sampling().
 *              It must also be called just once per collector session.
 * @param       None
 *
 * @return      Status
 ******************************************************************************/
static DRV_STATUS
abstract_Start_Threads (
    S8     *pcfg_buf
)
{
    U32 status    = VT_SUCCESS;

    if (pcfg_buf == NULL) {
        SEPAGENT_PRINT_ERROR("got NULL pcfg_buf!\n");
        return VT_SAM_ERROR;
    }

    counting_mode = DRV_CONFIG_emon_mode((DRV_CONFIG)pcfg_buf);

    if (agent_mode == GUEST_VM_AGENT || agent_mode == HOST_VM_AGENT) {
        sched_switch_enabled = TRUE;
    }
    // generating seed_name
    if (data_transfer_mode == DELAYED_TRANSFER) {
        seed_name = calloc(1, MAXNAMELEN * sizeof(S8));
        if (seed_name == NULL) {
            SEPAGENT_PRINT_ERROR("Unable to allocate memory for seed_name\n");
            return VT_NO_MEMORY;
        }
        DRV_SNPRINTF(seed_name, MAXNAMELEN, MAXNAMELEN, "/tmp/lwp%lu_", (unsigned long)(((DRV_CONFIG)pcfg_buf)->u1.seed_name));
        SEPAGENT_PRINT_DEBUG("seedname %s\n",seed_name);
    }
    if (!counting_mode) {
        status = abstract_Spawn_Pthreads(abs_num_cpus, NULL);
    }

    if (status != VT_SUCCESS) {
        SEPAGENT_PRINT_ERROR("Unable to prepare the driver to start sampling.\n");
    }
    return status;
}

/******************************************************************************
 * @fn          abstract_Start_Threads_UNC()
 *
 * @brief       All buffers and temp files for uncore
 *              are set up and the system will be prepared to start sampling.
 *              It must be called just before ABSTRACT_Start_Sampling().
 *              This must also be called just once per sampling session.
 *
 * @param       None
 *
 * @return      Status
 ******************************************************************************/
static DRV_STATUS
abstract_Start_Threads_UNC (
    U32 num_packages
)
{
    U32 status = VT_SUCCESS;

    if (!counting_mode && (agent_mode != GUEST_VM_AGENT)) {
        status = abstract_Spawn_Pthreads_UNC(num_packages);
        abs_num_packages = num_packages;
        unc_threads_spawn = TRUE;
    }
    if (status != VT_SUCCESS) {
        SEPAGENT_PRINT_ERROR("Unable to prepare the driver to start sampling.\n");
    }
    return status;
}

/******************************************************************************
 * @fn          abstract_Stop_Threads()
 *
  * @brief      Stop threads.
 *
 * @param       None
 *
 * @return      Status
 ******************************************************************************/
static DRV_STATUS
abstract_Stop_Threads (
    void
)
{
    U32 status   = VT_SUCCESS;
    U32 num_cpus = 0;

    if (!counting_mode) {
        abstract_Join_Pthreads(abs_num_cpus);
    }
    if (unc_threads_spawn) {
        abstract_Join_Pthreads_UNC(abs_num_packages);
    }

    return status;
}

/******************************************************************************
 * @fn          abstract_Set_OSID(S8 *buf)
 *
  * @brief      set osid
 *
 * @param       IN osid - pointer to osid
 *
 * @return      None
 ******************************************************************************/
static VOID
abstract_Set_OSID(
    S8 *buf
)
{
    if (buf == NULL) {
        SEPAGENT_PRINT_ERROR("got NULL buf for OSID!\n");
        return;
    }
    memcpy(&agent_osid, buf, sizeof(U32));
}


/* ------------------------------------------------------------------------- */
/*!
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
)
{
    DRV_STATUS status = VT_SUCCESS;

    if (num_cpus == NULL) {
        SEPAGENT_PRINT_ERROR("got NULL num_cpus!\n");
        return VT_SAM_ERROR;
    }

    *num_cpus = 0;
    // retrieve the number of CPUS
    status = abstract_Do_IOCTL_R(DRV_OPERATION_NUM_CORES,
                                 (VOID *)num_cpus,
                                 sizeof(S32));
    abs_num_cpus = *num_cpus;
    return status;
}

/* ------------------------------------------------------------------------- */
/*!
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
)
{
    U32        version = 0;
    DRV_STATUS status;

    status = abstract_Do_IOCTL_R(DRV_OPERATION_VERSION,
                                 (PVOID)(&version),
                                 sizeof(U32));
    if (status != VT_SUCCESS) {
        return 0;
    }

    return version;
}

/* ------------------------------------------------------------------------- */
/*!
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
ABSTRACT_Get_Agent_Mode(
    U32 *agentmode,
    U32 transfer_mode
)
{
    DRV_STATUS status;

    if (agentmode == NULL) {
        SEPAGENT_PRINT_ERROR("got NULL agentmode!\n");
        return VT_SAM_ERROR;
    }

    *agentmode = 0;

    status = abstract_Do_IOCTL_R(DRV_OPERATION_GET_AGENT_MODE,
                                 (PVOID)(agentmode),
                                 sizeof(U32));
    if (status == VT_SUCCESS) {
        agent_mode = *agentmode;
    }
    data_transfer_mode = transfer_mode;

    return status;
}

/* ------------------------------------------------------------------------- */
/*!
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
)
{
    struct stat kvm_stat;
    stat( "/dev/kvm" , &kvm_stat);
    if (S_ISCHR(kvm_stat.st_mode)) {
        return TRUE;
    }
    return FALSE;
}


/* ------------------------------------------------------------------------- */
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
ABSTRACT_Get_Drv_Setup_Info(
    DRV_SETUP_INFO     drv_setup_info
)
{
    U32 status = VT_SUCCESS;

    if (drv_setup_info == NULL) {
        SEPAGENT_PRINT_ERROR("got NULL drv_setup_info!\n");
        return VT_SAM_ERROR;
    }

    status = abstract_Do_IOCTL_R(DRV_OPERATION_GET_DRV_SETUP_INFO,
                                (VOID*)drv_setup_info,
                                sizeof(DRV_SETUP_INFO_NODE));

    return status;
}


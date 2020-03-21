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
#include "lwpmudrv_version.h"
#include "lwpmudrv_types.h"
#include "rise_errors.h"
#include "lwpmudrv_ecb.h"
#include "lwpmudrv_ioctl.h"
#include "lwpmudrv_struct.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <pthread.h>
#include <sys/types.h>
#include "abstract.h"
#include "abstract_service.h"
#include "communication.h"
#include "collection_traces.h"
#include "sepagent_parser.h"
#include "log.h"

static int  num_cpus           = 0;
S8         *sepagent_debug_var = NULL;
FILE       *fptr = NULL;
U32         data_transfer_mode = IMMEDIATE_TRANSFER; // default transfer mode

#define MILLION       1000000

/* ------------------------------------------------------------------------- */
/*!
 * @fn       VOID UTILITY_Read_Cpuid
 *
 * @brief    executes the cpuid_function of cpuid and returns values
 *
 * @param  IN   cpuid_function
 *         OUT  rax  - results of the cpuid instruction in the
 *         OUT  rbx  - corresponding registers
 *         OUT  rcx
 *         OUT  rdx
 *
 * @return   none
 *
 * <I>Special Notes:</I>
 *              <NONE>
 *
 */
static void
sepagent_Read_Cpuid (
    U64   cpuid_function,
    U64  *rax_value,
    U64  *rbx_value,
    U64  *rcx_value,
    U64  *rdx_value
)
{
#if defined(DRV_IA32)
    /* GCC trashes ebx so save it and -mdynamic-no-pic will not work as
     * platform_picker need to be a dylib.
     */
    U32        selector = cpuid_function;
    U32        data[4];

    asm("pushl %%ebx            \n\t"
        "cpuid                  \n\t"
        "movl %%ebx, %%esi      \n\t"
        "popl %%ebx             \n\t"
        : "=a"(data[0]),
        "=S"(data[1]),
        "=c"(data[2]),
        "=d"(data[3])
        :"a"(selector));
    *rax_value = (U64)(data[0]);
    *rbx_value = (U64)(data[1]);
    *rcx_value = (U64)(data[2]);
    *rdx_value = (U64)(data[3]);
#endif

#if defined(DRV_EM64T)
    /* GCC trashes ebx so save it and -mdynamic-no-pic will not work as
     * platform_picker need to be a dylib.
     */
    U32        selector = cpuid_function;
    U64        data[4];

    asm("push %%rbx            \n\t"
        "cpuid                  \n\t"
        "movq %%rbx, %%rsi      \n\t"
        "pop %%rbx             \n\t"
        : "=a"(data[0]),
        "=S"(data[1]),
        "=c"(data[2]),
        "=d"(data[3])
        :"a"(selector));
    *rax_value = (U64)(data[0]);
    *rbx_value = (U64)(data[1]);
    *rcx_value = (U64)(data[2]);
    *rdx_value = (U64)(data[3]);
#endif

    return;
}

/*!
 * @fn  U64 lwpmu_Get_Tsc_Frequency(void)
 *
 * @param none
 *
 * @return U64
 *
 * @brief  Get processor reference frequency.
 */
static U64
sepagent_Get_Tsc_Frequency (
    void
)
{
    U64        rxs[4];
    char       buf[64] = {0};

    int        i       = 0;
    int        j       = 0;

    char*      pos     = NULL;
    int        hi      = 0;
    int        lo      = 0;
    char       c       = 0;
    int        num     = 0;

    U64 tsc_freq       = 0;

    sepagent_Read_Cpuid(0x80000000, &rxs[0], &rxs[1], &rxs[2], &rxs[3]);
    // check that processor brand string is supported
    if (rxs[0] > 0x80000004) {
        int count = 0;
        for (i = 0; i < 3; i++) {
            sepagent_Read_Cpuid(0x80000002 + i, &rxs[0], &rxs[1], &rxs[2], &rxs[3]);
            for (j = 0; j < 4; j++) {
                memcpy(&buf[count * sizeof(U32)], &rxs[j], sizeof(U32));
                count++;
            }
        }
        pos = strrchr(&buf[0], ' ');
        if (pos) {
            num = sscanf(pos, " %d.%d%1cHz\n", &hi, &lo, &c);
            if (num == 3) {
                tsc_freq = hi * 100 + lo;
                if (c == 'M') {
                    tsc_freq *= 10000;
                }
                else if (c == 'G') {
                    tsc_freq *= 10000000UL;
                }
                else if (c == 'T') {
                    tsc_freq *= 10000000000UL;
                }
            }
        }
    }
    return tsc_freq;
}

void sepagent_version_info() {

    SEP_VERSION_NODE    driver_version;
    SEP_VERSION_NODE_sep_version(&driver_version) = ABSTRACT_Version();

    fprintf(stdout, "Analysis Communication Agent User Mode Version: %d.%d.%d %s\n",
        SEP_MAJOR_VERSION, SEP_MINOR_VERSION, SEP_API_VERSION, SEP_RELEASE_STRING);

    fprintf(stdout, "SEP Driver Version: ");
    if (SEP_VERSION_NODE_sep_version(&driver_version) != 0) {
        fprintf(stdout, "%d.%d.%d %s\n",
                        SEP_VERSION_NODE_major(&driver_version),
                        SEP_VERSION_NODE_minor(&driver_version),
                        SEP_VERSION_NODE_api(&driver_version),
                        SEP_RELEASE_STRING);
    }
    else {
        fprintf(stdout, "Error retrieving SEP driver version\n");
    }
}


/* ------------------------------------------------------------------------- */
/*!
 * @fn          void sepagent_Print_Version (void)
 *
 * @brief       Print the SEP Version Information
 *
 * @param       status
 *
 * @return      NONE
 *
 * <I>Special Notes:</I>
 */
int sepagent_Print_Version(
    void
)
{
    U64                 tsc_freq;
    DRV_SETUP_INFO_NODE drv_setup_info;
    int                 status;

    sepagent_version_info();

    fprintf(stdout, "Processors configured ...... %ld\n", sysconf(_SC_NPROCESSORS_ONLN));//get_nprocs());
    fprintf(stdout, "Processors online ...... %ld\n", sysconf(_SC_NPROCESSORS_CONF)); //get_nprocs_conf());

    ABSTRACT_Num_CPUs(&num_cpus);
    fprintf(stdout, "Processors configured (Driver)...... %d\n", num_cpus);

    tsc_freq = sepagent_Get_Tsc_Frequency();
    fprintf(stdout, "TSC Freq .................. %.2f MHz\n", (double)tsc_freq/MILLION);

    status = ABSTRACT_Get_Drv_Setup_Info(&drv_setup_info);
    if (status == VT_SUCCESS) {
        fprintf(stdout, "Driver configs:");
        if (DRV_SETUP_INFO_nmi_mode(&drv_setup_info)) {
            fprintf(stdout, " Non-Maskable Interrupt");
        }
        else {
            fprintf(stdout, " Maskable Interrupt");
        }

        if (DRV_SETUP_INFO_pebs_ignored_by_pti(&drv_setup_info)) {
            fprintf(stdout, ", PEBS OFF (by OS security)");
        }

        if (DRV_SETUP_INFO_matrix_inaccessible(&drv_setup_info)) {
            fprintf(stdout, ", OFFCORE OFF (by OS security)");
        }
        fprintf(stdout, "\n");
        if (DRV_SETUP_INFO_vmm_mode(&drv_setup_info) && DRV_SETUP_INFO_vmm_vendor(&drv_setup_info)) {
            fprintf(stdout, "Virtualization platform: %s VM on ",
                    DRV_SETUP_INFO_vmm_guest_vm(&drv_setup_info)? "Guest":"Host");
            switch(DRV_SETUP_INFO_vmm_vendor(&drv_setup_info)) {
                case DRV_VMM_KVM:
                    fprintf(stdout, "KVM\n");
                    break;
                case DRV_VMM_XEN:
                    fprintf(stdout, "Xen\n");
                    break;
                case DRV_VMM_HYPERV:
                    fprintf(stdout, "Hyper-V \n");
                    break;
                case DRV_VMM_VMWARE:
                    fprintf(stdout, "VMware\n");
                    break;
                case DRV_VMM_ACRN:
                    fprintf(stdout, "ACRN\n");
                    break;
                default:
                    fprintf(stdout, "Unknown VMM\n");
            }
        }
    }
    fprintf(stdout, "%s\n", SEP_PRODUCT_COPYRIGHT);

    return VT_SUCCESS;
}


/* ------------------------------------------------------------------------- */
/*!
 * @fn          S32  sepagent_Open_Data_Channels ( agent_mode )
 *
 * @brief       Establish all the data channels required to transfer data
 *
 * @param       OUT agent_mode  - agent mode
 *
 * @return      Status
 *
 * <I>Special Notes:</I>
 *              <NONE>
 */
static S32
sepagent_Open_Data_Channels(
    U32 agent_mode
)
{
    int i = 0;
    S32 ret = VT_SUCCESS;

    if (agent_mode == NATIVE_AGENT || agent_mode == HOST_VM_AGENT) {
        for (i = 0; i < num_cpus; i++) {
            ret = COMM_Open_Data_On_Target(i, COMM_DATA_CPU);
            if (ret != VT_SUCCESS) {
                return ret;
            }
        }
        ret = COMM_Open_Data_On_Target(COMM_UNCORE_CONN_ID, COMM_DATA_UNCORE);
        if (ret != VT_SUCCESS) {
            return ret;
        }
    }
    ret = COMM_Open_Data_On_Target(COMM_MODULE_CONN_ID, COMM_DATA_MODULE);
    if (ret != VT_SUCCESS) {
        return ret;
    }
    if (agent_mode == HOST_VM_AGENT || agent_mode == GUEST_VM_AGENT) {
        for (i = 0; i < num_cpus; i++) {
            ret = COMM_Open_Data_On_Target(i, COMM_DATA_SIDEBAND);
            if (ret != VT_SUCCESS) {
                return ret;
            }
        }
    }

    return ret;
}


/* ------------------------------------------------------------------------- */
/*!
 * @fn          S32  sepagent_Close_Data_Channels ( agent_mode )
 *
 * @brief       Close all the data channels opened for data transfer
 *
 * @param       OUT agent_mode  - agent mode
 *
 * @return      Status
 *
 * <I>Special Notes:</I>
 *              <NONE>
 */
static S32
sepagent_Close_Data_Channels(
    U32 agent_mode
)
{
    int i = 0;
    S32 ret;
    S32 status = VT_SUCCESS;

    if (agent_mode == NATIVE_AGENT || agent_mode == HOST_VM_AGENT) {
        for (i = 0; i < num_cpus; i++) {
            ret = COMM_Close_Data_On_Target(i, COMM_DATA_CPU);
            if (status == VT_SUCCESS) {
                status = ret;
            }
        }
        ret = COMM_Close_Data_On_Target(COMM_UNCORE_CONN_ID, COMM_DATA_UNCORE);
        if (status == VT_SUCCESS) {
            status = ret;
        }
    }
    ret = COMM_Close_Data_On_Target(COMM_MODULE_CONN_ID, COMM_DATA_MODULE);
    if (status == VT_SUCCESS) {
        status = ret;
    }
    if (agent_mode == HOST_VM_AGENT || agent_mode == GUEST_VM_AGENT) {
        for (i = 0; i < num_cpus; i++) {
            ret = COMM_Close_Data_On_Target(i, COMM_DATA_SIDEBAND);
            if (status == VT_SUCCESS) {
                status = ret;
            }
        }
    }

    return status;
}


int main(int argc, char* argv[])
{
    IOCTL_ARGS_NODE ioctl_arg;
    U32             cmd;
    int             status;
    U64             rax, rbx, rcx, rdx;
    int             i                  = 0;
    S32             ret                = VT_SUCCESS;
    S32             size               = MAX_STRING_LENGTH;
    U64             tsc_freq           = 0;
    U32             agent_mode         = NATIVE_AGENT;

    DRV_GETENV(sepagent_debug_var, size, "SEPAGENT_DEBUG");
    if (sepagent_debug_var  != NULL){
        fptr = fopen(sepagent_debug_var, "w");
        if (fptr == NULL) {
          fprintf(stderr, "Can't create file %s for logging!\n",sepagent_debug_var);
        }
    }

    status = SEPAGENT_Parser(argc, argv, &data_transfer_mode);
    if (status != VT_SUCCESS) {
        SEPAGENT_Print_Help();
        exit(-1);
    }
    else {
        sepagent_version_info();
    }

    status = ABSTRACT_Num_CPUs(&num_cpus);
    if (status != VT_SUCCESS) {
        SEPAGENT_PRINT_ERROR("Check if sep driver is loaded with appropriate permissions \n");
        exit(-1);
    }

    fprintf(stdout, "Analysis Communication Agent running in ");
    switch(agent_mode) {
        case HOST_VM_AGENT:
            fprintf(stdout, "Host VM mode");
            break;
        case GUEST_VM_AGENT:
            fprintf(stdout, "Guest VM mode");
            break;
        case NATIVE_AGENT:
            fprintf(stdout, "Native mode");
            break;
        default:
            fprintf(stdout, "Invalid mode");
    }
    switch(data_transfer_mode) {
        case IMMEDIATE_TRANSFER:
            fprintf(stdout, " with immediate data transfer\n");
            break;
        case DELAYED_TRANSFER:
            fprintf(stdout, " with delayed data transfer\n");
            break;
    }

    fprintf(stdout, "Number of cpus ..... %d \n", num_cpus);

    tsc_freq = sepagent_Get_Tsc_Frequency();
    sepagent_Read_Cpuid(1, &rax, &rbx, &rcx, &rdx);

    while (ret == VT_SUCCESS) {  // Make the connection ready for next collection
        cmd = 0;
        ret = COMM_Open_Control_On_Target(0, rax, tsc_freq, agent_mode, data_transfer_mode, num_cpus);
        if (ret != VT_SUCCESS) {
            COMM_Close_Control_On_Target();
            break;
        }

        ret = sepagent_Open_Data_Channels(agent_mode);
        if (ret != VT_SUCCESS) {
            ret = sepagent_Close_Data_Channels(agent_mode);
            break;
        }

        while (1) {
            cmd = 0;
            ret = COMM_Receive_Control_Request_On_Target(&cmd, &ioctl_arg, -1);

            if (ret == VT_SUCCESS) {
                if (cmd == 0) {
                    if (ioctl_arg.len_drv_to_usr > 0 && ioctl_arg.buf_drv_to_usr) {
                        free(ioctl_arg.buf_drv_to_usr);
                        ioctl_arg.buf_drv_to_usr = 0;
                        ioctl_arg.len_drv_to_usr = 0;
                    }
                    if (ioctl_arg.len_usr_to_drv > 0 && ioctl_arg.buf_usr_to_drv) {
                        free(ioctl_arg.buf_usr_to_drv);
                        ioctl_arg.buf_usr_to_drv = 0;
                        ioctl_arg.len_usr_to_drv = 0;
                    }
                    continue;
                }

                ret = ABSTRACT_Send_IOCTL(cmd, &ioctl_arg);
            }

            ret = COMM_Send_Control_Response_On_Target(cmd, &ioctl_arg, ret, FALSE, -1);

            if (ioctl_arg.len_drv_to_usr > 0 && ioctl_arg.buf_drv_to_usr) {
                free(ioctl_arg.buf_drv_to_usr);
                ioctl_arg.buf_drv_to_usr = 0;
                ioctl_arg.len_drv_to_usr = 0;
            }
            if (ioctl_arg.len_usr_to_drv > 0 && ioctl_arg.buf_usr_to_drv) {
                free(ioctl_arg.buf_usr_to_drv);
                ioctl_arg.buf_usr_to_drv = 0;
                ioctl_arg.len_usr_to_drv = 0;
            }

            if (ret != VT_SUCCESS) {
                ret = sepagent_Close_Data_Channels(agent_mode);
                break;
            }

            if (cmd == DRV_OPERATION_STOP) {
                ret = sepagent_Close_Data_Channels(agent_mode);
            }
            if (cmd == DRV_OPERATION_TERMINATE) {
                break;
            }
        }

        ret = COMM_Close_Control_On_Target();
    }

    return VT_SUCCESS;
}

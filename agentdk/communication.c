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

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/utsname.h>
#include <unistd.h>

#include "lwpmudrv_defines.h"
#include "lwpmudrv_types.h"
#include "rise_errors.h"
#include "lwpmudrv_ioctl.h"
#include "lwpmudrv_ecb.h"
#include "lwpmudrv_struct.h"
#include "lwpmudrv_version.h"
#include "communication.h"
#include "collection_traces.h"
#include "log.h"

static int                 control_socket, server_socket;
static int                *data_socket;
static struct              sockaddr_in  server_socket_info, control_socket_info;
static CONTROL_FIRST_MSG   first_control_msg = NULL;
static struct              utsname sysinfo;
static S32                 num_cpus = 0;
static U32                 num_of_data_connections = 0;
static U32                 num_of_total_connections = 0;

S32
comm_Get_Data_Socket_Array_Index (
    U32 conn_id,
    U32 conn_type
)
{
    S32 idx = -1;

    switch (conn_type)
    {
        case COMM_DATA_CPU:
            idx = conn_id;
            break;
        case COMM_DATA_MODULE:
            idx = num_cpus;
            break;
        case COMM_DATA_UNCORE:
            idx = num_cpus + 1;
            break;
        case COMM_DATA_SIDEBAND:
            idx = num_cpus + 2 + conn_id;
            break;
        default:
            SEPAGENT_PRINT_ERROR("Invalid conn_type=%u, conn_id=%u\n", conn_type, conn_id);
    };

    if (idx >= num_of_data_connections) {
        SEPAGENT_PRINT_ERROR("socket idx is bigger than the number of data connections allowed %d\n", idx);
        idx = -1;
    }

    return idx;
}

S32
COMM_Open_Control_On_Target (
    U32 mode,
    U64 cpuid_rax,
    U64 tsc_freq,
    U32 agent_mode,
    U32 transfer_mode,
    U32 num_of_cpus
)
{
    S32                     data_size        = 0;
    S32                     data_transferred = 0;
    S32                     socket_size;
    int                     saddr_len        = 0;
    TARGET_STATUS_MSG_NODE  status_msg;
    S32                     sent_bytes       = 0;
    S8                      ip_addr_str[INET_ADDRSTRLEN];
    struct sockaddr_in     *addr_ptr;
    S32                     rcvbuff_size     = CONTROL_SOCKET_RECV_BUF_SIZE;
    S32                     sendbuff_size    = DATA_SOCKET_SEND_BUF_SIZE;
    S32                     retcode          = VT_SUCCESS;
    U32                     offset;

    num_cpus                = num_of_cpus;
    num_of_data_connections = num_cpus * 2 + NUM_NONCORE_DATA_CONNECTIONS;
    num_of_total_connections = num_of_data_connections + 1;

    if (server_socket == 0) {
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        SEPAGENT_PRINT_ERROR("Could not create a socket\n");
        return VT_INTERNAL_ERROR;
    }

    server_socket_info.sin_family      = AF_INET;
    server_socket_info.sin_addr.s_addr = htonl(INADDR_ANY);
    server_socket_info.sin_port        = htons(DEFAULT_CONTROL_PORT);

    /*
       Default input (SO_RCVBUF) buffer is about 32000 bytes and
       it is usefull for high-size sent data but
       control command usually sends about 100 bytes.
       As result TCP ack delay mechanism adds 200ms delay for each low-size receive.
       Count of GE events * delay = ~10 secs start collection delay.
       Decreasing buffer to 128 bytes provides collection run immediately.
    */
    if (setsockopt(server_socket, SOL_SOCKET, SO_RCVBUF, &rcvbuff_size, sizeof(rcvbuff_size)) < 0) {
        SEPAGENT_PRINT_ERROR("Could not set socket recieve buffer\n" );
        return VT_COMM_RECV_BUF_RESIZE_ERROR;
    }

    /*
       send blocks until kernel can accept all of your data, and/or the network stack asynchronously de-queueing data and pushing it to the network card, thus freeing space in the buffer.
       Default SO_SNDBUF buffer is 32,767 bytes which is in-kernel buffer size available for that socket.
       If buffer of high-size sent than send might hang.
       Solution 1: By Increasing SO_SNDBUF buffer size to allow buffer of size upto 1MB (1,048,576 bytes)
       If problem still exists, it could be because of constraints on TCP sendbuffer memory space each TCP socket can use (command sysctl net.ipv4.tcp_wmem (linux)).
       Solution 2: To change system settings (temporary)
    */
    if (setsockopt(server_socket, SOL_SOCKET, SO_SNDBUF, &sendbuff_size, sizeof(sendbuff_size)) < 0) {
        SEPAGENT_PRINT_ERROR("Could not set socket send buffer\n" );
        return VT_COMM_SEND_BUF_RESIZE_ERROR;
    }

    if (bind(server_socket, (struct sockaddr *)&server_socket_info,
             sizeof(server_socket_info)) < 0) {
        SEPAGENT_PRINT_ERROR("Couldn't bind socket");
        return VT_COMM_BIND_ERROR;
    }
    }

    // Need the number of cpus + 2 connections for control and data
    if (listen(server_socket, num_of_total_connections) < 0) {
        SEPAGENT_PRINT_ERROR("Couldn't listen on socket");
        return VT_COMM_LISTEN_ERROR;
    }

    socket_size = sizeof(control_socket_info);
    SEPAGENT_PRINT("Waiting for control connection from host on port %d...\n", DEFAULT_CONTROL_PORT);
    if ((control_socket = accept(server_socket,
                            (struct sockaddr*)&control_socket_info,
                &socket_size)) < 0) {
        SEPAGENT_PRINT_ERROR("Couldn't accept on socket");
        return VT_COMM_ACCEPT_ERROR;
    }
    addr_ptr = (struct sockaddr_in*)&control_socket_info;
    inet_ntop(AF_INET, &addr_ptr, ip_addr_str, INET_ADDRSTRLEN);
    SEPAGENT_PRINT("Received control connection request from host (%s)\n", ip_addr_str);
    if (!(first_control_msg = (CONTROL_FIRST_MSG) malloc(sizeof(CONTROL_FIRST_MSG_NODE)))) {
        SEPAGENT_PRINT_ERROR("Couldn't allocate buffer for the first msg\n");
        return VT_NO_MEMORY;
    }

    // Read the header data at first msg
    while(data_transferred < sizeof(CONTROL_FIRST_MSG_NODE)) {
        if ((data_size = recv(control_socket, (S8*)first_control_msg+data_transferred, sizeof(CONTROL_FIRST_MSG_NODE)-data_transferred, 0)) < 0) {
            SEPAGENT_PRINT_ERROR("Couldn't receive the first msg\n");
            return VT_COMM_RECV_ERROR;
        } else if (!data_size) {
            SEPAGENT_PRINT_ERROR("Connection closed by remote\n");
            return VT_COMM_CONNECTION_CLOSED_BY_REMOTE;
        } else {
            data_transferred += data_size;
        }
    }

    SEPAGENT_PRINT_DEBUG("msg size %u\n", CONTROL_FIRST_MSG_msg_size(first_control_msg));
    SEPAGENT_PRINT_DEBUG("interface version %u\n", CONTROL_FIRST_MSG_proto_version(first_control_msg));
    SEPAGENT_PRINT_DEBUG("per_cpu_buffer_size %u\n", CONTROL_FIRST_MSG_per_cpu_buffer_size(first_control_msg));

    if (CONTROL_FIRST_MSG_msg_size(first_control_msg) != sizeof(CONTROL_FIRST_MSG_NODE)) {
        retcode = VT_COMM_NOT_COMPATIBLE;
    }

    if (data_socket) {
        SEPAGENT_PRINT_ERROR("Data sockets are already established. Can't set up data channels\n");
        retcode = VT_COMM_DATA_CHANNEL_UNAVAILABLE;
    }

    data_socket = (int *)malloc(sizeof(int) * num_of_data_connections);
    if (!data_socket) {
        SEPAGENT_PRINT_ERROR("Couldn't allocate buffer for data sockets\n");
        return VT_NO_MEMORY;

    }
    memset(data_socket, 0, sizeof(int) * num_of_data_connections);

    // Need to utilize the buffer size and return error code if any
    memset(&status_msg, 0, sizeof(TARGET_STATUS_MSG_NODE));

    TARGET_STATUS_MSG_msg_size(&status_msg) = sizeof(status_msg);
    TARGET_STATUS_MSG_proto_version(&status_msg) = PROTOCOL_VERSION;

    if (uname(&sysinfo) == -1) {
        SEPAGENT_PRINT_ERROR("Failed to collect system info via uname\n");
        strcpy(REMOTE_OS_INFO_sysname(TARGET_STATUS_MSG_os_info(&status_msg)), "Unknown OS");
        strcpy(REMOTE_OS_INFO_release(TARGET_STATUS_MSG_os_info(&status_msg)), "");
        strcpy(REMOTE_OS_INFO_version(TARGET_STATUS_MSG_os_info(&status_msg)), "");
    }
    else {
        SEPAGENT_PRINT_DEBUG("sysinfo.sysname %s\n", sysinfo.sysname);
        SEPAGENT_PRINT_DEBUG("sysinfo.release %s\n", sysinfo.release);
        SEPAGENT_PRINT_DEBUG("sysinfo.version %s\n", sysinfo.version);

        DRV_STRCPY(REMOTE_OS_INFO_sysname(TARGET_STATUS_MSG_os_info(&status_msg)), OSINFOLEN, sysinfo.sysname);
        DRV_STRCPY(REMOTE_OS_INFO_release(TARGET_STATUS_MSG_os_info(&status_msg)), OSINFOLEN, sysinfo.release);
        DRV_STRCPY(REMOTE_OS_INFO_version(TARGET_STATUS_MSG_os_info(&status_msg)), OSINFOLEN, sysinfo.version);
    }
    if (mode) {
        // Currently, only advanced hotspot mode is allowed for auto collection
        REMOTE_SWITCH_auto_mode(TARGET_STATUS_MSG_collect_switch(&status_msg))   = 1;
        REMOTE_SWITCH_adv_hotspot(TARGET_STATUS_MSG_collect_switch(&status_msg)) = 1;
        if (mode == 2) {
            REMOTE_SWITCH_lbr_callstack(TARGET_STATUS_MSG_collect_switch(&status_msg)) = 1;
        }
    }
    REMOTE_SWITCH_agent_mode(TARGET_STATUS_MSG_collect_switch(&status_msg))         = (U32)agent_mode;
    REMOTE_SWITCH_data_transfer_mode(TARGET_STATUS_MSG_collect_switch(&status_msg)) = (U32)transfer_mode;
    if (agent_mode == HOST_VM_AGENT || agent_mode == GUEST_VM_AGENT) {
        REMOTE_SWITCH_sched_switch_enabled(TARGET_STATUS_MSG_collect_switch(&status_msg)) = 1;
    }
    if (agent_mode != GUEST_VM_AGENT) {
       REMOTE_SWITCH_uncore_supported(TARGET_STATUS_MSG_collect_switch(&status_msg)) = 1;
    }

    REMOTE_HARDWARE_INFO_num_cpus(TARGET_STATUS_MSG_hardware_info(&status_msg))  = num_cpus;
    REMOTE_HARDWARE_INFO_family(TARGET_STATUS_MSG_hardware_info(&status_msg))    = (U32)(cpuid_rax >>  8 & 0x0f);
    REMOTE_HARDWARE_INFO_model(TARGET_STATUS_MSG_hardware_info(&status_msg))     = (U32)(cpuid_rax >> 12 & 0xf0);
    REMOTE_HARDWARE_INFO_model(TARGET_STATUS_MSG_hardware_info(&status_msg))    |= (U32)(cpuid_rax >>  4 & 0x0f);
    REMOTE_HARDWARE_INFO_stepping(TARGET_STATUS_MSG_hardware_info(&status_msg))  = (U32)(cpuid_rax & 0x0f);
    REMOTE_HARDWARE_INFO_tsc_frequency(TARGET_STATUS_MSG_hardware_info(&status_msg))  = tsc_freq;

    TARGET_STATUS_MSG_os_info_size(&status_msg) = sizeof(REMOTE_OS_INFO_NODE);
    TARGET_STATUS_MSG_collect_switch_size(&status_msg) = sizeof(REMOTE_SWITCH_NODE);
    TARGET_STATUS_MSG_hardware_info_size(&status_msg) = sizeof(REMOTE_HARDWARE_INFO_NODE);
    offset = sizeof(status_msg.s1);
    TARGET_STATUS_MSG_os_info_offset(&status_msg) = offset;
    offset += sizeof(REMOTE_OS_INFO_NODE);
    TARGET_STATUS_MSG_collect_switch_offset(&status_msg) = offset;
    offset += sizeof(REMOTE_SWITCH_NODE);
    TARGET_STATUS_MSG_hardware_info_offset(&status_msg) = offset;
    TARGET_STATUS_MSG_status(&status_msg) = retcode;

    sent_bytes = send(control_socket, (void*)&status_msg, sizeof(TARGET_STATUS_MSG_NODE), 0);
    if (sent_bytes < 0 || sent_bytes != sizeof(TARGET_STATUS_MSG_NODE)) {
        SEPAGENT_PRINT_ERROR("Couldn't send the target status message\n");
        return VT_COMM_SEND_ERROR;
    }

    return retcode;
}

S32
COMM_Receive_Control_Request_On_Target (
    U32        *cmd,
    IOCTL_ARGS  ioctl_arg,
    S32         trace_idx
)
{
    S32                data_size         = 0;
    S32                data_transferred  = 0;
    CONTROL_MSG_HEADER header_msg        = NULL;
    S8                *ptr               = NULL;
    S32                retcode           = VT_SUCCESS;

    if (!ioctl_arg || !cmd) {
        SEPAGENT_PRINT_ERROR("ioctl_arg or cmd pointers is NULL!\n");
        return VT_UNEXPECTED_NULL_PTR;
    }
    memset(ioctl_arg, 0, sizeof(IOCTL_ARGS_NODE));

    if (!(header_msg = (CONTROL_MSG_HEADER)malloc(sizeof(CONTROL_MSG_HEADER_NODE)))) {
        SEPAGENT_PRINT_ERROR("Couldn't allocate message header buffer\n");
        return VT_NO_MEMORY;
    }
    memset(header_msg, 0, sizeof(CONTROL_MSG_HEADER_NODE));

    if (trace_idx == -1) {
        // Read the header data at first
        while (data_transferred < sizeof(CONTROL_MSG_HEADER_NODE)) {
            if ((data_size = recv(control_socket, (S8*)header_msg+data_transferred, sizeof(CONTROL_MSG_HEADER_NODE)-data_transferred, 0)) < 0) {
                SEPAGENT_PRINT_ERROR("Couldn't receive header data\n");
                free(header_msg);
                return VT_COMM_RECV_ERROR;
            } else if (!data_size) {
                SEPAGENT_PRINT_ERROR("Connection closed by remote\n");
                free(header_msg);
                return VT_COMM_CONNECTION_CLOSED_BY_REMOTE;
            } else {
                data_transferred += data_size;
            }
        }
    }
    else {
        if (trace_idx >= trace_length ||
            trace_idx < 0) {
            SEPAGENT_PRINT_ERROR("Incorrect trace index requested\n");
            free(header_msg);
            return VT_INTERNAL_ERROR;
        }
        memcpy(header_msg, (S8*)(cmd_traces+(trace_idx*CMD_TRACE_SIZE)), sizeof(CONTROL_MSG_HEADER_NODE));
    }

    SEPAGENT_PRINT_DEBUG("header size %d\n", CONTROL_MSG_HEADER_header_size(header_msg));
    SEPAGENT_PRINT_DEBUG("interface version %d\n", CONTROL_MSG_HEADER_proto_version(header_msg));
    SEPAGENT_PRINT_DEBUG("command id %d\n", CONTROL_MSG_HEADER_command_id(header_msg));
    SEPAGENT_PRINT_DEBUG("input size %llu\n", CONTROL_MSG_HEADER_to_target_data_size(header_msg));
    SEPAGENT_PRINT_DEBUG("output_size %llu\n", CONTROL_MSG_HEADER_from_target_data_size(header_msg));

    if (CONTROL_MSG_HEADER_header_size(header_msg) != sizeof(CONTROL_MSG_HEADER_NODE)) {
        retcode = VT_COMM_NOT_COMPATIBLE;
    }

    *cmd = CONTROL_MSG_HEADER_command_id(header_msg);

    ioctl_arg->len_drv_to_usr = CONTROL_MSG_HEADER_from_target_data_size(header_msg);
    ioctl_arg->len_usr_to_drv = CONTROL_MSG_HEADER_to_target_data_size(header_msg);
    if (CONTROL_MSG_HEADER_to_target_data_size(header_msg)) {
        //ptr = (S8*)calloc(1, ioctl_arg->len_usr_to_drv);
        ptr = (S8*)calloc(ioctl_arg->len_usr_to_drv, 1);
        if (!ptr) {
            SEPAGENT_PRINT_ERROR("Couldn't allocate usr_to_drv buffer size %llu\n", ioctl_arg->len_usr_to_drv);
            free(header_msg);
            return VT_NO_MEMORY;
        }
        ioctl_arg->buf_usr_to_drv = ptr;

        if (trace_idx == -1) {
            data_size = 0;
            data_transferred = 0;
            while(data_transferred < ioctl_arg->len_usr_to_drv) {
                if ((data_size = recv(control_socket, ioctl_arg->buf_usr_to_drv+data_transferred, ioctl_arg->len_usr_to_drv-data_transferred, 0)) < 0) {
                    SEPAGENT_PRINT_ERROR("Couldn't receive data\n");
                    free(header_msg);
                    return VT_COMM_RECV_ERROR;
                } else if (!data_size) {
                    SEPAGENT_PRINT_ERROR("Connection closed by remote\n");
                    free(header_msg);
                    return VT_COMM_CONNECTION_CLOSED_BY_REMOTE;
                } else {
                    data_transferred += data_size;
                }
            }
        } else {
            if (trace_idx >= trace_length ||
                trace_idx < 0) {
                free(header_msg);

                return VT_INTERNAL_ERROR;
            }
            memcpy(ioctl_arg->buf_usr_to_drv, (S8*)(arg_traces+(trace_idx*ARG_TRACE_SIZE)), ioctl_arg->len_usr_to_drv);
        }
    }
    if (ioctl_arg->len_drv_to_usr > 0) {
        ptr = (S8*)malloc(ioctl_arg->len_drv_to_usr);
        if (!ptr) {
            SEPAGENT_PRINT_ERROR("Couldn't allocate drv_to_usr buffer size %llu\n", ioctl_arg->len_drv_to_usr);
            free(header_msg);
            return VT_NO_MEMORY;
        }
        ioctl_arg->buf_drv_to_usr = ptr;
    }

    free(header_msg);

    return retcode;
}


S32
COMM_Send_Control_Response_On_Target (
    U32        cmd,
    IOCTL_ARGS ioctl_arg,
    S32        status,
    DRV_BOOL   record_mode,
    S32        trace_idx
)
{
    CONTROL_MSG_HEADER header_msg = NULL;
    S32                sent_bytes = 0;

    if (!ioctl_arg) {
        SEPAGENT_PRINT_ERROR("ioctl_arg is NULL!\n");
        return VT_UNEXPECTED_NULL_PTR;
    }

    if (!(header_msg = (CONTROL_MSG_HEADER)malloc(sizeof(CONTROL_MSG_HEADER_NODE)))) {
        SEPAGENT_PRINT_ERROR("Couldn't allocate message header buffer\n");
        return VT_NO_MEMORY;
    }
    memset(header_msg, 0, sizeof(CONTROL_MSG_HEADER_NODE));

    CONTROL_MSG_HEADER_header_size(header_msg) = sizeof(CONTROL_MSG_HEADER_NODE);
    CONTROL_MSG_HEADER_proto_version(header_msg) = PROTOCOL_VERSION;
    CONTROL_MSG_HEADER_command_id(header_msg) = cmd;
    CONTROL_MSG_HEADER_status(header_msg) = status;
    if (ioctl_arg->len_drv_to_usr && ioctl_arg->buf_drv_to_usr) {
        if (status == VT_SUCCESS) {
            CONTROL_MSG_HEADER_from_target_data_size(header_msg) = ioctl_arg->len_drv_to_usr;
        }
    }

    if (!record_mode) {
        sent_bytes = send(control_socket, (void*)header_msg, sizeof(CONTROL_MSG_HEADER_NODE), 0);
        if (sent_bytes < 0 || sent_bytes != sizeof(CONTROL_MSG_HEADER_NODE)) {
            SEPAGENT_PRINT_ERROR("Couldn't send the command response header for cmd=%d\n", cmd);
            free(header_msg);
            return VT_COMM_SEND_ERROR;
        }
        if (status != VT_SUCCESS) {
            SEPAGENT_PRINT_DEBUG("Sent the status %d to host\n", status);
            free(header_msg);
            return VT_SUCCESS;
        }
    }

    if (ioctl_arg->len_drv_to_usr && ioctl_arg->buf_drv_to_usr) {
        if (!record_mode) {
            if (trace_idx == -1) {
                sent_bytes = send(control_socket, (void*)ioctl_arg->buf_drv_to_usr, ioctl_arg->len_drv_to_usr, 0);
            } else {
                sent_bytes = send(control_socket, ret_traces[trace_idx], ioctl_arg->len_drv_to_usr, 0);
            }
            if (sent_bytes < 0 || sent_bytes != ioctl_arg->len_drv_to_usr) {
                SEPAGENT_PRINT_ERROR("Couldn't send the command response data for cmd=%d\n", cmd);
                free(header_msg);
                return VT_COMM_SEND_ERROR;
            }
        } else {
            if (trace_idx >= trace_length ||
                trace_idx < 0) {
                SEPAGENT_PRINT_ERROR("Incorrect trace index requested\n");
                free(header_msg);
                return VT_INTERNAL_ERROR;
            }
            ret_traces[trace_idx] = (S8 *)malloc(ioctl_arg->len_drv_to_usr);
            if (!ret_traces[trace_idx]) {
                SEPAGENT_PRINT_ERROR("Couldn't allocate return message memory for advance collection \n");
                free(header_msg);
                return VT_NO_MEMORY;
            }
            memcpy((S8*)ret_traces[trace_idx], ioctl_arg->buf_drv_to_usr, ioctl_arg->len_drv_to_usr);
        }
    } else {
        if (record_mode) {
            if (trace_idx >= 0 && trace_idx < trace_length) {
                ret_traces[trace_idx] = NULL;
            }
        }
    }

    free(header_msg);

    return VT_SUCCESS;
}


S32
COMM_Close_Control_On_Target ()
{
    if (data_socket) {
        free(data_socket);
        data_socket = NULL;
    }

    close(control_socket);
    //close(server_socket);
    control_socket = 0;

    if (first_control_msg) {
        free(first_control_msg);
    }

    return VT_SUCCESS;
}


S32
COMM_Open_Data_On_Target (
    U32 conn_id,
    U32 conn_type
)
{
    S32             sent_bytes       = 0;
    S32             socket_size      = 0;
    DATA_FIRST_MSG  first_msg        = NULL;
    S32             socket_idx;

    if (!first_control_msg) {
        SEPAGENT_PRINT_ERROR("first control msg is NULL\n");
        return VT_INTERNAL_ERROR;
    }

    socket_idx = comm_Get_Data_Socket_Array_Index(conn_id, conn_type);

    if (socket_idx < 0) {
        SEPAGENT_PRINT_ERROR("Invalid data socket array index %d\n", socket_idx);
        return VT_INTERNAL_ERROR;
    }

    if (socket_idx >= num_of_data_connections) {
        SEPAGENT_PRINT_ERROR("could not create data connection id %d\n", socket_idx);
        return VT_INTERNAL_ERROR;
    }

    socket_size = sizeof(control_socket_info);
    SEPAGENT_PRINT("Waiting for data connection from host ...\n");
    if ((data_socket[socket_idx] = accept(server_socket,
                            (struct sockaddr*)&control_socket_info,
                &socket_size)) < 0) {
        SEPAGENT_PRINT_ERROR("Couldn't accept on socket");
        return VT_COMM_ACCEPT_ERROR;
    }
    SEPAGENT_PRINT("Received a data connection request from host with idx=%d, conn_id=%u, conn_type=%u\n", socket_idx, conn_id, conn_type);

    if (!(first_msg = (DATA_FIRST_MSG)malloc(sizeof(DATA_FIRST_MSG_NODE)))) {
        SEPAGENT_PRINT_ERROR("Couldn't allocate message first msg buffer\n");
        return VT_NO_MEMORY;
    }
    memset(first_msg, 0, sizeof(DATA_FIRST_MSG_NODE));

    DATA_FIRST_MSG_proto_version(first_msg) = PROTOCOL_VERSION;
    DATA_FIRST_MSG_data_type(first_msg) = (U16)conn_type;
    DATA_FIRST_MSG_data_id(first_msg) = (U16)conn_id;

    SEPAGENT_PRINT_DEBUG("interface version %u\n", DATA_FIRST_MSG_proto_version(first_msg));
    SEPAGENT_PRINT_DEBUG("data_type %u\n", DATA_FIRST_MSG_data_type(first_msg));
    SEPAGENT_PRINT_DEBUG("data_id %u\n", DATA_FIRST_MSG_data_id(first_msg));

    sent_bytes = send(data_socket[socket_idx], (void*)first_msg, sizeof(DATA_FIRST_MSG_NODE), 0);
    free(first_msg);
    if (sent_bytes < 0 || sent_bytes != sizeof(DATA_FIRST_MSG_NODE)) {
        SEPAGENT_PRINT_ERROR("Couldn't send the first data message\n");
        return VT_COMM_SEND_ERROR;
    }

    return VT_SUCCESS;
}


S32
COMM_Send_Data_On_Target (
    U32   conn_id,
    U32   conn_type,
    void *buffer,
    S32   buffer_size
)
{
//#define MAX_SEND_BUFFER_LEN 4096

    S32 sent_bytes = 0;
    S32 i;
    S32 total_sent_bytes = 0;
    S32 send_size = buffer_size;
    S32 failed_attempts = 0;
    S32 socket_idx;

    socket_idx = comm_Get_Data_Socket_Array_Index(conn_id, conn_type);

    if (socket_idx < 0) {
        SEPAGENT_PRINT_ERROR("Invalid data socket array index %d\n", socket_idx);
        return VT_INTERNAL_ERROR;
    }

    if (socket_idx >= num_of_data_connections) {
        SEPAGENT_PRINT_ERROR("could not create data connection id %d\n", socket_idx);
        return VT_INTERNAL_ERROR;
    }

    if (!buffer || !buffer_size) {
        SEPAGENT_PRINT_ERROR("buffer or buffer_size are invalid for connection %d\n", data_socket[socket_idx]);
        return VT_UNEXPECTED_NULL_PTR;
    }

    while (total_sent_bytes < buffer_size) {
        SEPAGENT_PRINT_DEBUG("Sending %d bytes, total_sent %d bytes\n", send_size, total_sent_bytes);
        sent_bytes = send(data_socket[socket_idx], (char *)buffer+total_sent_bytes, send_size, 0);

        if (sent_bytes < 0 || sent_bytes != send_size) {
            failed_attempts++;
            if (failed_attempts < 3) {
                SEPAGENT_PRINT_ERROR("Couldn't send data for socketid=%d, conn_id=%u, conn_type=%u\n", data_socket[socket_idx], conn_id, conn_type);
                perror("ERROR sending DATA:");
                return VT_COMM_SEND_ERROR;
            } else {
                sleep(failed_attempts);
                continue;
            }
        }
        SEPAGENT_PRINT_DEBUG("Sent %d bytes on connection %d, failed_attempts=%d\n", sent_bytes, data_socket[socket_idx], failed_attempts);
        failed_attempts = 0;
        total_sent_bytes += sent_bytes;

        send_size = buffer_size-total_sent_bytes;
    }

    return VT_SUCCESS;
}


S32
COMM_Close_Data_On_Target (
    U32 conn_id,
    U32 conn_type
)
{
    S32 socket_idx;

    socket_idx = comm_Get_Data_Socket_Array_Index(conn_id, conn_type);

    if (socket_idx < 0) {
        SEPAGENT_PRINT_ERROR("Invalid data socket array index %d\n", socket_idx);
        return VT_INTERNAL_ERROR;
    }

    if (socket_idx >= num_of_data_connections) {
        SEPAGENT_PRINT_ERROR("could not create data connection id %d\n", socket_idx);
        return VT_INTERNAL_ERROR;
    }
    close(data_socket[socket_idx]);
    data_socket[socket_idx] = 0;

    return VT_SUCCESS;
}

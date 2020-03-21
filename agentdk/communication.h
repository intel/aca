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

#ifndef _COMMUNICATION_H_
#define _COMMUNICATION_H_

#if defined(__cplusplus)
extern "C" {
#endif


#define  PROTOCOL_VERSION             6
#define  DEFAULT_CONTROL_PORT         9321
#define  DEFAULT_MSG_BUFFER_SIZE      4096
#define  DEFAULT_CONNECTION_TIMEOUT   60
#define  NUM_NONCORE_DATA_CONNECTIONS 2
#define  MAX_NUM_CPUS                 64

/* Maximum number of data connections = MAX_NUM_CPUS (1 per CPU for core data) +
   NUM_NONCORE_DATA_CONNECTIONS ( 1 for module and 1 for uncore data) + MAX_NUM_CPUS (1 per CPU for sideband data) */
#define  MAX_DATA_CONNECTION          (2*MAX_NUM_CPUS+NUM_NONCORE_DATA_CONNECTIONS)
#define  MODULE_DATA_CONNECTION       MAX_NUM_CPUS
#define  UNCORE_DATA_CONNECTION       (MAX_NUM_CPUS+1)
#define  SIDEBAND_DATA_CONN_OFFSET    (MAX_NUM_CPUS+2)
#define  CONTROL_SOCKET_RECV_BUF_SIZE 128
#define  COMM_MODULE_CONN_ID          0
#define  COMM_UNCORE_CONN_ID          0
#define  COMM_RECV_MAX_TIME_ALLOWED   30

// to send large data without significant wait/hang on send(), increase the socket send buf size by setting SO_SNDBUF to 1MB
#define  DATA_SOCKET_SEND_BUF_SIZE    (1 << 20)

/*
 * Print macros for messages
 */
#define SEPAGENT_NAME           "sepagent"
#define SEPAGENT_MAJOR_VERSION   1
#define SEPAGENT_MINOR_VERSION   0
#define SEPAGENT_MSG_PREFIX      SEPAGENT_NAME""STRINGIFY(SEPAGENT_MAJOR_VERSION)"_"STRINGIFY(SEPAGENT_MINOR_VERSION)":"



typedef struct CONTROL_FIRST_MSG_NODE_S   CONTROL_FIRST_MSG_NODE;
typedef        CONTROL_FIRST_MSG_NODE    *CONTROL_FIRST_MSG;

struct CONTROL_FIRST_MSG_NODE_S {
    U32  msg_size;
    U32  proto_version;
    U32  per_cpu_buffer_size;
    U32  reserved1;
    U64  reserved2;
};

#define CONTROL_FIRST_MSG_msg_size(msg)            (msg)->msg_size
#define CONTROL_FIRST_MSG_proto_version(msg)       (msg)->proto_version
#define CONTROL_FIRST_MSG_per_cpu_buffer_size(msg) (msg)->per_cpu_buffer_size

typedef struct TARGET_STATUS_MSG_NODE_S   TARGET_STATUS_MSG_NODE;
typedef        TARGET_STATUS_MSG_NODE    *TARGET_STATUS_MSG;

struct TARGET_STATUS_MSG_NODE_S {
    struct {
        U32                    msg_size;
        U32                    proto_version;
        S32                    status;
        U32                    reserved1;
        U64                    reserved2;
        U32                    os_info_offset;
        U32                    os_info_size;
        U32                    collect_switch_offset;
        U32                    collect_switch_size;
        U32                    hardware_info_offset;
        U32                    hardware_info_size;
    } s1;
    REMOTE_OS_INFO_NODE        os_info;
    REMOTE_SWITCH_NODE         collect_switch;
    REMOTE_HARDWARE_INFO_NODE  hardware_info;
};

#define TARGET_STATUS_MSG_msg_size(msg)              (msg)->s1.msg_size
#define TARGET_STATUS_MSG_proto_version(msg)         (msg)->s1.proto_version
#define TARGET_STATUS_MSG_status(msg)                (msg)->s1.status
#define TARGET_STATUS_MSG_os_info_offset(msg)        (msg)->s1.os_info_offset
#define TARGET_STATUS_MSG_collect_switch_offset(msg) (msg)->s1.collect_switch_offset
#define TARGET_STATUS_MSG_hardware_info_offset(msg)  (msg)->s1.hardware_info_offset
#define TARGET_STATUS_MSG_os_info_size(msg)          (msg)->s1.os_info_size
#define TARGET_STATUS_MSG_collect_switch_size(msg)   (msg)->s1.collect_switch_size
#define TARGET_STATUS_MSG_hardware_info_size(msg)    (msg)->s1.hardware_info_size
#define TARGET_STATUS_MSG_os_info(msg)               (msg)->os_info
#define TARGET_STATUS_MSG_collect_switch(msg)        (msg)->collect_switch
#define TARGET_STATUS_MSG_hardware_info(msg)         (msg)->hardware_info


typedef struct CONTROL_MSG_HEADER_NODE_S   CONTROL_MSG_HEADER_NODE;
typedef        CONTROL_MSG_HEADER_NODE    *CONTROL_MSG_HEADER;

struct CONTROL_MSG_HEADER_NODE_S {
    U32  header_size;
    U32  proto_version;
    U32  command_id;
    S32  status;
    U64  to_target_data_size;
    U64  from_target_data_size;
    U64  reserved1;
    U64  reserved2;
};

#define CONTROL_MSG_HEADER_header_size(msg)            (msg)->header_size
#define CONTROL_MSG_HEADER_proto_version(msg)          (msg)->proto_version
#define CONTROL_MSG_HEADER_command_id(msg)             (msg)->command_id
#define CONTROL_MSG_HEADER_status(msg)                 (msg)->status
#define CONTROL_MSG_HEADER_to_target_data_size(msg)    (msg)->to_target_data_size
#define CONTROL_MSG_HEADER_from_target_data_size(msg)  (msg)->from_target_data_size


typedef enum {
    COMM_DATA_CPU = 0,
    COMM_DATA_MODULE,
    COMM_DATA_UNCORE,
    COMM_DATA_SIDEBAND
} COMM_DATA_TYPE;

typedef struct DATA_FIRST_MSG_NODE_S   DATA_FIRST_MSG_NODE;
typedef        DATA_FIRST_MSG_NODE    *DATA_FIRST_MSG;

struct DATA_FIRST_MSG_NODE_S {
    U32  proto_version;
    U16  data_type;
    U16  data_id;
};

#define DATA_FIRST_MSG_proto_version(msg)        (msg)->proto_version
#define DATA_FIRST_MSG_data_type(msg)            (msg)->data_type
#define DATA_FIRST_MSG_data_id(msg)              (msg)->data_id

S32 COMM_Open_Control_On_Target(DRV_BOOL mode, U64 cpuid_rax, U64 tsc_freq, U32 agent_mode, U32 transfer_mode, U32 num_cpus);
S32 COMM_Receive_Control_Request_On_Target(U32 *cmd, IOCTL_ARGS ioctl_arg, S32 trace_idx);
S32 COMM_Send_Control_Response_On_Target(U32 cmd, IOCTL_ARGS ioctl_arg, S32 status, DRV_BOOL record_mode, S32 trace_idx);
S32 COMM_Close_Control_On_Target();
S32 COMM_Open_Data_On_Target(U32 conn_id, U32 conn_type);
S32 COMM_Send_Data_On_Target(U32 conn_id, U32 conn_type, void *buffer, S32 buffer_size);
S32 COMM_Close_Data_On_Target(U32 conn_id, U32 conn_type);

#if defined(__cplusplus)
}
#endif

#endif


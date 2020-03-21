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






#ifndef _UTILITY_H_
#define _UTILITY_H_

/**
// Data Types and Macros
*/
#pragma pack(push, 1)

#pragma pack(pop)

/*
 *  These routines have macros defined in asm/system.h
 */
#define SYS_Local_Irq_Enable()       local_irq_enable()
#define SYS_Local_Irq_Disable()      local_irq_disable()
#define SYS_Local_Irq_Save(flags)    local_irq_save(flags)
#define SYS_Local_Irq_Restore(flags) local_irq_restore(flags)

#include <asm/msr.h>

extern U64
SYS_Read_MSR (U32 msr);

extern void
SYS_Write_MSR (U32 msr, U64 val);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0) || (LINUX_VERSION_CODE >= KERNEL_VERSION(3,5,0) && defined(CONFIG_UIDGID_STRICT_TYPE_CHECKS))
#define DRV_GET_UID(p)      p->cred->uid.val
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
#define DRV_GET_UID(p)      p->cred->uid
#else
#define DRV_GET_UID(p)      p->uid
#endif

extern void SYS_Perfvec_Handler (void);

extern void *SYS_get_stack_ptr0 (void);
extern void *SYS_get_stack_ptr3 (void);
extern void *SYS_get_user_fp (void);
extern short SYS_Get_cs (void);

#if defined(DRV_IA32)
extern void *SYS_Get_GDT_Base_HWR(void);   /// GDT base from hardware GDTR
extern U64   SYS_Get_TSC(void);

#define SYS_Get_GDT_Base SYS_Get_GDT_Base_HWR
#endif

#if defined(DRV_EM64T)
extern unsigned short SYS_Get_Code_Selector0 (void);
extern void SYS_Get_GDT_Base (void **);
#endif

extern void SYS_IO_Delay (void);
#define SYS_Inb(port)       inb(port)
#define SYS_Outb(byte,port) outb(byte,port)

/* typedef int                 OSSTATUS; */

/*
 * Lock implementations
 */
#define SYS_Locked_Inc(var)              atomic_inc((var))
#define SYS_Locked_Dec(var)              atomic_dec((var))

extern void  UTILITY_Read_TSC (U64* pTsc);


extern void
UTILITY_down_read_mm (
    struct mm_struct *mm
);

extern void
UTILITY_up_read_mm(struct mm_struct *mm);

extern void
UTILITY_Read_Cpuid(
    U64  cpuid_function,
    U64 *rax_value,
    U64 *rbx_value,
    U64 *rcx_value,
    U64 *rdx_value
);

extern  DISPATCH
UTILITY_Configure_CPU (U32);

#if defined(DRV_IA32)
asmlinkage void SYS_Get_CSD (U32, U32 *, U32 *);
#endif

/* ------------------------------------------------------------------------- */
/*!
 * @fn       extern unsigned long UTILITY_Find_Symbol (const char* name)
 *
 * @brief    Finds the address of the specified kernel symbol.
 *
 * @param    const char* name - name of the symbol to look for
 *
 * @return   Symbol address (0 if could not find)
 *
 * <I>Special Notes:</I>
 *           This wrapper is needed due to kallsyms_lookup_name not being exported
 *           in kernel version 2.6.32.*.
 *           Careful! This code is *NOT* multithread-safe or reentrant! Should only
 *           be called from 1 context at a time!
 */
extern unsigned long
UTILITY_Find_Symbol (const char* name);



/************************************************************************/
/*********************** DRIVER LOG DECLARATIONS ************************/
/************************************************************************/

#define DRV_LOG_COMPILER_MEM_BARRIER()             asm volatile("" : : : "memory")

#define DRV_LOG_DEFAULT_LOAD_VERBOSITY            (LOG_CHANNEL_MOSTWHERE                         | LOG_CONTEXT_ALL)
#define DRV_LOG_DEFAULT_INIT_VERBOSITY            (LOG_CHANNEL_MEMLOG    | LOG_CHANNEL_AUXMEMLOG | LOG_CONTEXT_ALL)
#define DRV_LOG_DEFAULT_DETECTION_VERBOSITY       (DRV_LOG_DEFAULT_INIT_VERBOSITY)
#define DRV_LOG_DEFAULT_ERROR_VERBOSITY           (LOG_CHANNEL_MOSTWHERE                         | LOG_CONTEXT_ALL)
#define DRV_LOG_DEFAULT_STATE_CHANGE_VERBOSITY    (DRV_LOG_DEFAULT_INIT_VERBOSITY)
#define DRV_LOG_DEFAULT_MARK_VERBOSITY            (LOG_CHANNEL_MOSTWHERE                         | LOG_CONTEXT_ALL)
#define DRV_LOG_DEFAULT_DEBUG_VERBOSITY           (LOG_CHANNEL_MEMLOG    | LOG_CHANNEL_PRINTK    | LOG_CONTEXT_ALL)
#define DRV_LOG_DEFAULT_FLOW_VERBOSITY            (LOG_CHANNEL_MEMLOG    | LOG_CHANNEL_AUXMEMLOG | LOG_CONTEXT_ALL)
#define DRV_LOG_DEFAULT_ALLOC_VERBOSITY           (LOG_VERBOSITY_NONE)
#define DRV_LOG_DEFAULT_INTERRUPT_VERBOSITY       (LOG_CHANNEL_MEMLOG                            | LOG_CONTEXT_ALL)
#define DRV_LOG_DEFAULT_TRACE_VERBOSITY           (LOG_VERBOSITY_NONE)
#define DRV_LOG_DEFAULT_REGISTER_VERBOSITY        (LOG_VERBOSITY_NONE)
#define DRV_LOG_DEFAULT_NOTIFICATION_VERBOSITY    (LOG_CHANNEL_MEMLOG                            | LOG_CONTEXT_ALL)
#define DRV_LOG_DEFAULT_WARNING_VERBOSITY         (LOG_CHANNEL_MOSTWHERE                         | LOG_CONTEXT_ALL)


/* ------------------------------------------------------------------------- */
/*!
 * @fn       extern void UTILITY_Log (U8 category, U8 in_notification, U8 secondary,
 *                                    const char* function_name, U32 func_name_len,
 *                                    U32 line_number, const char* format_string, ...)
 *
 * @brief    Checks whether and where the message should be logged, and logs it as appropriate.
 *
 * @param    U8          category        - message category
 *           U8          in_notification - whether or not we are in a notification/OS callback context
 *                                         (this information cannot be reliably obtained without passing
 *                                         it through the stack)
 *           U8          secondary       - secondary information field for the message
 *           const char* function_name   - name of the calling function
 *           U32         func_name_len   - length of the name of the calling function (more efficient
 *                                         to pass it as parameter than finding it back at runtime)
 *           U32         line_number     - line number of the call site
 *           const char* format_string   - classical format string for printf-like functions
 *           ...                         - elements to print
 *
 * @return   none
 *
 * <I>Special Notes:</I>
 *           Used to keep track of the IOCTL operation currently being processed.
 *           This information is saved in the log buffer (globally), as well as
 *           in every log entry.
 *           NB: only IOCTLs for which grabbing the ioctl mutex is necessary
 *           should be kept track of this way.
 */
extern VOID
UTILITY_Log (
    U8          category,
    U8          in_notification,
    U8          secondary,
    const char* function_name,
    U32         func_name_len,
    U32         line_number,
    const char* format_string,
    ...
);

/* ------------------------------------------------------------------------- */
/*!
 * @fn       extern DRV_STATUS UTILITY_Driver_Log_Init (void)
 *
 * @brief    Allocates and initializes the driver log buffer.
 *
 * @param    none
 *
 * @return   OS_SUCCESS on success, OS_NO_MEM on error.
 *
 * <I>Special Notes:</I>
 *           Should be (successfully) run before any non-LOAD log calls.
 *           Allocates memory without going through CONTROL_Allocate (to avoid
 *           complicating the instrumentation of CONTROL_* functions): calling
 *           UTILITY_Driver_Log_Free is necessary to free the log structure.
 *           Falls back to vmalloc when contiguous physical memory cannot be
 *           allocated. This does not impact runtime behavior, but may impact
 *           the easiness of retrieving the log from a core dump if the system
 *           crashes.
 */
extern DRV_STATUS
UTILITY_Driver_Log_Init (void);

/* ------------------------------------------------------------------------- */
/*!
 * @fn       extern DRV_STATUS UTILITY_Driver_Log_Free (void)
 *
 * @brief    Frees the driver log buffer.
 *
 * @param    none
 *
 * @return   OS_SUCCESS on success, OS_NO_MEM on error.
 *
 * <I>Special Notes:</I>
 *           Should be done before unloading the driver.
 *           See UTILITY_Driver_Log_Init for details.
 */
extern void
UTILITY_Driver_Log_Free (VOID);

/* ------------------------------------------------------------------------- */
/*!
 * @fn       extern void UTILITY_Driver_Set_Active_Ioctl (U32 ioctl)
 *
 * @brief    Sets the 'active_ioctl' global to the specified value.
 *
 * @param    U32 ioctl - ioctl/drvop code to use
 *
 * @return   none
 *
 * <I>Special Notes:</I>
 *           Used to keep track of the IOCTL operation currently being processed.
 *           This information is saved in the log buffer (globally), as well as
 *           in every log entry.
 *           NB: only IOCTLs for which grabbing the ioctl mutex is necessary
 *           should be kept track of this way.
 */
extern void
UTILITY_Driver_Set_Active_Ioctl (U32);

/* ------------------------------------------------------------------------- */
/*!
 * @fn       extern const char** UTILITY_Log_Category_Strings (void)
 *
 * @brief    Accessor function for the log category string array
 *
 * @param    none
 *
 * @return   none
 *
 * <I>Special Notes:</I>
 *           Only needed for cosmetic purposes when adjusting category verbosities.
 */
extern const char**
UTILITY_Log_Category_Strings (void);


extern DRV_LOG_BUFFER driver_log_buffer;
extern volatile U8    active_ioctl;

#define DRV_LOG()                          driver_log_buffer
#define DRV_LOG_VERBOSITY(category)      ((DRV_LOG_BUFFER_verbosities(DRV_LOG()))[category])
#define SEP_IN_NOTIFICATION                1

#define SEP_DRV_RAW_LOG(category, in_notification, second, message, ...) UTILITY_Log(category, in_notification, second, __func__, sizeof(__func__), __LINE__, message, ##__VA_ARGS__)
#define SEP_DRV_ULK_LOG(category, in_notification, second, message, ...) UTILITY_Log(category, in_notification, second, __func__, sizeof(__func__), __LINE__, message, ##__VA_ARGS__)

#define SEP_DRV_LOG_INCREMENT_NB_ACTIVE_INTERRUPTS()                    \
    __sync_fetch_and_add(                                               \
        &DRV_LOG_BUFFER_nb_active_interrupts(DRV_LOG()),                \
        1);                                                             \
    __sync_fetch_and_add(                                               \
        &DRV_LOG_BUFFER_nb_interrupts(DRV_LOG()),                       \
        1);

#define SEP_DRV_LOG_DECREMENT_NB_ACTIVE_INTERRUPTS()                    \
    __sync_fetch_and_add(                                               \
        &DRV_LOG_BUFFER_nb_active_interrupts(DRV_LOG()),                \
        -1);

#define SEP_DRV_LOG_INCREMENT_NB_ACTIVE_NOTIFICATIONS()                 \
    __sync_fetch_and_add(                                               \
        &DRV_LOG_BUFFER_nb_active_notifications(DRV_LOG()),             \
        1);                                                             \
    __sync_fetch_and_add(                                               \
        &DRV_LOG_BUFFER_nb_notifications(DRV_LOG()),                    \
        1);

#define SEP_DRV_LOG_DECREMENT_NB_ACTIVE_NOTIFICATIONS()                 \
    __sync_fetch_and_add(                                               \
        &DRV_LOG_BUFFER_nb_active_notifications(DRV_LOG()),             \
        -1);

#define SEP_DRV_LOG_INCREMENT_NB_STATE_TRANSITIONS()                    \
    __sync_fetch_and_add(                                               \
        &DRV_LOG_BUFFER_nb_driver_state_transitions(DRV_LOG()),         \
        1);

#define SEP_DRV_LOG_DISAMBIGUATE()                                      \
    __sync_fetch_and_add(                                               \
        &DRV_LOG_BUFFER_disambiguator(DRV_LOG()),                       \
        1);

/************************************************************************/
/************************** CATEGORY LOG APIs ***************************/
/************************************************************************/

// ERROR, WARNING and LOAD are always compiled in...
#define SEP_DRV_LOG_ERROR(message, ...)                          SEP_DRV_RAW_LOG(DRV_LOG_CATEGORY_ERROR,          0, DRV_LOG_NOTHING,  message, ##__VA_ARGS__)
#define SEP_DRV_LOG_WARNING(message, ...)                        SEP_DRV_RAW_LOG(DRV_LOG_CATEGORY_WARNING,        0, DRV_LOG_NOTHING,  message, ##__VA_ARGS__)
#define SEP_DRV_LOG_NOTIFICATION_ERROR(in_notif, message, ...)   SEP_DRV_RAW_LOG(DRV_LOG_CATEGORY_ERROR,   in_notif, DRV_LOG_NOTHING,  message, ##__VA_ARGS__)
#define SEP_DRV_LOG_NOTIFICATION_WARNING(in_notif, message, ...) SEP_DRV_RAW_LOG(DRV_LOG_CATEGORY_WARNING, in_notif, DRV_LOG_NOTHING,  message, ##__VA_ARGS__)
#define SEP_DRV_LOG_LOAD(message, ...)                                                            \
    do {                                                                                          \
        if (DRV_LOG()) {                                                                           \
            SEP_DRV_RAW_LOG(DRV_LOG_CATEGORY_LOAD, 0, DRV_LOG_NOTHING, message, ##__VA_ARGS__);   \
        }                                                                                         \
        else if (DRV_LOG_DEFAULT_LOAD_VERBOSITY & LOG_CHANNEL_PRINTK) {                           \
            printk(KERN_ERR SEP_MSG_PREFIX " " message "\n", ##__VA_ARGS__);                      \
        }                                                                                         \
    } while(0);

#if defined(DRV_MINIMAL_LOGGING)    // MINIMAL LOGGING MODE
#define SEP_DRV_LOG_INIT(message, ...)              {}
#define SEP_DRV_LOG_INIT_IN(message, ...)           {}
#define SEP_DRV_LOG_INIT_OUT(message, ...)          {}
#define SEP_DRV_LOG_DETECTION(message, ...)         {}
#define SEP_DRV_LOG_MARK(message, ...)              {}
#define SEP_DRV_LOG_DEBUG(message, ...)             {}
#define SEP_DRV_LOG_DEBUG_IN(message, ...)          {}
#define SEP_DRV_LOG_DEBUG_OUT(message, ...)         {}
#define SEP_DRV_LOG_FLOW_IN(message, ...)           {}
#define SEP_DRV_LOG_FLOW_OUT(message, ...)          {}
#define SEP_DRV_LOG_ALLOC(message, ...)             {}
#define SEP_DRV_LOG_ALLOC_IN(message, ...)          {}
#define SEP_DRV_LOG_ALLOC_OUT(message, ...)         {}
#define SEP_DRV_LOG_INTERRUPT_IN(message, ...)      SEP_DRV_LOG_INCREMENT_NB_ACTIVE_INTERRUPTS();
#define SEP_DRV_LOG_INTERRUPT_OUT(message, ...)     SEP_DRV_LOG_DECREMENT_NB_ACTIVE_INTERRUPTS();
#define SEP_DRV_LOG_NOTIFICATION_IN(message, ...)   SEP_DRV_LOG_INCREMENT_NB_ACTIVE_NOTIFICATIONS();
#define SEP_DRV_LOG_NOTIFICATION_OUT(message, ...)  SEP_DRV_LOG_DECREMENT_NB_ACTIVE_NOTIFICATIONS();
#define SEP_DRV_LOG_STATE_TRANSITION(former_state, new_state, message, ...) \
            (void) former_state;                                            \
            SEP_DRV_LOG_INCREMENT_NB_STATE_TRANSITIONS();                   \
            DRV_LOG_BUFFER_driver_state(DRV_LOG()) = new_state;
#else                               // REGULAR LOGGING MODE (PART 1 / 2)
#define SEP_DRV_LOG_INIT(message, ...)          SEP_DRV_RAW_LOG(DRV_LOG_CATEGORY_INIT,      0, DRV_LOG_NOTHING,  message, ##__VA_ARGS__)
#define SEP_DRV_LOG_INIT_IN(message, ...)       SEP_DRV_RAW_LOG(DRV_LOG_CATEGORY_INIT,      0, DRV_LOG_FLOW_IN,  message, ##__VA_ARGS__)
#define SEP_DRV_LOG_INIT_OUT(message, ...)      SEP_DRV_RAW_LOG(DRV_LOG_CATEGORY_INIT,      0, DRV_LOG_FLOW_OUT, message, ##__VA_ARGS__)
#define SEP_DRV_LOG_DETECTION(message, ...)     SEP_DRV_RAW_LOG(DRV_LOG_CATEGORY_DETECTION, 0, DRV_LOG_NOTHING,  message, ##__VA_ARGS__)
#define SEP_DRV_LOG_MARK(message, ...)          SEP_DRV_RAW_LOG(DRV_LOG_CATEGORY_MARK,      0, DRV_LOG_NOTHING,  message, ##__VA_ARGS__)
#define SEP_DRV_LOG_DEBUG(message, ...)         SEP_DRV_RAW_LOG(DRV_LOG_CATEGORY_DEBUG,     0, DRV_LOG_NOTHING,  message, ##__VA_ARGS__)
#define SEP_DRV_LOG_DEBUG_IN(message, ...)      SEP_DRV_RAW_LOG(DRV_LOG_CATEGORY_DEBUG,     0, DRV_LOG_FLOW_IN,  message, ##__VA_ARGS__)
#define SEP_DRV_LOG_DEBUG_OUT(message, ...)     SEP_DRV_RAW_LOG(DRV_LOG_CATEGORY_DEBUG,     0, DRV_LOG_FLOW_OUT, message, ##__VA_ARGS__)
#define SEP_DRV_LOG_FLOW_IN(message, ...)       SEP_DRV_RAW_LOG(DRV_LOG_CATEGORY_FLOW,      0, DRV_LOG_FLOW_IN,  message, ##__VA_ARGS__)
#define SEP_DRV_LOG_FLOW_OUT(message, ...)      SEP_DRV_RAW_LOG(DRV_LOG_CATEGORY_FLOW,      0, DRV_LOG_FLOW_OUT, message, ##__VA_ARGS__)
#define SEP_DRV_LOG_ALLOC(message, ...)         SEP_DRV_ULK_LOG(DRV_LOG_CATEGORY_ALLOC,     0, DRV_LOG_NOTHING,  message, ##__VA_ARGS__)
#define SEP_DRV_LOG_ALLOC_IN(message, ...)      SEP_DRV_ULK_LOG(DRV_LOG_CATEGORY_ALLOC,     0, DRV_LOG_FLOW_IN,  message, ##__VA_ARGS__)
#define SEP_DRV_LOG_ALLOC_OUT(message, ...)     SEP_DRV_ULK_LOG(DRV_LOG_CATEGORY_ALLOC,     0, DRV_LOG_FLOW_OUT, message, ##__VA_ARGS__)
#define SEP_DRV_LOG_INTERRUPT_IN(message, ...)                              \
    SEP_DRV_LOG_INCREMENT_NB_ACTIVE_INTERRUPTS();                           \
    SEP_DRV_RAW_LOG(                                                        \
        DRV_LOG_CATEGORY_INTERRUPT,                                         \
        0,                                                                  \
        DRV_LOG_FLOW_IN,                                                    \
        message,                                                            \
        ##__VA_ARGS__);
#define SEP_DRV_LOG_INTERRUPT_OUT(message, ...)                             \
    SEP_DRV_RAW_LOG(                                                        \
        DRV_LOG_CATEGORY_INTERRUPT,                                         \
        0,                                                                  \
        DRV_LOG_FLOW_OUT,                                                   \
        message,                                                            \
        ##__VA_ARGS__);                                                     \
    SEP_DRV_LOG_DECREMENT_NB_ACTIVE_INTERRUPTS();
#define SEP_DRV_LOG_NOTIFICATION_IN(message, ...)                           \
    SEP_DRV_LOG_INCREMENT_NB_ACTIVE_NOTIFICATIONS();                        \
    SEP_DRV_RAW_LOG(                                                        \
        DRV_LOG_CATEGORY_NOTIFICATION,                                      \
        1,                                                                  \
        DRV_LOG_FLOW_IN,                                                    \
        message,                                                            \
        ##__VA_ARGS__);
#define SEP_DRV_LOG_NOTIFICATION_OUT(message, ...)                          \
    SEP_DRV_RAW_LOG(                                                        \
        DRV_LOG_CATEGORY_NOTIFICATION,                                      \
        1,                                                                  \
        DRV_LOG_FLOW_OUT,                                                   \
        message,                                                            \
        ##__VA_ARGS__);                                                     \
    SEP_DRV_LOG_DECREMENT_NB_ACTIVE_NOTIFICATIONS();
#define SEP_DRV_LOG_STATE_TRANSITION(former_state, new_state, message, ...) \
            SEP_DRV_LOG_INCREMENT_NB_STATE_TRANSITIONS();                   \
            DRV_LOG_BUFFER_driver_state(DRV_LOG()) = new_state;             \
            SEP_DRV_RAW_LOG(DRV_LOG_CATEGORY_STATE_CHANGE,                  \
            0,                                                              \
            ((U8)former_state << 4) | ((U8)new_state & 0xF),                \
            message,                                                        \
            ##__VA_ARGS__);
#endif

#if defined(DRV_MAXIMAL_LOGGING)    // MAXIMAL LOGGING MODE
#define SEP_DRV_LOG_TRACE(message, ...)                                 SEP_DRV_ULK_LOG(DRV_LOG_CATEGORY_TRACE,          0, DRV_LOG_NOTHING,  message, ##__VA_ARGS__)
#define SEP_DRV_LOG_TRACE_IN(message, ...)                              SEP_DRV_ULK_LOG(DRV_LOG_CATEGORY_TRACE,          0, DRV_LOG_FLOW_IN,  message, ##__VA_ARGS__)
#define SEP_DRV_LOG_TRACE_OUT(message, ...)                             SEP_DRV_ULK_LOG(DRV_LOG_CATEGORY_TRACE,          0, DRV_LOG_FLOW_OUT, message, ##__VA_ARGS__)
#define SEP_DRV_LOG_REGISTER_IN(message, ...)                           SEP_DRV_ULK_LOG(DRV_LOG_CATEGORY_REGISTER,       0, DRV_LOG_FLOW_IN,  message, ##__VA_ARGS__)
#define SEP_DRV_LOG_REGISTER_OUT(message, ...)                          SEP_DRV_ULK_LOG(DRV_LOG_CATEGORY_REGISTER,       0, DRV_LOG_FLOW_OUT, message, ##__VA_ARGS__)
#define SEP_DRV_LOG_NOTIFICATION_TRACE(in_notif, message, ...)          SEP_DRV_ULK_LOG(DRV_LOG_CATEGORY_TRACE,   in_notif, DRV_LOG_NOTHING,  message, ##__VA_ARGS__)
#define SEP_DRV_LOG_NOTIFICATION_TRACE_IN(in_notif, message, ...)       SEP_DRV_ULK_LOG(DRV_LOG_CATEGORY_TRACE,   in_notif, DRV_LOG_FLOW_IN,  message, ##__VA_ARGS__)
#define SEP_DRV_LOG_NOTIFICATION_TRACE_OUT(in_notif, message, ...)      SEP_DRV_ULK_LOG(DRV_LOG_CATEGORY_TRACE,   in_notif, DRV_LOG_FLOW_OUT, message, ##__VA_ARGS__)
#else                               // REGULAR LOGGING MODE (PART 2 / 2)
#define SEP_DRV_LOG_TRACE(message, ...)                                 {}
#define SEP_DRV_LOG_TRACE_IN(message, ...)                              {}
#define SEP_DRV_LOG_TRACE_OUT(message, ...)                             {}
#define SEP_DRV_LOG_REGISTER_IN(message, ...)                           {}
#define SEP_DRV_LOG_REGISTER_OUT(message, ...)                          {}
#define SEP_DRV_LOG_NOTIFICATION_TRACE(in_notif, message, ...)          {}
#define SEP_DRV_LOG_NOTIFICATION_TRACE_IN(in_notif, message, ...)       {}
#define SEP_DRV_LOG_NOTIFICATION_TRACE_OUT(in_notif, message, ...)      {}
#endif


/************************************************************************/
/************************* FACILITATOR MACROS ***************************/
/************************************************************************/

#define SEP_DRV_LOG_ERROR_INIT_OUT(message, ...)      \
    SEP_DRV_LOG_ERROR       (message, ##__VA_ARGS__); \
    SEP_DRV_LOG_INIT_OUT    (message, ##__VA_ARGS__);

#define SEP_DRV_LOG_ERROR_FLOW_OUT(message, ...)      \
    SEP_DRV_LOG_ERROR       (message, ##__VA_ARGS__); \
    SEP_DRV_LOG_FLOW_OUT    (message, ##__VA_ARGS__);

#define SEP_DRV_LOG_ERROR_TRACE_OUT(message, ...)     \
    SEP_DRV_LOG_ERROR       (message, ##__VA_ARGS__); \
    SEP_DRV_LOG_TRACE_OUT   (message, ##__VA_ARGS__);

#define SEP_DRV_LOG_ERROR_ALLOC_OUT(message, ...)     \
    SEP_DRV_LOG_ERROR       (message, ##__VA_ARGS__); \
    SEP_DRV_LOG_ALLOC_OUT   (message, ##__VA_ARGS__);

#define SEP_DRV_LOG_WARNING_FLOW_OUT(message, ...)    \
    SEP_DRV_LOG_WARNING     (message, ##__VA_ARGS__); \
    SEP_DRV_LOG_FLOW_OUT    (message, ##__VA_ARGS__);

#define SEP_DRV_LOG_WARNING_TRACE_OUT(message, ...)   \
    SEP_DRV_LOG_WARNING     (message, ##__VA_ARGS__); \
    SEP_DRV_LOG_TRACE_OUT   (message, ##__VA_ARGS__);

#define SEP_DRV_LOG_WARNING_ALLOC_OUT(message, ...)   \
    SEP_DRV_LOG_WARNING     (message, ##__VA_ARGS__); \
    SEP_DRV_LOG_ALLOC_OUT   (message, ##__VA_ARGS__);

#define SEP_DRV_LOG_INIT_TRACE_OUT(message, ...)      \
    SEP_DRV_LOG_INIT        (message, ##__VA_ARGS__); \
    SEP_DRV_LOG_TRACE_OUT   (message, ##__VA_ARGS__);

#define SEP_DRV_LOG_WARNING_NOTIFICATION_OUT(message, ...) \
    SEP_DRV_LOG_WARNING          (message, ##__VA_ARGS__); \
    SEP_DRV_LOG_NOTIFICATION_OUT (message, ##__VA_ARGS__);


/************************************************************************/
/************************* DRIVER STATE MACROS **************************/
/************************************************************************/


/* ------------------------------------------------------------------------- */
/*!
 * @fn       extern U32 UTILITY_Change_Driver_State (U32 allowed_prior_states, U32 state, const char* func, U32 line_number)
 *
 * @brief    Updates the driver state (if the transition is legal).
 *
 * @param    U32 allowed_prior_states   - the bitmask representing the states from which the transition is allowed to occur
 *           U32 state                  - the destination state
 *           const char* func           - the callsite's function's name
 *           U32 line_number            - the callsite's line number
 *
 * @return   1 in case of success, 0 otherwise
 *
 * <I>Special Notes:</I>
 *
 */
extern U32
UTILITY_Change_Driver_State (
    U32         allowed_prior_states,
    U32         state,
    const char* func,
    U32         line_number
);

#define GET_DRIVER_STATE()                               GLOBAL_STATE_current_phase(driver_state)
#define CHANGE_DRIVER_STATE(allowed_prior_states, state) UTILITY_Change_Driver_State(allowed_prior_states, state, __func__, __LINE__)
#define DRIVER_STATE_IN(state, states)                   (!!(MATCHING_STATE_BIT(state) & (states)))

#endif


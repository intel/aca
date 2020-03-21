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





#include "lwpmudrv_defines.h"
#include <linux/version.h>
#include <linux/mm.h>
#include <asm/apic.h>

#include "lwpmudrv_types.h"
#include "rise_errors.h"
#include "lwpmudrv_ecb.h"
#include "lwpmudrv_struct.h"
#include "lwpmudrv.h"
#include "control.h"
#include "utility.h"
#include "apic.h"
#include "sys_info.h"

#define VTSA_CPUID VTSA_CPUID_X86

extern U64              total_ram;
static IOCTL_SYS_INFO  *ioctl_sys_info      = NULL;
static size_t           ioctl_sys_info_size = 0;
static U32             *cpuid_entry_count   = NULL;
static U32             *cpuid_total_count   = NULL;
       U32             *cpu_built_sysinfo   = NULL;

static U32             cpu_threads_per_core  = 1;

#define VTSA_NA64       ((U64) -1)
#define VTSA_NA32       ((U32) -1)
#define VTSA_NA         ((U32) -1)

#define SYS_INFO_NUM_SETS(rcx)             ((rcx) + 1)
#define SYS_INFO_LINE_SIZE(rbx)            (((rbx) & 0xfff) + 1)
#define SYS_INFO_LINE_PARTITIONS(rbx)      ((((rbx) >> 12) & 0x3ff) + 1)
#define SYS_INFO_NUM_WAYS(rbx)             ((((rbx) >> 22) & 0x3ff) + 1)

#define SYS_INFO_CACHE_SIZE(rcx,rbx) (SYS_INFO_NUM_SETS((rcx))        *    \
                                      SYS_INFO_LINE_SIZE((rbx))       *    \
                                      SYS_INFO_LINE_PARTITIONS((rbx)) *    \
                                      SYS_INFO_NUM_WAYS((rbx)))

#define MSR_FB_PCARD_ID_FUSE  0x17    // platform id fuses MSR

#define LOW_PART(x)     (x & 0xFFFFFFFF)

/* ------------------------------------------------------------------------- */
/*!
 * @fn static U64 sys_info_nbits(number)
 *
 * @param number  - the number to check
 * @return the number of bit.
 *
 * @brief  This routine gets the number of useful bits with the given number.
 *         It will round the number up to power of 2, and adjust to 0 based number.
 *         sys_info_nbits(0x3) = 2
 *         sys_info_nbits(0x4) = 2
 *
 */
static U64
sys_info_nbits (
    U64    number
)
{
    U64 i;

    SEP_DRV_LOG_TRACE_IN("Number: %llx.", number); // is %llu portable in the kernel?

    if (number < 2) {
        SEP_DRV_LOG_TRACE_OUT("Res: %u. (early exit)", (U32) number);
        return number;
    }

    // adjust to 0 based number, and round up to power of 2
    number--;
    for (i = 0; number > 0; i++) {
        number >>= 1;
    }

    SEP_DRV_LOG_TRACE_OUT("Res: %u.", (U32) i);
    return i;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn static U64 sys_info_bitmask(nbits)
 *
 * @param number  - the number of bits
 * @return  the bit mask for the nbits number
 *
 * @brief  This routine gets the bitmask for the nbits number.
 */
static U64
sys_info_bitmask (
    U64    nbits
)
{
    U64 mask = 0;

    SEP_DRV_LOG_TRACE_IN("Nbits: %u.", (U32) nbits);

    mask = (U64)(1<<nbits);
    mask--;

    SEP_DRV_LOG_TRACE_OUT("Res: %llx.", mask);

    return mask;
}


/* ------------------------------------------------------------------------- */
/*!
 * @fn static void sys_info_Get_Num_Cpuid_Funcs(basic_funcs, basic_4_funcs, extended_funcs)
 *
 * @param basic_functions    - pointer to the number of basic functions
 * @param basic_4_funcs      - pointer to the basic 4 functions
 * @param extended_funcs     - pointer to the number of extended functions
 * @return total number of cpuid functions
 *
 * @brief  This routine gets the number of basic and extended cpuid functions.
 *
 */
static U32
sys_info_Get_Num_Cpuid_Funcs (
    OUT U32 *basic_funcs,
    OUT U32 *basic_4_funcs,
    OUT U32 *extended_funcs
)
{
    U64 num_basic_funcs      = 0x0LL;
    U64 num_basic_4_funcs    = 0x0LL;
    U64 num_extended_funcs   = 0x0LL;
    U64 rax;
    U64 rbx;
    U64 rcx;
    U64 rdx;
    U64 i;
    U32 res;

    SEP_DRV_LOG_TRACE_IN("");

    UTILITY_Read_Cpuid(0, &num_basic_funcs, &rbx, &rcx, &rdx);
    UTILITY_Read_Cpuid(0x80000000, &num_extended_funcs, &rbx, &rcx, &rdx);

    if (num_extended_funcs & 0x80000000) {
        num_extended_funcs -= 0x80000000;
    }

    //
    // make sure num_extended_funcs is not bogus
    //
    if (num_extended_funcs > 0x1000) {
        num_extended_funcs = 0;
    }

    //
    // if number of basic funcs is greater than 4, figure out how many
    // time we should call CPUID with eax = 0x4.
    //
    num_basic_4_funcs = 0;
    if (num_basic_funcs >= 4) {
        for (i = 0, rax = (U64)-1; (rax & 0x1f) != 0; i++) {
            rcx = i;
            UTILITY_Read_Cpuid(4, &rax, &rbx, &rcx, &rdx);
        }
        num_basic_4_funcs = i - 1;
    }
    if (num_basic_funcs >= 0xb) {
        i = 0;
        do {
            rcx = i;
            UTILITY_Read_Cpuid(0xb, &rax, &rbx, &rcx, &rdx);
            i++;
        } while (!(LOW_PART(rax) == 0 && LOW_PART(rbx) == 0));
        num_basic_4_funcs += i;
    }
    if (num_basic_funcs >= 0xf) {
        num_basic_4_funcs += 1; // subleafs 0x0, 0x1
    }

    if (num_basic_funcs >= 0x10) {
        num_basic_4_funcs += 3; // subleafs 0x0 - 0x3
    }

    SEP_DRV_LOG_TRACE("Num_basic_4_funcs = %llx.", num_basic_4_funcs);

    //
    // adjust number to include 0 and 0x80000000 functions.
    //
    num_basic_funcs++;
    num_extended_funcs++;

    SEP_DRV_LOG_TRACE("num_basic_funcs: %llx, num_extended_funcs: %llx.", num_basic_funcs, num_extended_funcs);

    //
    // fill-in the parameter for the caller
    //
    if (basic_funcs != NULL) {
        *basic_funcs = (U32) num_basic_funcs;
    }
    if (basic_4_funcs != NULL) {
        *basic_4_funcs = (U32) num_basic_4_funcs;
    }
    if (extended_funcs != NULL) {
        *extended_funcs = (U32) num_extended_funcs;
    }

    res = (U32) (num_basic_funcs + num_basic_4_funcs + num_extended_funcs);
    SEP_DRV_LOG_TRACE_OUT("Res: %u.", res);
    return res;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn static void sys_info_Get_Cpuid_Entry_Cpunt(buffer)
 *
 * @param  buffer    - pointer to the buffer to hold the info
 * @return None
 *
 * @brief  Service Routine to query the CPU for the number of entries needed
 *
 */
static VOID
sys_info_Get_Cpuid_Entry_Count (
    PVOID    buffer
)
{
    U32 current_processor;
    U32 *current_cpu_buffer;

    SEP_DRV_LOG_TRACE_IN("Buffer: %p.", buffer);

    current_processor = CONTROL_THIS_CPU();
    SEP_DRV_LOG_TRACE("Beginning on CPU %u.", current_processor);

    current_cpu_buffer = (U32 *) ((U8 *) buffer + current_processor * sizeof(U32));

#if defined(ALLOW_ASSERT)
    ASSERT(((U8 *) current_cpu_buffer + sizeof(U32)) <=
           ((U8 *) current_cpu_buffer + GLOBAL_STATE_num_cpus(driver_state) * sizeof(U32)));
#endif
    *current_cpu_buffer = sys_info_Get_Num_Cpuid_Funcs(NULL, NULL, NULL);

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn static U32 sys_info_Get_Cpuid_Buffer_Size(cpuid_entries)
 *
 * @param    cpuid_entries   - number of cpuid entries
 * @return   size of buffer needed in bytes
 *
 * @brief  This routine returns number of bytes needed to hold the CPU_CS_INFO
 * @brief  structure.
 *
 */
static U32
sys_info_Get_Cpuid_Buffer_Size (
    U32 cpuid_entries
)
{
    U32  cpuid_size;
    U32  buffer_size;

    SEP_DRV_LOG_TRACE_IN("");

    cpuid_size  = sizeof(VTSA_CPUID);

    buffer_size = sizeof(IOCTL_SYS_INFO) +
                  sizeof(VTSA_GEN_ARRAY_HDR) +
                  sizeof(VTSA_NODE_INFO) +
                  sizeof(VTSA_GEN_ARRAY_HDR) +
                  GLOBAL_STATE_num_cpus(driver_state) * sizeof(VTSA_GEN_PER_CPU) +
                  GLOBAL_STATE_num_cpus(driver_state) * sizeof(VTSA_GEN_ARRAY_HDR) +
                  cpuid_entries * cpuid_size;

    SEP_DRV_LOG_TRACE_OUT("Res: %u.", buffer_size);

    return buffer_size;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn extern void sys_info_Fill_CPUID(...)
 *
 * @param        num_cpuids,
 * @param        basic_funcs,
 * @param        extended_funcs,
 * @param        cpu,
 * @param       *current_cpuid
 * @param       *gen_per_cpu,
 * @param       *local_gpc
 *
 * @return   None
 *
 * @brief  This routine is called to build per cpu information.
 * @brief  Fills in the cpuid for the processor in the right location in the buffer
 *
 */
static void
sys_info_Fill_CPUID (
    U32                  num_cpuids,
    U32                  basic_funcs,
    U32                  extended_funcs,
    U32                  cpu,
    VTSA_CPUID          *current_cpuid,
    VTSA_GEN_PER_CPU    *gen_per_cpu,
    VTSA_GEN_PER_CPU    *local_gpc
)
{
    U32                  i, index, j;
    U64                  cpuid_function;
    U64                  rax, rbx, rcx, rdx;
    VTSA_CPUID          *cpuid_el;
    U32                  shift_nbits_core         = 0;
    U32                  shift_nbits_pkg          = 0;
    U32                  shift_nbits_module       = 0;
    U32                  family                   = 0;
    U32                  model                    = 0;
    DRV_BOOL             ht_supported             = FALSE;
    U32                  apic_id                  = 0;
    U32                  num_logical_per_physical = 0;
    U32                  cores_per_die            = 1;
    U32                  thread_id                = 0;
    U32                  core_id                  = 0;
    U32                  package_id               = 0;
    U32                  module_id                = 0;
    U32                  cores_sharing_cache      = 0;
    U32                  cache_mask_width         = 0;
    U32                  num_cores                = 0;
    U32                  level_type               = 0;
    U32                  num_dies_per_pkg         = 0;
    DRV_BOOL             multi_die                = FALSE;
    U32                  num_sub_leafs            = 0;
    DRV_BOOL             is_function_1f_available = FALSE;
    DRV_BOOL             is_hetero_platform       = FALSE;

    SEP_DRV_LOG_TRACE_IN("CPU: %u.", cpu);

    apic_id = CPU_STATE_apic_id(&pcb[cpu]);
    SEP_DRV_LOG_TRACE("cpu %u apic_id 0x%x.", cpu, apic_id);

    for (i = 0, index = 0; index < num_cpuids; i++) {
        cpuid_function = (i < basic_funcs) ? i : (0x80000000 + i - basic_funcs);

       if (cpuid_function == 0x4) {
            for (j = 0, rax = (U64)-1; (rax & 0x1f) != 0; j++) {
                rcx = j;
                UTILITY_Read_Cpuid(cpuid_function, &rax, &rbx, &rcx, &rdx);
                cpuid_el = &current_cpuid[index];
                index++;

#if defined(ALLOW_ASSERT)
                ASSERT(((U8 *)cpuid_el + sizeof(VTSA_CPUID)) <= cpuid_buffer_limit);
#endif

                VTSA_CPUID_X86_cpuid_eax_input(cpuid_el) = (U32) cpuid_function;
                VTSA_CPUID_X86_cpuid_eax(cpuid_el)       = (U32) rax;
                VTSA_CPUID_X86_cpuid_ebx(cpuid_el)       = (U32) rbx;
                VTSA_CPUID_X86_cpuid_ecx(cpuid_el)       = (U32) rcx;
                VTSA_CPUID_X86_cpuid_edx(cpuid_el)       = (U32) rdx;
                SEP_DRV_LOG_TRACE("cpu=%u, func=0x%x - rax=0x%x, rbx=0x%x, rcx=0x%x, rdx=0x%x",
                                  cpu, (U32)cpuid_function, (U32)rax, (U32)rbx, (U32)rcx, (U32)rdx);

                if ((rax & 0x1f) != 0) {
                    local_gpc = &gen_per_cpu[cpu];
                    if (((rax >> 5) & 0x3) == 2) {
                        VTSA_GEN_PER_CPU_cpu_cache_L2(local_gpc) =
                                   (U32)(SYS_INFO_CACHE_SIZE(rcx,rbx) >> 10);
                        SEP_DRV_LOG_TRACE("L2 Cache: %x.", VTSA_GEN_PER_CPU_cpu_cache_L2(local_gpc));
                        cores_sharing_cache = ((U16)(rax >> 14) & 0xfff) + 1;
                        SEP_DRV_LOG_TRACE("CORES_SHARING_CACHE=%d j=%d cpu=%d.", cores_sharing_cache, j, cpu);
                    }

                    if (((rax >> 5) & 0x3) == 3) {
                        VTSA_GEN_PER_CPU_cpu_cache_L3(local_gpc) =
                                    (U32)(SYS_INFO_CACHE_SIZE(rcx,rbx) >> 10);
                        SEP_DRV_LOG_TRACE("L3 Cache: %x.", VTSA_GEN_PER_CPU_cpu_cache_L3(local_gpc));
                    }
                }
                if (j == 0) {
                    cores_per_die = ((U16)(rax >> 26) & 0x3f) + 1;
                }
            }
            if (cores_sharing_cache != 0) {
                cache_mask_width = (U32)sys_info_nbits(cores_sharing_cache);
                SEP_DRV_LOG_TRACE("CACHE MASK WIDTH=%x.", cache_mask_width);
            }
        }
        else if (cpuid_function == 0x7) {
            rcx = 0;
            UTILITY_Read_Cpuid(cpuid_function, &rax, &rbx, &rcx, &rdx);
            is_hetero_platform = (rdx >> 15) & 1 ? TRUE : FALSE;
        }
        else if (cpuid_function == 0xb ||
                 cpuid_function == 0x1f) {
            // As for SDM: CPUID leaf 1FH is a preferred superset to leaf 0BH
            // Since we sequentially loop the CPUID, if 1FH is available then replace the info from 08H
            if (cpuid_function == 0x1f) {
                is_function_1f_available = TRUE;
            }

            j = 0;
            do {
                rcx = j;
                UTILITY_Read_Cpuid(cpuid_function, &rax, &rbx, &rcx, &rdx);
                cpuid_el = &current_cpuid[index];
                index++;

#if defined(ALLOW_ASSERT)
                ASSERT(((U8 *)cpuid_el + sizeof(VTSA_CPUID_X86)) <= cpuid_buffer_limit);
#endif

                VTSA_CPUID_X86_cpuid_eax_input(cpuid_el) = (U32) cpuid_function;
                VTSA_CPUID_X86_cpuid_eax(cpuid_el)       = (U32) rax;
                VTSA_CPUID_X86_cpuid_ebx(cpuid_el)       = (U32) rbx;
                VTSA_CPUID_X86_cpuid_ecx(cpuid_el)       = (U32) rcx;
                VTSA_CPUID_X86_cpuid_edx(cpuid_el)       = (U32) rdx;
                SEP_DRV_LOG_TRACE("cpu=%u, func=0x%x, j=%u - rax=0x%x, rbx=0x%x, rcx=0x%x, rdx=0x%x",
                                  cpu, (U32)cpuid_function, j, (U32)rax, (U32)rbx, (U32)rcx, (U32)rdx);

                level_type = (U32)(rcx >> 8 & 0xff);
                if (level_type == 1) {
                    shift_nbits_core = rax & 0x1f;   //No. of bits to shift APIC ID to get Core ID
                }

                if (cpuid_function == 0xb) {
                    if (level_type == 2) {
                        shift_nbits_pkg  = rax & 0x1f;    //No. of bits to shift APIC ID to get Pkg ID
                    }
                }
                else if (cpuid_function == 0x1f) {
                    if (level_type      == 2) {
                        shift_nbits_module = rax & 0x1f;    //No. of bits to shift APIC ID to get Module ID
                        shift_nbits_pkg = rax & 0x1f;       //No. of bits to shift APIC ID to get Pkg ID

                    }
                    if (level_type      == 3 ||
                        level_type      == 4 ||
                        level_type      == 5) {
                        if (level_type == 5) {
                            multi_die = TRUE;
                        }
                        shift_nbits_pkg = rax & 0x1f;    //No. of bits to shift APIC ID to get Pkg ID
                    }
                }

                j++;
            } while (!(LOW_PART(rax) == 0 && LOW_PART(rbx) == 0));
        }
        else if (cpuid_function == 0xf ||
                 cpuid_function == 0x10) {
            num_sub_leafs = (cpuid_function == 0xf) ? 2 : 4;

            for (j = 0; j < num_sub_leafs; j++) {
                rcx = j;
                UTILITY_Read_Cpuid(cpuid_function, &rax, &rbx, &rcx, &rdx);
                cpuid_el = &current_cpuid[index];
                index++;
#if defined(ALLOW_ASSERT)
                ASSERT(((U8 *)cpuid_el + sizeof(VTSA_CPUID)) <= cpuid_buffer_limit);
#endif

                VTSA_CPUID_X86_cpuid_eax_input(cpuid_el) = (U32) cpuid_function;
                VTSA_CPUID_X86_cpuid_eax(cpuid_el)       = (U32) rax;
                VTSA_CPUID_X86_cpuid_ebx(cpuid_el)       = (U32) rbx;
                VTSA_CPUID_X86_cpuid_ecx(cpuid_el)       = (U32) rcx;
                VTSA_CPUID_X86_cpuid_edx(cpuid_el)       = (U32) rdx;
                SEP_DRV_LOG_TRACE("cpu=%u, leaf=0x%x, sleaf=0x%x - rax=0x%x, rbx=0x%x, rcx=0x%x, rdx=0x%x",
                                  cpu, (U32)cpuid_function, j, (U32)rax, (U32)rbx, (U32)rcx, (U32)rdx);
            }
        }
        else {
            UTILITY_Read_Cpuid(cpuid_function, &rax, &rbx, &rcx, &rdx);
            cpuid_el = &current_cpuid[index];
            index++;

            SEP_DRV_LOG_TRACE("Cpu %u: num_cpuids = %u i = %u index = %u.",
                            cpu, num_cpuids, i, index);

#if defined(ALLOW_ASSERT)
            ASSERT(((U8 *)cpuid_el + sizeof(VTSA_CPUID_X86)) <= cpuid_buffer_limit);

            ASSERT(((U8 *)cpuid_el + sizeof(VTSA_CPUID_X86)) <=
                   ((U8 *)current_cpuid + (num_cpuids * sizeof(VTSA_CPUID_X86))));
#endif

            VTSA_CPUID_X86_cpuid_eax_input(cpuid_el) = (U32) cpuid_function;
            VTSA_CPUID_X86_cpuid_eax(cpuid_el)       = (U32) rax;
            VTSA_CPUID_X86_cpuid_ebx(cpuid_el)       = (U32) rbx;
            VTSA_CPUID_X86_cpuid_ecx(cpuid_el)       = (U32) rcx;
            VTSA_CPUID_X86_cpuid_edx(cpuid_el)       = (U32) rdx;
            SEP_DRV_LOG_TRACE("cpu=%u, func=0x%x - rax=0x%x, rbx=0x%x, rcx=0x%x, rdx=0x%x",
                              cpu, (U32)cpuid_function, (U32)rax, (U32)rbx, (U32)rcx, (U32)rdx);

            if (cpuid_function == 0) {
                if ((U32)rbx == 0x756e6547  &&
                    (U32)rcx == 0x6c65746e  &&
                    (U32)rdx == 0x49656e69) {
                    VTSA_GEN_PER_CPU_platform_id(local_gpc) = SYS_Read_MSR(MSR_FB_PCARD_ID_FUSE);
                }
            }
            else if (cpuid_function == 1) {
                family    = (U32)(rax >>  8 & 0x0f);
                model     = (U32)(rax >> 12 & 0xf0);  /* extended model bits */
                model    |= (U32)(rax >>  4 & 0x0f);
                ht_supported             = (rdx >> 28) & 1 ? TRUE : FALSE;
                num_logical_per_physical = (U32)((rbx & 0xff0000) >> 16);
                if (num_logical_per_physical == 0) {
                    num_logical_per_physical = 1;
                }
            }
            else if (cpuid_function == 0xa) {
                VTSA_GEN_PER_CPU_arch_perfmon_ver(local_gpc) = (U32)(rax & 0xFF);
                VTSA_GEN_PER_CPU_num_gp_counters(local_gpc) = (U32)((rax>>8) & 0xFF);
                VTSA_GEN_PER_CPU_num_fixed_counters(local_gpc) = (U32)(rdx & 0x1F);
            }
            else if (cpuid_function == 0x1a && is_hetero_platform) {
                VTSA_GEN_PER_CPU_cpu_core_type(local_gpc) = (U32)((rax>>24) & 0xFF);
            }
        }
    }

    // set cpu_cache_L2 if not already set using 0x80000006 function
    if (gen_per_cpu[cpu].cpu_cache_L2 == VTSA_NA && extended_funcs >= 6) {

        UTILITY_Read_Cpuid(0x80000006, &rax, &rbx, &rcx, &rdx);
        VTSA_GEN_PER_CPU_cpu_cache_L2(local_gpc) = (U32)(rcx >> 16);
    }

    if (!ht_supported || num_logical_per_physical == cores_per_die) {
        threads_per_core[cpu] = 1;
        thread_id             = 0;
    }
    else {
        // each core has 4 threads for MIC system, otherwise, it has 2 threads when ht is enabled
        threads_per_core[cpu] = cpu_threads_per_core;
        thread_id             = (U16)(apic_id & (cpu_threads_per_core-1));
    }

    package_id = apic_id >> shift_nbits_pkg;

    if (is_function_1f_available) {
        core_id    = (apic_id >> shift_nbits_core) & sys_info_bitmask(shift_nbits_module - shift_nbits_core);
        module_id  = (apic_id >> shift_nbits_module) & sys_info_bitmask(shift_nbits_pkg - shift_nbits_module);
        if (multi_die) {
            num_dies_per_pkg = (U32)1 << (shift_nbits_pkg - shift_nbits_module); // num_dies in a pkg = 2 ^ (CPUID bit width for dies)
            package_id = (num_dies_per_pkg * package_id) + module_id;
            // In the examples below, num_dies is same num_modules, die_id is same as module_id
            // eg: num_dies=2, init_pkg_id=0, die_id=0 -> final_pkg_id = 0
            //     num_dies=2, init_pkg_id=2, die_id=1 -> final_pkg_id = (2 * 2) + 1 = 5
        }
    }
    else {
        core_id    = (apic_id >> shift_nbits_core) & sys_info_bitmask(shift_nbits_pkg - shift_nbits_core);

        if (cache_mask_width) {
            module_id = (U32)(core_id/2);
        }
    }

    SEP_DRV_LOG_TRACE("MODULE ID=%d CORE ID=%d for cpu=%d PACKAGE ID=%d.", module_id, core_id, cpu, package_id);
    SEP_DRV_LOG_TRACE("Num_logical_per_physical=%d cores_per_die=%d.", num_logical_per_physical, cores_per_die);
    SEP_DRV_LOG_TRACE("Package_id %d, apic_id %x.", package_id, apic_id);
    SEP_DRV_LOG_TRACE("Sys_info_nbits[cores_per_die,threads_per_core[%u]]: [%lld,%lld].", cpu, sys_info_nbits(cores_per_die), sys_info_nbits(threads_per_core[cpu]));

    VTSA_GEN_PER_CPU_cpu_intel_processor_number(local_gpc) = VTSA_NA32;
    VTSA_GEN_PER_CPU_cpu_package_num(local_gpc)            = (U16)package_id;
    VTSA_GEN_PER_CPU_cpu_core_num(local_gpc)               = (U16)core_id;
    VTSA_GEN_PER_CPU_cpu_hw_thread_num(local_gpc)          = (U16)thread_id;
    VTSA_GEN_PER_CPU_cpu_threads_per_core(local_gpc)       = (U16)threads_per_core[cpu];
    VTSA_GEN_PER_CPU_cpu_module_num(local_gpc)             = (U16)module_id;
    num_cores                                              = GLOBAL_STATE_num_cpus(driver_state)/threads_per_core[cpu];
    VTSA_GEN_PER_CPU_cpu_num_modules(local_gpc)            = (U16)(num_cores/2);  // Relavent to Atom processors, Always 2
    GLOBAL_STATE_num_modules(driver_state)                 = VTSA_GEN_PER_CPU_cpu_num_modules(local_gpc);
    SEP_DRV_LOG_TRACE("MODULE COUNT=%d.", GLOBAL_STATE_num_modules(driver_state));

    core_to_package_map[cpu]   = package_id;
    core_to_phys_core_map[cpu] = core_id;
    core_to_thread_map[cpu]    = thread_id;

    if (num_packages < package_id + 1) {
        num_packages = package_id + 1;
    }

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

/* ------------------------------------------------------------------------- */
/*!
* @fn static void sys_info_Update_Hyperthreading_Info(buffer)
*
* @param    buffer  -  points to the base of GEN_PER_CPU structure
* @return   None
*
* @brief  This routine is called to update per cpu information based on HT ON/OFF.
*
*/
static VOID
sys_info_Update_Hyperthreading_Info (
    VOID    *buffer
)
{
    U32                 cpu;
    VTSA_GEN_PER_CPU    *gen_per_cpu, *local_gpc;
    U32                 i = 0;
    U32                 num_cores = 0;

    SEP_DRV_LOG_TRACE_IN("");

    cpu        = CONTROL_THIS_CPU();

    // get the GEN_PER_CPU entry for the current processor.
    gen_per_cpu = (VTSA_GEN_PER_CPU*) buffer;

    // Update GEN_PER_CPU
    local_gpc                                   = &(gen_per_cpu[cpu]);

    // check how many CPUs are on thread 0, then we can know the number of physical cores.
    for (i = 0; i < (U32)GLOBAL_STATE_num_cpus(driver_state); i++) {
        if (core_to_thread_map[i] == 0) {
            num_cores++;
        }
    }

    threads_per_core[cpu] = (U32)(GLOBAL_STATE_num_cpus(driver_state)/num_cores);

    if (VTSA_GEN_PER_CPU_cpu_threads_per_core(local_gpc) != (U16)threads_per_core[cpu]) {
        VTSA_GEN_PER_CPU_cpu_threads_per_core(local_gpc)       = (U16)threads_per_core[cpu];
        VTSA_GEN_PER_CPU_cpu_num_modules(local_gpc)            = (U16)(num_cores/2);
        GLOBAL_STATE_num_modules(driver_state)                 = VTSA_GEN_PER_CPU_cpu_num_modules(local_gpc);
    }
    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn static void sys_info_Build_Percpu(buffer)
 *
 * @param    buffer  -  points to the base of GEN_PER_CPU structure
 * @return   None
 *
 * @brief  This routine is called to build per cpu information.
 *
 */
static VOID
sys_info_Build_Percpu (
    VOID    *buffer
)
{
    U32                  basic_funcs, basic_4_funcs, extended_funcs;
    U32                  num_cpuids;
    U32                  cpu;
    VTSA_CPUID          *current_cpuid;
    VTSA_GEN_ARRAY_HDR  *cpuid_gen_array_hdr;
    VTSA_GEN_PER_CPU    *gen_per_cpu, *local_gpc;
    VTSA_FIXED_SIZE_PTR *fsp;
    U8                  *cpuid_gen_array_hdr_base;
#if defined(ALLOW_ASSERT)
    U8                  *cpuid_buffer_limit;
#endif

    SEP_DRV_LOG_TRACE_IN("Buffer: %p.", buffer);

    cpu        = CONTROL_THIS_CPU();
    num_cpuids = (U32) sys_info_Get_Num_Cpuid_Funcs(&basic_funcs,
                                                    &basic_4_funcs,
                                                    &extended_funcs);

    // get the GEN_PER_CPU entry for the current processor.
    gen_per_cpu = (VTSA_GEN_PER_CPU*) buffer;
    SEP_DRV_LOG_TRACE("cpu %x: gen_per_cpu = %p.", cpu, gen_per_cpu);

    // get GEN_ARRAY_HDR and cpuid array base
    cpuid_gen_array_hdr_base = (U8 *) gen_per_cpu +
                               GLOBAL_STATE_num_cpus(driver_state) * sizeof(VTSA_GEN_PER_CPU);

    SEP_DRV_LOG_TRACE("cpuid_gen_array_hdr_base = %p.", cpuid_gen_array_hdr_base);
    SEP_DRV_LOG_TRACE("cpu = %x.", cpu);
    SEP_DRV_LOG_TRACE("cpuid_total_count[cpu] = %x.", cpuid_total_count[cpu]);
    SEP_DRV_LOG_TRACE("sizeof(VTSA_CPUID) = %lx.", sizeof(VTSA_CPUID));

    cpuid_gen_array_hdr = (VTSA_GEN_ARRAY_HDR *) ((U8 *) cpuid_gen_array_hdr_base  +
                                                  sizeof(VTSA_GEN_ARRAY_HDR) * cpu +
                                                  cpuid_total_count[cpu] * sizeof(VTSA_CPUID));

    // get current cpuid array base.
    current_cpuid = (VTSA_CPUID *) ((U8 *) cpuid_gen_array_hdr + sizeof(VTSA_GEN_ARRAY_HDR));
#if defined(ALLOW_ASSERT)
    // get the absolute buffer limit
    cpuid_buffer_limit = (U8 *)ioctl_sys_info +
                              GENERIC_IOCTL_size(&IOCTL_SYS_INFO_gen(ioctl_sys_info));
#endif

    //
    // Fill in GEN_PER_CPU
    //
    local_gpc                                   = &(gen_per_cpu[cpu]);

    if (VTSA_GEN_PER_CPU_cpu_intel_processor_number(local_gpc)) {
        SEP_DRV_LOG_TRACE_OUT("Early exit (VTSA_GEN_PER_CPU_cpu_intel_processor_number).");
        return;
    }
    VTSA_GEN_PER_CPU_cpu_number(local_gpc)      = cpu;
    VTSA_GEN_PER_CPU_cpu_core_type(local_gpc)   = 0;
    VTSA_GEN_PER_CPU_cpu_speed_mhz(local_gpc)   = VTSA_NA32;
    VTSA_GEN_PER_CPU_cpu_fsb_mhz(local_gpc)     = VTSA_NA32;

    fsp                                        = &VTSA_GEN_PER_CPU_cpu_cpuid_array(local_gpc);
    VTSA_FIXED_SIZE_PTR_is_ptr(fsp)            = 0;
    VTSA_FIXED_SIZE_PTR_fs_offset(fsp)         = (U64) ((U8 *)cpuid_gen_array_hdr -
                                                 (U8 *)&IOCTL_SYS_INFO_sys_info(ioctl_sys_info));

    /*
     * Get the time stamp difference between this cpu and cpu 0.
     * This value will be used by user mode code to generate standardize
     * time needed for sampling over time (SOT) functionality.
     */
    VTSA_GEN_PER_CPU_cpu_tsc_offset(local_gpc)  =  TSC_SKEW(cpu);


    //
    // fill GEN_ARRAY_HDR
    //
    fsp  = &VTSA_GEN_ARRAY_HDR_hdr_next_gen_hdr(cpuid_gen_array_hdr);
    VTSA_GEN_ARRAY_HDR_hdr_size(cpuid_gen_array_hdr)          = sizeof(VTSA_GEN_ARRAY_HDR);
    VTSA_FIXED_SIZE_PTR_is_ptr(fsp)                           = 0;
    VTSA_FIXED_SIZE_PTR_fs_offset(fsp)                        = 0;
    VTSA_GEN_ARRAY_HDR_array_num_entries(cpuid_gen_array_hdr) = num_cpuids;
    VTSA_GEN_ARRAY_HDR_array_entry_size(cpuid_gen_array_hdr)  = sizeof(VTSA_CPUID);
    VTSA_GEN_ARRAY_HDR_array_type(cpuid_gen_array_hdr)        = GT_CPUID;
#if defined(DRV_IA32)
    VTSA_GEN_ARRAY_HDR_array_subtype(cpuid_gen_array_hdr)     = GST_X86;
#elif defined(DRV_EM64T)
    VTSA_GEN_ARRAY_HDR_array_subtype(cpuid_gen_array_hdr)     = GST_EM64T;
#endif

    //
    // fill out cpu id information
    //
    sys_info_Fill_CPUID (num_cpuids,
                         basic_funcs,
                         extended_funcs,
                         cpu,
                         current_cpuid,
                         gen_per_cpu,
                         local_gpc);
    /*
     *  Mark cpu info on this cpu as successfully built
     */
    cpu_built_sysinfo[cpu] = 1;

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn static void sys_info_Get_Processor_Info(NULL)
 *
 * @param    None
 * @return   None
 *
 * @brief  This routine is called to get global informaton on the processor in general,
 *         it include:
 *             cpu_thread_per_core
 *
 */
static VOID
sys_info_Get_Processor_Info (
    VOID    *param
)
{
    U64          rax;
    U64          rbx;
    U64          rcx;
    U64          rdx;
    U32          family;
    U32          model;
    DRV_BOOL     ht_supported             = FALSE;

    SEP_DRV_LOG_TRACE_IN("");

    // read cpuid with function 1 to find family/model
    UTILITY_Read_Cpuid(1, &rax, &rbx, &rcx, &rdx);
    family    = (U32)(rax >>  8 & 0x0f);
    model     = (U32)(rax >> 12 & 0xf0);  /* extended model bits */
    model    |= (U32)(rax >>  4 & 0x0f);
    if (is_Knights_family(family, model)) {
        cpu_threads_per_core = 4;
    }
    else {
        ht_supported  = (rdx >> 28) & 1 ? TRUE : FALSE;
        if  (ht_supported) {
            cpu_threads_per_core = 2;
        }
        else {
            cpu_threads_per_core = 1;
        }
    }

    SEP_DRV_LOG_TRACE_OUT("");
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn extern void SYS_Info_Build(void)
 *
 * @param    None
 * @return   None
 *
 * @brief  This is the driver routine that constructs the VTSA_SYS_INFO
 * @brief  structure used to report system information into the tb5 file
 *
 */
extern U32
SYS_INFO_Build (
    VOID
)
{
    VTSA_GEN_ARRAY_HDR  *gen_array_hdr;
    VTSA_NODE_INFO      *node_info;
    VTSA_SYS_INFO       *sys_info;
    VTSA_FIXED_SIZE_PTR *fsp;
    U8                  *gen_per_cpu;
    U32                  buffer_size;
    U32                  total_cpuid_entries;
    U32                  i;
    struct sysinfo       k_sysinfo;
    int                  me;
    U32                  res;

    SEP_DRV_LOG_TRACE_IN("");
    SEP_DRV_LOG_TRACE("Entered.");

    if (ioctl_sys_info) {
        /* The sys info has already been computed.  Do not redo */
        buffer_size = GENERIC_IOCTL_size(&IOCTL_SYS_INFO_gen(ioctl_sys_info));
        return buffer_size - sizeof(GENERIC_IOCTL);
    }

    si_meminfo(&k_sysinfo);

    buffer_size = GLOBAL_STATE_num_cpus(driver_state) * sizeof(U32);
    cpu_built_sysinfo = CONTROL_Allocate_Memory(buffer_size);
    if (cpu_built_sysinfo == NULL) {
        SEP_DRV_LOG_ERROR_TRACE_OUT("Cpu_built_sysinfo memory alloc failed!");
        return 0;
    }

    cpuid_entry_count = CONTROL_Allocate_Memory(buffer_size);
    if (cpuid_entry_count == NULL) {
        cpu_built_sysinfo = CONTROL_Free_Memory(cpu_built_sysinfo);
        SEP_DRV_LOG_ERROR_TRACE_OUT("Memory alloc failed for cpuid_entry_count!");
        return 0;
    }

    cpuid_total_count = CONTROL_Allocate_Memory(buffer_size);
    if (cpuid_total_count == NULL) {
        cpu_built_sysinfo = CONTROL_Free_Memory(cpu_built_sysinfo);
        cpuid_entry_count = CONTROL_Free_Memory(cpuid_entry_count);
        SEP_DRV_LOG_ERROR_TRACE_OUT("Memory alloc failed for cpuid_total_count!");
        return 0;
    }

    // checking on family-model to set threads_per_core as 4: MIC,  2: ht-on; 1: rest
    sys_info_Get_Processor_Info(NULL);
    CONTROL_Invoke_Parallel(sys_info_Get_Cpuid_Entry_Count, (VOID *)cpuid_entry_count);
    total_cpuid_entries = 0;
    for (i = 0; i < (U32)GLOBAL_STATE_num_cpus(driver_state); i++) {
         //if cpu is offline, set its cpuid count same as cpu0
         if (cpuid_entry_count[i] == 0) {
             cpuid_entry_count[i] = cpuid_entry_count[0];
             cpu_built_sysinfo[i] = 0;
         }
         cpuid_total_count[i]  = total_cpuid_entries;
         total_cpuid_entries  += cpuid_entry_count[i];
    }

    ioctl_sys_info_size = sys_info_Get_Cpuid_Buffer_Size(total_cpuid_entries);
    ioctl_sys_info      = CONTROL_Allocate_Memory(ioctl_sys_info_size);
    if (ioctl_sys_info == NULL) {
        cpuid_entry_count = CONTROL_Free_Memory(cpuid_entry_count);
        cpuid_total_count = CONTROL_Free_Memory(cpuid_total_count);

        SEP_DRV_LOG_ERROR_TRACE_OUT("Memory alloc failed for ioctl_sys_info!");
//        return STATUS_INSUFFICIENT_RESOURCES;
        return 0;
    }

    //
    // fill in ioctl and cpu_cs_info fields.
    //
    GENERIC_IOCTL_size(&IOCTL_SYS_INFO_gen(ioctl_sys_info)) = ioctl_sys_info_size;
    GENERIC_IOCTL_ret(&IOCTL_SYS_INFO_gen(ioctl_sys_info))  = VT_SUCCESS;

    sys_info = &IOCTL_SYS_INFO_sys_info(ioctl_sys_info);
    VTSA_SYS_INFO_min_app_address(sys_info)        = VTSA_NA64;
    VTSA_SYS_INFO_max_app_address(sys_info)        = VTSA_NA64;
    VTSA_SYS_INFO_page_size(sys_info)              = k_sysinfo.mem_unit;
    VTSA_SYS_INFO_allocation_granularity(sys_info) = k_sysinfo.mem_unit;

    //
    // offset from ioctl_sys_info
    //
    VTSA_FIXED_SIZE_PTR_is_ptr(&VTSA_SYS_INFO_node_array(sys_info))    = 0;
    VTSA_FIXED_SIZE_PTR_fs_offset(&VTSA_SYS_INFO_node_array(sys_info)) = sizeof(VTSA_SYS_INFO);

    //
    // fill in node_info array header
    //
    gen_array_hdr = (VTSA_GEN_ARRAY_HDR *) ((U8 *) sys_info +
                     VTSA_FIXED_SIZE_PTR_fs_offset(&VTSA_SYS_INFO_node_array(sys_info)));

    SEP_DRV_LOG_TRACE("Gen_array_hdr = %p.", gen_array_hdr);
    fsp = &VTSA_GEN_ARRAY_HDR_hdr_next_gen_hdr(gen_array_hdr);
    VTSA_FIXED_SIZE_PTR_is_ptr(fsp)                     = 0;
    VTSA_FIXED_SIZE_PTR_fs_offset(fsp)                  = 0;

    VTSA_GEN_ARRAY_HDR_hdr_size(gen_array_hdr)          = sizeof(VTSA_GEN_ARRAY_HDR);
    VTSA_GEN_ARRAY_HDR_array_num_entries(gen_array_hdr) = 1;
    VTSA_GEN_ARRAY_HDR_array_entry_size(gen_array_hdr)  = sizeof(VTSA_NODE_INFO);
    VTSA_GEN_ARRAY_HDR_array_type(gen_array_hdr)        = GT_NODE;
    VTSA_GEN_ARRAY_HDR_array_subtype(gen_array_hdr)     = GST_UNK;

    //
    // fill in node_info
    //
    node_info = (VTSA_NODE_INFO *) ((U8 *) gen_array_hdr + sizeof(VTSA_GEN_ARRAY_HDR));
    SEP_DRV_LOG_TRACE("Node_info = %p.", node_info);

    VTSA_NODE_INFO_node_type_from_shell(node_info) = VTSA_NA32;

    VTSA_NODE_INFO_node_id(node_info)              = VTSA_NA32;
    VTSA_NODE_INFO_node_num_available(node_info)   = GLOBAL_STATE_num_cpus(driver_state);
    VTSA_NODE_INFO_node_num_used(node_info)        = VTSA_NA32;
    total_ram                                      = k_sysinfo.totalram << PAGE_SHIFT;
    VTSA_NODE_INFO_node_physical_memory(node_info) = total_ram;

    fsp = &VTSA_NODE_INFO_node_percpu_array(node_info);
    VTSA_FIXED_SIZE_PTR_is_ptr(fsp)      = 0;
    VTSA_FIXED_SIZE_PTR_fs_offset(fsp)   = sizeof(VTSA_SYS_INFO)      +
                                           sizeof(VTSA_GEN_ARRAY_HDR) +
                                           sizeof(VTSA_NODE_INFO);
    //
    // fill in gen_per_cpu array header
    //
    gen_array_hdr = (VTSA_GEN_ARRAY_HDR *) ((U8 *) sys_info + VTSA_FIXED_SIZE_PTR_fs_offset(fsp));
    SEP_DRV_LOG_TRACE("Gen_array_hdr = %p.", gen_array_hdr);

    fsp = &VTSA_GEN_ARRAY_HDR_hdr_next_gen_hdr(gen_array_hdr);
    VTSA_FIXED_SIZE_PTR_is_ptr(fsp)                     = 0;
    VTSA_FIXED_SIZE_PTR_fs_offset(fsp)                  = 0;

    VTSA_GEN_ARRAY_HDR_hdr_size(gen_array_hdr)          = sizeof(VTSA_GEN_ARRAY_HDR);
    VTSA_GEN_ARRAY_HDR_array_num_entries(gen_array_hdr) = GLOBAL_STATE_num_cpus(driver_state);
    VTSA_GEN_ARRAY_HDR_array_entry_size(gen_array_hdr)  = sizeof(VTSA_GEN_PER_CPU);
    VTSA_GEN_ARRAY_HDR_array_type(gen_array_hdr)        = GT_PER_CPU;

#if defined(DRV_IA32)
    VTSA_GEN_ARRAY_HDR_array_subtype(gen_array_hdr)     = GST_X86;
#elif defined(DRV_EM64T)
    VTSA_GEN_ARRAY_HDR_array_subtype(gen_array_hdr)     = GST_EM64T;
#endif

    gen_per_cpu = (U8 *) gen_array_hdr + sizeof(VTSA_GEN_ARRAY_HDR);

    me     = 0;
    CONTROL_Invoke_Parallel(APIC_Init, NULL);
    CONTROL_Invoke_Parallel(sys_info_Build_Percpu, (VOID *)gen_per_cpu);
    CONTROL_Invoke_Parallel(sys_info_Update_Hyperthreading_Info, (VOID *)gen_per_cpu);

    /*
     * Cleanup - deallocate memory that is no longer needed
     */
    cpuid_entry_count = CONTROL_Free_Memory(cpuid_entry_count);

    res = ioctl_sys_info_size - sizeof(GENERIC_IOCTL);

    SEP_DRV_LOG_TRACE_OUT("Res: %u.", res);
    return res;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn extern void SYS_Info_Transfer(buf_usr_to_drv, len_usr_to_drv)
 *
 * @param  buf_usr_to_drv      - pointer to the buffer to write the data into
 * @param  len_usr_to_drv  - length of the buffer passed in
 *
 * @brief  Transfer the data collected via the SYS_INFO_Build routine
 * @brief  back to the caller.
 *
 */
extern VOID
SYS_INFO_Transfer (
    PVOID           buf_usr_to_drv,
    unsigned long   len_usr_to_drv
)
{
    unsigned long exp_size;
    ssize_t       unused;

    SEP_DRV_LOG_TRACE_IN("Buffer: %p, buffer_len: %u.", buf_usr_to_drv, (U32) len_usr_to_drv);

    if (ioctl_sys_info == NULL || len_usr_to_drv == 0) {
        SEP_DRV_LOG_ERROR_TRACE_OUT("Ioctl_sys_info is NULL or len_usr_to_drv is 0!");
        return;
    }
    exp_size = GENERIC_IOCTL_size(&IOCTL_SYS_INFO_gen(ioctl_sys_info)) - sizeof(GENERIC_IOCTL);
    if (len_usr_to_drv < exp_size) {
        SEP_DRV_LOG_ERROR_TRACE_OUT("Insufficient Space!");
        return;
    }
    unused = copy_to_user(buf_usr_to_drv, &(IOCTL_SYS_INFO_sys_info(ioctl_sys_info)), len_usr_to_drv);
    if (unused) {
    // no-op ... eliminates "variable not used" compiler warning
    }

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn extern void SYS_Info_Destroy(void)
 *
 * @param    None
 * @return   None
 *
 * @brief  Free any memory associated with the sys info before unloading the driver
 *
 */
extern VOID
SYS_INFO_Destroy (
    void
)
{
    SEP_DRV_LOG_TRACE_IN("");

    cpuid_total_count   = CONTROL_Free_Memory(cpuid_total_count);
    cpu_built_sysinfo   = CONTROL_Free_Memory(cpu_built_sysinfo);
    ioctl_sys_info      = CONTROL_Free_Memory(ioctl_sys_info);
    ioctl_sys_info_size = 0;

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn extern void SYS_INFO_Build_Cpu(PVOID param)
 *
 * @param    PVOID param
 * @return   None
 *
 * @brief  call routine to populate cpu info
 *
 */
extern VOID
SYS_INFO_Build_Cpu(
    PVOID param
)
{
    VTSA_GEN_ARRAY_HDR  *gen_array_hdr;
    VTSA_NODE_INFO      *node_info;
    VTSA_SYS_INFO       *sys_info;
    VTSA_FIXED_SIZE_PTR *fsp;
    U8                  *gen_per_cpu;

    SEP_DRV_LOG_TRACE_IN("");

    if (!ioctl_sys_info) {
        SEP_DRV_LOG_ERROR_TRACE_OUT("Ioctl_sys_info is null!");
        return;
    }
    sys_info = &IOCTL_SYS_INFO_sys_info(ioctl_sys_info);
    gen_array_hdr = (VTSA_GEN_ARRAY_HDR *) ((U8 *) sys_info +
                     VTSA_FIXED_SIZE_PTR_fs_offset(&VTSA_SYS_INFO_node_array(sys_info)));
    SEP_DRV_LOG_TRACE("Gen_array_hdr = %p.", gen_array_hdr);

    node_info = (VTSA_NODE_INFO *) ((U8 *) gen_array_hdr + sizeof(VTSA_GEN_ARRAY_HDR));
    SEP_DRV_LOG_TRACE("Node_info = %p.", node_info);
    fsp = &VTSA_NODE_INFO_node_percpu_array(node_info);

    gen_array_hdr = (VTSA_GEN_ARRAY_HDR *) ((U8 *) sys_info + VTSA_FIXED_SIZE_PTR_fs_offset(fsp));
    SEP_DRV_LOG_TRACE("Gen_array_hdr = %p.", gen_array_hdr);
    gen_per_cpu = (U8 *) gen_array_hdr + sizeof(VTSA_GEN_ARRAY_HDR);

    sys_info_Build_Percpu((VOID *)gen_per_cpu);
    sys_info_Update_Hyperthreading_Info((VOID *)gen_per_cpu);

    SEP_DRV_LOG_TRACE_OUT("");
    return;
}


/*COPYRIGHT**
    Copyright (C) 2013-2020 Intel Corporation.  All Rights Reserved.

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
#include "lwpmudrv_types.h"
#include "inc/control.h"
#include "inc/utility.h"
#include "inc/sepdrv_p_state.h"

/*!
 * @fn     OS_STATUS SEPDRV_P_STATE_Read
 *
 * @brief  Reads the APERF and MPERF counters into the buffer provided for the purpose
 *
 * @param  buffer  - buffer to read the counts into
 *
 * @param  pcpu - pcpu struct that contains the previous APERF/MPERF values
 *
 * @return OS_SUCCESS if read succeeded, otherwise error
 *
 * @note
 */
extern OS_STATUS
SEPDRV_P_STATE_Read (
    S8 *buffer,
    CPU_STATE pcpu
)
{
    U64  *samp  = (U64 *)buffer;
    U64  new_APERF = 0;
    U64  new_MPERF = 0;

    SEP_DRV_LOG_TRACE_IN("Buffer: %p, pcpu: %p.", buffer, pcpu);

    if ((samp == NULL) || (pcpu == NULL)) {
        SEP_DRV_LOG_ERROR_TRACE_OUT("OS_INVALID (!samp || !pcpu).");
        return OS_INVALID;
    }

    new_APERF = SYS_Read_MSR(DRV_APERF_MSR);
    new_MPERF = SYS_Read_MSR(DRV_MPERF_MSR);

    if (CPU_STATE_last_p_state_valid(pcpu)) {
        // there is a previous APERF/MPERF value
        if ((CPU_STATE_last_aperf(pcpu)) > new_APERF) {
            // a wrap-around has occurred.
            samp[1] = CPU_STATE_last_aperf(pcpu) - new_APERF;
        }
        else {
            samp[1] = new_APERF - CPU_STATE_last_aperf(pcpu);
        }

        if ((CPU_STATE_last_mperf(pcpu)) > new_MPERF) {
            // a wrap-around has occurred.
            samp[0] = CPU_STATE_last_mperf(pcpu) - new_MPERF;
        }
        else {
            samp[0] = new_MPERF - CPU_STATE_last_mperf(pcpu);
        }
    }
    else {
        // there is no previous valid APERF/MPERF values, thus no delta calculations
        (CPU_STATE_last_p_state_valid(pcpu)) = TRUE;
        samp[0] = 0;
        samp[1] = 0;
    }

    CPU_STATE_last_aperf(pcpu) = new_APERF;
    CPU_STATE_last_mperf(pcpu) = new_MPERF;

    SEP_DRV_LOG_TRACE_OUT("OS_SUCCESS.");
    return OS_SUCCESS;
}


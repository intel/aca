/*COPYRIGHT**
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






**COPYRIGHT*/






#ifndef _PMU_LIST_H_INC_
#define _PMU_LIST_H_INC_


extern U32 drv_type;

/************************************************************/
/*
 * PMU list API for checking valid PMU list
 *
 ************************************************************/

/*!
 * @fn         DRV_BOOL PMU_LIST_Check_MSR (U32)
 *
 * @brief      Search the MSR address in the list
 *
 * @param      MSR address to be checked
 *
 * @return     TRUE if the MSR address is found in the list,
 *             FALSE otherwise
 */
extern DRV_BOOL
PMU_LIST_Check_MSR (
    U32              msr_id
);


/*!
 * @fn         DRV_BOOL PMU_LIST_Check_PCI (U8, U8, U8, U32)
 *
 * @brief      Search the PCI programming info in the list
 *
 * @param      bus_id - PCI bus id
 *             device_id - PCI device id
 *             func_id - PCI function id
 *             offset - PCI offset
 *
 * @return     TRUE if the PCI information is found in the list,
 *             FALSE otherwise
 */
extern DRV_BOOL
PMU_LIST_Check_PCI (
    U8              bus_id,
    U8              device_id,
    U8              func_id,
    U32             offset
);


/*!
 * @fn         DRV_BOOL PMU_LIST_Check_MMIO (PMU_MMIO_BAR_INFO_NODE,
 *                                           PMU_MMIO_BAR_INFO_NODE,
 *                                           U32)
 *
 * @brief      Search the MMIO programming info in the list
 *
 * @param      primary - pimary MMIO BAR programming info
 *             secondary - secondary MMIO BAR programming info
 *             offset - MMIO offset
 *
 * @return     TRUE if the MMIO information is found in the list,
 *             FALSE otherwise
 */
extern DRV_BOOL
PMU_LIST_Check_MMIO (
    PMU_MMIO_BAR_INFO_NODE   primary,
    PMU_MMIO_BAR_INFO_NODE   secondary,
    U32                      offset
);


/*!
 * @fn         OS_STATUS PMU_LIST_Initialize (S32 *)
 * @brief      Detect the CPU id and locate the applicable PMU list
 *
 * @param      index value of the whitelist. -1 if not found. 
 *
 * @return     OS_SUCCESS
 */
extern OS_STATUS
PMU_LIST_Initialize (
    S32 *idx
);


/*!
 * @fn         OS_STATUS PMU_LIST_Build_MSR_List (void)
 * @brief      Build the MSR search tree
 *
 * @param      None 
 *
 * @return     OS_SUCCESS
 */
extern OS_STATUS
PMU_LIST_Build_MSR_List (
    void
);

/*!
 * @fn         OS_STATUS PMU_LIST_Build_PCI_List (void)
 * @brief      Build the PCI search tree
 *
 * @param      None 
 *
 * @return     OS_SUCCESS
 */
extern OS_STATUS
PMU_LIST_Build_PCI_List (
    void
);

/*!
 * @fn         OS_STATUS PMU_LIST_Build_MMIO_List (void)
 * @brief      Build the MMIO search tree
 *
 * @param      None 
 *
 * @return     OS_SUCCESS
 */
extern OS_STATUS
PMU_LIST_Build_MMIO_List (
    void
);

/*!
 * @fn         OS_STATUS PMU_LIST_Clean_Up (void)
 * @brief      Clean up all the search trees
 *
 * @param      None 
 *
 * @return     OS_SUCCESS
 */
extern OS_STATUS
PMU_LIST_Clean_Up (
    void
);

#endif


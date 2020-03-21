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






#include "lwpmudrv_defines.h"
#include "lwpmudrv_types.h"
#include "rise_errors.h"
#include "lwpmudrv_ecb.h"
#include "lwpmudrv_struct.h"
#include "inc/control.h"
#include "inc/utility.h"

#include "inc/pmu_info_struct.h"
#include "inc/pmu_info_pci.h"
#include "inc/pmu_info_mmio.h"
#include "inc/pmu_info_msr.h"
#include "inc/pmu_info.h"
#include "inc/pmu_list.h"

#define USE_RANGE_OPTIMIZATION

static S32 pmu_info_index = -1;

static PMU_SEARCH_NODE *msr_root = NULL;
static PMU_SEARCH_NODE *pci_root = NULL;
static PMU_SEARCH_NODE *mmio_root = NULL;

static void
pmu_list_Lookup_PMU_Info (
    const PMU_INFO_NODE *pmu_list,
    U32                  family,
    U32                  model,
    U32                  stepping
)
{
    S32 i = 0;

    while (!(pmu_list[i].family == 0 && pmu_list[i].model == 0 && pmu_list[i].stepping_to == 0)) {
        if (pmu_list[i].family == family && pmu_list[i].model == model &&
            pmu_list[i].stepping_from <= stepping && pmu_list[i].stepping_to >= stepping) {
            pmu_info_index = i;
            return;
        }
        i++;
    }
}


/****************************************************************************************
 * Common helper fuctions for search algorithm
 ****************************************************************************************/

static S32
pmu_list_Max_Height (
    PMU_SEARCH_NODE *node_left,
    PMU_SEARCH_NODE *node_right
)
{
    if (node_left && node_right) {
        return  (node_left->height > node_right->height) ? node_left->height : node_right->height ;
    }
    else if (node_left) {
        return node_left->height;
    }
    else if (node_right) {
        return node_right->height;
    }
    return 0;
}

static PMU_SEARCH_NODE*
pmu_list_Right_Rotate (
    PMU_SEARCH_NODE *node
)
{
    PMU_SEARCH_NODE *nn, *r_child_nn;

    nn = node->left;
    r_child_nn = nn->right;

    // Rotate
    nn->right = node;        // node becomes right child
    node->left = r_child_nn; // original right child becomes left child

    // update height
    node->height = 1 + pmu_list_Max_Height(node->left, node->right);
    nn->height = 1 + pmu_list_Max_Height(nn->left, nn->right);

    return nn;
}

static PMU_SEARCH_NODE*
pmu_list_Left_Rotate (
    PMU_SEARCH_NODE *node
)
{
    PMU_SEARCH_NODE *nn, *l_child_nn;

    nn = node->right;
    l_child_nn = nn->left;

    // Rotate
    nn->left = node;        // node becomes left child
    node->right = l_child_nn; // original left child becomes right child

    // update height
    node->height = 1 + pmu_list_Max_Height(node->left, node->right);
    nn->height = 1 + pmu_list_Max_Height(nn->left, nn->right);

    return nn;
}


static PMU_SEARCH_NODE*
pmu_list_Create_Node (
    U64   key,
    U16   range,
    void *addr
)
{
    PMU_SEARCH_NODE *temp = (PMU_SEARCH_NODE *)CONTROL_Allocate_Memory(sizeof(PMU_SEARCH_NODE));
    temp->key = key;
    temp->range = range;
    temp->left = NULL;
    temp->right = NULL;
    temp->height = 1;
    temp->addr  = addr;
    SEP_DRV_LOG_TRACE("Added <key, range>:: < %x,%x>", key, range);
    return temp;
}


static void
pmu_list_Delete_Tree (
    PMU_SEARCH_NODE* node
)
{
    if (node == NULL) {
        return;
    }
    pmu_list_Delete_Tree(node->left);
    pmu_list_Delete_Tree(node->right);

    node->left = NULL;
    node->right = NULL;
    SEP_DRV_LOG_TRACE("Delete <key, range>:: <%x, %x>", node->key, node->range);
    CONTROL_Free_Memory(node);
}


/****************************************************************************************
 * Range is not used: for PCI and MMIO
 ****************************************************************************************/

static PMU_SEARCH_NODE*
pmu_list_Balance_Tree (
    PMU_SEARCH_NODE *node,
    U64              key
)
{

    S32 height_delta = 0;

    if (node->left && node->right) {
        height_delta = node->left->height - node->right->height;
    }
    else if (node->left) {
        height_delta = node->left->height;
    }
    else if (node->right) {
        height_delta = 0 - (node->right->height);
    }

    if (height_delta == 0) {
        // tree is balanced
        return node;
    }
    // if Delta > 1, balance left tree
    else if ((height_delta > 1) && (node->key > key)) {
       node = pmu_list_Right_Rotate(node);
    }
    else if ((height_delta > 1) && (node->key < key)) {
        node->left = pmu_list_Left_Rotate(node->left);
        node = pmu_list_Right_Rotate(node);
    }
    // if Delta < -1, balance right tree
    else if ((height_delta < -1) && (node->key < key)) {
        node = pmu_list_Left_Rotate(node);
    }
    else if ((height_delta < -1) && (node->key > key)) {
        node->right = pmu_list_Right_Rotate(node->right);
        node = pmu_list_Left_Rotate(node);
    }
    return node;
}

static PMU_SEARCH_NODE*
pmu_list_Insert_Node (
    PMU_SEARCH_NODE *node,
    U64              key,
    void            *addr
)
{
    if (node == NULL) {
        // make it root, range = 0
        node = pmu_list_Create_Node(key, 0, addr);
    }
    else if (node->key < key) {
        // insert to right sub tree
        node->right = pmu_list_Insert_Node(node->right, key, addr);
    }
    else if (node->key > key) {
        // insert to left sub tree
        node->left = pmu_list_Insert_Node(node->left, key, addr);
    }
    else {
        // do nothing
        return node;
    }

    // update height
    node->height = 1 + pmu_list_Max_Height(node->left, node->right);

    // Balance the tree
    return pmu_list_Balance_Tree(node, key);
}

static PMU_SEARCH_NODE*
pmu_list_Binary_Search (
    PMU_SEARCH_NODE *node,
    U64              key
)
{
    if (node == NULL) {
        return NULL;
    }
    if (node->key == key) {
        return node;
    }
    else if (node->key < key) {
        return pmu_list_Binary_Search(node->right, key);
    }
    else {
        return pmu_list_Binary_Search(node->left, key);
    }
}


/****************************************************************************************
 * Range is used : for MSR
 ****************************************************************************************/

#if defined(USE_RANGE_OPTIMIZATION)
static PMU_SEARCH_NODE*
pmu_list_Balance_Tree_Range (
    PMU_SEARCH_NODE *node,
    U64              key,
    U16              range
)
{
    S32 height_delta = 0;

    if (node->left && node->right) {
        height_delta = (node->left->height) - (node->right->height);
    }
    else if (node->left) {
        height_delta = node->left->height;
    }
    else if (node->right) {
        height_delta = 0 - (node->right->height);
    }

    if (height_delta == 0) {
        // tree is balanced
        return node;
    }
    // if Delta > 1, balance left tree
    else if ((height_delta > 1) && ((node->key > key) && ((node->key + range) > key))) {
        node = pmu_list_Right_Rotate(node);
    }
    else if ((height_delta > 1) && ((node->key < key) && ((node->key + range) < key))) {
        node->left = pmu_list_Left_Rotate(node->left);
        node = pmu_list_Right_Rotate(node);
    }
    // if Delta < -1, balance right tree
    else if ((height_delta < -1) && ((node->key < key) && ((node->key + range) < key))) {
        node = pmu_list_Left_Rotate(node);
    }
    else if ((height_delta < -1) && ((node->key > key) && ((node->key + range) > key))) {
        node->right = pmu_list_Right_Rotate(node->right);
        node = pmu_list_Left_Rotate(node);
    }
    return node;
}

static PMU_SEARCH_NODE*
pmu_list_Insert_Node_Range (
    PMU_SEARCH_NODE *node,
    U64              key,
    U16              range,
    void            *addr
)
{
    if (node == NULL) {
        // make it root
        node = pmu_list_Create_Node(key, range, addr);
    }
    else if (node->key < key) {
        if (node->key + node->range < key) {
            // case 6: new key and range is greater then existing key and range i.e.,
            // (new_key > old_key) & (new_key > old_key + old_range)
            // insert to right subtree (new_key, new_range)
            node->right = pmu_list_Insert_Node_Range(node->right, key, range, addr);
        }
        else if (node->key + node->range < key + range) {
            // case 3 : <new_key, new_range> overlaps with <old_key, old_range> i.e.,
            // (old_key + old_range > new_key > old_key)  & (new_key + new_range > old_key + old_range)
            // update range (old_key, <range = (new_key+new_range)-old_key>)
            SEP_DRV_LOG_TRACE("Case 3: Updated <key, range>:: <%x, %x> -> <%x, %x>",
                    node->key, node->range, node->key, ((key + range) - (node->key)));
                node->range = (U16)(key - node->key) + range;
            return node;
        }
        else {
            // case 1 : <new_key, new_range> is subset of <old_key, old_range>,
            // (old_key + old_range > new_key > old_key) & (new_key + new_range < old_key + old_range)
            // do nothing
            return node;
        }
    }
    else if (node->key > key) {
        if (node->key > key + range){ 
            // case 5 : new key and range is less than existing key i.e.,
            // (new_key < old_key) & (new_key + new_range < old_key)
            // insert to left subtree (new_key, new_range)
            node->left = pmu_list_Insert_Node_Range(node->left, key, range, addr);
        }
        else if (node->key + node->range < key + range) {
            // case 2: <new_key, new_range> is superset of <old_key, old_range>
            // (new_key < old_key) & (new_key + new_range > old_key + old_range)
            // insert to left subtree (new_key , <range = old_key-new_key-1> ) and
            // update current node (old_key , <range = (new_key+new_range)-old_key>)
            SEP_DRV_LOG_TRACE("Case 2: Split to new <key, range>:: <%x, %x> -> <%x, %x>",
                                    key, range, key, (node->key - key - 1));
            node->left = pmu_list_Insert_Node_Range(node->left, key, ((U16)(node->key - key) - 1)  , addr);
            if (node->key + node->range < key + range ) {
                SEP_DRV_LOG_TRACE("Case 2: Updated <key, range>:: <%x, %x> -> <%x, %x>",
                                    node->key, node->range, node->key, ((key + range) - (node->key)));
                node->range = range - (U16)(node->key - key);
            }
        }
        else {
            // case 4: <new_key, new_range> overlaps with <old_key, old_range> i.e.,
            // (new_key < old_key) & (new_key + new_range > old_key )
            // insert to left tree (new_key , <range = old_key-new_key-1> )
            SEP_DRV_LOG_TRACE("Case 4: Split to new <key, range>:: <%x, %x> -> <%x, %x>",
                                    key, range, key, (node->key - key - 1));
            node->left = pmu_list_Insert_Node_Range(node->left, key, ((U16)(node->key - key) - 1) , addr);
        }
    }
    else {
        if (range > node->range){
            // case 7: (new_key == old_key) & (new_range > old_range)
            // update current node (old_key, new_range)
            SEP_DRV_LOG_TRACE("Case 7: Updated <key, range>:: <%x, %x> -> <%x, %x> ",
                                    node->key, node->range, node->key, range);
            node->range = range;
        }
        return node;
    }

    // update height
    node->height = 1 + pmu_list_Max_Height(node->left, node->right);

    // Balance the tree
    return pmu_list_Balance_Tree_Range(node, key, range);
}



static PMU_SEARCH_NODE*
pmu_list_Binary_Search_Range (
    PMU_SEARCH_NODE *node,
    U64              key
)
{
    if (node == NULL) {
        return NULL;
    }

    if ((key >= (node->key)) && (key <= (node->key + node->range))) {
        // key found
        return node;
    }
    else if (key > (node->key)) {
        // search right tree
        return pmu_list_Binary_Search_Range(node->right, key);
    }
    else if (key < node->key) {
        // search left tree
        return pmu_list_Binary_Search_Range(node->left, key);
    }
    else {
        // unexpected case - something wrong
        return NULL;
    }
}

static OS_STATUS
pmu_list_Create_MSR_Tree_Range (
    const PMU_INFO_NODE *pmu_list
)
{
    S32 j = 0;
    PMU_MSR_INFO_NODE *list;

    if (pmu_info_index == -1) {
        return -1;
    }
    if (pmu_list[pmu_info_index].msr_info_list == NULL) {
        return -1;
    }
    while (pmu_list[pmu_info_index].msr_info_list[j] != 0) {
        list = pmu_list[pmu_info_index].msr_info_list[j++];

        while ((*list).msr_id != 0x0) {
            SEP_DRV_LOG_TRACE("Incoming <key, range>:: <%x, %x> ", (*list).msr_id, (*list).range);
            msr_root = pmu_list_Insert_Node_Range(msr_root, (*list).msr_id, (*list).range, (void *)list);
            list++;
        }
    }
    return 0;
}
#else

static OS_STATUS
pmu_list_Create_MSR_Tree (
    const PMU_INFO_NODE *pmu_list
)
{
    S32 j = 0;
    PMU_MSR_INFO_NODE *list;

    if (pmu_info_index == -1) {
        return -1;
    }
    if (pmu_list[pmu_info_index].msr_info_list == NULL) {
        return -1;
    }
    while (pmu_list[pmu_info_index].msr_info_list[j] != 0) {
        list = pmu_list[pmu_info_index].msr_info_list[j++];

        while ((*list).msr_id != 0) {
            if ((*list).range != 0) {
                // populate entry for each MSR value for range
                S32 i = 0;
                for (i = 0 ; i <= (*list).range; i++) {
                    msr_root = pmu_list_Insert_Node(msr_root, (*list).msr_id + i, (void *)list);
                }
            }
            else {
                msr_root = pmu_list_Insert_Node(msr_root, (*list).msr_id, (void *)list);
            }
            list++;
        }
    }
    return 0;
}
#endif


extern DRV_BOOL
PMU_LIST_Check_MSR (
    U32              msr_id
)
{
    PMU_SEARCH_NODE* temp;

    SEP_DRV_LOG_TRACE_IN("");

    if (pmu_info_index == -1 || msr_root == NULL) {
        SEP_DRV_LOG_TRACE_OUT("Success");
        return FALSE;
    }

#if !defined(USE_RANGE_OPTIMIZATION)
    temp = pmu_list_Binary_Search(msr_root, msr_id);
#else
    temp = pmu_list_Binary_Search_Range(msr_root, msr_id);
#endif

    // returning search node so that it can be used if any reference to static node is required
    if (temp == NULL) {
        SEP_DRV_LOG_TRACE_OUT("Failure");
        return FALSE;
    }
    else {
        SEP_DRV_LOG_TRACE_OUT("Success");
        return TRUE;
    }
}

extern DRV_BOOL
PMU_LIST_Check_PCI (
    U8              bus_id,
    U8              dev_num,
    U8              func_num,
    U32             offset
)
{
    PMU_PCI_INFO_NODE  key;
    PMU_SEARCH_NODE   *temp;

    SEP_DRV_LOG_TRACE_IN("");

    if (pmu_info_index == -1 || pci_root == NULL) {
        SEP_DRV_LOG_TRACE_OUT("Success");
        return FALSE;
    }

    SEP_DRV_MEMSET(&key, 0, sizeof(PMU_PCI_INFO_NODE));

    key.u.s.dev = dev_num;
    key.u.s.func = func_num;
    key.u.s.offset = offset;

    temp = pmu_list_Binary_Search(pci_root, key.u.reg);
    if (temp != NULL){
        SEP_DRV_LOG_TRACE_OUT("Success");
        return TRUE;
    }
    else {
        SEP_DRV_LOG_TRACE_OUT("Success");
        return FALSE;
    }
}

extern DRV_BOOL
PMU_LIST_Check_MMIO (
    PMU_MMIO_BAR_INFO_NODE   primary,
    PMU_MMIO_BAR_INFO_NODE   secondary,
    U32                      offset
)
{
    PMU_SEARCH_NODE         *temp;
    U64                      key;
    PMU_MMIO_UNIT_INFO_NODE *unit_info = NULL;
    DRV_BOOL                 ret = FALSE;

    SEP_DRV_LOG_TRACE_IN("");

    if (pmu_info_index == -1 || mmio_root == NULL) {
        SEP_DRV_LOG_TRACE_OUT("Success");
        return FALSE;
    }

    if (primary.bar_prog_type == MMIO_SINGLE_BAR_TYPE) {
        key = (U64)primary.u.reg << 32 | offset;
    }
    else if (primary.bar_prog_type == MMIO_DUAL_BAR_TYPE) {
        key = (U64)secondary.u.reg << 32 | offset;
    }
    else if (primary.bar_prog_type == MMIO_DIRECT_BAR_TYPE) {
        key = (U64)primary.mask << 32 | offset;
    }
    else {
        SEP_DRV_LOG_TRACE("Invalid BAR prog type %d", primary.bar_prog_type);
        SEP_DRV_LOG_TRACE_OUT("Success");
        return FALSE;
    }

    temp = pmu_list_Binary_Search(mmio_root, key);
    if (temp != NULL){
        if (primary.bar_prog_type == MMIO_DIRECT_BAR_TYPE) {
            ret = TRUE;
        }
        else if (primary.bar_prog_type == MMIO_SINGLE_BAR_TYPE) {
            unit_info = (PMU_MMIO_UNIT_INFO_NODE *)temp->addr;
            if (unit_info && (unit_info->primary.mask == primary.mask) &&
                (unit_info->primary.shift == primary.shift)) {
                ret = TRUE;
            }
        }
        else if (primary.bar_prog_type == MMIO_DUAL_BAR_TYPE) {
            unit_info = (PMU_MMIO_UNIT_INFO_NODE *)temp->addr;
            if (unit_info && (unit_info->secondary.mask == secondary.mask) &&
                (unit_info->secondary.shift == secondary.shift) &&
                (unit_info->primary.u.s.offset == primary.u.s.offset) &&
                (unit_info->primary.mask == primary.mask) &&
                (unit_info->primary.shift == primary.shift)) {
                ret = TRUE;
            }
        }
    }

    SEP_DRV_LOG_TRACE_OUT("Success");
    return ret;
}


extern OS_STATUS
PMU_LIST_Initialize (
    S32 *idx
)
{
    U64           rax, rbx, rcx, rdx;
    U32           family, model, stepping;

    SEP_DRV_LOG_TRACE_IN("");

    UTILITY_Read_Cpuid(0x1, &rax, &rbx, &rcx, &rdx);

    family    = (U32)(rax >>  8 & 0x0f);
    model     = (U32)(rax >> 12 & 0xf0);  /* extended model bits */
    model    |= (U32)(rax >>  4 & 0x0f);
    stepping  = (U32)(rax & 0x0f);

    pmu_info_index = -1;
    pmu_list_Lookup_PMU_Info(pmu_info_list, family, model, stepping);

    if (idx) {
        *idx = pmu_info_index;
    }

    SEP_DRV_LOG_LOAD("PMU check enabled! F%x.M%x.S%x index=%d drv_type=%s\n",
        family, model, stepping, pmu_info_index, drv_type_str);

    SEP_DRV_LOG_TRACE_OUT("Success");
    return OS_SUCCESS;
}


extern OS_STATUS
PMU_LIST_Build_MSR_List (
    void
)
{
    S32 status = OS_SUCCESS;

    SEP_DRV_LOG_TRACE_IN("");

    if (pmu_info_index == -1 || !pmu_info_list[pmu_info_index].msr_info_list) {
        SEP_DRV_LOG_LOAD("No MSR list information detected!\n");
        SEP_DRV_LOG_TRACE_OUT("Success");
        return status;
    }

#if !defined(USE_RANGE_OPTIMIZATION)
    status = pmu_list_Create_MSR_Tree(pmu_info_list);
#else
    status = pmu_list_Create_MSR_Tree_Range(pmu_info_list);
#endif

    SEP_DRV_LOG_TRACE_OUT("Success");

    return status;
}

extern OS_STATUS
PMU_LIST_Build_PCI_List (
    void
)
{
    U32                     unit_idx = 0;
    U32                     reg_idx = 0;
    PMU_PCI_INFO_NODE       key;
    PMU_PCI_UNIT_INFO_NODE *unit_info_list = pmu_info_list[pmu_info_index].pci_info_list;

    SEP_DRV_LOG_TRACE_IN("");

    if (pmu_info_index == -1 || !unit_info_list) {
        SEP_DRV_LOG_LOAD("No PCI list information detected!\n");
        SEP_DRV_LOG_TRACE_OUT("Success");
        return OS_SUCCESS;
    }

    SEP_DRV_MEMSET(&key, 0, sizeof(PMU_PCI_INFO_NODE));

    while (unit_info_list[unit_idx].reg_offset_list) { //Iterate through unit list
        reg_idx = 0;
        key.u.s.dev = (U8)unit_info_list[unit_idx].dev;
        key.u.s.func = (U8)unit_info_list[unit_idx].func;

        while (unit_info_list[unit_idx].reg_offset_list[reg_idx] != 0x0) { //Iterate through offset list
            key.u.s.offset = unit_info_list[unit_idx].reg_offset_list[reg_idx];
            pci_root = pmu_list_Insert_Node(pci_root, key.u.reg, (void *)(&unit_info_list[unit_idx]));
            reg_idx++;
        }
        unit_idx++;
    }

    SEP_DRV_LOG_TRACE_OUT("Success");
    return OS_SUCCESS;
}

extern OS_STATUS
PMU_LIST_Build_MMIO_List (
    void
)
{
    U32                      unit_idx = 0;
    U32                      reg_idx = 0;
    U64                      key;
    PMU_MMIO_UNIT_INFO_NODE *unit_info_list = pmu_info_list[pmu_info_index].mmio_info_list;

    SEP_DRV_LOG_TRACE_IN("");

    if (pmu_info_index == -1 || !unit_info_list) {
        SEP_DRV_LOG_LOAD("No MMIO list information detected!\n");
        SEP_DRV_LOG_TRACE_OUT("Success");
        return OS_SUCCESS;
    }

    SEP_DRV_MEMSET(&key, 0, sizeof(U64));

    while (unit_info_list[unit_idx].reg_offset_list) { //Iterate through unit list
        reg_idx = 0;
        while (unit_info_list[unit_idx].reg_offset_list[reg_idx] != 0x0) { //Iterate through offset list
            switch (unit_info_list[unit_idx].primary.bar_prog_type) {
                case MMIO_SINGLE_BAR_TYPE:
                    key = (U64)unit_info_list[unit_idx].primary.u.reg << 32 |
                          (U64)unit_info_list[unit_idx].reg_offset_list[reg_idx];
                    mmio_root = pmu_list_Insert_Node(mmio_root, key, (void *)(&unit_info_list[unit_idx]));
                    break;
                case MMIO_DUAL_BAR_TYPE:
                    key = (U64)unit_info_list[unit_idx].secondary.u.reg << 32 |
                          (U64)unit_info_list[unit_idx].reg_offset_list[reg_idx];
                    mmio_root = pmu_list_Insert_Node(mmio_root, key, (void *)(&unit_info_list[unit_idx]));
                    break;
                case MMIO_DIRECT_BAR_TYPE:
                    key = (U64)unit_info_list[unit_idx].primary.mask << 32 |
                          (U64)unit_info_list[unit_idx].reg_offset_list[reg_idx];
                    mmio_root = pmu_list_Insert_Node(mmio_root, key, (void *)(&unit_info_list[unit_idx]));
                    break;
            }
            reg_idx++;
        }
        unit_idx++;
    }

    SEP_DRV_LOG_TRACE_OUT("Success");
    return OS_SUCCESS;
}

extern OS_STATUS
PMU_LIST_Clean_Up (
    void
)
{
    SEP_DRV_LOG_TRACE_IN("");

    pmu_info_index = -1;

    if (msr_root) {
        pmu_list_Delete_Tree(msr_root);
        msr_root = NULL;
    }

    if (pci_root) {
        pmu_list_Delete_Tree(pci_root);
        pci_root = NULL;
    }

    if (mmio_root) {
        pmu_list_Delete_Tree(mmio_root);
        mmio_root = NULL;
    }

    SEP_DRV_LOG_TRACE_OUT("Success");
    return OS_SUCCESS;
}



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






#ifndef _PMU_INFO_STRUCT_H_INC_
#define _PMU_INFO_STRUCT_H_INC_

// Data Structures for storing entire PMU list
typedef struct PMU_MSR_INFO_NODE_S PMU_MSR_INFO_NODE;
struct PMU_MSR_INFO_NODE_S {
    U64	   msr_id;
    U64    mask;
    U16    range;
    U16    dynamic; // to be updated
};

typedef struct PMU_PCI_INFO_NODE_S PMU_PCI_INFO_NODE;
struct PMU_PCI_INFO_NODE_S {
    union {
        struct {
            U64 bus    : 8;   
            U64 dev    : 5;
            U64 func   : 3;
            U64 offset : 16;
            U64 rsvd   : 32;
        } s;
        U64 reg;
    } u;
};

typedef struct PMU_PCI_UNIT_INFO_NODE_S PMU_PCI_UNIT_INFO_NODE;
struct PMU_PCI_UNIT_INFO_NODE_S {
    U32  dev;
    U32  func;
    U16 *reg_offset_list;
};

typedef struct PMU_MMIO_BAR_INFO_NODE_S PMU_MMIO_BAR_INFO_NODE;
struct PMU_MMIO_BAR_INFO_NODE_S {
    union {
        struct {
            U32 bus    : 8;
            U32 dev    : 5;
            U32 func   : 3;
            U32 offset : 16;
        } s;
        U32 reg;
    } u;
    U8  shift;
    U8	bar_prog_type;
    U16	reserved;
    U64 mask;
};

enum {
    MMIO_SINGLE_BAR_TYPE = 1,
    MMIO_DUAL_BAR_TYPE,
    MMIO_DIRECT_BAR_TYPE
};

typedef struct PMU_MMIO_UNIT_INFO_NODE_S PMU_MMIO_UNIT_INFO_NODE;
struct PMU_MMIO_UNIT_INFO_NODE_S {
    PMU_MMIO_BAR_INFO_NODE   primary;
    PMU_MMIO_BAR_INFO_NODE   secondary;
    U32                     *reg_offset_list;
};

typedef struct PMU_INFO_NODE_S PMU_INFO_NODE;
struct PMU_INFO_NODE_S {
    U16                        family;
    U16                        model;
    U16                        stepping_from;
    U16                        stepping_to;
    PMU_MSR_INFO_NODE        **msr_info_list;
    PMU_PCI_UNIT_INFO_NODE    *pci_info_list;
    PMU_MMIO_UNIT_INFO_NODE   *mmio_info_list;
};

// Data Structure for search operation
typedef struct PMU_SEARCH_NODE_S PMU_SEARCH_NODE;
struct PMU_SEARCH_NODE_S
{
    U64                  key;    // common for MSR/PCI/MMIO
    void                *addr;   // copy address of static node
    PMU_SEARCH_NODE     *left;
    PMU_SEARCH_NODE     *right;
    U16                  height; // For balancing the search tree
    U16                  range;  // For MSR
};

#endif



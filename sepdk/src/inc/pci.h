/*COPYRIGHT**
    Copyright (C) 2012-2020 Intel Corporation.  All Rights Reserved.

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





#ifndef _PCI_H_
#define _PCI_H_

#include "lwpmudrv_defines.h"

/*
 * PCI Config Address macros
 */
#define PCI_ENABLE                          0x80000000

#define PCI_ADDR_IO                         0xCF8
#define PCI_DATA_IO                         0xCFC

#define BIT0                                0x1
#define BIT1                                0x2

/*
 * Macro for forming a PCI configuration address
 */
#define FORM_PCI_ADDR(bus,dev,fun,off)     (((PCI_ENABLE))          |   \
                                            ((bus & 0xFF) << 16)    |   \
                                            ((dev & 0x1F) << 11)    |   \
                                            ((fun & 0x07) <<  8)    |   \
                                            ((off & 0xFF) <<  0))

#define VENDOR_ID_MASK                        0x0000FFFF
#define DEVICE_ID_MASK                        0xFFFF0000
#define DEVICE_ID_BITSHIFT                    16
#define LOWER_4_BYTES_MASK                    0x00000000FFFFFFFF
#define MAX_BUSNO                             256
#define NEXT_ADDR_OFFSET                      4
#define NEXT_ADDR_SHIFT                       32
#define DRV_IS_PCI_VENDOR_ID_INTEL            0x8086
#define MAX_PCI_DEVS                          32

#define CONTINUE_IF_NOT_GENUINE_INTEL_DEVICE(value, vendor_id, device_id)    \
    {                                                                        \
        vendor_id = value & VENDOR_ID_MASK;                                  \
        device_id = (value & DEVICE_ID_MASK) >> DEVICE_ID_BITSHIFT;          \
                                                                             \
        if (vendor_id != DRV_IS_PCI_VENDOR_ID_INTEL) {                       \
            continue;                                                        \
        }                                                                    \
                                                                             \
    }

#define CHECK_IF_GENUINE_INTEL_DEVICE(value, vendor_id, device_id, valid)    \
    {                                                                        \
        vendor_id = value & VENDOR_ID_MASK;                                  \
        device_id = (value & DEVICE_ID_MASK) >> DEVICE_ID_BITSHIFT;          \
                                                                             \
        valid = 1;                                                           \
        if (vendor_id != DRV_IS_PCI_VENDOR_ID_INTEL) {                       \
            valid = 0;                                                       \
        }                                                                    \
                                                                             \
    }


typedef struct SEP_MMIO_NODE_S SEP_MMIO_NODE;

struct SEP_MMIO_NODE_S {
    U64    physical_address;
    U64    virtual_address;
    U64    map_token;
    U32    size;
};

#define SEP_MMIO_NODE_physical_address(x)   ((x)->physical_address)
#define SEP_MMIO_NODE_virtual_address(x)    ((x)->virtual_address)
#define SEP_MMIO_NODE_map_token(x)          ((x)->map_token)
#define SEP_MMIO_NODE_size(x)               ((x)->size)


extern OS_STATUS
PCI_Map_Memory (
    SEP_MMIO_NODE *node,
    U64            phy_address,
    U32            map_size
);

extern void
PCI_Unmap_Memory (
    SEP_MMIO_NODE *node
);

extern int
PCI_Read_From_Memory_Address (
    U32 addr,
    U32* val
);

extern int
PCI_Write_To_Memory_Address (
    U32 addr,
    U32 val
);



/*** UNIVERSAL PCI ACCESSORS ***/

extern VOID PCI_Initialize (
    VOID
);

extern U32 PCI_Read_U32 (
    U32    bus,
    U32    device,
    U32    function,
    U32    offset
);

extern U32 PCI_Read_U32_Valid (
    U32    bus,
    U32    device,
    U32    function,
    U32    offset,
    U32    invalid_value
);

extern U64 PCI_Read_U64 (
    U32    bus,
    U32    device,
    U32    function,
    U32    offset
);

extern U64 PCI_Read_U64_Valid (
    U32    bus,
    U32    device,
    U32    function,
    U32    offset,
    U64    invalid_value
);

extern U32 PCI_Write_U32 (
    U32    bus,
    U32    device,
    U32    function,
    U32    offset,
    U32    value
);

extern U32 PCI_Write_U64 (
    U32    bus,
    U32    device,
    U32    function,
    U32    offset,
    U64    value
);


/*** UNIVERSAL MMIO ACCESSORS ***/

extern U32
PCI_MMIO_Read_U32 (
    U64    virtual_address_base,
    U32    offset
);

extern U64
PCI_MMIO_Read_U64 (
    U64    virtual_address_base,
    U32    offset
);

extern void
PCI_MMIO_Write_U32 (
    U64    virtual_address_base,
    U32    offset,
    U32    value
);

extern void
PCI_MMIO_Write_U64 (
    U64    virtual_address_base,
    U32    offset,
    U64    value
);


#endif


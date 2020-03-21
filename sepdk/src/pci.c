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
#include <linux/errno.h>
#include <linux/pci.h>
#include <linux/types.h>
#include <asm/page.h>
#include <asm/io.h>

#include "lwpmudrv_types.h"
#include "rise_errors.h"
#include "lwpmudrv_ecb.h"

#include "inc/lwpmudrv.h"
#include "inc/pci.h"
#include "inc/utility.h"


struct pci_bus* pci_buses[MAX_BUSNO] = {0};


/* ------------------------------------------------------------------------- */
/*!
 * @fn extern VOID PCI_Initialize(VOID)
 *
 * @param   none
 *
 * @return  none
 *
 * @brief   Initializes the pci_buses array.
 *
 */
extern VOID PCI_Initialize (
    VOID
)
{
    U32 i;
    U32 num_found_buses = 0;

    SEP_DRV_LOG_INIT_IN("Initializing pci_buses...");

    for (i = 0; i < MAX_BUSNO; i++) {
        pci_buses[i] = pci_find_bus(0, i);
        if (pci_buses[i]) {
            SEP_DRV_LOG_DETECTION("Found PCI bus 0x%x at %p.", i, pci_buses[i]);
            num_found_buses++;
        }
        SEP_DRV_LOG_TRACE("pci_buses[%u]: %p.", i, pci_buses[i]);
    }

    SEP_DRV_LOG_INIT_OUT("Found %u buses.", num_found_buses);
}


/* ------------------------------------------------------------------------- */
/*!
 * @fn extern U32 PCI_Read_U32(bus, device, function, offset)
 *
 * @param    bus        - target bus
 * @param    device     - target device
 * @param    function   - target function
 * @param    offset     - target register offset
 *
 * @return  Value at this location
 *
 * @brief   Reads a U32 from PCI configuration space
 *
 */
extern U32 PCI_Read_U32 (
    U32    bus,
    U32    device,
    U32    function,
    U32    offset
)
{
    U32 res   = 0;
    U32 devfn = (device << 3) | (function & 0x7);

    SEP_DRV_LOG_REGISTER_IN("Will read BDF(%x:%x:%x)[0x%x](4B)...", bus, device, function, offset);

    if (bus < MAX_BUSNO && pci_buses[bus]) {
        pci_buses[bus]->ops->read(pci_buses[bus], devfn, offset, 4, &res);
    }
    else {
        SEP_DRV_LOG_ERROR("Could not read BDF(%x:%x:%x)[0x%x]: bus not found!", bus, device, function, offset);
    }

    SEP_DRV_LOG_REGISTER_OUT("Has read BDF(%x:%x:%x)[0x%x](4B): 0x%x.", bus, device, function, offset, res);
    return res;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn extern U32 PCI_Read_U32_Valid(bus, device, function, offset, invalid_value)
 *
 * @param    bus            - target bus
 * @param    device         - target device
 * @param    function       - target function
 * @param    offset         - target register offset
 * @param    invalid_value  - value against which to compare the PCI-obtained value
 *
 * @return  Value at this location (if value != invalid_value), 0 otherwise
 *
 * @brief   Reads a U32 from PCI configuration space
 *
 */
extern U32 PCI_Read_U32_Valid (
    U32    bus,
    U32    device,
    U32    function,
    U32    offset,
    U32    invalid_value
)
{
    U32 res   = 0;
    U32 devfn = (device << 3) | (function & 0x7);

    SEP_DRV_LOG_REGISTER_IN("Will read BDF(%x:%x:%x)[0x%x](4B)...", bus, device, function, offset);

    if (bus < MAX_BUSNO && pci_buses[bus]) {
        pci_buses[bus]->ops->read(pci_buses[bus], devfn, offset, 4, &res);
        if (res == invalid_value) {
            res = 0;
            SEP_DRV_LOG_REGISTER_OUT("Has read BDF(%x:%x:%x)[0x%x](4B): 0x%x (invalid value).", bus, device, function, offset, res);
        }
        else {
            SEP_DRV_LOG_REGISTER_OUT("Has read BDF(%x:%x:%x)[0x%x](4B): 0x%x.", bus, device, function, offset, res);
        }
    }
    else {
        SEP_DRV_LOG_REGISTER_OUT("Could not read BDF(%x:%x:%x)[0x%x]: bus not found!", bus, device, function, offset);
    }

    return res;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn extern U32 PCI_Read_U64(bus, device, function, offset)
 *
 * @param   bus        - target bus
 * @param   device     - target device
 * @param   function   - target function
 * @param   offset     - target register offset
 *
 * @return  Value at this location
 *
 * @brief   Reads a U64 from PCI configuration space
 *
 */
extern U64 PCI_Read_U64 (
    U32    bus,
    U32    device,
    U32    function,
    U32    offset
)
{
    U64 res   = 0;
    U32 devfn = (device << 3) | (function & 0x7);

    SEP_DRV_LOG_REGISTER_IN("Will read BDF(%x:%x:%x)[0x%x](8B)...", bus, device, function, offset);

    if (bus < MAX_BUSNO && pci_buses[bus]) {
        pci_buses[bus]->ops->read(pci_buses[bus], devfn, offset    , 4, (U32*)&res);
        pci_buses[bus]->ops->read(pci_buses[bus], devfn, offset + 4, 4, ((U32*)&res) + 1);
    }
    else {
        SEP_DRV_LOG_ERROR("Could not read BDF(%x:%x:%x)[0x%x]: bus not found!", bus, device, function, offset);
    }

    SEP_DRV_LOG_REGISTER_OUT("Has read BDF(%x:%x:%x)[0x%x](8B): 0x%llx.", bus, device, function, offset, res);
    return res;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn extern U32 PCI_Read_U64_Valid(bus, device, function, offset, invalid_value)
 *
 * @param   bus             - target bus
 * @param   device          - target device
 * @param   function        - target function
 * @param   offset          - target register offset
 * @param   invalid_value   - value against which to compare the PCI-obtained value
 *
 * @return  Value at this location (if value != invalid_value), 0 otherwise
 *
 * @brief   Reads a U64 from PCI configuration space
 *
 */
extern U64 PCI_Read_U64_Valid (
    U32    bus,
    U32    device,
    U32    function,
    U32    offset,
    U64    invalid_value
)
{
    U64 res   = 0;
    U32 devfn = (device << 3) | (function & 0x7);

    SEP_DRV_LOG_REGISTER_IN("Will read BDF(%x:%x:%x)[0x%x](8B)...", bus, device, function, offset);

    if (bus < MAX_BUSNO && pci_buses[bus]) {
        pci_buses[bus]->ops->read(pci_buses[bus], devfn, offset    , 4, (U32*)&res);
        pci_buses[bus]->ops->read(pci_buses[bus], devfn, offset + 4, 4, ((U32*)&res) + 1);

        if (res == invalid_value) {
            res = 0;
            SEP_DRV_LOG_REGISTER_OUT("Has read BDF(%x:%x:%x)[0x%x](8B): 0x%llx (invalid value).", bus, device, function, offset, res);
        }
        else {
            SEP_DRV_LOG_REGISTER_OUT("Has read BDF(%x:%x:%x)[0x%x](8B): 0x%llx.", bus, device, function, offset, res);
        }
    }
    else {
        SEP_DRV_LOG_REGISTER_OUT("Could not read BDF(%x:%x:%x)[0x%x]: bus not found!", bus, device, function, offset);
    }

    return res;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn extern U32 PCI_Write_U32(bus, device, function, offset, value)
 *
 * @param    bus            - target bus
 * @param    device         - target device
 * @param    function       - target function
 * @param    offset         - target register offset
 * @param    value          - value to write
 *
 * @return  0 in case of success, 1 otherwise
 *
 * @brief    Writes a U32 to PCI configuration space
 *
 */
extern U32 PCI_Write_U32 (
    U32    bus,
    U32    device,
    U32    function,
    U32    offset,
    U32    value
)
{
    U32 res   = 0;
    U32 devfn = (device << 3) | (function & 0x7);

    SEP_DRV_LOG_REGISTER_IN("Will write BDF(%x:%x:%x)[0x%x](4B): 0x%x...", bus, device, function, offset, value);

    if (bus < MAX_BUSNO && pci_buses[bus]) {
        pci_buses[bus]->ops->write(pci_buses[bus], devfn, offset, 4, value);
        SEP_DRV_LOG_REGISTER_OUT("Has written BDF(%x:%x:%x)[0x%x](4B): 0x%x.", bus, device, function, offset, value);
    }
    else {
        SEP_DRV_LOG_ERROR("Could not write BDF(%x:%x:%x)[0x%x]: bus not found!", bus, device, function, offset);
        res = 1;
        SEP_DRV_LOG_REGISTER_OUT("Failed to write BDF(%x:%x:%x)[0x%x](4B): 0x%x.", bus, device, function, offset, value);
    }

    return res;
}


/* ------------------------------------------------------------------------- */
/*!
 * @fn extern U32 PCI_Write_U64(bus, device, function, offset, value)
 *
 * @param    bus            - target bus
 * @param    device         - target device
 * @param    function       - target function
 * @param    offset         - target register offset
 * @param    value          - value to write
 *
 * @return  0 in case of success, 1 otherwise
 *
 * @brief    Writes a U64 to PCI configuration space
 *
 */
extern U32 PCI_Write_U64 (
    U32    bus,
    U32    device,
    U32    function,
    U32    offset,
    U64    value
)
{
    U32 res   = 0;
    U32 devfn = (device << 3) | (function & 0x7);

    SEP_DRV_LOG_REGISTER_IN("Will write BDF(%x:%x:%x)[0x%x](8B): 0x%llx...", bus, device, function, offset, value);

    if (bus < MAX_BUSNO && pci_buses[bus]) {
        pci_buses[bus]->ops->write(pci_buses[bus], devfn, offset    , 4, (U32)value);
        pci_buses[bus]->ops->write(pci_buses[bus], devfn, offset + 4, 4, (U32)(value>>32));
        SEP_DRV_LOG_REGISTER_OUT("Has written BDF(%x:%x:%x)[0x%x](8B): 0x%llx.", bus, device, function, offset, value);
    }
    else {
        SEP_DRV_LOG_ERROR("Could not write BDF(%x:%x:%x)[0x%x]: bus not found!", bus, device, function, offset);
        res = 1;
        SEP_DRV_LOG_REGISTER_OUT("Failed to write BDF(%x:%x:%x)[0x%x](8B): 0x%llx.", bus, device, function, offset, value);
    }

    return res;
}



/* ------------------------------------------------------------------------- */
/*!
 * @fn extern int PCI_Read_From_Memory_Address(addr, val)
 *
 * @param    addr    - physical address in mmio
 * @param   *value  - value at this address
 *
 * @return  status
 *
 * @brief   Read memory mapped i/o physical location
 *
 */
extern int
PCI_Read_From_Memory_Address (
    U32 addr,
    U32* val
)
{
    U32 aligned_addr, offset, value;
    PVOID base;

    SEP_DRV_LOG_TRACE_IN("Addr: %x, val_pointer: %p.", addr, val);

    if (addr <= 0) {
        SEP_DRV_LOG_ERROR_TRACE_OUT("OS_INVALID (addr <= 0!).");
        return OS_INVALID;
    }

    SEP_DRV_LOG_TRACE("Preparing to reading physical address: %x.", addr);
    offset       = addr & ~PAGE_MASK;
    aligned_addr = addr & PAGE_MASK;
    SEP_DRV_LOG_TRACE("Aligned physical address: %x, offset: %x.", aligned_addr, offset);

    base = ioremap_nocache(aligned_addr, PAGE_SIZE);
    if (base == NULL) {
        SEP_DRV_LOG_ERROR_TRACE_OUT("OS_INVALID (mapping failed!).");
        return OS_INVALID;
    }

    SEP_DRV_LOG_REGISTER_IN("Will read PCI address %u (mapped at %p).", addr, base + offset);
    value = readl(base+offset);
    SEP_DRV_LOG_REGISTER_OUT("Read PCI address %u (mapped at %p): %x.", addr, base + offset, value);

    *val = value;

    iounmap(base);

    SEP_DRV_LOG_TRACE_OUT("OS_SUCCESS.");
    return OS_SUCCESS;
}


/* ------------------------------------------------------------------------- */
/*!
 * @fn extern int PCI_Write_To_Memory_Address(addr, val)
 *
 * @param   addr   - physical address in mmio
 * @param   value  - value to be written
 *
 * @return  status
 *
 * @brief   Write to memory mapped i/o physical location
 *
 */
extern int
PCI_Write_To_Memory_Address (
    U32 addr,
    U32 val
)
{
    U32 aligned_addr, offset;
    PVOID base;

    SEP_DRV_LOG_TRACE_IN("Addr: %x, val: %x.", addr, val);

    if (addr <= 0) {
        SEP_DRV_LOG_ERROR_TRACE_OUT("OS_INVALID (addr <= 0!).");
        return OS_INVALID;
    }

    SEP_DRV_LOG_TRACE("Preparing to writing physical address: %x (val: %x).", addr, val);
    offset       = addr & ~PAGE_MASK;
    aligned_addr = addr & PAGE_MASK;
    SEP_DRV_LOG_TRACE("Aligned physical address: %x, offset: %x (val: %x).", aligned_addr, offset, val);

    base = ioremap_nocache(aligned_addr, PAGE_SIZE);
    if (base == NULL) {
        SEP_DRV_LOG_ERROR_TRACE_OUT("OS_INVALID (mapping failed!).");
        return OS_INVALID;
    }

    SEP_DRV_LOG_REGISTER_IN("Will write PCI address %u (mapped at %p): %x.", addr, base + offset, val);
    writel(val,base+offset);
    SEP_DRV_LOG_REGISTER_OUT("Wrote PCI address %u (mapped at %p): %x.", addr, base + offset, val);

    iounmap(base);

    SEP_DRV_LOG_TRACE_OUT("OS_SUCCESS.");
    return OS_SUCCESS;
}


/* ------------------------------------------------------------------------- */
/*!
 * @fn extern U32 PCI_Map_Memory(SEP_MMIO_NODE *node, U64 phy_address, U64 map_size)
 *
 * @param    node        - MAP NODE to use
 * @param    phy_address - Address to be mapped
 * @param    map_size    - Amount of memory to map (has to be a multiple of 4k)
 *
 * @return   OS_SUCCESS or OS_INVALID
 *
 * @brief    Maps a physical address to a virtual address
 *
 */
extern OS_STATUS
PCI_Map_Memory (
    SEP_MMIO_NODE *node,
    U64            phy_address,
    U32            map_size
)
{
    U8  *res;

    SEP_DRV_LOG_INIT_IN("Node: %p, phy_address: %llx, map_size: %u.", node, phy_address, map_size);

    if (!node        ||
        !phy_address ||
        !map_size) {
        SEP_DRV_LOG_ERROR_INIT_OUT("Invalid parameters, aborting!");
        return OS_INVALID;
    }

    res = ioremap_nocache(phy_address, map_size);
    if (!res) {
        SEP_DRV_LOG_ERROR_INIT_OUT("Map operation failed!");
        return OS_INVALID;
    }

    SEP_MMIO_NODE_physical_address(node) = (UIOP)phy_address;
    SEP_MMIO_NODE_virtual_address(node)  = (UIOP)res;
    SEP_MMIO_NODE_map_token(node)        = (UIOP)res;
    SEP_MMIO_NODE_size(node)             = map_size;

    SEP_DRV_LOG_INIT_OUT("Addr:0x%llx->0x%llx,tok:0x%llx,sz:%u.",
        SEP_MMIO_NODE_physical_address(node),
        SEP_MMIO_NODE_virtual_address(node),
        SEP_MMIO_NODE_map_token(node),
        SEP_MMIO_NODE_size(node));
    return OS_SUCCESS;
}


/* ------------------------------------------------------------------------- */
/*!
 * @fn extern void PCI_Unmap_Memory(SEP_MMIO_NODE *node)
 *
 * @param   node - memory map node to clean up
 *
 * @return  none
 *
 * @brief   Unmaps previously mapped memory
 *
 */
extern void
PCI_Unmap_Memory (
    SEP_MMIO_NODE *node
)
{
    SEP_DRV_LOG_INIT_IN("Unmapping node %p.", node);

    if (node) {
        if (SEP_MMIO_NODE_size(node)) {
            SEP_DRV_LOG_TRACE("Unmapping token 0x%llx (0x%llx->0x%llx)[%uB].",
                SEP_MMIO_NODE_map_token(node),
                SEP_MMIO_NODE_physical_address(node),
                SEP_MMIO_NODE_virtual_address(node),
                SEP_MMIO_NODE_size(node));
            iounmap((void*)(UIOP)SEP_MMIO_NODE_map_token(node));
            SEP_MMIO_NODE_size(node)             = 0;
            SEP_MMIO_NODE_map_token(node)        = 0;
            SEP_MMIO_NODE_virtual_address(node)  = 0;
            SEP_MMIO_NODE_physical_address(node) = 0;
        }
        else {
            SEP_DRV_LOG_TRACE("Already unmapped.");
        }
    }

    SEP_DRV_LOG_INIT_OUT("Unmapped node %p.", node);
    return;
}



/* ------------------------------------------------------------------------- */
/*!
 * @fn U32 PCI_MMIO_Read_U32(virtual_address_base, offset)
 *
 * @param   virtual_address_base   - Virtual address base
 * @param   offset                 - Register offset
 *
 * @return  U32 read from an MMIO register
 *
 * @brief   Reads U32 value from MMIO
 *
 */
extern U32
PCI_MMIO_Read_U32 (
    U64   virtual_address_base,
    U32   offset
)
{
    U32  temp_u32 = 0LL;
    U32 *computed_address;

    computed_address = (U32*)(((char*)(UIOP)virtual_address_base) + offset);

    SEP_DRV_LOG_REGISTER_IN("Will read U32(0x%llx + 0x%x = 0x%p).", virtual_address_base, offset, computed_address);

    if (!virtual_address_base) {
        SEP_DRV_LOG_ERROR("Invalid base for U32(0x%llx + 0x%x = 0x%p)!", virtual_address_base, offset, computed_address);
        temp_u32 = 0;
    }
    else {
        temp_u32 = *computed_address;
    }

    SEP_DRV_LOG_REGISTER_OUT("Has read U32(0x%llx + 0x%x): 0x%x.", virtual_address_base, offset, temp_u32);
    return temp_u32;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn U64 PCI_MMIO_Read_U64(virtual_address_base, offset)
 *
 * @param   virtual_address_base   - Virtual address base
 * @param   offset                 - Register offset
 *
 * @return  U64 read from an MMIO register
 *
 * @brief   Reads U64 value from MMIO
 *
 */
extern U64
PCI_MMIO_Read_U64 (
    U64   virtual_address_base,
    U32   offset
)
{
    U64  temp_u64 = 0LL;
    U64 *computed_address;

    computed_address = (U64*)(((char*)(UIOP)virtual_address_base) + offset);

    SEP_DRV_LOG_REGISTER_IN("Will read U64(0x%llx + 0x%x = 0x%p).", virtual_address_base, offset, computed_address);

    if (!virtual_address_base) {
        SEP_DRV_LOG_ERROR("Invalid base for U32(0x%llx + 0x%x = 0x%p)!", virtual_address_base, offset, computed_address);
        temp_u64 = 0;
    }
    else {
        temp_u64 = *computed_address;
    }

    SEP_DRV_LOG_REGISTER_OUT("Has read U64(0x%llx + 0x%x): 0x%llx.", virtual_address_base, offset, temp_u64);
    return temp_u64;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn void PCI_MMIO_Write_U32(virtual_address_base, offset, value)
 *
 * @param   virtual_address_base   - Virtual address base
 * @param   offset                 - Register offset
 * @param   value                  - Value to write
 *
 * @return  U32 write into an MMIO register
 *
 * @brief   Writes U32 value to MMIO
 *
 */
extern void
PCI_MMIO_Write_U32 (
    U64   virtual_address_base,
    U32   offset,
    U32   value
)
{
    U32 *computed_address;

    computed_address = (U32*)(((char*)(UIOP)virtual_address_base) + offset);

    SEP_DRV_LOG_REGISTER_IN("Writing 0x%x to U32(0x%llx + 0x%x = 0x%p).", value, virtual_address_base, offset, computed_address);

    if (!virtual_address_base) {
        SEP_DRV_LOG_ERROR("Invalid base for U32(0x%llx + 0x%x = 0x%p)!", virtual_address_base, offset, computed_address);
    }
    else {
        *computed_address = value;
    }

    SEP_DRV_LOG_REGISTER_OUT("Has written 0x%x to U32(0x%llx + 0x%x).", value, virtual_address_base, offset);
    return;
}

/* ------------------------------------------------------------------------- */
/*!
 * @fn void PCI_MMIO_Write_U64(virtual_address_base, offset, value)
 *
 * @param   virtual_address_base   - Virtual address base
 * @param   offset                 - Register offset
 * @param   value                  - Value to write
 *
 * @return  U64 write into an MMIO register
 *
 * @brief   Writes U64 value to MMIO
 *
 */
extern void
PCI_MMIO_Write_U64 (
    U64   virtual_address_base,
    U32   offset,
    U64   value
)
{
    U64 *computed_address;

    computed_address = (U64*)(((char*)(UIOP)virtual_address_base) + offset);

    SEP_DRV_LOG_REGISTER_IN("Writing 0x%llx to U64(0x%llx + 0x%x = 0x%p).", value, virtual_address_base, offset, computed_address);

    if (!virtual_address_base) {
        SEP_DRV_LOG_ERROR("Invalid base for U32(0x%llx + 0x%x = 0x%p)!", virtual_address_base, offset, computed_address);
    }
    else {
        *computed_address = value;
    }

    SEP_DRV_LOG_REGISTER_OUT("Has written 0x%llx to U64(0x%llx + 0x%x).", value, virtual_address_base, offset);
    return;
}


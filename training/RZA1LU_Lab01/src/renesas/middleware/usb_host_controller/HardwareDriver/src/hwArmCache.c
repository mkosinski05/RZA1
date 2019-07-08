/***********************************************************************************
 FILE NAME       hwArmCache.c

 DESCRIPTION     GNU sample cache control functions

 CPU Type        GNU
 Copyright   : 2014 Renesas Technology Europe Ltd.
 Copyright   : 2014 Renesas Technology Corporation.
 All Rights Reserved
 ***********************************************************************************/

/***********************************************************************************
 Revision History
 DD.MM.YYYY OSO-UID Description
 DD.MM.YYYY RTE-MAB First Release
 ***********************************************************************************/

/***********************************************************************************
 System Includes
 ***********************************************************************************/

/***********************************************************************************
 User Includes
 ***********************************************************************************/

#include "hwArmCache.h"
#include "cache-l2x0.h"
/***********************************************************************************
 Defines
 ***********************************************************************************/

#define CACHE_LINE_SIZE        32

/***********************************************************************************
 Constant Macros
 ***********************************************************************************/

/***********************************************************************************
 Function Macros
 ***********************************************************************************/

/***********************************************************************************
 Enumerated Types
 ***********************************************************************************/

/***********************************************************************************
 Typedefs
 ***********************************************************************************/

typedef struct st_v2p_map
{
    /* Start address of the virtual memory */
    size_t stVirtualStart;
    /* End address of the virtual memory */
    size_t stVirtualEnd;
    /* The physical memory address to which the virtual memory is mapped */
    size_t stPhysical;
    /* The address mask used for this section */
    size_t stAddressMask;
    /* Flag to show if the address uses the L2 cache */
    bool_t bfL2Cache;
} v2pmap_t;

/***********************************************************************************
 Constant Data
 ***********************************************************************************/

static const v2pmap_t gp_v2p_mapping[] =
{
         /* Internal RAM */
        {
             0x20900000, 0x209FFFFF, 0x60900000, 0x000FFFFF, false,
        },
        {
            0x20400000, 0x204FFFFF, 0x60800000, 0x000FFFFF, false,
        },
        {
            0x20300000, 0x203FFFFF, 0x60700000, 0x000FFFFF, false,
        },
        {
             0x20800000, 0x208FFFFF, 0x60600000, 0x000FFFFF, false,
        },
        {
            0x20700000, 0x207FFFFF, 0x60500000, 0x000FFFFF, false,
        },
        {
            0x20200000, 0x202FFFFF, 0x60400000, 0x000FFFFF, false,
        },
        {
            0x20100000, 0x201FFFFF, 0x60300000, 0x000FFFFF, false,
        },
        {
            0x20600000, 0x206FFFFF, 0x60200000, 0x000FFFFF, false,
        },
        {
            0x20500000, 0x205FFFFF, 0x60100000, 0x000FFFFF, false,
        },
        {
             0x20000000, 0x200FFFFF, 0x60000000, 0x000FFFFF, false,
        },

        /* SDRAM */
        {
            0x08000000, 0x0BFFFFFF, 0x48000000, 0x0FFFFFFF, true,
        }
};
/***********************************************************************************
 Function Prototypes
 ***********************************************************************************/

static v2pmap_t *get_section_mapping (void *pvAddress, size_t stLength, void **ppvMappedAddress,
        size_t *pstSegmentLength);
static v2pmap_t *get_address_translation_table_entry (void *pvAddress);
static void *translate_to_physical_address (void *pvAddress, v2pmap_t *pV2PMap);
static size_t get_length_to_end_of_section (void *pvAddress, v2pmap_t *pV2PMap);

/***********************************************************************************
 External Function Prototypes
 ***********************************************************************************/

extern void v7_dma_inv_range (void *pvStart, void *pvEnd);
extern void v7_dma_clean_range (void *pvStart, void *pvEnd);

/***********************************************************************************
 External Variables
 ***********************************************************************************/

/***********************************************************************************
 Global Variables
 ***********************************************************************************/

/***********************************************************************************
 Public Functions
 ***********************************************************************************/

/**********************************************************************************
 Function Name: cacheInvalidate
 Description:   Function to invalidate L1 and L2 cache over the desired memory area.
 If the memory area crosses a virtual to physical memory boundary
 then the function should be called again with the updated virtual
 address and length. In this case the function returns true.
 Parameters:    OUT ppvPhysicalAddress - Pointer to the physical memory address
 OUT pstSegmentLength - Pointer to the length of the segment invalidated
 IN  pvVirtualAddress - The start of the memory to be invalidated
 IN  stLength - The length of memory to be invalidated
 Return value:  true if a mapping boundary is crossed
 **********************************************************************************/
bool_t cacheInvalidate (void **ppvPhysicalAddress, size_t *pstSegmentLength, void *pvVirtualAddress, size_t stLength)
{
    v2pmap_t *pV2PMap;
    /* 1. Translate the virtual address to a physical address */
    void *pvPhysicalAddress = pvVirtualAddress;
    /* 2. Get the length of the memory until the next physical or virtual boundary */
    size_t stSegmentLength = stLength;
    bool_t bfResult = false;
    /* Look up the virtual to physical mapping */
    pV2PMap = get_section_mapping(pvVirtualAddress, stLength, &pvPhysicalAddress, &stSegmentLength);
    /* Check to see if this request crosses a page boundary */
    if (stSegmentLength != stLength)
    {
        /* A mapping boundary has been crossed */
        bfResult = true;
    }
    /* 3. Invalidate the cache over the length of memory */
    if (pV2PMap)
    {
        /* Flush:
         To mark a cache line as being not valid by clearing the valid bit. This must be done
         whenever the line does not contain a valid cache entry. For example, after a cache flush
         all lines are invalid. */
        v7_dma_inv_range(pvVirtualAddress, (void*) ((uint8_t*) pvVirtualAddress + stSegmentLength + CACHE_LINE_SIZE));
        if (pV2PMap->bfL2Cache)
        {
            l2x0_flush_range((unsigned long) pvVirtualAddress,
                    (unsigned long) ((uint8_t*) pvVirtualAddress + stSegmentLength + CACHE_LINE_SIZE));
        }
        __asm__ volatile ("dsb");
    }
    else
    {
        /* Do Nothing */
    }
    /* Check to see if the caller requires the physical address */
    if (ppvPhysicalAddress)
    {
        *ppvPhysicalAddress = pvPhysicalAddress;
    }
    /* Check to see if the caller requires the segment length */
    if (pstSegmentLength)
    {
        *pstSegmentLength = stSegmentLength;
    }
    /* Return the result */
    return bfResult;
}
/**********************************************************************************
 End of function  cacheInvalidate
 ***********************************************************************************/

/**********************************************************************************
 Function Name: cacheClear
 Description:   Function to write back L1 and L2 cache over the desired memory area.
 If the memory area crosses a virtual to physical memory boundary
 then the function should be called again with the updated virtual
 address and length. In this case the function returns true.
 Parameters:    OUT ppvPhysicalAddress - Pointer to the physical memory address
 OUT pstSegmentLength - Pointer to the length of the segment cleared
 IN  pvVirtualAddress - The start of the memory to be cleared
 IN  stLength - The length of memory to be cleared
 Return value:  true if a mapping boundary is crossed
 **********************************************************************************/
bool_t cacheClear (void **ppvPhysicalAddress, size_t *pstSegmentLength, void *pvVirtualAddress, size_t stLength)
{
    v2pmap_t *pV2PMap;
    /* 1. Translate the virtual address to a phical address */
    void *pvPhysicalAddress = pvVirtualAddress;
    /* 2. Get the length of the memory until the next physical or virtual boundary */
    size_t stSegmentLength = stLength;
    bool_t bfResult = false;
    /* Look up the virtual to physical mapping */
    pV2PMap = get_section_mapping(pvVirtualAddress, stLength, &pvPhysicalAddress, &stSegmentLength);
    /* Check to see if this request crosses a page boundary */
    if (stSegmentLength != stLength)
    {
        /* A mapping boundary has been crossed */
        bfResult = true;
    }
    /* 3. Clean the cache over the length of memory */
    if (pV2PMap)
    {
        /* Clean:
         A cache line that has not been modified while it is in the cache is said to be clean. To
         clean a cache is to write dirty cache entries into main memory. If a cache line is clean,
         it is not written on a cache miss because the next level of memory contains the same
         data as the cache. */

        v7_dma_clean_range(pvVirtualAddress, (void*) ((uint8_t*) pvVirtualAddress + stSegmentLength + CACHE_LINE_SIZE));
        if (pV2PMap->bfL2Cache)
        {
            l2x0_clean_range((unsigned long) pvVirtualAddress,
                    (unsigned long) ((uint8_t*) pvVirtualAddress + stSegmentLength + CACHE_LINE_SIZE));
        }
        __asm__ volatile ("dsb");
    }
    else
    {
        // No mapping - therefore no cache control
    }
    /* Check to see if the caller requires the physical address */
    if (ppvPhysicalAddress)
    {
        *ppvPhysicalAddress = pvPhysicalAddress;
    }
    /* Check to see if the caller requires the segment length */
    if (pstSegmentLength)
    {
        *pstSegmentLength = stSegmentLength;
    }
    /* Return the result */
    return bfResult;
}
/**********************************************************************************
 End of function  cacheClear
 ***********************************************************************************/

/***********************************************************************************
 Private Functions
 ***********************************************************************************/

/**********************************************************************************
 Function Name: get_section_mapping
 Description:   Function to get the section mapping for a virtual address. If the
 section crosses a page boundary then the length returned will be
 adjusted to the end of the segment
 Parameters:    IN  pvAddress - The address to get the mapping for
 IN  stLength - The length of the section
 OUT ppvMappedAddress - Pointer to the mapped address
 OUT pstSegmentLength - The length of the segment
 Return value:  Pointer to the entry in the address mapping table if found
 **********************************************************************************/
v2pmap_t *get_section_mapping (void *pvAddress, size_t stLength, void **ppvMappedAddress, size_t *pstSegmentLength)
{
    void *pvEndAddress = (void*) ((uint8_t*) pvAddress + (stLength - 1U));
    v2pmap_t *pV2PStart = get_address_translation_table_entry(pvAddress);
    v2pmap_t *pV2pEnd = get_address_translation_table_entry(pvEndAddress);
    /* Check for translation of the address */
    if (ppvMappedAddress)
    {
        /* If a mapping was found */
        if (pV2PStart)
        {
            /* Translate the virtual address to the physical address */
            *ppvMappedAddress = translate_to_physical_address(pvAddress, pV2PStart);
        }
        else
        {
            /* No address translation was found so return the address unchanged */
            *ppvMappedAddress = pvAddress;
        }
    }
    /* Check for segment length translation */
    if (pstSegmentLength)
    {
        /* If a mapping was found */
        if (pV2PStart)
        {
            /* If it exists in one section */
            if (pV2PStart == pV2pEnd)
            {
                /* Return the length unchanged */
                *pstSegmentLength = stLength;
            }
            else
            {
                /* Return the length to the end of the section */
                *pstSegmentLength = get_length_to_end_of_section(pvAddress, pV2PStart);
            }
        }
        else
        {
            /* No address translation was found so return the length unchanged */
            *pstSegmentLength = stLength;
        }
    }
    return pV2PStart;
}
/**********************************************************************************
 End of function  get_section_mapping
 ***********************************************************************************/

/**********************************************************************************
 Function Name: get_address_translation_table_entry
 Description:   Function to get the address translation table entry for a given address
 Parameters:    IN  pvAddress - The address to find
 Return value:  Pointer to the table entry or NULL if not found
 **********************************************************************************/
static v2pmap_t *get_address_translation_table_entry (void *pvAddress)
{
    v2pmap_t *pV2PMap = NULL;
    int32_t iIndex = sizeof(gp_v2p_mapping) / sizeof(v2pmap_t);
    /* Look for the address within the table */
    while (iIndex--)
    {
        if (((size_t) pvAddress >= gp_v2p_mapping[iIndex].stVirtualStart)
                && ((size_t) pvAddress <= gp_v2p_mapping[iIndex].stVirtualEnd))
        {
            pV2PMap = (v2pmap_t*) &gp_v2p_mapping[iIndex];
            break;
        }
    }
    return pV2PMap;
}
/**********************************************************************************
 End of function  get_address_translation_table_entry
 ***********************************************************************************/

/**********************************************************************************
 Function Name: translate_to_physical_address
 Description:   Function to translate the virtual address into the physical one
 Parameters:    IN  pvAddress - The address to translate
 IN  pV2PMap - Pointer to the mapping information
 Return value:  The translated address
 **********************************************************************************/
static void *translate_to_physical_address (void *pvAddress, v2pmap_t *pV2PMap)
{
    /* Mask the upper order address bits */
    size_t stAddress = (size_t) pvAddress & pV2PMap->stAddressMask;
    /* Combine with the upper order address bits of the physical address */
    stAddress |= pV2PMap->stPhysical;
    /* Return the physical address */
    return (void*) stAddress;
}
/**********************************************************************************
 End of function  translate_to_physical_address
 ***********************************************************************************/

/**********************************************************************************
 Function Name: get_length_to_end_of_section
 Description:   Function to truncate the length to the end of the mapped section
 Parameters:    IN  pvAddress - The address
 IN  pV2PMap - Pointer to the mapping information
 Return value:  Length to the end of the section
 **********************************************************************************/
static size_t get_length_to_end_of_section (void *pvAddress, v2pmap_t *pV2PMap)
{
    size_t stResult = (pV2PMap->stVirtualEnd + 1UL) - (size_t) pvAddress;
    return stResult;
}
/**********************************************************************************
 End of function  get_length_to_end_of_section
 ***********************************************************************************/

#ifdef _ADDRESS_TRANSLATION_TEST_

/***********************************************************************************
 Constant Data
 ***********************************************************************************/

const struct st_test_data
{
    size_t stTestAddress;
    size_t stTestLength;
    size_t stResultAddress;
    size_t stResultLength;
}
gpc_test_vectors[] =
{
    0x20000000, 0x00100000, 0x60000000, 0x00100000,
    0x200FFFFF, 0x00000001, 0x600FFFFF, 0x00000001,
    0x200FFFF0, 0x00000020, 0x600FFFF0, 0x00000010,
    0x20000000, 0x00200000, 0x60000000, 0x00100000,
    0x20100000, 0x00100000, 0x60300000, 0x00100000,
    0x201FFFFF, 0x00000001, 0x603FFFFF, 0x00000001,
    0x201FFFF0, 0x00000020, 0x603FFFF0, 0x00000010,
    0x20100000, 0x00200000, 0x60300000, 0x00100000,
    0x20200000, 0x00100000, 0x60400000, 0x00100000,
    0x202FFFFF, 0x00000001, 0x604FFFFF, 0x00000001,
    0x202FFFF0, 0x00000020, 0x604FFFF0, 0x00000010,
    0x20200000, 0x00200000, 0x60400000, 0x00100000,
    0x20300000, 0x00100000, 0x60700000, 0x00100000,
    0x203FFFFF, 0x00000001, 0x607FFFFF, 0x00000001,
    0x203FFFF0, 0x00000020, 0x607FFFF0, 0x00000010,
    0x20300000, 0x00200000, 0x60700000, 0x00100000,
    0x20400000, 0x00100000, 0x60800000, 0x00100000,
    0x204FFFFF, 0x00000001, 0x608FFFFF, 0x00000001,
    0x204FFFF0, 0x00000020, 0x608FFFF0, 0x00000010,
    0x20400000, 0x00200000, 0x60800000, 0x00100000,
    0x20500000, 0x00100000, 0x60100000, 0x00100000,
    0x205FFFFF, 0x00000001, 0x601FFFFF, 0x00000001,
    0x205FFFF0, 0x00000020, 0x601FFFF0, 0x00000010,
    0x20500000, 0x00200000, 0x60100000, 0x00100000,
    0x20600000, 0x00100000, 0x60200000, 0x00100000,
    0x206FFFFF, 0x00000001, 0x602FFFFF, 0x00000001,
    0x206FFFF0, 0x00000020, 0x602FFFF0, 0x00000010,
    0x20600000, 0x00200000, 0x60200000, 0x00100000,
    0x20700000, 0x00100000, 0x60500000, 0x00100000,
    0x207FFFFF, 0x00000001, 0x605FFFFF, 0x00000001,
    0x207FFFF0, 0x00000020, 0x605FFFF0, 0x00000010,
    0x20700000, 0x00200000, 0x60500000, 0x00100000,
    0x20800000, 0x00100000, 0x60600000, 0x00100000,
    0x208FFFFF, 0x00000001, 0x606FFFFF, 0x00000001,
    0x208FFFF0, 0x00000020, 0x606FFFF0, 0x00000010,
    0x20800000, 0x00200000, 0x60600000, 0x00100000,
    0x20900000, 0x00100000, 0x60900000, 0x00100000,
    0x209FFFFF, 0x00000001, 0x609FFFFF, 0x00000001,
    0x209FFFF0, 0x00000020, 0x609FFFF0, 0x00000010,
    0x20900000, 0x00200000, 0x60900000, 0x00100000,
    0x08000000, 0x02000000, 0x48000000, 0x02000000,
    0x09FFFFFF, 0x00000001, 0x49FFFFFF, 0x00000001,
    0x0B000000, 0x00000020, 0x4B000000, 0x00000020,
    0x0BFFFFFF, 0x02000000, 0x4BFFFFFF, 0x00000001,
    0x60000000, 0x00100000, 0x60000000, 0x00100000,
    0x60100000, 0x00100000, 0x60100000, 0x00100000,
    0x60200000, 0x00100000, 0x60200000, 0x00100000,
    0x60300000, 0x00100000, 0x60300000, 0x00100000,
    0x60400000, 0x00100000, 0x60400000, 0x00100000,
    0x60500000, 0x00100000, 0x60500000, 0x00100000,
    0x60600000, 0x00100000, 0x60600000, 0x00100000,
    0x60700000, 0x00100000, 0x60700000, 0x00100000,
    0x60800000, 0x00100000, 0x60800000, 0x00100000,
    0x60900000, 0x00100000, 0x60900000, 0x00100000
};

/**********************************************************************************
 Function Name: testAddressTranslation
 Description:   Function to test the address translation functions
 Parameters:    IN  pFile - Pointer to a file stream to print the results to
 Return value:  none
 **********************************************************************************/
void testAddressTranslation(FILE *pFile)
{
    int32_t iIndex = sizeof(gpc_test_vectors) / sizeof(struct st_test_data);
    int32_t iErrorCount = 0;
    while (iIndex--)
    {
        void *pvMappedAddress;
        size_t stSegmentLength;
        if (!get_section_mapping((void*)gpc_test_vectors[iIndex].stTestAddress,
                        gpc_test_vectors[iIndex].stTestLength,
                        &pvMappedAddress,
                        &stSegmentLength))
        {
            /* No translation required as the area passed is already a physical address */
            pvMappedAddress = (void*)gpc_test_vectors[iIndex].stTestAddress;
            stSegmentLength = gpc_test_vectors[iIndex].stTestLength;
        }
        fprintf(pFile, "Address 0x%p Length %lu -> 0x%p , %lu ",
                gpc_test_vectors[iIndex].stTestAddress,
                gpc_test_vectors[iIndex].stTestLength,
                pvMappedAddress, stSegmentLength);
        if (pvMappedAddress == gpc_test_vectors[iIndex].stResultAddress)
        {
            fprintf(pFile, "Address OK ");
        }
        else
        {
            fprintf(pFile, "Address NG ");
            iErrorCount++;
        }
        if (stSegmentLength == gpc_test_vectors[iIndex].stResultLength)
        {
            fprintf(pFile, "Length OK\r\n");
        }
        else
        {
            fprintf(pFile, "Length NG\r\n");
            iErrorCount++;
        }
    }
    fprintf(pFile, "Error count = %d\r\n", iErrorCount);
}
/**********************************************************************************
 End of function  testAddressTranslation
 ***********************************************************************************/
#endif
/***********************************************************************************
 End  Of File
 ***********************************************************************************/

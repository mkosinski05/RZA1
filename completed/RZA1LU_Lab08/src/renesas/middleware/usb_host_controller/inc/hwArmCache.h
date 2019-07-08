/***********************************************************************************
FILE NAME       hwArmCache.h

DESCRIPTION     GNU sample cache control functions

CPU Type        GNU
Copyright   : 2014 Renesas Technology Europe Ltd.
Copyright   : 2014 Renesas Technology Corporation.
                    All Rights Reserved
***********************************************************************************/

#ifndef HWARMCACHE_H_INCLUDED
#define HWARMCACHE_H_INCLUDED

/***********************************************************************************
Revision History
DD.MM.YYYY OSO-UID Description
DD.MM.YYYY RTE-MAB First Release
***********************************************************************************/

/***********************************************************************************
System Includes
***********************************************************************************/

#include <stdio.h>

/***********************************************************************************
User Includes
***********************************************************************************/

#include "r_typedefs.h"

/***********************************************************************************
Defines
***********************************************************************************/

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

/***********************************************************************************
Constant Data
***********************************************************************************/

/***********************************************************************************
Function Prototypes
***********************************************************************************/

/***********************************************************************************
External Variables
***********************************************************************************/

/***********************************************************************************
Global Variables
***********************************************************************************/

/***********************************************************************************
Public Functions
***********************************************************************************/

/***********************************************************************************
Private Functions
***********************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

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
extern  bool_t cacheInvalidate(void      **ppvPhysicalAddress,
                               size_t    *pstSegmentLength,
                               void      *pvVirtualAddress,
                               size_t    stLength);

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
extern  bool_t cacheClear(void      **ppvPhysicalAddress,
                          size_t    *pstSegmentLength,
                          void      *pvVirtualAddress,
                          size_t    stLength);

#ifdef _ADDRESS_TRANSLATION_TEST_
/**********************************************************************************
Function Name: testAddressTranslation
Description:   Function to test the address translation functions
Parameters:    IN  pFile - Pointer to a file stream to print the results to
Return value:  none
**********************************************************************************/
extern  void testAddressTranslation(FILE *pFile);
#endif

#ifdef __cplusplus
}
#endif

#endif                              /* HWARMCACHE_H_INCLUDED */

/**********************************************************************************
End  Of File
***********************************************************************************/

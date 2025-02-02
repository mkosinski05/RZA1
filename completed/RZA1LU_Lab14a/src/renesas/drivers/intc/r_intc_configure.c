/*******************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only
* intended for use with Renesas products. No other uses are authorized. This
* software is owned by Renesas Electronics Corporation and is protected under
* all applicable laws, including copyright laws.
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
* THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT
* LIMITED TO WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
* AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED.
* TO THE MAXIMUM EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS
* ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES SHALL BE LIABLE
* FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR
* ANY REASON RELATED TO THIS SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE
* BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software
* and to discontinue the availability of this software. By using this software,
* you agree to the additional terms and conditions found by accessing the
* following link:
* http://www.renesas.com/disclaimer
*
* Copyright (C) 2016 Renesas Electronics Corporation. All rights reserved.
*******************************************************************************/
/*******************************************************************************
* File Name     : intc.c
* Device(s)     : RZ/A1L
* Tool-Chain    : GNUARM-NONE-EABI-v16.01
* H/W Platform  : Platform Independent
* Description   : Sample Program - Interrupt process
*******************************************************************************/
/*******************************************************************************
* History       : DD.MM.YYYY Version Description
*               : 06.07.2016 1.00    Ported from RZA1H
*******************************************************************************/

/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/
/* Default  type definition header */
#include "r_typedefs.h"

/* Device driver header */
#include "dev_drv.h"

/* I/O Register root header */
#include "iodefine_cfg.h"

/* INTC Driver Header */
#include "compiler_settings.h"
#include "r_intc.h"


/******************************************************************************
Macro definitions
******************************************************************************/
/* ==== Total number of registers ==== */
#define INTC_ICDISR_REG_TOTAL   (((uint16_t)INTC_ID_TOTAL / 32) + 1)
#define INTC_ICDICFR_REG_TOTAL  (((uint16_t)INTC_ID_TOTAL / 16) + 1)
#define INTC_ICDIPR_REG_TOTAL   (((uint16_t)INTC_ID_TOTAL /  4) + 1)
#define INTC_ICDIPTR_REG_TOTAL  (((uint16_t)INTC_ID_TOTAL /  4) + 1)
#define INTC_ICDISER_REG_TOTAL  (((uint16_t)INTC_ID_TOTAL / 32) + 1)
#define INTC_ICDICER_REG_TOTAL  (((uint16_t)INTC_ID_TOTAL / 32) + 1)

/******************************************************************************
Private global variables and functions
******************************************************************************/
/* Initial value table of Interrupt Configuration Registers */
static uint32_t intc_icdicfrn_table[] =
{
                           /*           Interrupt ID */
    0xAAAAAAAA,            /* ICDICFR0  :  15 to   0 */
    0x00000055,            /* ICDICFR1  :  19 to  16 */
    0xFFFD5555,            /* ICDICFR2  :  47 to  32 */
    0x555FFFFF,            /* ICDICFR3  :  63 to  48 */
    0x55555555,            /* ICDICFR4  :  79 to  64 */
    0x55555555,            /* ICDICFR5  :  95 to  80 */
    0x55555555,            /* ICDICFR6  : 111 to  96 */
    0x55555555,            /* ICDICFR7  : 127 to 112 */
    0x5555F555,            /* ICDICFR8  : 143 to 128 */
    0x55555555,            /* ICDICFR9  : 159 to 144 */
    0x55555555,            /* ICDICFR10 : 175 to 160 */
    0xF5555555,            /* ICDICFR11 : 191 to 176 */
    0xF555F555,            /* ICDICFR12 : 207 to 192 */
    0x5555F555,            /* ICDICFR13 : 223 to 208 */
    0x55555555,            /* ICDICFR14 : 239 to 224 */
    0x55555555,            /* ICDICFR15 : 255 to 240 */
    0x55555555,            /* ICDICFR16 : 271 to 256 */
    0xFD555555,            /* ICDICFR17 : 287 to 272 */
    0x55555557,            /* ICDICFR18 : 303 to 288 */
    0x55555555,            /* ICDICFR19 : 319 to 304 */
    0x55555555,            /* ICDICFR20 : 335 to 320 */
    0x5F555555,            /* ICDICFR21 : 351 to 336 */
    0xFD55555F,            /* ICDICFR22 : 367 to 352 */
    0x55555557,            /* ICDICFR23 : 383 to 368 */
    0x55555555,            /* ICDICFR24 : 399 to 384 */
    0x55555555,            /* ICDICFR25 : 415 to 400 */
    0x55555555,            /* ICDICFR26 : 431 to 416 */
    0x55555555,            /* ICDICFR27 : 447 to 432 */
    0x55555555,            /* ICDICFR28 : 463 to 448 */
    0x55555555,            /* ICDICFR29 : 479 to 464 */
    0x55555555,            /* ICDICFR30 : 495 to 480 */
    0x55555555,            /* ICDICFR31 : 511 to 496 */
    0x55555555,            /* ICDICFR32 : 527 to 512 */
    0x55555555,            /* ICDICFR33 : 543 to 528 */
    0x55555555,            /* ICDICFR34 : 559 to 544 */
    0x55555555,            /* ICDICFR35 : 575 to 560 */
    0x00155555             /* ICDICFR36 : 586 to 576 */
};


/******************************************************************************
* Function Name: R_INTC_RegistIntFunc
* Description  : Registers the function specified by the func to the element 
*              : specified by the int_id in the INTC interrupt handler function
*              : table.
* Arguments    : uint16_t int_id         : Interrupt ID
*              : void (* func)(uint32_t) : Function to be registered to INTC
*              :                         : interrupt hander table
* Return Value : DEVDRV_SUCCESS          : Success of registration of INTC 
*              :                         : interrupt handler function
*              : DEVDRV_ERROR            : Failure of registration of INTC 
*              :                         : interrupt handler function
******************************************************************************/
int32_t R_INTC_RegistIntFunc (uint16_t int_id, void (* func)(uint32_t int_sense))
{
    /* ==== Argument check ==== */
    if (int_id >= INTC_ID_TOTAL)
    {
        /* Argument error */
        return DEVDRV_ERROR;
    }

    userdef_intc_regist_int_func(int_id, func);

    return DEVDRV_SUCCESS;
}
/*******************************************************************************
 End of function R_INTC_RegistIntFunc
 *******************************************************************************/


/******************************************************************************
* Function Name: R_INTC_UnRegistIntFunc
* Description  : UnRegisters the function specified by the func to the element
*              : specified by the int_id in the INTC interrupt handler function
*              : table.
* Arguments    : uint16_t int_id         : Interrupt ID
* Return Value : DEVDRV_SUCCESS          : Success of registration of INTC
*              :                         : interrupt handler function
*              : DEVDRV_ERROR            : Failure of registration of INTC
*              :                         : interrupt handler function
******************************************************************************/
int32_t R_INTC_UnRegistIntFunc (uint16_t int_id)
{
    /* ==== Argument check ==== */
    if (int_id >= INTC_ID_TOTAL)
    {
        /* Argument error */
        return DEVDRV_ERROR;
    }

    userdef_intc_unregist_int_func(int_id);

    R_INTC_Update_Isr_Log_Entry(int_id, ISR_ENTRY_UNUSED);

    return DEVDRV_SUCCESS;
}
/*******************************************************************************
 End of function R_INTC_RegistUnintFunc
 *******************************************************************************/



/******************************************************************************
* Function Name: R_INTC_Init
* Description  : Executes initial setting for the INTC.
*              : The interrupt mask level is set to 31 to receive interrupts 
*              : with the interrupt priority level 0 to 30.
* Arguments    : none
* Return Value : none
******************************************************************************/
void R_INTC_Init (void)
{
    uint16_t offset;
    volatile uint32_t * paddr;

    /* ==== Initial setting 1 to receive GIC interrupt request ==== */
    /* Interrupt Security Registers setting */
    paddr = (volatile uint32_t *)&INTC.ICDISR0;

    for (offset = 0; offset < INTC_ICDISR_REG_TOTAL; offset++)
    {
        /* Set all interrupts to be secured */
        (*(paddr + offset)) = 0x00000000uL;
    }

    /* Interrupt Configuration Registers setting */
    paddr = (volatile uint32_t *)&INTC.ICDICFR0;

    for (offset = 0; offset < INTC_ICDICFR_REG_TOTAL; offset++)
    {
        (*(paddr + offset)) = intc_icdicfrn_table[offset];
    }

    /* Interrupt Priority Registers setting */
    paddr = (volatile uint32_t *)&INTC.ICDIPR0;

    for (offset = 0; offset < INTC_ICDIPR_REG_TOTAL; offset++)
    {
        /* Set the priority for all interrupts to 31 */
        (*(paddr + offset)) = 0xF8F8F8F8uL;
    }

    /* Interrupt Processor Targets Registers setting */
    /* Initialise ICDIPTR8 to ICDIPTRn                     */
    /* (n = The number of interrupt sources / 4)           */
    /*   - ICDIPTR0 to ICDIPTR4 are dedicated for main CPU */
    /*   - ICDIPTR5 is dedicated for sub CPU               */
    /*   - ICDIPTR6 to 7 are reserved                      */
    paddr = (volatile uint32_t *)&INTC.ICDIPTR0;

    for (offset = 8; offset < INTC_ICDIPTR_REG_TOTAL; offset++)
    {
        /* Set the target for all interrupts to main CPU */
        (*(paddr + offset)) = 0x01010101uL;
    }

    /* Interrupt Clear-Enable Registers setting */
    paddr = (volatile uint32_t *)&INTC.ICDICER0;

    for (offset = 0; offset < INTC_ICDICER_REG_TOTAL; offset++)
    {
         /* Set all interrupts to be disabled */
        (*(paddr + offset)) = 0xFFFFFFFFuL;
    }

    /* Interrupt Priority Mask Register setting */
    /* Enable priorities for all interrupts */
    R_INTC_SetMaskLevel(31);

    /* Binary Point Register setting */
    /* Group priority field [7:3], Sub-priority field [2:0](Do not use) */
    INTC.ICCBPR = 0x00000002uL;

    /* CPU Interface Control Register setting */
    INTC.ICCICR = 0x00000003uL;

    /* Initial setting 2 to receive GIC interrupt request */
    /* Distributor Control Register setting */
    INTC.ICDDCR = 0x00000001uL;
}
/*******************************************************************************
 End of function R_INTC_Init
 *******************************************************************************/


/******************************************************************************
* Function Name: R_INTC_Enable
* Description  : Enables interrupt of the ID specified by the int_id.
* Arguments    : uint16_t int_id : Interrupt ID
* Return Value : DEVDRV_SUCCESS  : Success to enable INTC interrupt
*              : DEVDRV_ERROR    : Failure to enable INTC interrupt
******************************************************************************/
int32_t R_INTC_Enable(uint16_t int_id)
{
    uint32_t mask;
    volatile uint32_t * paddr;

    /* ==== Argument check ==== */
    if (int_id >= INTC_ID_TOTAL)
    {
        /* Argument error */
        return DEVDRV_ERROR;
    }

    /* ICDISERn has 32 sources in the 32 bits               */
    /* The n can be calculated by int_id / 32               */
    /* The bit field width is 1 bit                         */
    /* The target bit can be calculated by (int_id % 32) * 1 */
    /* ICDICERn does not effect on writing "0"              */
    /* The bits except for the target write "0"             */
    paddr = (volatile uint32_t *)&INTC.ICDISER0;
    mask = 1;

    /* Create mask data */
    mask = (mask << (int_id % 32));

    /* Write ICDISERn   */
    (*(paddr + (int_id / 32))) = mask;

    return DEVDRV_SUCCESS;
}
/*******************************************************************************
 End of function R_INTC_Enable
 *******************************************************************************/


/******************************************************************************
* Function Name: R_INTC_Disable
* Description  : Disables interrupt of the ID specified by the int_id.
* Arguments    : uint16_t int_id : Interrupt ID
* Return Value : DEVDRV_SUCCESS  : Success to disable INTC interrupt
*              : DEVDRV_ERROR    : Failure to disable INTC interrupt
******************************************************************************/
int32_t R_INTC_Disable (uint16_t int_id)
{
    uint32_t mask;
    volatile uint32_t * paddr;

    /* ==== Argument check ==== */
    if (int_id >= INTC_ID_TOTAL)
    {
        /* Argument error */
        return DEVDRV_ERROR;
    }

    /* ICDICERn has 32 sources in the 32 bits               */
    /* The n can be calculated by int_id / 32               */
    /* The bit field width is 1 bit                         */
    /* The target bit can be calculated by (int_id % 32) * 1 */
    /* ICDICERn does no effect on writing "0"               */
    /* Other bits except for the target write "0"           */
    paddr = (volatile uint32_t *)&INTC.ICDICER0;
    mask = 1;

    /* Create mask data */
    mask = (mask << (int_id % 32));

    /* Write ICDICERn   */
    (*(paddr + (int_id / 32))) = mask;

    return DEVDRV_SUCCESS;
}
/*******************************************************************************
 End of function R_INTC_Disable
 *******************************************************************************/

extern void R_INTC_Update_Isr_Log_Entry(uint16_t entry, uint8_t priority);

/******************************************************************************
* Function Name: R_INTC_SetPriority
* Description  : Sets the priority level of the ID specified by the int_id to 
*              : the priority level specified by the priority.
* Arguments    : uint16_t int_id   : Interrupt ID
*              : uint8_t  priority : Interrupt priority level (0 to 31)
* Return Value : DEVDRV_SUCCESS    : Success of INTC interrupt priority level setting
*              : DEVDRV_ERROR      : Failure of INTC interrupt priority level setting
******************************************************************************/
int32_t R_INTC_SetPriority (uint16_t int_id, uint8_t priority)
{
    uint32_t icdipr;
    uint32_t mask;
    volatile uint32_t * paddr;

    /* ==== Argument check ==== */
    if ((int_id >= INTC_ID_TOTAL) || (priority >= 32))
    {
        return DEVDRV_ERROR;        /* Argument error */
    }

    R_INTC_Update_Isr_Log_Entry(int_id, priority);

    /* Priority[7:3] of ICDIPRn is valid bit */
    priority = (uint8_t)(priority << 3);

    /* ICDIPRn has 4 sources in the 32 bits                 */
    /* The n can be calculated by int_id / 4                */
    /* The bit field width is 8 bits                        */
    /* The target bit can be calculated by (int_id % 4) * 8 */
    paddr = (volatile uint32_t *)&INTC.ICDIPR0;

    /* Read ICDIPRn */
    icdipr = (*(paddr + (int_id / 4)));

    /* ---- Mask ----      */
    mask = (uint32_t)0x000000FFuL;

    /* Shift to target bit */
    mask = (mask << ((int_id % 4) * 8));

    /* Clear priority      */
    icdipr &= (~mask);

    /* ---- Priority ----  */
    mask = (uint32_t)priority;

    /* Shift to target bit */
    mask = (mask << ((int_id % 4) * 8));

    /* Set priority        */
    icdipr |= mask;

    /* Write ICDIPRn */
    (*(paddr + (int_id / 4))) = icdipr;

    return DEVDRV_SUCCESS;
}
/*******************************************************************************
 End of function R_INTC_SetPriority
 *******************************************************************************/


/******************************************************************************
* Function Name: R_INTC_SetMaskLevel
* Description  : Sets the interrupt mask level specified by the mask_level.
* Arguments    : uint8_t mask_level : Interrupt mask level (0 to 31)
* Return Value : DEVDRV_SUCCESS     : Success of INTC interrupt mask level setting
*              : DEVDRV_ERROR       : Failure of INTC interrupt mask level setting
******************************************************************************/
int32_t R_INTC_SetMaskLevel (uint8_t mask_level)
{
    volatile uint8_t dummy_buf_8b = 0;

    UNUSED_VARIABLE(dummy_buf_8b);

    /* ==== Argument check ==== */
    if (mask_level >= 32)
    {
        /* Argument error */
        return DEVDRV_ERROR;
    }

    /* Priority[7:3] of ICDIPRn is valid bit */
    mask_level  = (uint8_t)(mask_level << 3);

    /* Write ICCPMR             */
    INTC.ICCPMR = mask_level;
    dummy_buf_8b = (uint8_t)INTC.ICCPMR;

    return DEVDRV_SUCCESS;
}
/*******************************************************************************
 End of function R_INTC_SetMaskLevel
 *******************************************************************************/


/******************************************************************************
* Function Name: R_INTC_GetMaskLevel
* Description  : Obtains the setting value of the interrupt mask level, and 
*              : returns the obtained value to the mask_level.
* Arguments    : uint8_t * mask_level : Interrupt mask level (0 to 31)
* Return Value : none
******************************************************************************/
void R_INTC_GetMaskLevel(uint8_t * mask_level)
{
    /* Read ICCPMR              */
    (*mask_level) = (uint8_t)INTC.ICCPMR;

    /* ICCPMR[7:3] is valid bit */
    (*mask_level) = ((*mask_level) >> 3);
}
/*******************************************************************************
 End of function R_INTC_GetMaskLevel
 *******************************************************************************/

/******************************************************************************
* Function Name: R_INTC_ClearPendingInt
* Description  : Clears pending status of an interrupt. 
* Arguments    : int_id - ID of interrupt to clear pending status of
* Return Value : none
******************************************************************************/
void R_INTC_ClearPendingInt(uint16_t int_id)
{
    volatile uint32_t * paddr;

    /* ==== Argument check ==== */
    if (int_id >= INTC_ID_TOTAL)
    {
        /* Argument error */
        return;        
    }

    /* ICDISPRn has 32 sources in the 32 bits                */
    /* The n can be calculated by int_id / 32                */
    /* The bit field width is 1 bit                          */
    /* The target bit can be calculated by (int_id % 32) * 1 */
    paddr = (volatile uint32_t *)&INTC.ICDISPR0;
    (*(paddr + (int_id / 32))) = (0x00000001 << (int_id % 32));
}
/*******************************************************************************
 End of function R_INTC_ClearPendingInt
 *******************************************************************************/

/******************************************************************************
* Function Name: R_INTC_GetPendingStatus
* Description  : Obtains the interrupt state of the interrupt specified by 
*              : int_id, and returns the obtained value to the *icdicpr.
* Arguments    : uint16_t int_id    : Interrupt ID
*              : uint32_t * icdicpr : Interrupt state of the interrupt 
*              :                    : specified by int_id
*              :                    :   1 : Pending or active and pending
*              :                    :   0 : Not pending
* Return Value : DEVDRV_SUCCESS : Success to obtain interrupt pending status
*              : DEVDRV_ERROR   : Failure to obtain interrupt pending status
******************************************************************************/
int32_t R_INTC_GetPendingStatus (uint16_t int_id, uint32_t * icdicpr)
{
    volatile uint32_t * paddr;

    /* ==== Argument check ==== */
    if (int_id >= INTC_ID_TOTAL)
    {
        return DEVDRV_ERROR;        /* Argument error */
    }

    /* ICDICPRn has 32 sources in the 32 bits               */
    /* The n can be calculated by int_id / 32               */
    /* The bit field width is 1 bit                         */
    /* The target bit can be calculated by (int_id % 32) * 1 */
    paddr = (volatile uint32_t *)&INTC.ICDICPR0;
    (*icdicpr) = (*(paddr + (int_id / 32)));     /* Read ICDICPRn */
    (*icdicpr) = (((*icdicpr) >> (int_id % 32)) & 0x00000001);

    return DEVDRV_SUCCESS;
}
/*******************************************************************************
 End of function R_INTC_GetPendingStatus
 *******************************************************************************/



/******************************************************************************
* Function Name: R_INTC_SetConfiguration
* Description  : Sets the interrupt detection mode of the ID specified by the 
*              : int_id to the detection mode specified by the int_sense.
* Arguments    : uint16_t int_id    : Interrupt ID (INTC_ID_TINT0 to INTC_ID_TINT170)
*              : uint32_t int_sense : Interrupt detection
*              :                    :   INTC_LEVEL_SENSITIVE : Level sense
*              :                    :   INTC_EDGE_TRIGGER    : Edge trigger
* Return Value : DEVDRV_SUCCESS : Success of INTC interrupt configuration setting
*              : DEVDRV_ERROR   : Failure of INTC interrupt configuration setting
******************************************************************************/
int32_t R_INTC_SetConfiguration (uint16_t int_id, uint32_t int_sense)
{
    uint32_t icdicfr;
    uint32_t mask;
    volatile uint32_t * paddr;

    /* ==== Argument check ==== */
    if (((int_id < INTC_ID_TINT0) || (int_id >= INTC_ID_TOTAL)) ||
                                              (int_sense > INTC_EDGE_TRIGGER))
    {
        /* Argument error */
        return DEVDRV_ERROR;
    }

    /* ICDICFRn has 16 sources in the 32 bits                          */
    /* The n can be calculated by int_id / 16                          */
    /* The bit field width is 2 bits                                   */
    /* Interrupt configuration bit assigned higher 1 bit in the 2 bits */
    /* The target bit can be calculated by ((int_id % 16) * 2) + 1     */
    paddr = (volatile uint32_t *)&INTC.ICDICFR0;

    /* Read ICDICFRn        */
    icdicfr = (*(paddr + (int_id / 16)));

    mask = 0x00000001uL;

    /* Shift to target bit  */
    mask <<= (((int_id % 16) * 2) + 1);

    if (INTC_LEVEL_SENSITIVE == int_sense)
    {
        /* Level sense setting  */
        icdicfr &= (~mask);
    }
    else
    {
        /* Edge trigger setting */
        icdicfr |= mask;
    }

    /* Write ICDICFRn       */
    (*(paddr + (int_id / 16))) = icdicfr;

    return DEVDRV_SUCCESS;
}
/*******************************************************************************
 End of function R_INTC_SetConfiguration
 *******************************************************************************/

/* END of File */


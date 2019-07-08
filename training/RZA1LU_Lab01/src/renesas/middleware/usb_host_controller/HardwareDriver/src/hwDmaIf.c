/******************************************************************************
 * DISCLAIMER
 * This software is supplied by Renesas Electronics Corporation and is only
 * intended for use with Renesas products. No other uses are authorized.
 * This software is owned by Renesas Electronics Corporation and is  protected
 * under all applicable laws, including copyright laws.
 * THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES
 * REGARDING THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY,
 * INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, FITNESS FOR  A
 * PARTICULAR PURPOSE AND NON-INFRINGEMENT.  ALL SUCH WARRANTIES ARE  EXPRESSLY
 * DISCLAIMED.
 * TO THE MAXIMUM EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS
 * ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES SHALL BE  LIABLE
 * FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES
 * FOR ANY REASON RELATED TO THIS SOFTWARE, EVEN IF RENESAS OR ITS
 * AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 * Renesas reserves the right, without notice, to make changes to this
 * software and to discontinue the availability of this software.
 * By using this software, you agree to the additional terms and
 * conditions found by accessing the following link:
 * http://www.renesas.com/disclaimer
 *******************************************************************************
 * Copyright (C) 2016 Renesas Electronics Corporation. All rights reserved.
 ******************************************************************************
 * File Name    : hwDmaIf.c
 * Version      : 1.10
 * Device(s)    : Renesas
 * Tool-Chain   : GNUARM-NONE-EABI v14.02
 * OS           : None
 * H/W Platform : RSK+
 * Description  : The hardware interface functions for the DMA. The functions
 *                here could abstract from the DMA hardware providing a simple
 *                interface for DMA usage. This is beyond the scope of this
 *                sample code so it has been made as simple as possible.
 *******************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 28.05.2015 1.0  First Release
 *              : 28.01.2016 1.10 Updated API to provide a level of HW
 *                                abstraction. USB functions still here for
 *                                legacy reasons. Improved GSCE compliance.
 ******************************************************************************/

/******************************************************************************
 WARNING!  IN ACCORDANCE WITH THE USER LICENCE THIS CODE MUST NOT BE CONVEYED
 OR REDISTRIBUTED IN COMBINATION WITH ANY SOFTWARE LICENSED UNDER TERMS THE
 SAME AS OR SIMILAR TO THE GNU GENERAL PUBLIC LICENCE
 ******************************************************************************/

/******************************************************************************
 User Includes
 ******************************************************************************/
#include "iodefine_cfg.h"
#include "compiler_settings.h"

#include "trace.h"
#include "hwDmaIf.h"
#include "r_intc.h"
#include "r_task_priority.h"
#include "dmac_iobitmask.h"

#ifdef R_SELF_LOAD_MIDDLEWARE_USB_HOST_CONTROLLER
#include "ddusbh.h"
#include "usbhConfig.h"
#include "usb20_iodefine.h"
#include "hwArmCache.h"
#endif

/******************************************************************************
 Function Macros
 ******************************************************************************/

/* Comment this line out to turn ON module trace in this file */
#undef _TRACE_ON_

#ifndef _TRACE_ON_
#undef TRACE
#define TRACE(x)
#endif

/* R_OS_SysLock / R_OS_SysUnlock are required when nested interrupts are supported in your
 * device. They are not required when nesting is impossible */
/* Remove comment on line below to turn nested interrupt support in this file */
#define NESTED_SUPPORT (1)

#define MAX_DMA_CHANNELS    (4)        /* Number of DMA channels supported */

/* DMA: RID and MID fields for the USB Channels */
/* Reference RZA1H HW Manual r01UH0403ej0200 Table 9.4 */
#define DMA_USB0_OUT_MID (0x20)
#define DMA_USB0_OUT_RID (0x3)
#define DMA_USB1_OUT_MID (0x22)
#define DMA_USB1_OUT_RID (0x3)

#define DMA_USB0_IN_MID (0x21)
#define DMA_USB0_IN_RID (0x3)
#define DMA_USB1_IN_MID (0x23)
#define DMA_USB1_IN_RID (0x3)

/******************************************************************************
 Function Prototypes
 ******************************************************************************/
//static void INT_D0FIFO0_COMPLETE (uint32_t dummy);
static void INT_D1FIFO0_COMPLETE (uint32_t dummy);
static void INT_D0FIFO1_COMPLETE (uint32_t dummy);
static void INT_D1FIFO1_COMPLETE (uint32_t dummy);
static void dmaFreeVectD0FIFO0 (void);
static void dmaFreeVectD0FIFO1 (void);
static void dmaFreeVectD1FIFO0 (void);
static void dmaFreeVectD1FIFO1 (void);

/******************************************************************************
 Global Variables
 ******************************************************************************/
/* Map of DMA channels - used in allocator/deallocator  functions */
static uint32_t gi_channel_map = 0;

/* The parameters for the call-back functions */
static void *pv_param_ch0 = NULL;
static void *pv_param_ch1 = NULL;

/* The call back function pointers */
static void (*gpf_complete0) (void *) = NULL;
static void (*gpf_complete1) (void *) = NULL;

/* The vector free functions */
static void (*gpfFreeVectUsbOutCh0) (void) = NULL;
static void (*gpfFreeVectUsbInCh1) (void) = NULL;

/* The length of the transfers used in USB functions*/
static size_t gst_lengthCh0 = 0UL;
static size_t gstTransferLengthCh0 = 0UL;
static uint16_t gwBlockSizeCh0 = 0;
static uint16_t gwNumBlocksCh0 = 0;
static size_t gst_lengthCh1 = 0UL;
static size_t gstTransferLengthCh1 = 0UL;
static uint16_t gwBlockSizeCh1 = 0;
static uint16_t gwNumBlocksCh1 = 0;


/******************************************************************************
 Public Functions
 ******************************************************************************/

/******************************************************************************
 Function Name: R_DMA_AllocFreeChannel
 Description:   Finds and allocates an available DMA channel.
 Arguments:     None
 Return value:  Channel allocated or -1 for error;
 ******************************************************************************/
int32_t R_DMA_AllocFreeChannel(void)
{
    int32_t ichannel;
    uint32_t i_channel_loc;
    uint8_t  by_channel_allocated = 0;
    int_t imask;

    imask = R_OS_SysLock(NULL); /* Lock the scheduler while finding a slot */

    for (ichannel = DMA_PRE_ALLOCATED_CHANNELS;
         ( (ichannel < MAX_DMA_CHANNELS) && (0 == by_channel_allocated) );
           ichannel++ )
    {
        i_channel_loc = (uint32_t)(1 << ichannel);
        if (0 == (gi_channel_map & i_channel_loc))
        {
            /* Assign the channel */
            gi_channel_map |= i_channel_loc;
            by_channel_allocated = 1; /* Found a channel - exit the search loop */
        }
    }

      R_OS_SysUnlock(NULL, imask); /* Unlock scheduler */

      if (MAX_DMA_CHANNELS == ichannel)
      {
        TRACE(("dmaAlloc: No Free DMA Channels available\r\n"));
        ichannel = -1;
      }
      else // found a slot
      {
        ichannel = (ichannel - 1);		/* Compensate as first bit is channel #0 */
        TRACE(("dmaAlloc: DMA Channel %d allocated\r\n", ichannel));
      }

    return ichannel;
}
/******************************************************************************
 End of function  R_DMA_AllocFreeChannel
 ******************************************************************************/

/******************************************************************************
 Function Name: R_DMA_DeAllocChannel
 Description:   Function to free a DMA channel
 Arguments:     IN  ichannel - The DMA channel to free
 Return value:  0 for success or -1 on error
 ******************************************************************************/
int32_t R_DMA_DeAllocChannel (uint32_t ichannel)
{
    uint32_t i_channel_loc;

    if (ichannel < MAX_DMA_CHANNELS)
    {
        i_channel_loc = (uint32_t)(1 << ichannel);

        /* Check that the channel is in use */
        if (gi_channel_map & i_channel_loc)
        {
            gi_channel_map &= (~i_channel_loc);
            TRACE(("dmaAlloc: DMA Channel %d freed\r\n", ichannel));
            return 0;
        }
        TRACE(("dmaAlloc: DMA Channel %d free request but not allocated\r\n", ichannel));
    }
    return (-1);
}
/******************************************************************************
 End of function  R_DMA_DeAllocChannel
 ******************************************************************************/


/******************************************************************************
 Function Name: R_DMA_Channel2Pointer
 Description:   Function to free a DMA channel
                Note this is very specific to the RZA1 and will need
                tweaking for other processors.

 Arguments:     IN  ichannel - The DMA channel to get pointers for
                OUT ppDMARegBase - Pointer to ichannel's DMA block base address
                OUT pintvect - interrupt vector associated with ichannel.


 Return value:  0 for failure or 1 for success
 ******************************************************************************/
uint32_t R_DMA_Channel2Pointer (uint32_t ichannel, void **ppDMARegBase, uint16_t* pintvect)
{

    if( !(ichannel >= MAX_DMA_CHANNELS) )
    {
        /* Work out the DMA Channel Base address */
        /* Note this is very specific to the RZA1 and will need
         * tweaking for processor change */
        *ppDMARegBase = (void *) &DMAC0;
        (*((uint8_t **) ppDMARegBase)) += ((sizeof(DMAC0)) * (ichannel));

        /* Between channel 7 and 8 are a set of shared
         * registers. Compensate for these.
         */
        if (ichannel > 7)
        {
            (*((uint8_t **) ppDMARegBase)) += 0x200;
        }

        /* Interrupt Vector Calculations */
        *pintvect = (uint16_t *) (INTC_ID_DMAINT0 + ichannel);

        return (1);
    }
    else // Invalid DMA channel requested
    {
        return (0);
    }

}
/******************************************************************************
 End of function  R_DMA_Channel2Pointer
 ******************************************************************************/



/******************************************************************************
 Function Name: R_DMA_SetChannelDMARS
 Description:   Sets up a DMA channel DMARS register to with values provided
 Arguments:     IN  ichannel - The DMA channel to setup DMARS for
                IN  by_rid   - RID value for the DMARS register
                IN  by_mid   - MID value for the DMARS register

 Return value:  0 for failure or 1 for success
 ******************************************************************************/
uint32_t R_DMA_SetChannelDMARS (uint32_t ichannel, uint8_t by_mid, uint8_t by_rid)
{
    volatile uint32_t* pdmars_pointer;
    uint8_t dmars_bit_shift;

    if( !(ichannel >= MAX_DMA_CHANNELS) )
    {
        /* Work out the DMA Channel DMARS address and shifts */
        /* Note this is very specific to the RZA1 and will need
         * tweaking for processor change */

        /* Work out details for the associated DMARS register */
        pdmars_pointer = (void*)&DMACDMARS0;
        pdmars_pointer += (ichannel / 2);

        /* Even ichannels use bottom 16 bits of DMARS and Odd channels the upper 16 bits */
        if (ichannel % 2)
        {
            dmars_bit_shift = 16;
        }
        else
        {
            dmars_bit_shift = 0;
        }

        *pdmars_pointer = *pdmars_pointer & (long unsigned int)(~(0x0000FFFF << dmars_bit_shift)); /* Clear current contents of channels DMARS */
        *pdmars_pointer = *pdmars_pointer | (long unsigned int)( ( (by_mid << 2) | (by_rid) ) << dmars_bit_shift );

        return (1);
    }
    else // Invalid DMA channel requested
    {
        return (0);
    }

}
/******************************************************************************
 End of function  R_DMA_SetChannelDMARS
 ******************************************************************************/




/******************************************************************************
 Start of Legacy DMA Related API Functions
 The following API functions are included to maintain comparability with
 legacy code.
 ******************************************************************************/



/******************************************************************************
 Function Name: dmaAlloc
                Note fcn name not GSCE compliant for legacy code compatibility
 Description:   Allocates the DMA channel requested, if free.
                This is supplied for legacy code, use R_DMA_AllocFreeChannel
                in preference to this function.
 Arguments:     IN ichannel - DMA Channel Requested
 Return value:  0 if Channel allocated or -1 for error;
 ******************************************************************************/
int32_t dmaAlloc (uint32_t ichannel)
{
    uint32_t i_channel_loc;
    int_t imask = R_OS_SysLock(NULL);

    if (ichannel < MAX_DMA_CHANNELS)
    {
        i_channel_loc = (uint32_t)(1 << ichannel);

        /* Check for channel already in use */
        if (gi_channel_map & i_channel_loc)
        {
            R_OS_SysUnlock(NULL, imask);
            TRACE(("dmaAlloc: DMA Channel %d already in use\r\n", ichannel));
            return -1;
        }

        /* Assign the channel */
        gi_channel_map |= i_channel_loc;

        R_OS_SysUnlock(NULL, imask);

        TRACE(("dmaAlloc: DMA Channel %d allocated\r\n", ichannel));
        return 0;
    }

    return -1;
}
/******************************************************************************
 End of function  dmaAlloc
 ******************************************************************************/

/******************************************************************************
 Function Name: dmaFree
                Note fcn name not GSCE compliant for legacy code compatibility
 Description:   Function to free a DMA channel.
                This is now just a redirector to the R_DMA_DeAllocChannel fcn.
 Arguments:     IN  ichannel - The DMA channel to free
 Return value:  0 for success or -1 on error
 ******************************************************************************/
int32_t dmaFree (uint32_t ichannel)
{
    return(R_DMA_DeAllocChannel(ichannel));
}
/******************************************************************************
 End of function  dmaFree
 ******************************************************************************/


/******************************************************************************
 End of Legacy DMA Related API Functions
 ******************************************************************************/






/******************************************************************************
 Function Name: dmaStartUsbOutCh0
 Description:   Function to start DMA channel 0 for a USB OUT transfer
 This is where the DMAC writes to the designated pipe FIFO.
 In this implementation the assignment is DMA Channel 0 always
 uses the USB D0FIFO.

 Direction: Buffer memory --> USB Channel

 Arguments:
 IN  pvSrc - Pointer to the 4 byte aligned source memory
 (CAUTION: If using internal SRAM ensure this is a *Mirrored Address* (6xxxxxxx)
 native (2xxxxxxx) addresses corrupts data when DMA transfer crosses blocks)
 IN  st_length - The length of data to transfer
 IN  p_fifo - Pointer to the destination FIFO
 IN  pv_param - Pointer to pass to the completion routine
 IN  pf_complete - Pointer to the completion routine
 Return value:  none
 ******************************************************************************/
void dmaStartUsbOutCh0 (void *pvSrc, size_t st_length, void *p_fifo, void *pv_param, void (*pf_complete) (void *pv_param))
{
#ifdef R_SELF_LOAD_MIDDLEWARE_USB_HOST_CONTROLLER

    PUSBTR prequest = (PUSBTR) pv_param;

#if NESTED_SUPPORT
    int_t imask = R_OS_SysLock(NULL);
#endif

    bool_t bf_invalidate_result = 1;
    void *pv_physical_rolling;
    uint8_t *pv_src_rolling;
    size_t st_src_len;
    size_t st_dest_len;

    volatile struct st_dmac_n* pdmac_block;
    uint32_t bresult;
    uint16_t int_vect; /* Vector associated with the DMA channel allocated */

    /* Get pointers to the DMA channel, using the pre-allocated channel define */
    bresult = R_DMA_Channel2Pointer(DMA_CHANNEL_USBH_OUT, (void*)&pdmac_block, &int_vect);

    if(1 == bresult)
    {
        /* Disable any pending xfers the DMA channel */
        pdmac_block->CHCTRL_n |= DMAC0_CHCTRL_n_CLREN;

        /* Reset channel status register DMA block */
        pdmac_block->CHCTRL_n |= DMAC0_CHCTRL_n_SWRST;

        R_INTC_Disable(int_vect);

        /* Disable standard USB Interrupts and register DMA Interrupts */
        /* USB Interrupts are only temp. disabled for the duration of the setup */
        if ( (&USB200/*&USB0*/) == prequest->pUSB)
        {
            /* Register DMA interrupt vector */
            R_INTC_RegistIntFunc(int_vect, INT_D0FIFO1_COMPLETE);

            gpfFreeVectUsbOutCh0 = dmaFreeVectD0FIFO0;

        }
        else
        {

            /* Register DMA interrupt vector */
            R_INTC_RegistIntFunc(int_vect, INT_D0FIFO1_COMPLETE);

            gpfFreeVectUsbOutCh0 = dmaFreeVectD0FIFO1;
        }

        st_src_len = st_length;
        pv_src_rolling = pvSrc;

        while (1 == bf_invalidate_result)
        {
            /* Make sure that any cache is flushed in to main memory */
            bf_invalidate_result = cacheClear(&pv_physical_rolling, &st_dest_len, pv_src_rolling, st_src_len);

            pv_src_rolling += st_dest_len;
            st_src_len -= st_dest_len;

            TRACE(("hwDmaIF: OUT Page Boundary crossed @ 0x%.8lX\r\n", st_dest_len));
        }

        /* Set the pipe number and completion routine */
        pv_param_ch0 = pv_param;
        gpf_complete0 = pf_complete;
        gwBlockSizeCh0 = prequest->pEndpoint->wPacketSize;
        gstTransferLengthCh0 = st_length;

        /* Set the remainder which will need to be processed by another transfer */
        gst_lengthCh0 = st_length - gstTransferLengthCh0;
        gwNumBlocksCh0 = (uint16_t) (gstTransferLengthCh0 / gwBlockSizeCh0);


        /* Setup RID and MID based on what USB host channel is used. */
        if ((&USB200/*&USB0*/) == prequest->pUSB)
        {
            /* MID, RID : Select USB0 channel 0 for DMA channel out */
            R_DMA_SetChannelDMARS(DMA_CHANNEL_USBH_OUT, DMA_USB0_OUT_MID, DMA_USB0_OUT_RID);
        }
        else    // USB1 used
        {
            /* MID, RID : Select USB1 channel 0 for DMA channel out */
            R_DMA_SetChannelDMARS(DMA_CHANNEL_USBH_OUT, DMA_USB1_OUT_MID, DMA_USB1_OUT_RID);
        }


    #ifdef ASSEMBLE_CHCFG
        /* The code here was used in development but included to allow
         * the reader an understanding of how the value for the single
         * write was calculated.
         */
    #ifdef USBH_FIFO_BIT_WIDTH == 32

        DMAC0.CHCFG_n = 0x00000000;

        /* Set Source Transfer Size 32 bits */
        DMAC0.CHCFG.BIT.DDS = 2;

        /* Set Destination Transfer Size 32 bits */
        DMAC0.CHCFG.BIT.SDS = 2;
    #else  /* 16 Bit Bus Width */
        /* Set Source Transfer Size 16 bits */
        DMAC0.CHCFG.BIT.DDS = 1;
        /* Set Destination Transfer Size 16 bits */
        DMAC0.CHCFG.BIT.SDS = 1;
    #endif

        /* Set Count direction of the destination address fixed */
        DMAC0.CHCFG.BIT.DAD = 1;

        /* Set Count direction of the source address Auto increment */
        DMAC0.CHCFG.BIT.SAD = 0;

        /* Set Channel Status Register DMA Mode to 0 (Register Mode) */
        DMAC0.CHCFG.BIT.DMS = 0;

        /* Set Channel Status Register Set to 0
         * (Select Executes the Next0 Register Set) */
        DMAC0.CHCFG.BIT.RSEL = 0;

        /* Set Channel Status Register Sweep Buffer Enable to 0
         * (Stops the DMA transfer without sweeping the buffer) */
        DMAC0.CHCFG.BIT.SBE = 0;

        /* Set Channel Status Register DMA Interrupt Mask to 0,  interrupt not masked */
        DMAC0.CHCFG.BIT.DEM = 0;

        /* Set Enabled Register Set Enable to 0 (Does not continue DMA transfers) */
        DMAC0.CHCFG.BIT.REN = 0;

        /* Set Register Select Switch to 0 (Does not invert RSEL automatically after
         * a DMA transaction) */
        DMAC0.CHCFG.BIT.RSW = 0;

        /* TM, AM, LVL, HIEN, LOEN, REQD set as per table 9.4 for on-chip peripherals */
        DMAC0.CHCFG.BIT.TM = 0; /* Single Transfer mode mode */
        DMAC0.CHCFG.BIT.AM = 1; /* ACK Mode - Level Mode */
        DMAC0.CHCFG.BIT.LVL = 1; /* Level - Detects based on level */
        DMAC0.CHCFG.BIT.HIEN = 1; /* Request detected at  High Level */
        DMAC0.CHCFG.BIT.LOEN = 0; /* Request not detected at Low Level */
        DMAC0.CHCFG.BIT.REQD = 1; /* DMAACK active when written */

        /* SEL : Set SEL setting */
        /* Depends on DMAC Channel being used */

        TRACE(("DMAC0.CHCFG_n = 0x%.8lX\r\n", DMAC0.CHCFG_n));
    #else

        /* The value of CHCFG here is derived from the above bit manipulations
         * and assembled here as one write for speed
         */
        pdmac_block->CHCFG_n = 0x00222168;

        /* Setup SEL bits. This is the lower 3 bits of the channel being used */
        pdmac_block->CHCFG_n &= ~DMAC0_CHCFG_n_SEL;
        pdmac_block->CHCFG_n |= (DMA_CHANNEL_USBH_OUT & 0x07)<<DMAC0_CHCFG_n_SEL_SHIFT;
    #endif

        pdmac_block->CHITVL_n = 0; /* No interval - DMA occupies bus until complete */
        pdmac_block->CHEXT_n = 0; /* Channel Extension - defaults */

        /* Set the source address */
        pdmac_block->N0SA_n = (uint32_t) pvSrc;

        /* Set the destination address register */
        pdmac_block->N0DA_n = (uint32_t) p_fifo;

        /* Set the Count Register */
        pdmac_block->N0TB_n = (uint32_t) gstTransferLengthCh0;

        TRACE(("hwDmaif: DMA out len = 0x%.8lX from address 0x%.8lX\r\n", gstTransferLengthCh0, pvSrc));

        /* Set the interrupt priority */
        R_INTC_SetPriority(int_vect, ISR_USBH_DMA_PRIORITY);

        /* Enable the DMA peripheral interrupts */
        R_INTC_Enable(int_vect);

        /* Enable the channel */
        pdmac_block->CHCTRL_n |= DMAC0_CHCTRL_n_SETEN;

    #if NESTED_SUPPORT
        R_OS_SysUnlock(NULL, imask);
    #endif
    }
    else
    {
        TRACE(("hwDmaif: DMA Channel out of range"));
    }
#endif /* R_SELF_LOAD_MIDDLEWARE_USB_HOST_CONTROLLER */
}
/******************************************************************************
 End of function  dmaStartUsbOutCh0
 ******************************************************************************/

/******************************************************************************
 Function Name: dmaGetUsbOutCh0Count
 Description:   Function to get the current DMA transfer count
 Parameters:    none
 Return value:  The value of the transfer count register
 ******************************************************************************/
unsigned long dmaGetUsbOutCh0Count (void)
{
    volatile struct st_dmac_n* pdmac_block;
    uint32_t bresult;
    uint16_t int_vect; /* Vector associated with the DMA channel allocated */

    /* Get pointers to the DMA channel, using the pre-allocated channel define */
    bresult = R_DMA_Channel2Pointer(DMA_CHANNEL_USBH_OUT, (void*)&pdmac_block, &int_vect);
    (void)bresult;
    /* This is only to detect transfer stop for purposes of time-out, however
     the number that is returned must be changing while the DMA working */
    return (unsigned long) (gst_lengthCh0 + (pdmac_block->CRTB_n * gwNumBlocksCh0));
}
/******************************************************************************
 End of function  dmaGetUsbOutCh0Count
 *****************************************************************************/

/******************************************************************************
 Function Name: dmaStopUsbOutCh0
 Description:   Function to stop a USB OUT DMA transfer on channel 0
 Parameters:    none
 Return value:  none
 ******************************************************************************/
void dmaStopUsbOutCh0 (void)
{
#if NESTED_SUPPORT
    int_t imask = R_OS_SysLock(NULL);
#endif

    volatile struct st_dmac_n* pdmac_block;
    uint32_t bresult;
    uint16_t int_vect; /* Vector associated with the DMA channel allocated */

    /* Get pointers to the DMA channel, using the pre-allocated channel define */
    bresult = R_DMA_Channel2Pointer(DMA_CHANNEL_USBH_OUT, (void*)&pdmac_block, &int_vect);
    (void)bresult;

    TRACE(("dmaStopUsbOutCh0: Called\r\n"));

    /* Disable the DMA channel */
    pdmac_block->CHCTRL_n |= DMAC0_CHCTRL_n_CLREN;

    /* Reset channel status register DMA block */
    pdmac_block->CHCTRL_n |= DMAC0_CHCTRL_n_SWRST;

    /* Clear DMARS association */
    R_DMA_SetChannelDMARS (DMA_CHANNEL_USBH_OUT, 0, 0);

    if (gpfFreeVectUsbOutCh0)
    {
        gpfFreeVectUsbOutCh0();
        gpfFreeVectUsbOutCh0 = NULL;
    }
#if NESTED_SUPPORT
    R_OS_SysUnlock(NULL, imask);
#endif
}
/******************************************************************************
 End of function  dmaStopUsbOutCh0
 ******************************************************************************/

/******************************************************************************
 Function Name: dmaStartUsbInCh1
 Description:   Function to start DMA channel 1 for a USB IN transfer
 This is where the DMAC writes to the designated pipe FIFO.
 In this implementation the assignment is DMA Channel 1 always
 uses the USB D1FIFO.

 Direction: USB Channel --> Buffer Memory

 Arguments:
 IN  pv_dest - Pointer to the 4 byte aligned destination memory
 (CAUTION: If using internal SRAM ensure this is a *Mirrored Address* (6xxxxxxx)
 native (2xxxxxxx) addresses corrupts data when DMA transfer crosses blocks)

 IN  st_length - The length of data to transfer
 IN  p_fifo - Pointer to the source FIFO
 IN  pv_param - Pointer to pass to the completion routine
 IN  pf_complete - Pointer to the completion routine
 Return value:  none
 ******************************************************************************/
void dmaStartUsbInCh1 (void *pv_dest, size_t st_length, void *p_fifo, void *pv_param, void (*pf_complete) (void *pv_param))
{
#ifdef R_SELF_LOAD_MIDDLEWARE_USB_HOST_CONTROLLER
    PUSBTR prequest = (PUSBTR) pv_param;
    bool_t bf_invalidate_result = 1;
    void *pv_physical_rolling;
    uint8_t *pv_src_rolling;
    size_t st_src_len;
    size_t st_dest_len;
    volatile struct st_dmac_n* pdmac_block;
    uint32_t bresult;
    uint16_t int_vect; /* Vector associated with the DMA channel allocated */

#if NESTED_SUPPORT
    int_t imask = R_OS_SysLock(NULL);
#endif

    /* Get pointers to the DMA channel, using the pre-allocated channel define */
    bresult = R_DMA_Channel2Pointer(DMA_CHANNEL_USBH_IN, (void*)&pdmac_block, &int_vect);
    (void)bresult;

    /* Disable any pending xfers the DMA channel */
    pdmac_block->CHCTRL_n |= DMAC0_CHCTRL_n_CLREN;

    /* Reset channel status register DMA block */
    pdmac_block->CHCTRL_n |= DMAC0_CHCTRL_n_SWRST;

    R_INTC_Disable(int_vect);

    /* Disable standard USB Interrupts and register DMA Interrupts */
    /* USB Interrupts are only temp. disabled for the duration of the setup */
    if ( (&USB200/*&USB0*/) == prequest->pUSB )
    {

        /* Register DMA interrupt vector */
        R_INTC_RegistIntFunc(int_vect, INT_D1FIFO0_COMPLETE);

        gpfFreeVectUsbInCh1 = dmaFreeVectD1FIFO0;

    }
    else
    {

        /* Register DMA interrupt vector */
        R_INTC_RegistIntFunc(int_vect, INT_D1FIFO1_COMPLETE);

        gpfFreeVectUsbInCh1 = dmaFreeVectD1FIFO1;
    }

    st_src_len = st_length;
    pv_src_rolling = pv_dest;

    while (1 == bf_invalidate_result)
    {
        /* Make sure that any cache is flushed in to main memory */
        bf_invalidate_result = cacheInvalidate(&pv_physical_rolling, &st_dest_len, pv_src_rolling, st_src_len); // CKG test

        pv_src_rolling += st_dest_len;
        st_src_len -= st_dest_len;

        TRACE(("hwDmaIF: IN Page Boundary crossed @ 0x%.8lX\r\n", st_dest_len));
    }

    /* Set the pipe number and completion routine */
    pv_param_ch1 = pv_param;
    gpf_complete1 = pf_complete;
    gwBlockSizeCh1 = prequest->pEndpoint->wPacketSize;
    gstTransferLengthCh1 = st_length;

    /* Set the remainder which will need to be processed by another transfer */
    gst_lengthCh1 = st_length - gstTransferLengthCh1;
    gwNumBlocksCh1 = (uint16_t) (gstTransferLengthCh1 / gwBlockSizeCh1);

    /* Setup RID and MID based on what USB host channel is used. */
    if ( (&USB200/*&USB0*/) == prequest->pUSB)
    {
        /* MID, RID : Select USB0 for DMA channel IN */
        R_DMA_SetChannelDMARS(DMA_CHANNEL_USBH_IN, DMA_USB0_IN_MID, DMA_USB0_IN_RID);
    }
    else    // USB1 used
    {
        /* MID, RID : Select USB1 for DMA channel IN */
        R_DMA_SetChannelDMARS(DMA_CHANNEL_USBH_IN, DMA_USB1_IN_MID, DMA_USB1_IN_RID);
    }


#ifdef ASSEMBLE_CHCFG
    /* The code here was used in development but included to allow
     * the reader an understanding of how the value for the single
     * write was calculated.
     */

    DMAC0.CHCFG_n = 0x00000000;

#ifdef USBH_FIFO_BIT_WIDTH == 32
    /* Set Source Transfer Size 32 bits */
    DMAC1.CHCFG.BIT.DDS = 2;
    /* Set Destination Transfer Size 32 bits */
    DMAC1.CHCFG.BIT.SDS = 2;
#else  /* 16 Bit Bus Width */
    /* Set Source Transfer Size 16 bits */
    DMAC1.CHCFG.BIT.DDS = 1;
    /* Set Destination Transfer Size 16 bits */
    DMAC1.CHCFG.BIT.SDS = 1;
#endif

    /* Set Count direction of the destination address Auto increment */
    DMAC1.CHCFG.BIT.DAD = 0;

    /* Set Count direction of the source address Fixed */
    DMAC1.CHCFG.BIT.SAD = 1;

    /* Set Channel Status Register DMA Mode to 0 (Register Mode) */
    DMAC1.CHCFG.BIT.DMS = 0;

    /* Set Channel Status Register Set to 0
     * (Select Executes the Next0 Register Set) */
    DMAC1.CHCFG.BIT.RSEL = 0;

    /* Set Channel Status Register Sweep Buffer Enable to 0
     * (Stops the DMA transfer without sweeping the buffer) */
    DMAC1.CHCFG.BIT.SBE = 0;

    /* Set Channel Status Register DMA Interrupt Mask to 0,  interrupt not masked */
    DMAC1.CHCFG.BIT.DEM = 0;

    /* Set Enabled Register Set Enable to 0 (Does not continue DMA transfers) */
    DMAC1.CHCFG.BIT.REN = 0;

    /* Set Register Select Switch to 0 (Does not invert RSEL automatically after
     * a DMA transaction) */
    DMAC1.CHCFG.BIT.RSW = 0;

    /* TM, AM, LVL, HIEN, LOEN, REQD */
    DMAC1.CHCFG.BIT.TM = 0; /* Single Transfer mode mode */
    DMAC1.CHCFG.BIT.AM = 1; /* ACK Mode - Level Mode */
    DMAC1.CHCFG.BIT.LVL = 1; /* Level - Detects based on level */
    DMAC1.CHCFG.BIT.HIEN = 1; /* Request detected at  High Level */
    DMAC1.CHCFG.BIT.LOEN = 0; /* Request not detected at Low Level */
    DMAC1.CHCFG.BIT.REQD = 0; /* DMAACK active when read */

    /* SEL : Set SEL setting */
    /* Depends on DMAC Channel  being used */

    Trace("DMAC1.CHCFG_n = 0x%.8lX\r\n", DMAC1.CHCFG_n);
#else

    /* The value of CHCFG here is derived from the above bit manipulations
     * and assembled here as one write for speed
     */
    pdmac_block->CHCFG_n = 0x00122160;

    /* Setup SEL bits. This is the lower 3 bits of the channel being used */
    pdmac_block->CHCFG_n &= ~DMAC0_CHCFG_n_SEL;
    pdmac_block->CHCFG_n |= (DMA_CHANNEL_USBH_IN & 0x07)<<DMAC0_CHCFG_n_SEL_SHIFT;
#endif

    pdmac_block->CHITVL_n = 0; /* No interval - DMA occupies bus until complete */
    pdmac_block->CHEXT_n = 0; /* Channel Extension - defaults */

    /* Set the source address */
    pdmac_block->N0SA_n = (uint32_t) p_fifo;

    /* Set the destination address register */
    pdmac_block->N0DA_n = (uint32_t) pv_dest;

    /* Set the Count Register */
    pdmac_block->N0TB_n = (uint32_t) gstTransferLengthCh1;

    TRACE(("hwDmaif: DMA in len = 0x%.8lX to address 0x%.8lX Int: %d\r\n", gstTransferLengthCh1, pv_dest, int_vect));

    /* Set the interrupt priority */
    R_INTC_SetPriority(int_vect, ISR_USBH_DMA_PRIORITY);

    /* Enable the DMA interrupts */
    R_INTC_Enable(int_vect);

    /* Enable the channel */
    pdmac_block->CHCTRL_n |= DMAC0_CHCTRL_n_SETEN;

#if NESTED_SUPPORT
    R_OS_SysUnlock(NULL, imask);
#endif
#endif /* R_SELF_LOAD_MIDDLEWARE_USB_HOST_CONTROLLER */

}
/******************************************************************************
 End of function  dmaStartUsbInCh1
 ******************************************************************************/

/******************************************************************************
 Function Name: dmaGetUsbInCh1Count
 Description:   Function to get the USB DMA transfer on channel 1
 Parameters:    none
 Return value:  The value of the transfer count register
 ******************************************************************************/
unsigned long dmaGetUsbInCh1Count (void)
{
    volatile struct st_dmac_n* pdmac_block;
    uint32_t bresult;
    uint16_t int_vect; /* Vector associated with the DMA channel allocated */

    /* Get pointers to the DMA channel, using the pre-allocated channel define */
    bresult = R_DMA_Channel2Pointer(DMA_CHANNEL_USBH_IN, (void*)&pdmac_block, &int_vect);
    (void) bresult;
    /* This is only to detect transfer stop for purposes of time-out, however
     the number that is returned must be changing while the DMA working */
    return (unsigned long) (gst_lengthCh1 + (pdmac_block->CRTB_n * gwNumBlocksCh1));
}
/******************************************************************************
 End of function  dmaGetUsbInCh1Count
 ******************************************************************************/

/******************************************************************************
 Function Name: dmaStopUsbInCh1
 Description:   Function to stop a USB OUT DMA transfer on channel 1
 Parameters:    none
 Return value:  none
 ******************************************************************************/
void dmaStopUsbInCh1 (void)
{
    volatile struct st_dmac_n* pdmac_block;
    uint16_t int_vect; /* Vector associated with the DMA channel allocated */

    TRACE(("dmaStopUsbInCh1: Called\r\n"));

    /* Get pointers to the DMA channel, using the pre-allocated channel define */
    R_DMA_Channel2Pointer(DMA_CHANNEL_USBH_IN, (void*)&pdmac_block, &int_vect);

    /* Disable the DMA channel */
    pdmac_block->CHCTRL_n |= DMAC0_CHCTRL_n_CLREN;

    /* Reset channel status register DMA block */
    pdmac_block->CHCTRL_n |= DMAC0_CHCTRL_n_SWRST;

    /* Clear DMARS association */
    R_DMA_SetChannelDMARS (DMA_CHANNEL_USBH_IN, 0, 0);

    if (gpfFreeVectUsbInCh1)
    {
        gpfFreeVectUsbInCh1();
        gpfFreeVectUsbInCh1 = NULL;
    }
}
/******************************************************************************
 End of function  dmaStopUsbInCh1
 ******************************************************************************/



/******************************************************************************
 Private Functions
 ******************************************************************************/

/*****************************************************************************
 * Function Name: dmaCompleteUsbCh0Out
 * Description  : Function to complete the USB DMA out transfer
 * Arguments    : none
 * Return Value : none
 ******************************************************************************/
static void dmaCompleteUsbCh0Out (void)
{
    volatile struct st_dmac_n* pdmac_block;
    uint32_t bresult;
    uint16_t int_vect; /* Vector associated with the DMA channel allocated */

    TRACE(("dmaCompleteUsbCh0Out: Called\r\n"));

    /* Get pointers to the DMA channel, using the pre-allocated channel define */
    bresult = R_DMA_Channel2Pointer(DMA_CHANNEL_USBH_OUT, (void*)&pdmac_block, &int_vect);
    (void)bresult;
    /* Check to see if there is more data to be transferred */
    if (gst_lengthCh0)
    {
        /* Start another transfer continuing with the same source and
         destination addresses */
        dmaStartUsbOutCh0( (void*)pdmac_block->N0SA_n, gst_lengthCh0,
                           (void*)pdmac_block->N0DA_n, pv_param_ch0, gpf_complete0);
    }
    else
    {
        /* If there is a completion routine */
        if (gpf_complete0)
        {
            /* Call it */
            gpf_complete0(pv_param_ch0);
        }
    }
}
/*****************************************************************************
 End of function  dmaCompleteUsbCh0Out
 ******************************************************************************/

/*****************************************************************************
 * Function Name: dmaCompleteUsbCh1In
 * Description  : Function to complete the USB DMA in transfer
 * Arguments    : none
 * Return Value : none
 ******************************************************************************/
static void dmaCompleteUsbCh1In (void)
{
    volatile struct st_dmac_n* pdmac_block;
    uint32_t bresult;
    uint16_t int_vect; /* Vector associated with the DMA channel allocated */
   // bool_t bf_invalidate_result;
   // void *pv_src_rolling;
   // size_t st_src_len;
   // size_t st_dest_len;

    TRACE(("dmaCompleteUsbCh1In: Called\r\n"));

    /* Get pointers to the DMA channel, using the pre-allocated channel define */
    bresult = R_DMA_Channel2Pointer(DMA_CHANNEL_USBH_IN, (void*)&pdmac_block, &int_vect);
    (void)bresult;

    /*  Software Countermeasure for DMA Restrictions are not required because
     the following conditions are satisfied:
     1. DMA/DTC is in block transfer mode when DISEL = 0
     2. The number of receive blocks matches the value set in the USB
     so that another transfer request is not generated until this
     interrupt handler is vectored */
    /* Check to see if there is more data to be transferred */
    if (gst_lengthCh1)
    {
        /* Start another transfer continuing with the same source and
         destination addresses */
        dmaStartUsbInCh1( (void*)pdmac_block->N0SA_n, gst_lengthCh1,
                          (void*)pdmac_block->N0DA_n, pv_param_ch1, gpf_complete1);
    }
    else
    {
        /* If there is a completion routine */
        if (gpf_complete1)
        {
            /* Call it */
            gpf_complete1(pv_param_ch1);
        }
    }
}
/*****************************************************************************
 End of function  dmaCompleteUsbCh1In
 ******************************************************************************/

/******************************************************************************
 Function Name: prv_int_d0fifoD0FIFO0_COMPLETE
 Description:   USB_D0FIFO0 (USB OUT  - host to device) end of transfer
 interrupt
 Parameters:    none
 Return value:  none
 ******************************************************************************/
//static void prv_int_d0fifoD0FIFO0_COMPLETE (void)
//{
//    dmaCompleteUsbCh0Out();
//}
/******************************************************************************
 End of function  prv_int_d0fifoD0FIFO0_COMPLETE
 ******************************************************************************/

/******************************************************************************
 Function Name: INT_D1FIFO0_COMPLETE
 Description:   USB_D1FIFO0 (USB IN  - device to host) end of transfer
 interrupt
 Parameters:    none
 Return value:  none
 ******************************************************************************/
static void INT_D1FIFO0_COMPLETE (uint32_t dummy)
{
	(void) dummy;
    dmaCompleteUsbCh1In();
}
/******************************************************************************
 End of function  INT_D1FIFO0_COMPLETE
 ******************************************************************************/

/******************************************************************************
 Function Name: INT_D0FIFO1_COMPLETE
 Description:   USB_D0FIFO1 (USB OUT  - host to device) end of transfer
 interrupt
 Parameters:    none
 Return value:  none
 ******************************************************************************/
static void INT_D0FIFO1_COMPLETE (uint32_t dummy)
{
	(void) dummy;

    dmaCompleteUsbCh0Out();
}
/******************************************************************************
 End of function  INT_D0FIFO1_COMPLETE
 ******************************************************************************/

/******************************************************************************
 Function Name: INT_D1FIFO1_COMPLETE
 Description:   USB_D1FIFO1 (USB IN  - device to host) end of transfer
 interrupt
 Parameters:    none
 Return value:  none
 ******************************************************************************/
static void INT_D1FIFO1_COMPLETE (uint32_t dummy)
{
	UNUSED_PARAM(dummy);
    dmaCompleteUsbCh1In();
}
/******************************************************************************
 End of function  INT_D1FIFO1_COMPLETE
 ******************************************************************************/

/******************************************************************************
 * Function Name: dmaFreeVectD0FIFO0
 * Description  : Function to free the D0FIFO0 vector
 * Arguments    : none
 * Return Value : none
 ******************************************************************************/
static void dmaFreeVectD0FIFO0 (void)
{

    /* Do Nothing */

}
/******************************************************************************
 End of function  dmaFreeVectD0FIFO0
 ******************************************************************************/

/******************************************************************************
 * Function Name: dmaFreeVectD0FIFO1
 * Description  : Function to free the D0FIFO1 vector
 * Arguments    : none
 * Return Value : none
 ******************************************************************************/
static void dmaFreeVectD0FIFO1 (void)
{

    /* Do Nothing */
}
/******************************************************************************
 End of function  dmaFreeVectD0FIFO1
 ******************************************************************************/

/******************************************************************************
 * Function Name: dmaFreeVectD1FIFO0
 * Description  : Function to free the D1FIFO0 vector
 * Arguments    : none
 * Return Value : none
 ******************************************************************************/
static void dmaFreeVectD1FIFO0 (void)
{

    /* Do Nothing */
}
/******************************************************************************
 End of function  dmaFreeVectD1FIFO0
 ******************************************************************************/

/******************************************************************************
 * Function Name: dmaFreeVectD1FIFO1
 * Description  : Function to free the D1FIFO1 vector
 * Arguments    : none
 * Return Value : none
 ******************************************************************************/
static void dmaFreeVectD1FIFO1 (void)
{

    /* Do Nothing */
}
/******************************************************************************
 End of function  dmaFreeVectD1FIFO1
 ******************************************************************************/

/******************************************************************************
 End  Of File
 ******************************************************************************/

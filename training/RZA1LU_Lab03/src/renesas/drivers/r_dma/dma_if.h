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
 * and to discontinue the availability of this software. By using this
 * software, you agree to the additional terms and conditions found by
 * accessing the following link:
 * http://www.renesas.com/disclaimer
*******************************************************************************
* Copyright (C) 2018 Renesas Electronics Corporation. All rights reserved.
 *****************************************************************************/
/******************************************************************************
 * @file         dma_if.h
 * @brief        DMA Driver interface headers
 * @version        1.00
 * @date           27.06.2018
 * H/W Platform    RZ/A1LU
 *****************************************************************************/
 /*****************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 30.06.2018 1.00 First Release
 *****************************************************************************/

/* Multiple inclusion prevention macro */
#ifndef R_SW_PKG_93_DMA_IF_H_INCLUDED
#define R_SW_PKG_93_DMA_IF_H_INCLUDED
/**************************************************************************//**
 * @ingroup R_SW_PKG_93_NONOS_DRIVERS
 * @defgroup R_SW_PKG_93_DMA_API DMA RZA1LU Driver
 *
 * @anchor R_SW_PKG_93_DMA_API_SUMMARY
 * @par Summary
 *
 * This is a simple DMA developed for the RZA1LU Stream-IT board.
 *
 * This is a device and board specific implementation of an ADC driver,
 * configured for the RZA1LU Stream-IT board.
 *
 * @anchor R_SW_PKG_93_DMA_API_INSTANCES
 * @par Known Implementations:
 * This driver is used in the RZA1LU Software Package.
 * @see RENESAS_APPLICATION_SOFTWARE_PACKAGE
 * @see RENESAS_OS_ABSTRACTION  Renesas OS Abstraction interface
 * @{
 *****************************************************************************/

/******************************************************************************
Includes <System Includes> , "Project Includes"
******************************************************************************/

#include "cmsis_os.h"
#include "r_errno.h"
#include "r_typedefs.h"
#include "mcu_board_select.h"
#include "compiler_settings.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


/*************************************************************************
 User Includes
*************************************************************************/

/*************************************************************************
 Defines
*************************************************************************/

/* for searching free channel */
#define DMA_ALLOC_CH  (-1)

/***********************************************************************************
* @brief      dma channels below this will not be affected by this driver,
*             to ensure compatibility with USBH which uses channels 0,1, with
*             potential to use 2, 3 as well.
***********************************************************************************/
#define DMA_START_CHANNEL   (4)

/*************************************************************************
 Enumerated Types
*************************************************************************/

/* Number of DMA channel */
typedef enum
{
    DMA_CH_0 = 0,
    DMA_CH_1 = 1,
    DMA_CH_2 = 2,
    DMA_CH_3 = 3,
    DMA_CH_4 = 4,
    DMA_CH_5 = 5,
    DMA_CH_6 = 6,
    DMA_CH_7 = 7,
    DMA_CH_8 = 8,
    DMA_CH_9 = 9,
    DMA_CH_10 = 10,
    DMA_CH_11 = 11,
    DMA_CH_12 = 12,
    DMA_CH_13 = 13,
    DMA_CH_14 = 14,
    DMA_CH_15 = 15,
    DMA_CH_NUM = 16  /*!< Number of DMA channel */
} dma_ch_num_t;

/* Unit Size of DMA transfer */
typedef enum
{
    DMA_UNIT_MIN =(-1),
    DMA_UNIT_1 = 0,   /*!< Unit Size of DMA transfer = 1byte */
    DMA_UNIT_2 = 1,   /*!< Unit Size of DMA transfer = 2byte */
    DMA_UNIT_4 = 2,   /*!< Unit Size of DMA transfer = 4byte */
    DMA_UNIT_8 = 3,   /*!< Unit Size of DMA transfer = 8byte */
    DMA_UNIT_16 = 4,  /*!< Unit Size of DMA transfer = 16byte */
    DMA_UNIT_32 = 5,  /*!< Unit Size of DMA transfer = 32byte */
    DMA_UNIT_64 = 6,  /*!< Unit Size of DMA transfer = 64byte */
    DMA_UNIT_128 = 7, /*!< Unit Size of DMA transfer = 128byte */
    DMA_UNIT_MAX = 8
} dma_unit_size_t;

/* DMA transfer resource */
typedef enum
{
    DMA_RS_OSTIM0     = 0x023,   /*!< OS Timer ch0 */
    DMA_RS_OSTIM1     = 0x027,   /*!< OS Timer ch1 */
    DMA_RS_TGI0A      = 0x043,   /*!< Multi Function Timer Pulse Unit2 ch0 */
    DMA_RS_TGI1A      = 0x047,   /*!< Multi Function Timer Pulse Unit2 ch1  */
    DMA_RS_TGI2A      = 0x04B,   /*!< Multi Function Timer Pulse Unit2 ch2  */
    DMA_RS_TGI3A      = 0x04F,   /*!< Multi Function Timer Pulse Unit2 ch3  */
    DMA_RS_TGI4A      = 0x053,   /*!< Multi Function Timer Pulse Unit2 ch4  */
    DMA_RS_TXI0       = 0x061,   /*!< FIFO Serial Communication Interface ch0 (TX) */
    DMA_RS_RXI0       = 0x062,   /*!< FIFO Serial Communication Interface ch0 (RX) */
    DMA_RS_TXI1       = 0x065,   /*!< FIFO Serial Communication Interface ch1 (TX) */
    DMA_RS_RXI1       = 0x066,   /*!< FIFO Serial Communication Interface ch1 (RX) */
    DMA_RS_TXI2       = 0x069,   /*!< FIFO Serial Communication Interface ch2 (TX) */
    DMA_RS_RXI2       = 0x06A,   /*!< FIFO Serial Communication Interface ch2 (RX) */
    DMA_RS_TXI3       = 0x06D,   /*!< FIFO Serial Communication Interface ch3 (TX) */
    DMA_RS_RXI3       = 0x06E,   /*!< FIFO Serial Communication Interface ch3 (RX) */
    DMA_RS_TXI4       = 0x071,   /*!< FIFO Serial Communication Interface ch4 (TX) */
    DMA_RS_RXI4       = 0x072,   /*!< FIFO Serial Communication Interface ch4 (RX) */
    DMA_RS_USB0_DMA0  = 0x083,   /*!< USB Module0 ch0 */
    DMA_RS_USB0_DMA1  = 0x087,   /*!< USB Module0 ch1 */
    DMA_RS_USB1_DMA0  = 0x08B,   /*!< USB Module1 ch0 */
    DMA_RS_USB1_DMA1  = 0x08F,   /*!< USB Module1 ch1 */
    DMA_RS_ADEND      = 0x093,   /*!< A/D Converter */
    DMA_RS_SDHI_0T    = 0x0C1,   /*!< SD Host Interface0 (TX) */
    DMA_RS_SDHI_0R    = 0x0C2,   /*!< SD Host Interface0 (RX) */
    DMA_RS_SDHI_1T    = 0x0C5,   /*!< SD Host Interface1 (RX) */
    DMA_RS_SDHI_1R    = 0x0C6,   /*!< SD Host Interface1 (TX) */
    DMA_RS_MMCT       = 0x0C9,   /*!< MMC Host Interface (TX) */
    DMA_RS_MMCR       = 0x0CA,   /*!< MMC Host Interface (RX) */
    DMA_RS_SSITXI0    = 0x0E1,   /*!< SSIF0 (TX) */
    DMA_RS_SSIRXI0    = 0x0E2,   /*!< SSIF0 (RX) */
    DMA_RS_SSITXI1    = 0x0E5,   /*!< SSIF1 (TX) */
    DMA_RS_SSIRXI1    = 0x0E6,   /*!< SSIF1 (RX) */
    DMA_RS_SSIRTI2    = 0x0EB,   /*!< SSIF2 (TX) */
    DMA_RS_SSITXI3    = 0x0ED,   /*!< SSIF2 (RTX) */
    DMA_RS_SSIRXI3    = 0x0EE,   /*!< SSIF3 (TX) */
    DMA_RS_SCUTXI0    = 0x101,   /*!< SCUX (FFD0) */
    DMA_RS_SCURXI0    = 0x102,   /*!< SCUX (FFU0) */
    DMA_RS_SCUTXI1    = 0x105,   /*!< SCUX (FFD1) */
    DMA_RS_SCURXI1    = 0x106,   /*!< SCUX (FFU1) */
    DMA_RS_SCUTXI2    = 0x109,   /*!< SCUX (FFD2) */
    DMA_RS_SCURXI2    = 0x10A,   /*!< SCUX (FFU2) */
    DMA_RS_SCUTXI3    = 0x10D,   /*!< SCUX (FFD3) */
    DMA_RS_SCURXI3    = 0x10E,   /*!< SCUX (FFU3) */
    DMA_RS_SPTI0      = 0x121,   /*!< SPI0 (TX) */
    DMA_RS_SPRI0      = 0x122,   /*!< SPI0 (RX) */
    DMA_RS_SPTI1      = 0x125,   /*!< SPI1 (TX) */
    DMA_RS_SPRI1      = 0x126,   /*!< SPI1 (RX) */
    DMA_RS_SPTI2      = 0x129,   /*!< SPI2 (TX) */
    DMA_RS_SPRI2      = 0x12A,   /*!< SPI2 (RX) */
    DMA_RS_SPDIFTXI   = 0x141,   /*!< SPDIF (TX) */
    DMA_RS_SPDIFRXI   = 0x142,   /*!< SPDIF (RX) */
    DMA_RS_SCITXI0    = 0x169,   /*!< Serial Communication Interface ch0 (TX) */
    DMA_RS_SCIRXI0    = 0x16A,   /*!< Serial Communication Interface ch0 (RX) */
    DMA_RS_SCITXI1    = 0x16D,   /*!< Serial Communication Interface ch1 (TX) */
    DMA_RS_SCIRXI1    = 0x16E,   /*!< Serial Communication Interface ch1 (RX) */
    DMA_RS_TI0        = 0x181,   /*!< IIC ch0 (TX) */
    DMA_RS_RI0        = 0x182,   /*!< IIC ch0 (RX) */
    DMA_RS_TI1        = 0x185,   /*!< IIC ch1 (TX) */
    DMA_RS_RI1        = 0x186,   /*!< IIC ch1 (RX) */
    DMA_RS_TI2        = 0x189,   /*!< IIC ch2 (TX) */
    DMA_RS_RI2        = 0x18A,   /*!< IIC ch2 (RX) */
    DMA_RS_TI3        = 0x18D,   /*!< IIC ch3 (TX) */
    DMA_RS_RI3        = 0x18E,   /*!< IIC ch3 (RX) */

	DMA_MEM_2_MEM     = 0x001	 /*!< memory to memory */
} dma_res_select_t;

/* DMA transfer direction */
typedef enum
{
    DMA_REQ_MIN =(-1),
    DMA_REQ_SRC = 0, /*!< Read DMA */
    DMA_REQ_DES = 1, /*!< Write DMA */
    DMA_REQ_MAX = 2
} dma_req_dir_t;

/* Address count direction */
typedef enum
{
    DMA_ADDR_MIN = (-1),
    DMA_ADDR_INCREMENT = 0, /*!< Address Count Increment */
    DMA_ADDR_FIX = 1,       /*!< Address Count Fix */
    DMA_ADDR_MAX = 2
} dma_addr_cnt_t;


/*************************************************************************
 Structures
*************************************************************************/

/* DMA Init parameter */
typedef struct
{
    bool_t   channel[DMA_CH_NUM]; /*!< Set enable channel */
    AIOCB    *p_aio;              /*!< set callback function (DMA error interrupt) */
}dma_drv_init_t;

/* DMA Setup parameter */
typedef struct
{
    dma_res_select_t   resource;     /*!< DMA Transfer Resource */
    dma_req_dir_t      direction;    /*!< DMA Transfer Direction */
    dma_unit_size_t    dst_width;    /*!< DMA Transfer Unit Size (Destination) */
    dma_unit_size_t    src_width;    /*!< DMA Transfer Unit Size (Source) */
    dma_addr_cnt_t     dst_cnt;      /*!< DMA Address Count (Destination) */
    dma_addr_cnt_t     src_cnt;      /*!< DMA Address Count (Source) */
    AIOCB              *p_aio;       /*!< set callback function (DMA end interrupt) */
} dma_ch_setup_t;

/* DMA Transfer @paramter */
typedef struct
{
    void       *src_addr;    /*!< Sorce Address */
    void       *dst_addr;    /*!< Destination Address */
    uint32_t   count;        /*!< DMA Transfer Size */
} dma_trans_data_t;

/***********************************************************************************
 Function Prototypes
***********************************************************************************/

/***********************************************************************************
* @brief      This function initializes the driver and must be called at system
*             start up, prior to any required DMA functionality being available.
*             This function also sets the enable or disable for each DMA channel
*             and DMA error call back function.
*
*
* @param[in]      p_dma_init_param: parameter of ch enable
*                                   and DMA error callback function.
* @param[in/out]  p_errno: get error code.
*                          (when p_errno is NULL, erroc code isn't set.)
*
* @retval         IOIF_ESUCCESS: Successfully initialized.
* @retval         -1:            Error occurred.
***********************************************************************************/

extern int_t R_DMA_Init(const dma_drv_init_t * const p_dma_init_param, int32_t * const p_errno);

/***********************************************************************************
* @brief      This function shutdown the driver, making DMA functionality
*             is no longer available. It can be carried out only in calls
*             of all channel free.
*
* @param[in/out] p_errno: Get error code.
*                          (when p_errno is NULL, erroc code isn't set.)
*
* @retval          IOIF_ESUCCESS: Successfully uninitialized.
* @retval          -1: Error occurred.
***********************************************************************************/

extern int_t R_DMA_UnInit(int32_t * const p_errno);

/***********************************************************************************
* @brief       This function allocates a DMA channel.
*              When channel is (-1), it looking for a free channel
*              When set channel to DMA channel number, a set channel is allocated
*
* @param[in] channel:  allocate channel.
*                       (when channel is (-1), it looking for a free channel.)
* @param[in/out] p_errno: Get error code.
*                          (when p_errno is NULL, erroc code isn't set.)
*
* @retval     channel_number:  Successfully allocated.
* @retval     -1:              Error occurred.
***********************************************************************************/

extern int_t R_DMA_Alloc(const int_t channel, int32_t * const p_errno);

/***********************************************************************************
* @brief       This function close a DMA channel.
*
* @param[in] channel:      close channel.
* @param[in/out] p_errno:  get error code. (when p_errno is NULL, erroc code isn't set.)
*
* retval IOIF_ESUCCESS:    Successfully allocate.
* retval -1:               Error occurred.
***********************************************************************************/

extern int_t R_DMA_Free(const int_t channel, int32_t * const p_errno);

/***********************************************************************************
* @brief       This function set up a DMA transfer parameter.
*              before calling R_DMA_Start(), please carry out this function.
*
* @param[in] channel     - set up channel.
* @param[in] p_ch_setup  - DMA transfer parameters.
* @param[in/out] p_errno - get error code. (when p_errno is NULL, erroc code isn't set.)
*
* @retval IOIF_ESUCCESS:   Successfully setup.
* @retval -1:              Error occurred.
***********************************************************************************/

extern int_t R_DMA_Setup(const int_t channel, const dma_ch_setup_t * const p_ch_setup,
                         int32_t * const p_errno);

/***********************************************************************************
* @brief       This function set up a DMA transfer address and start DMA.
*
* @param[in] channel:      DMA start channel.
* @param[in] p_ch_setup:   DMA address parameters.
* @param[in/out] p_errno:  Get error code.
*                          (when p_errno is NULL, erroc code isn't set.)
*
* @retval IOIF_ESUCCESS:  Successfully DMA start.
* @retval -1:             Error occurred.
***********************************************************************************/

extern int_t R_DMA_Start(const int_t channel, const dma_trans_data_t * const p_dma_data,
                         int32_t * const p_errno);

/***********************************************************************************
* @brief        This function set up a continuous DMA transfer address
*               and start continuous DMA.
*
* @param[in] channel:      Continuous DMA start channel.
* @param[in] p_ch_setup:   Continuous DMA address parameters.
* @param[in/out] p_errno:  Get error code.
*                          (when p_errno is NULL, erroc code isn't set.)
*
* @retval IOIF_ESUCCESS: Successfully continuous DMA start.
* @retval -1:            Error occurred.
***********************************************************************************/

extern int_t R_DMA_NextData(const int_t channel, const dma_trans_data_t * const p_dma_data,
                            int32_t * const p_errno);

/***********************************************************************************
* @brief       This function cancel DMA transfer.
*              Continous DMA also stops at the same time.
*              Please call this function during DMA transfer.
*
* @param[in] channel:      Chancel DMA start channel.
* @param[out] p_remain:    Remain size of DMA transfer.
* @param[in/out] p_errno:  Get error code. (when p_errno is NULL, erroc code isn't set.)
*
* @retval IOIF_ESUCCESS:   Successfully cancel.
* @retval -1:              Error occurred.
***********************************************************************************/

extern int_t R_DMA_Cancel(const int_t channel, uint32_t * const p_remain, int32_t * const p_errno);


/***********************************************************************************
* @brief This function get DMA driver version.
*
* @retval driver_version: Current version of driver.
***********************************************************************************/

extern uint16_t R_DMA_GetVersion(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* R_SW_PKG_93_DMA_IF_H_INCLUDED */
/**************************************************************************//**
 * @} (end addtogroup)
 *****************************************************************************/

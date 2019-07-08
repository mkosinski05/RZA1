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
 * @headerfile     r_adc.h
 * @brief          Simple interface for ADC AN0 connected to board
 * @version        1.00
 * @date           27.06.2018
 * H/W Platform    RZ/A1LU
 *****************************************************************************/
 /*****************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 30.06.2018 1.00 First Release
 *****************************************************************************/

/* Multiple inclusion prevention macro */
#ifndef R_SW_PKG_93_ADC_API_H_INCLUDED
#define R_SW_PKG_93_ADC_API_H_INCLUDED
/**************************************************************************//**
 * @ingroup R_SW_PKG_93_NONOS_DRIVERS
 * @defgroup R_SW_PKG_93_ADC_API ADC RZA1LU Driver
 *
 * @anchor R_SW_PKG_93_ADC_API_SUMMARY
 * @par Summary
 *
 * This is a simple ADC Low Level Driver developed for the RZA1LU Stream-IT
 * board.
 *
 * This is a device and board specific implementation of an ADC driver,
 * configured for the RZA1LU Stream-IT board.
 *
 * @anchor R_SW_PKG_93_ADC_API_INSTANCES
 * @par Known Implementations:
 * This driver is used in the RZA1LU Software Package.
 * @see RENESAS_APPLICATION_SOFTWARE_PACKAGE
 *
 * @anchor R_SW_PKG_93_ADC_API_SEE_ALSO

 * @see RENESAS_OS_ABSTRACTION  Renesas OS Abstraction interface
 * @{
 *****************************************************************************/

/******************************************************************************
Variable Externs
******************************************************************************/
/* ADC result storage */
uint32_t g_adc_result;

/******************************************************************************
Functions Prototypes
******************************************************************************/

/**
 * @brief Enables and configures AN0 ADC channel.
 *
 * @return None.
 */
extern void R_ADC_Open(void);

/**
 * @brief Disables the ADC
 *
 * @return None.
 */
extern void R_ADC_Close(void);

/**
 * @brief Read value from AN0
 *
 * @return None.
 */
extern uint16_t R_ADC_Read(void);

/* ADC_H*/
#endif
/**************************************************************************//**
 * @} (end addtogroup)
 *****************************************************************************/
/* End of File */


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
* Copyright (C) 2013-2017 Renesas Electronics Corporation. All rights reserved.
*******************************************************************************/

/*******************************************************************************
* File Name   : ssif_dma.c
* $Rev: 8500 $
* $Date:: 2018-06-18 16:55:54 +0100#$
* Description : SSIF driver DMA functions
******************************************************************************/

/*******************************************************************************
Includes <System Includes>, "Project Includes"
*******************************************************************************/
#include "ssif.h"
#include "ssif_int.h"
#include "dma_if.h"
#include "fcntl.h"
#include "Renesas_RZ_A1.h"
#include "mcu_board_select.h"

/*******************************************************************************
Macro definitions
*******************************************************************************/
#define SSIF_DUMMY_DMA_BUF_SIZE (4096u)

/*
 * The common multiple of 2, 4, 6, 8, 12, 24 and 32
 * which are all sampling sizes
 */
#define SSIF_DUMMY_DMA_TRN_SIZE (4032u)

#if (TARGET_RZA1 <= TARGET_RZA1LU)
#else /* TARGET_RZA1H */
#define SSIF_ROMDEC_DMA_SIZE    (2352u)
#endif /* (TARGET_RZA1 <= TARGET_RZA1LU) */

/*******************************************************************************
Typedef definitions
*******************************************************************************/

/*******************************************************************************
Exported global variables (to be accessed by other files)
*******************************************************************************/

/*******************************************************************************
Private global variables and functions
*******************************************************************************/

static void SSIF_DMA_TxCallback(union sigval param);
static void SSIF_DMA_RxCallback(union sigval param);

static const dma_res_select_t gb_ssif_dma_tx_resource[SSIF_NUM_CHANS] =
{
    DMA_RS_SSITXI0,
    DMA_RS_SSITXI1,
    DMA_RS_SSIRTI2,
#if (TARGET_RZA1 <= TARGET_RZA1LU)
    DMA_RS_SSITXI3
#else /* TARGET_RZA1H */
    DMA_RS_SSITXI3,
    DMA_RS_SSIRTI4,
    DMA_RS_SSITXI5
#endif /* (TARGET_RZA1 <= TARGET_RZA1LU) */
};

static const dma_res_select_t gb_ssif_dma_rx_resource[SSIF_NUM_CHANS] =
{
    DMA_RS_SSIRXI0,
    DMA_RS_SSIRXI1,
    DMA_RS_SSIRTI2,
#if (TARGET_RZA1 <= TARGET_RZA1LU)
    DMA_RS_SSIRXI3
#else /* TARGET_RZA1H */
    DMA_RS_SSIRXI3,
    DMA_RS_SSIRTI4,
    DMA_RS_SSIRXI5
#endif /* (TARGET_RZA1 <= TARGET_RZA1LU) */
};

static AIOCB gb_ssif_dma_tx_end_aiocb[SSIF_NUM_CHANS];
static AIOCB gb_ssif_dma_rx_end_aiocb[SSIF_NUM_CHANS];

static dma_trans_data_t gb_ssif_txdma_dummy_trparam[SSIF_NUM_CHANS];
static dma_trans_data_t gb_ssif_rxdma_dummy_trparam[SSIF_NUM_CHANS];

static uint32_t ssif_tx_dummy_buf[SSIF_DUMMY_DMA_BUF_SIZE / sizeof(uint32_t)];
static uint32_t ssif_rx_dummy_buf[SSIF_DUMMY_DMA_BUF_SIZE / sizeof(uint32_t)];

/******************************************************************************
* Function Name: SSIF_InitDMA
* @brief         Allocate and Setup DMA_CH for specified SSIF channel.
*
*                Description:<br>
*
* @param[in,out] p_info_ch  :channel object.
* @retval        IOIF_ESUCCESS   :Success.
* @retval        error code :Failure.
******************************************************************************/
int_t SSIF_InitDMA(ssif_info_ch_t* const p_info_ch)
{
    int_t ercd = IOIF_ESUCCESS;
    int_t dma_ret;
    uint32_t ssif_ch;
    int32_t dma_ercd;
    dma_ch_setup_t  dma_ch_setup;

    if (NULL == p_info_ch)
    {
        ercd = IOIF_EFAULT;
    }
    else
    {
        ssif_ch = p_info_ch->channel;

        /* allocate DMA Channel for write(if necessary) */
        if (IOIF_ESUCCESS == ercd)
        {
            if (O_RDONLY == p_info_ch->openflag)
            {
                p_info_ch->dma_tx_ch = -1;
            }
            else
            {
                dma_ret = R_DMA_Alloc(DMA_ALLOC_CH, &dma_ercd);

                if (IOIF_EERROR == dma_ret)
                {
                    p_info_ch->dma_tx_ch = -1;
                    ercd = IOIF_ENOMEM;
                }
                else
                {
                    p_info_ch->dma_tx_ch = dma_ret;
                    ercd = IOIF_ESUCCESS;
                }
            }
        }

        /* allocate DMA Channel for read(if necessary) */
        if (IOIF_ESUCCESS == ercd)
        {
            if (O_WRONLY == p_info_ch->openflag)
            {
                p_info_ch->dma_rx_ch = -1;
            }
            else
            {
                dma_ret = R_DMA_Alloc(DMA_ALLOC_CH, &dma_ercd);

                if (IOIF_EERROR == dma_ret)
                {
                    p_info_ch->dma_rx_ch = -1;
                    ercd = IOIF_ENOMEM;
                }
                else
                {
                    p_info_ch->dma_rx_ch = dma_ret;
                    ercd = IOIF_ESUCCESS;
                }
            }
        }

        /* setup DMA channel for write(if necessary) */
        if (IOIF_ESUCCESS == ercd)
        {
            if (O_RDONLY != p_info_ch->openflag)
            {
                AIOCB* const p_tx_aio = &gb_ssif_dma_tx_end_aiocb[ssif_ch];

                p_tx_aio->aio_sigevent.sigev_notify = 0;
                p_tx_aio->aio_sigevent.sigev_notify = SIGEV_THREAD;
                p_tx_aio->aio_sigevent.sigev_value.sival_ptr = (void*)p_info_ch;
                p_tx_aio->aio_sigevent.sigev_notify_function = &SSIF_DMA_TxCallback;

                dma_ch_setup.resource = gb_ssif_dma_tx_resource[ssif_ch];
                dma_ch_setup.direction = DMA_REQ_DES;
                dma_ch_setup.dst_width = DMA_UNIT_4;
                dma_ch_setup.src_width = DMA_UNIT_4;
                dma_ch_setup.dst_cnt = DMA_ADDR_FIX;
                dma_ch_setup.src_cnt = DMA_ADDR_INCREMENT;
                dma_ch_setup.p_aio = p_tx_aio;

                dma_ret = R_DMA_Setup(p_info_ch->dma_tx_ch, &dma_ch_setup, &dma_ercd);

                if (IOIF_EERROR == dma_ret)
                {
                    ercd = IOIF_EFAULT;
                }
            }
        }

        /* setup DMA channel for read(if necessary) */
        if (IOIF_ESUCCESS == ercd)
        {
            if (O_WRONLY != p_info_ch->openflag)
            {
                AIOCB* const p_rx_aio = &gb_ssif_dma_rx_end_aiocb[ssif_ch];
                p_rx_aio->aio_sigevent.sigev_notify = SIGEV_THREAD;
                p_rx_aio->aio_sigevent.sigev_value.sival_ptr = (void*)p_info_ch;
                p_rx_aio->aio_sigevent.sigev_notify_function = &SSIF_DMA_RxCallback;

                dma_ch_setup.resource = gb_ssif_dma_rx_resource[ssif_ch];
                dma_ch_setup.direction = DMA_REQ_SRC;
                dma_ch_setup.dst_width = DMA_UNIT_4;
                dma_ch_setup.src_width = DMA_UNIT_4;
                dma_ch_setup.src_cnt = DMA_ADDR_FIX;
                dma_ch_setup.p_aio = p_rx_aio;
#if (TARGET_RZA1 <= TARGET_RZA1LU)
                dma_ch_setup.dst_cnt = DMA_ADDR_INCREMENT;
#else /* TARGET_RZA1H */
                if (SSIF_CFG_ENABLE_ROMDEC_DIRECT
                    != p_info_ch->romdec_direct.mode)
                {
                    dma_ch_setup.dst_cnt = DMA_ADDR_INCREMENT;
                }
                else
                {
                    dma_ch_setup.dst_cnt = DMA_ADDR_FIX;
                }
#endif /* (TARGET_RZA1 <= TARGET_RZA1LU) */

                dma_ret = R_DMA_Setup(p_info_ch->dma_rx_ch, &dma_ch_setup, &dma_ercd);

                if (IOIF_EERROR == dma_ret)
                {
                    ercd = IOIF_EFAULT;
                }
            }
        }

        /* start DMA dummy transfer for write(if necessary) */
        if (IOIF_ESUCCESS == ercd)
        {
            if (O_RDONLY != p_info_ch->openflag)
            {
                /* setup short dummy transfer */
                gb_ssif_txdma_dummy_trparam[ssif_ch].src_addr = (void*)&ssif_tx_dummy_buf[0];
                gb_ssif_txdma_dummy_trparam[ssif_ch].dst_addr = (void*)&g_ssireg[ssif_ch]->SSIFTDR;
                gb_ssif_txdma_dummy_trparam[ssif_ch].count = SSIF_DUMMY_DMA_TRN_SIZE;

                dma_ret = R_DMA_NextData(p_info_ch->dma_tx_ch, &gb_ssif_txdma_dummy_trparam[ssif_ch], &dma_ercd);
                if (IOIF_EERROR == dma_ret)
                {
                    ercd = IOIF_EFAULT;
                }
                else
                {
                    dma_ret = R_DMA_Start(p_info_ch->dma_tx_ch, &gb_ssif_txdma_dummy_trparam[ssif_ch], &dma_ercd);
                    if (IOIF_EERROR == dma_ret)
                    {
                        ercd = IOIF_EFAULT;
                    }
                }
            }
        }

        /* start DMA dummy transfer for read(if necessary) */
        if (IOIF_ESUCCESS == ercd)
        {
            if (O_WRONLY != p_info_ch->openflag)
            {
#if (TARGET_RZA1 <= TARGET_RZA1LU)
                /* setup short dummy transfer */
                gb_ssif_rxdma_dummy_trparam[ssif_ch].src_addr = (void*)&g_ssireg[ssif_ch]->SSIFRDR;
                gb_ssif_rxdma_dummy_trparam[ssif_ch].dst_addr = (void*)&ssif_rx_dummy_buf[0];
                gb_ssif_rxdma_dummy_trparam[ssif_ch].count = SSIF_DUMMY_DMA_TRN_SIZE;
#else /* TARGET_RZA1H */
                if (SSIF_CFG_ENABLE_ROMDEC_DIRECT
                    != p_info_ch->romdec_direct.mode)
                {
                    /* setup short dummy transfer */
                    gb_ssif_rxdma_dummy_trparam[ssif_ch].src_addr = (void*)&g_ssireg[ssif_ch]->SSIFRDR;
                    gb_ssif_rxdma_dummy_trparam[ssif_ch].dst_addr = (void*)&ssif_rx_dummy_buf[0];
                    gb_ssif_rxdma_dummy_trparam[ssif_ch].count = SSIF_DUMMY_DMA_TRN_SIZE;
                }
                else
                {
                    /* setup ROMDEC direct input transfer */
                    gb_ssif_rxdma_dummy_trparam[ssif_ch].src_addr = (void*)&g_ssireg[ssif_ch]->SSIFRDR;
                    gb_ssif_rxdma_dummy_trparam[ssif_ch].dst_addr = (void*)&ROMDEC.STRMDIN0;
                    gb_ssif_rxdma_dummy_trparam[ssif_ch].count = SSIF_ROMDEC_DMA_SIZE;
                }
#endif /* (TARGET_RZA1 <= TARGET_RZA1LU) */

                dma_ret = R_DMA_NextData(p_info_ch->dma_rx_ch, &gb_ssif_rxdma_dummy_trparam[ssif_ch], &dma_ercd);
                if (IOIF_EERROR == dma_ret)
                {
                    ercd = IOIF_EFAULT;
                }
                else
                {
                    dma_ret = R_DMA_Start(p_info_ch->dma_rx_ch, &gb_ssif_rxdma_dummy_trparam[ssif_ch], &dma_ercd);
                    if (IOIF_EERROR == dma_ret)
                    {
                        ercd = IOIF_EFAULT;
                    }
                }
            }
        }

        /* enable ssif transfer */
        if (IOIF_ESUCCESS == ercd)
        {
            /* clear status and enable error interrupt */
            SSIF_EnableErrorInterrupt(ssif_ch);

            /* enable end interrupt */
            g_ssireg[ssif_ch]->SSIFCR |= SSIF_FCR_BIT_TIE | SSIF_FCR_BIT_RIE;

            if (O_RDWR == p_info_ch->openflag)
            {
                /* start write and read DMA at the same time */
                g_ssireg[ssif_ch]->SSICR  |= SSIF_CR_BIT_TEN | SSIF_CR_BIT_REN;
            }
            else if (O_WRONLY == p_info_ch->openflag)
            {
                /* start write DMA only */
                g_ssireg[ssif_ch]->SSICR  |= SSIF_CR_BIT_TEN;
            }
            else if (O_RDONLY == p_info_ch->openflag)
            {
                /* start read DMA only */
                g_ssireg[ssif_ch]->SSICR  |= SSIF_CR_BIT_REN;
            }
            else
            {
                ercd = IOIF_EINVAL;
            }
        }

        /* cleanup dma resources when error occured */
        if (IOIF_ESUCCESS != ercd)
        {
            if (-1 != p_info_ch->dma_tx_ch)
            {
                uint32_t remain;
                dma_ret = R_DMA_Cancel(p_info_ch->dma_tx_ch, &remain, &dma_ercd);
                if (IOIF_EERROR == dma_ret)
                {
                    /* NON_NOTICE_ASSERT: unexpected dma error */
                }
            }

            if (-1 != p_info_ch->dma_rx_ch)
            {
                uint32_t remain;
                dma_ret = R_DMA_Cancel(p_info_ch->dma_rx_ch, &remain, &dma_ercd);
                if (IOIF_EERROR == dma_ret)
                {
                    /* NON_NOTICE_ASSERT: unexpected dma error */
                }
            }

            if (-1 != p_info_ch->dma_tx_ch)
            {
                dma_ret = R_DMA_Free(p_info_ch->dma_tx_ch, &dma_ercd);
                if (IOIF_EERROR == dma_ret)
                {
                    /* NON_NOTICE_ASSERT: unexpected dma error */
                }
                p_info_ch->dma_tx_ch = -1;
            }

            if (-1 != p_info_ch->dma_rx_ch)
            {
                dma_ret = R_DMA_Free(p_info_ch->dma_rx_ch, &dma_ercd);
                if (IOIF_EERROR == dma_ret)
                {
                    /* NON_NOTICE_ASSERT: unexpected dma error */
                }
                p_info_ch->dma_rx_ch = -1;
            }
        }
    }

    return ercd;
}

/******************************************************************************
* Function Name: SSIF_UnInitDMA
* @brief         Free DMA_CH for specified SSIF channel.
*
*                Description:<br>
*
* @param[in,out] p_info_ch  :channel object.
* @retval        none
******************************************************************************/
void SSIF_UnInitDMA(ssif_info_ch_t* const p_info_ch)
{
    int_t dma_ret;
    int32_t dma_ercd;

    if (NULL == p_info_ch)
    {
        /* NON_NOTICE_ASSERT: illegal pointer */
    }
    else
    {
        if (-1 != p_info_ch->dma_tx_ch)
        {
            uint32_t remain;
            dma_ret = R_DMA_Cancel(p_info_ch->dma_tx_ch, &remain, &dma_ercd);
            if (IOIF_EERROR == dma_ret)
            {
                /* NON_NOTICE_ASSERT: unexpected dma error */
            }
        }

        if (-1 != p_info_ch->dma_rx_ch)
        {
            uint32_t remain;
            dma_ret = R_DMA_Cancel(p_info_ch->dma_rx_ch, &remain, &dma_ercd);
            if (IOIF_EERROR == dma_ret)
            {
                /* NON_NOTICE_ASSERT: unexpected dma error */
            }
        }

        if (-1 != p_info_ch->dma_tx_ch)
        {
            dma_ret = R_DMA_Free(p_info_ch->dma_tx_ch, &dma_ercd);
            if (IOIF_EERROR == dma_ret)
            {
                /* NON_NOTICE_ASSERT: unexpected dma error */
            }
            p_info_ch->dma_tx_ch = -1;
        }

        if (-1 != p_info_ch->dma_rx_ch)
        {
            dma_ret = R_DMA_Free(p_info_ch->dma_rx_ch, &dma_ercd);
            if (IOIF_EERROR == dma_ret)
            {
                /* NON_NOTICE_ASSERT: unexpected dma error */
            }
            p_info_ch->dma_rx_ch = -1;
        }
    }

    return;
}

/******************************************************************************
* Function Name: SSIF_RestartDMA
* @brief         Setup DMA_CH for specified SSIF channel(without allocate)
*
*                Description:<br>
*
* @param[in,out] p_info_ch  :channel object.
* @retval        IOIF_ESUCCESS   :Success.
* @retval        error code :Failure.
******************************************************************************/
int_t SSIF_RestartDMA(ssif_info_ch_t* const p_info_ch)
{
    int_t ercd = IOIF_ESUCCESS;
    int_t dma_ret;
    uint32_t ssif_ch;
    int32_t dma_ercd;
    dma_ch_setup_t  dma_ch_setup;

    if (NULL == p_info_ch)
    {
        ercd = IOIF_EFAULT;
    }
    else
    {
        ssif_ch = p_info_ch->channel;

        /* setup DMA channel for write(if necessary) */
        if (IOIF_ESUCCESS == ercd)
        {
            if (O_RDONLY != p_info_ch->openflag)
            {
                AIOCB* const p_tx_aio = &gb_ssif_dma_tx_end_aiocb[ssif_ch];
                p_tx_aio->aio_sigevent.sigev_notify = SIGEV_THREAD;
                p_tx_aio->aio_sigevent.sigev_value.sival_ptr = (void*)p_info_ch;
                p_tx_aio->aio_sigevent.sigev_notify_function = &SSIF_DMA_TxCallback;

                dma_ch_setup.resource = gb_ssif_dma_tx_resource[ssif_ch];
                dma_ch_setup.direction = DMA_REQ_DES;
                dma_ch_setup.dst_width = DMA_UNIT_4;
                dma_ch_setup.src_width = DMA_UNIT_4;
                dma_ch_setup.dst_cnt = DMA_ADDR_FIX;
                dma_ch_setup.src_cnt = DMA_ADDR_INCREMENT;
                dma_ch_setup.p_aio = p_tx_aio;

                dma_ret = R_DMA_Setup(p_info_ch->dma_tx_ch, &dma_ch_setup, &dma_ercd);

                if (IOIF_EERROR == dma_ret)
                {
                    ercd = IOIF_EFAULT;
                }
            }
        }

        /* setup DMA channel for read(if necessary) */
        if (IOIF_ESUCCESS == ercd)
        {
            if (O_WRONLY != p_info_ch->openflag)
            {
                AIOCB* const p_rx_aio = &gb_ssif_dma_rx_end_aiocb[ssif_ch];
                p_rx_aio->aio_sigevent.sigev_notify = SIGEV_THREAD;
                p_rx_aio->aio_sigevent.sigev_value.sival_ptr = (void*)p_info_ch;
                p_rx_aio->aio_sigevent.sigev_notify_function = &SSIF_DMA_RxCallback;

                dma_ch_setup.resource = gb_ssif_dma_rx_resource[ssif_ch];
                dma_ch_setup.direction = DMA_REQ_SRC;
                dma_ch_setup.dst_width = DMA_UNIT_4;
                dma_ch_setup.src_width = DMA_UNIT_4;
                dma_ch_setup.dst_cnt = DMA_ADDR_INCREMENT;
                dma_ch_setup.src_cnt = DMA_ADDR_FIX;
                dma_ch_setup.p_aio = p_rx_aio;

                dma_ret = R_DMA_Setup(p_info_ch->dma_rx_ch, &dma_ch_setup, &dma_ercd);

                if (IOIF_EERROR == dma_ret)
                {
                    ercd = IOIF_EFAULT;
                }
            }
        }

        /* start DMA dummy transfer for write(if necessary) */
        if (IOIF_ESUCCESS == ercd)
        {
            if (O_RDONLY != p_info_ch->openflag)
            {
                /* setup short dummy transfer */
                gb_ssif_txdma_dummy_trparam[ssif_ch].src_addr = (void*)&ssif_tx_dummy_buf[0];
                gb_ssif_txdma_dummy_trparam[ssif_ch].dst_addr = (void*)&g_ssireg[ssif_ch]->SSIFTDR;
                gb_ssif_txdma_dummy_trparam[ssif_ch].count = SSIF_DUMMY_DMA_TRN_SIZE;

                dma_ret = R_DMA_NextData(p_info_ch->dma_tx_ch, &gb_ssif_txdma_dummy_trparam[ssif_ch], &dma_ercd);
                if (IOIF_EERROR == dma_ret)
                {
                    ercd = IOIF_EFAULT;
                }
                else
                {
                    dma_ret = R_DMA_Start(p_info_ch->dma_tx_ch, &gb_ssif_txdma_dummy_trparam[ssif_ch], &dma_ercd);
                    if (IOIF_EERROR == dma_ret)
                    {
                        ercd = IOIF_EFAULT;
                    }
                }
            }
        }

        /* start DMA dummy transfer for read(if necessary) */
        if (IOIF_ESUCCESS == ercd)
        {
            if (O_WRONLY != p_info_ch->openflag)
            {
                /* setup short dummy transfer */
                gb_ssif_rxdma_dummy_trparam[ssif_ch].src_addr = (void*)&g_ssireg[ssif_ch]->SSIFRDR;
                gb_ssif_rxdma_dummy_trparam[ssif_ch].dst_addr = (void*)&ssif_rx_dummy_buf[0];
                gb_ssif_rxdma_dummy_trparam[ssif_ch].count = SSIF_DUMMY_DMA_TRN_SIZE;

                dma_ret = R_DMA_NextData(p_info_ch->dma_rx_ch, &gb_ssif_rxdma_dummy_trparam[ssif_ch], &dma_ercd);
                if (IOIF_EERROR == dma_ret)
                {
                    ercd = IOIF_EFAULT;
                }
                else
                {
                    dma_ret = R_DMA_Start(p_info_ch->dma_rx_ch, &gb_ssif_rxdma_dummy_trparam[ssif_ch], &dma_ercd);
                    if (IOIF_EERROR == dma_ret)
                    {
                        ercd = IOIF_EFAULT;
                    }
                }
            }
        }

        /* enable ssif transfer */
        if (IOIF_ESUCCESS == ercd)
        {
            /* clear status and enable error interrupt */
            SSIF_EnableErrorInterrupt(ssif_ch);

            /* enable end interrupt */
            g_ssireg[ssif_ch]->SSIFCR |= SSIF_FCR_BIT_TIE | SSIF_FCR_BIT_RIE;

            if (O_RDWR == p_info_ch->openflag)
            {
                /* start write and read DMA at the same time */
                g_ssireg[ssif_ch]->SSICR  |= SSIF_CR_BIT_TEN | SSIF_CR_BIT_REN;
            }
            else if (O_WRONLY == p_info_ch->openflag)
            {
                /* start write DMA only */
                g_ssireg[ssif_ch]->SSICR  |= SSIF_CR_BIT_TEN;
            }
            else if (O_RDONLY == p_info_ch->openflag)
            {
                /* start read DMA only */
                g_ssireg[ssif_ch]->SSICR  |= SSIF_CR_BIT_REN;
            }
            else
            {
                ercd = IOIF_EINVAL;
            }
        }
    }

    return ercd;
}

/******************************************************************************
* Function Name: SSIF_CancelDMA
* @brief         Pause DMA transfer for specified SSIF channel.
*
*                Description:<br>
*
* @param[in,out] p_info_ch  :channel object.
* @retval        none
******************************************************************************/
void SSIF_CancelDMA(const ssif_info_ch_t* const p_info_ch)
{
    int_t dma_ret;
    int32_t dma_ercd;

    if (NULL == p_info_ch)
    {
        /* NON_NOTICE_ASSERT: illegal pointer */
    }
    else
    {
        if (-1 != p_info_ch->dma_tx_ch)
        {
            uint32_t remain;
            dma_ret = R_DMA_Cancel(p_info_ch->dma_tx_ch, &remain, &dma_ercd);
            if (IOIF_EERROR == dma_ret)
            {
                /* NON_NOTICE_ASSERT: unexpected dma error */
            }
        }

        if (-1 != p_info_ch->dma_rx_ch)
        {
            uint32_t remain;
            dma_ret = R_DMA_Cancel(p_info_ch->dma_rx_ch, &remain, &dma_ercd);
            if (IOIF_EERROR == dma_ret)
            {
                /* NON_NOTICE_ASSERT: unexpected dma error */
            }
        }
    }

    return;
}

/******************************************************************************
Private functions
******************************************************************************/

/******************************************************************************
* Function Name: SSIF_DMA_TxCallback
* @brief         DMA callback function
*
*                Description:<br>
*
* @param[in]     param      :callback param
* @retval        none
******************************************************************************/
static void SSIF_DMA_TxCallback(const union sigval param)
{
    ssif_info_ch_t* const p_info_ch = param.sival_ptr;
    uint32_t ssif_ch;
    dma_trans_data_t dma_data_next;
    int_t ercd = IOIF_ESUCCESS;
    int_t ret;


    if (NULL == p_info_ch)
    {
        /* NON_NOTICE_ASSERT: illegal pointer */
    }
    else
    {
        ssif_ch = p_info_ch->channel;

        if (NULL == p_info_ch->p_aio_tx_curr)
        {
            /* now complete dummy transfer, It isn't neccessary to signal. */
        }
        else
        {
            /* now complete user request transfer, Signal to application */

            /* return aio complete */
            p_info_ch->p_aio_tx_curr->aio_return = (ssize_t)p_info_ch->p_aio_tx_curr->aio_nbytes;
            ahf_complete(&p_info_ch->tx_que, p_info_ch->p_aio_tx_curr);
        }

        /* copy next to curr(even if it's NULL) */
        p_info_ch->p_aio_tx_curr = p_info_ch->p_aio_tx_next;

        /* get next request(It's maybe NULL) */
        p_info_ch->p_aio_tx_next = ahf_removehead(&p_info_ch->tx_que);

        if (NULL != p_info_ch->p_aio_tx_next)
        {
            /* add user request */
            dma_data_next.dst_addr = (void*)&g_ssireg[ssif_ch]->SSIFTDR;
            dma_data_next.src_addr = (void*)p_info_ch->p_aio_tx_next->aio_buf;
            dma_data_next.count = (uint32_t)p_info_ch->p_aio_tx_next->aio_nbytes;

            ret = R_DMA_NextData(p_info_ch->dma_tx_ch, &dma_data_next, (int32_t * const)&ercd);
            if (IOIF_EERROR == ret)
            {
                /* NON_NOTICE_ASSERT: unexpected DMA error */
            }
        }
        else
        {
            /* add dummy request */
            ret = R_DMA_NextData(p_info_ch->dma_tx_ch, &gb_ssif_txdma_dummy_trparam[ssif_ch], (int32_t * const)&ercd);
            if (IOIF_EERROR == ret)
            {
                /* NON_NOTICE_ASSERT: unexpected DMA error */
            }
        }
    }

    return;
}

/******************************************************************************
* Function Name: SSIF_DMA_RxCallback
* @brief         DMA callback function
*
*                Description:<br>
*
* @param[in]     param      :callback param
* @retval        none
******************************************************************************/
static void SSIF_DMA_RxCallback(const union sigval param)
{
    ssif_info_ch_t* const p_info_ch = param.sival_ptr;
    uint32_t ssif_ch;
    dma_trans_data_t dma_data_next;
    int_t ercd = IOIF_ESUCCESS;
    int_t ret;

    if (NULL == p_info_ch)
    {
        /* NON_NOTICE_ASSERT: illegal pointer */
    }
    else
    {
        ssif_ch = p_info_ch->channel;

        if (NULL == p_info_ch->p_aio_rx_curr)
        {
            /* now complete dummy transfer, It isn't neccessary to signal. */
        }
        else
        {
            /* now complete user request transfer, Signal to application */

            /* return aio complete */
            p_info_ch->p_aio_rx_curr->aio_return = (ssize_t)p_info_ch->p_aio_rx_curr->aio_nbytes;
            ahf_complete(&p_info_ch->rx_que, p_info_ch->p_aio_rx_curr);
        }

        /* copy next to curr(even if it's NULL) */
        p_info_ch->p_aio_rx_curr = p_info_ch->p_aio_rx_next;

        /* get next request(It's maybe NULL) */
        p_info_ch->p_aio_rx_next = ahf_removehead(&p_info_ch->rx_que);

        if (NULL != p_info_ch->p_aio_rx_next)
        {
            /* add user request */
            dma_data_next.src_addr = (void*)&g_ssireg[ssif_ch]->SSIFRDR;
            dma_data_next.dst_addr = (void*)p_info_ch->p_aio_rx_next->aio_buf;
            dma_data_next.count = (uint32_t)p_info_ch->p_aio_rx_next->aio_nbytes;

            ret = R_DMA_NextData(p_info_ch->dma_rx_ch, &dma_data_next, (int32_t * const)&ercd);
            if (IOIF_EERROR == ret)
            {
                /* NON_NOTICE_ASSERT: unexpected DMA error */
            }
        }
        else
        {
            /* add dummy request */
            ret = R_DMA_NextData(p_info_ch->dma_rx_ch, &gb_ssif_rxdma_dummy_trparam[ssif_ch], (int32_t * const)&ercd);
            if (IOIF_EERROR == ret)
            {
                /* NON_NOTICE_ASSERT: unexpected DMA error */
            }
        }
    }

    return;
}

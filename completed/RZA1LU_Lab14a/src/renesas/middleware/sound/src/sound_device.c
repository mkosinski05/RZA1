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
 * Copyright (C) 2014-2017 Renesas Electronics Corporation. All rights reserved.
 *******************************************************************************/
/**************************************************************************//**
 * @file         sound_device.c
 * $Rev: 8475 $
 * $Date:: 2018-06-18 11:21:41 +0100#$
 * @brief        sound driver functions
 ******************************************************************************/

/******************************************************************************
 Includes   <System Includes> , "Project Includes"
 ******************************************************************************/
#include "sound.h"
#include "mcu_board_select.h"
#include "dev_drv.h"
#include "riic_max9856_if.h"

/******************************************************************************
 Typedef definitions
 ******************************************************************************/

/******************************************************************************
 Macro definitions
 ******************************************************************************/
#define SOUND_CH_0_PRV_ (0u)
#define SOUND_CH_1_PRV_ (1u)
#define SOUND_CH_2_PRV_ (2u)
#define SOUND_CH_3_PRV_ (3u)
#if (TARGET_RZA1 <= TARGET_RZA1LU)
#else /* TARGET_RZA1H */
    #define SOUND_CH_4_PRV_ (4u)
    #define SOUND_CH_5_PRV_ (5u)
#endif /* (TARGET_RZA1 <= TARGET_RZA1LU) */

#define SOUND_LINER_DBTBL_SIZE_PRV_      (101U)
#define SOUND_MUTEDB_PRV_ (1)
/******************************************************************************
 Imported global variables and functions (from other files)
 ******************************************************************************/

/******************************************************************************
 Exported global variables and functions (to be accessed by other files)
 ******************************************************************************/

/******************************************************************************
 Private global variables and functions
 ******************************************************************************/
static const int32_t g_sound_linear_to_decibels[SOUND_LINER_DBTBL_SIZE_PRV_] =
{
/* positive value indicates mute */
SOUND_MUTEDB_PRV_,

/* These decibel values come from Excel formula 1000*LOG10(POWER(x,4)/POWER(100,4))
 with the values representing -80dB to 0dB attenuation.
 They're multiplied by 100 so, for example, -8000 below is actually -80.00 dB
 It's up to the codec to find the nearest match within it's
 own capabilities.
 */

-8000, -6796, -6092, -5592, -5204, -4887, -4620, -4388, -4183, -4000, -3834, -3683, -3544, -3415, -3296, -3184, -3078,
        -2979, -2885, -2796, -2711, -2630, -2553, -2479, -2408, -2340, -2275, -2211, -2150, -2092, -2035, -1979, -1926,
        -1874, -1824, -1775, -1727, -1681, -1636, -1592, -1549, -1507, -1466, -1426, -1387, -1349, -1312, -1275, -1239,
        -1204, -1170, -1136, -1103, -1070, -1039, -1007, -977, -946, -917, -887, -859, -830, -803, -775, -748, -722,
        -696, -670, -645, -620, -595, -571, -547, -523, -500, -477, -454, -432, -409, -388, -366, -345, -324, -303,
        -282, -262, -242, -222, -202, -183, -164, -145, -126, -107, -89, -71, -53, -35, -17, 0 };

/**************************************************************************//**
 * Function Name: sound_init
 * @brief         SOUND driver initialize.
 *
 *                Description:<br>
 *
 * @param         none
 * @retval        DEVDRV_SUCCESS   :Success.
 * @retval        error code :Failure.
 ******************************************************************************/
int32_t sound_init (void)
{
    int32_t ercd = DEVDRV_SUCCESS;

    ercd = RIIC_MAX9856_Open();

    return (ercd);
}
/*******************************************************************************
 End of function sound_init
 ******************************************************************************/

/**************************************************************************//**
 * Function Name: sound_un_init
 * @brief         SOUND driver uninitialize.
 *
 *                Description:<br>
 *
 * @param         none
 * @retval        DEVDRV_SUCCESS   :Success.
 * @retval        error code :Failure.
 ******************************************************************************/
int32_t sound_un_init (void)
{
    int32_t ercd = DEVDRV_SUCCESS;

    ercd = RIIC_MAX9856_Close();

    return (ercd);
}
/*******************************************************************************
 End of function sound_un_init
 ******************************************************************************/

/**************************************************************************//**
 * Function Name: sound_open
 * @brief         Initialize DAC.
 *
 *                Description:<br>
 *                Establish a connection with DAC.<br>
 *                And setup defaults.
 * @param[in]     chnl       :SSIF Channel #
 * @retval        DEVDRV_SUCCESS   :Success.
 * @retval        error code :Failure.
 ******************************************************************************/
int32_t sound_open (const uint32_t chnl)
{
    int32_t ercd;

    switch (chnl)
    {
        case SOUND_CH_0_PRV_:
        {
#if ((TARGET_BOARD == TARGET_BOARD_RSK) || (TARGET_BOARD == TARGET_BOARD_STREAM_IT2))
            ercd = sound_max9856_open();
#else
            ercd = DEVDRV_ERROR;
#endif /* ((TARGET_BOARD == TARGET_BOARD_RSK) || (TARGET_BOARD == TARGET_BOARD_STREAM_IT2)) */
        }
        break;

        case SOUND_CH_1_PRV_:
        {
            ercd = DEVDRV_ERROR;
        }
        break;

        case SOUND_CH_2_PRV_:
        {
            ercd = DEVDRV_ERROR;
        }
        break;

        case SOUND_CH_3_PRV_:
        {
            ercd = DEVDRV_ERROR;
        }
        break;

#if (TARGET_RZA1 <= TARGET_RZA1LU)
#else /* TARGET_RZA1H */
            case SOUND_CH_4_PRV_:
            {
                ercd = DEVDRV_ERROR;
            }
            break;
            case SOUND_CH_5_PRV_:
            {
                ercd = DEVDRV_ERROR;
            }
            break;
#endif /* (TARGET_RZA1 <= TARGET_RZA1LU) */
        default:
        {
            ercd = DEVDRV_ERROR;
        }
        break;
    }

    return (ercd);
}
/*******************************************************************************
 End of function sound_open
 ******************************************************************************/

/**************************************************************************//**
 * Function Name: sound_close
 * @brief         Uninitialize DAC.
 *
 *                Description:<br>
 *                Set Mute.<br>
 *                And close an established connection with DAC.
 * @param[in]     chnl       :SSIF Channel #
 * @retval        DEVDRV_SUCCESS   :Success.
 * @retval        error code :Failure.
 ******************************************************************************/
int32_t sound_close (const uint32_t chnl)
{
    int32_t ercd;

    switch (chnl)
    {
        case SOUND_CH_0_PRV_:
        {
#if ((TARGET_BOARD == TARGET_BOARD_RSK) || (TARGET_BOARD == TARGET_BOARD_STREAM_IT2))
            ercd = sound_max9856_close();
#else
            ercd = DEVDRV_ERROR;
#endif /* ((TARGET_BOARD == TARGET_BOARD_RSK) || (TARGET_BOARD == TARGET_BOARD_STREAM_IT2)) */
        }
        break;

        case SOUND_CH_1_PRV_:
        {
            ercd = DEVDRV_ERROR;
        }
        break;

        case SOUND_CH_2_PRV_:
        {
            ercd = DEVDRV_ERROR;
        }
        break;

        case SOUND_CH_3_PRV_:
        {
            ercd = DEVDRV_ERROR;
        }
        break;

#if (TARGET_RZA1 <= TARGET_RZA1LU)
#else /* TARGET_RZA1H */
            case SOUND_CH_4_PRV_:
            {
                ercd = DEVDRV_ERROR;
            }
            break;

            case SOUND_CH_5_PRV_:
            {
                ercd = DEVDRV_ERROR;
            }
            break;

#endif /* (TARGET_RZA1 <= TARGET_RZA1LU) */
        default:
        {
            ercd = DEVDRV_ERROR;
        }
        break;
    }

    return (ercd);
}
/*******************************************************************************
 End of function sound_close
 ******************************************************************************/

/**************************************************************************//**
 * Function Name: sound_set_volume
 * @brief         Set spk volume to DAC.
 *
 *                Description:<br>
 *                Set volume.
 * @param[in]     chnl       :SSIF Channel #
 * @param[in]     vol        :volume(0 - 100)
 * @retval        DEVDRV_SUCCESS   :Success.
 * @retval        error code :Failure.
 ******************************************************************************/
int32_t sound_set_volume (const uint32_t chnl, const uint32_t vol)
{
    int32_t ercd;

    switch (chnl)
    {
        case SOUND_CH_0_PRV_:
        {
#if ((TARGET_BOARD == TARGET_BOARD_RSK) || (TARGET_BOARD == TARGET_BOARD_STREAM_IT2))
            if (0 == vol)
            {
                ercd = sound_max9856_set_mute();
            }
            else
            {
                /* casting to int32_t to match function arguments */
                ercd = sound_max9856_set_volume((int32_t)vol, (int32_t)vol);
            }
#else
            ercd = DEVDRV_ERROR;
#endif /* ((TARGET_BOARD == TARGET_BOARD_RSK) || (TARGET_BOARD == TARGET_BOARD_STREAM_IT2)) */
        }
        break;

        case SOUND_CH_1_PRV_:
        {
            ercd = DEVDRV_ERROR;
        }
        break;

        case SOUND_CH_2_PRV_:
        {
            ercd = DEVDRV_ERROR;
        }
        break;

        case SOUND_CH_3_PRV_:
        {
            ercd = DEVDRV_ERROR;
        }
        break;

#if (TARGET_RZA1 <= TARGET_RZA1LU)
#else /* TARGET_RZA1H */
            case SOUND_CH_4_PRV_:
            {
                ercd = DEVDRV_ERROR;
            }
            break;

            case SOUND_CH_5_PRV_:
            {
                ercd = DEVDRV_ERROR;
            }
            break;
#endif /* (TARGET_RZA1 <= TARGET_RZA1LU) */

        default:
        {
            ercd = DEVDRV_ERROR;
        }
        break;
    }

    return (ercd);
}
/*******************************************************************************
 End of function sound_set_volume
 ******************************************************************************/

/**************************************************************************//**
 * Function Name: sound_set_mic_volume
 * @brief         Set mic volume to DAC.
 *
 *                Description:<br>
 *                Set volume.
 * @param[in]     chnl       :SSIF Channel #
 * @param[in]     vol        :volume(0 - 100)
 * @retval        DEVDRV_SUCCESS   :Success.
 * @retval        error code :Failure.
 ******************************************************************************/
int32_t sound_set_mic_volume (const uint32_t chnl, const uint32_t vol)
{
    int32_t ercd;

    switch (chnl)
    {
        case SOUND_CH_0_PRV_:
        {
#if ((TARGET_BOARD == TARGET_BOARD_RSK) || (TARGET_BOARD == TARGET_BOARD_STREAM_IT2))
            if (0u == vol)
            {
                ercd = sound_max9856_set_mic_mute();
            }
            else
            {
                /* casting to int32_t to match function arguments */
                ercd = sound_max9856_set_mic_volume((int32_t)vol, (int32_t)vol);
            }
#else
            ercd = DEVDRV_ERROR;
#endif /* ((TARGET_BOARD == TARGET_BOARD_RSK) || (TARGET_BOARD == TARGET_BOARD_STREAM_IT2)) */
        }
        break;

        case SOUND_CH_1_PRV_:
        {
            ercd = DEVDRV_ERROR;
        }
        break;

        case SOUND_CH_2_PRV_:
        {
            ercd = DEVDRV_ERROR;
        }
        break;

        case SOUND_CH_3_PRV_:
        {
            ercd = DEVDRV_ERROR;
        }
        break;

#if (TARGET_RZA1 <= TARGET_RZA1LU)
#else /* TARGET_RZA1H */
            case SOUND_CH_4_PRV_:
            {
                ercd = DEVDRV_ERROR;
            }
            break;

            case SOUND_CH_5_PRV_:
            {
                ercd = DEVDRV_ERROR;
            }
            break;
#endif /* (TARGET_RZA1 <= TARGET_RZA1LU) */

        default:
        {
            ercd = DEVDRV_ERROR;
        }
        break;
    }

    return (ercd);
}
/*******************************************************************************
 End of function sound_set_mic_volume
 ******************************************************************************/

/**************************************************************************//**
 * Function Name: sound_set_sampling_rate
 * @brief         Set sampling rate to DAC
 *
 *                Description:<br>
 *                Set sampling rate.
 * @param[in]     chnl       :SSIF Channel #
 * @param[in]     freq       :sampling rate(8k - 96k)
 * @retval        DEVDRV_SUCCESS   :Success.
 * @retval        error code :Failure.
 ******************************************************************************/
int32_t sound_set_sampling_rate (const uint32_t chnl, const uint32_t freq)
{
    int32_t ercd;

    switch (chnl)
    {
        case SOUND_CH_0_PRV_:
        {
#if ((TARGET_BOARD == TARGET_BOARD_RSK) || (TARGET_BOARD == TARGET_BOARD_STREAM_IT2))
            ercd = sound_max9856_set_sampling_rate(freq);
#else
            ercd = DEVDRV_ERROR;
#endif /* ((TARGET_BOARD == TARGET_BOARD_RSK) || (TARGET_BOARD == TARGET_BOARD_STREAM_IT2)) */
        }
        break;

        case SOUND_CH_1_PRV_:
        {
            ercd = DEVDRV_ERROR;
        }
        break;

        case SOUND_CH_2_PRV_:
        {
            ercd = DEVDRV_ERROR;
        }
        break;

        case SOUND_CH_3_PRV_:
        {
            ercd = DEVDRV_ERROR;
        }
        break;

#if (TARGET_RZA1 <= TARGET_RZA1LU)
#else /* TARGET_RZA1H */
            case SOUND_CH_4_PRV_:
            {
                ercd = DEVDRV_ERROR;
            }
            break;

            case SOUND_CH_5_PRV_:
            {
                ercd = DEVDRV_ERROR;
            }
            break;

#endif /* (TARGET_RZA1 <= TARGET_RZA1LU) */
        default:
        {
            ercd = DEVDRV_ERROR;
        }
        break;
    }

    return (ercd);
}
/*******************************************************************************
 End of function sound_set_sampling_rate
 ******************************************************************************/

/**************************************************************************//**
 * Function Name: sound_set_format
 * @brief         Set I2S BUS format to DAC.
 *
 *                Description:<br>
 *                Set I2S BUS format.
 * @param[in]     chnl       :SSIF Channel #
 * @param[in]     fmt        :format(I2S, LEFT, RIGHT)
 * @param[in]     dwdsz      :dataword size(16bit, 18bit, 20bit, 24bit, 32bit)
 * @retval        DEVDRV_SUCCESS   :Success.
 * @retval        error code :Failure.
 ******************************************************************************/
int32_t sound_set_format (const uint32_t chnl, const uint32_t fmt, const uint32_t dwdsz)
{
    int32_t ercd;

    switch (chnl)
    {
        case SOUND_CH_0_PRV_:
        {
#if ((TARGET_BOARD == TARGET_BOARD_RSK) || (TARGET_BOARD == TARGET_BOARD_STREAM_IT2))
            ercd = sound_max9856_set_audio_format(fmt, dwdsz);
#else
            ercd = DEVDRV_ERROR;
#endif /* ((TARGET_BOARD == TARGET_BOARD_RSK) || (TARGET_BOARD == TARGET_BOARD_STREAM_IT2)) */
        }
        break;

        case SOUND_CH_1_PRV_:
        {
            ercd = DEVDRV_ERROR;
        }
        break;

        case SOUND_CH_2_PRV_:
        {
            ercd = DEVDRV_ERROR;
        }
        break;

        case SOUND_CH_3_PRV_:
        {
            ercd = DEVDRV_ERROR;
        }
        break;

#if (TARGET_RZA1 <= TARGET_RZA1LU)
#else /* TARGET_RZA1H */
            case SOUND_CH_4_PRV_:
            {
                ercd = DEVDRV_ERROR;
            }
            break;

            case SOUND_CH_5_PRV_:
            {
                ercd = DEVDRV_ERROR;
                ]
                break;

#endif /* (TARGET_RZA1 <= TARGET_RZA1LU) */

        default:
        {
            ercd = DEVDRV_ERROR;
        }
        break;
    }

    return (ercd);
}
/*******************************************************************************
 End of function sound_set_format
 ******************************************************************************/

/******************************************************************************
 Private global variables and functions
 ******************************************************************************/


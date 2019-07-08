/******************************************************************************
 Includes   <System Includes> , "Project Includes"
 ******************************************************************************/
#ifdef   __cplusplus
extern "C"
{
#endif

#include <ctype.h>

#ifndef __ICCARM__
#include "stdbool.h"
#endif
#include "FreeRTOS.h"
#include "task.h"
#include "r_typedefs.h"
#include "r_camera_if.h"
#include "r_camera_module.h"
#include "r_vdc_portsetting.h"
#include "compiler_settings.h"
#include "r_rvapi_header.h"
#include "r_display_init.h"
#include "string.h"

/******************************************************************************
 Macro definitions
 ******************************************************************************/
/* diplay layer for rectangle */
#if defined FRAME_BUFFER_BITS_PER_PIXEL_16
#define DATA_SIZE_PER_PIC      (2u)
#elif defined FRAME_BUFFER_BITS_PER_PIXEL_32
#define DATA_SIZE_PER_PIC      (4u)
#else
#error "Set bits per pixel"
#endif

#define FRAMEBUFFER_WIDTH   480
#define FRAMEBUFFER_HEIGHT  272

#define FRAMEBUFFER_LAYER_NUM      (2u)
#define FRAMEBUFFER_STRIDE  (((FRAMEBUFFER_WIDTH * DATA_SIZE_PER_PIC) + 31u) & ~31u)

/* Frame buffer stride: Frame buffer stride should be set to a multiple of 32 or 128
 in accordance with the frame buffer burst transfer mode. */
#if ( TARGET_BOARD == TARGET_BOARD_STREAM_IT2 )
#define     VIDEO_BUFFER_STRIDE         (((CAP_CEU_SIZE_HW * 2u) + 31u) & ~31u)
#define     VIDEO_BUFFER_HEIGHT         (CAP_CEU_SIZE_VW)
#define     VIDEO_BUFFER_NUM            (1u)
#elif ( TARGET_BOARD == TARGET_BOARD_RSK )
#define     VIDEO_BUFFER_STRIDE         (((CAP_VDC5_SIZE_HW * 2u) + 31u) & ~31u)
#define     VIDEO_BUFFER_HEIGHT         (CAP_VDC5_SIZE_VW)
#define     VIDEO_BUFFER_NUM            (2u)
#else
#error ERROR: Invalid board defined.
#endif

/* Display area */
#if 0
#define     DISP_AREA_HS                (80u)
#define     DISP_AREA_VS                (16u)
#else
#define     DISP_AREA_HS                (160u)
#define     DISP_AREA_VS                (16u)
#endif
#define     DISP_AREA_HW                (320u)
#define     DISP_AREA_VW                (240u)

/******************************************************************************
 Private global variables and functions
 ******************************************************************************/

/******************************************************************************
 Imported global variables and functions (from other files)
 ******************************************************************************/

extern ceu_error_t video_init_ceu(void);

/****************************************************************************/
/* VRAM definitions                                                         */
/****************************************************************************/
static volatile int32_t vsync_count = 0;
static int draw_buffer_index = 0;

#if __ICCARM__ == 1
#pragma data_alignment=8
uint8_t framebuffer[FRAMEBUFFER_LAYER_NUM][FRAMEBUFFER_STRIDE * FRAMEBUFFER_HEIGHT] @ "VRAM_SECTION0";
#else
uint8_t framebuffer[FRAMEBUFFER_LAYER_NUM][FRAMEBUFFER_STRIDE * FRAMEBUFFER_HEIGHT] __attribute__ ((section(".VRAM_SECTION0")));
uint8_t video_buffer[(VIDEO_BUFFER_STRIDE * VIDEO_BUFFER_HEIGHT) * VIDEO_BUFFER_NUM] __attribute__ ((section(".VRAM_SECTION0")));
#endif

static void camera_task (void *parameters)
{
	while (1) {
		R_RVAPI_CaptureStartCEU ((void *) video_buffer, NULL, VIDEO_BUFFER_STRIDE);
		while (R_RVAPI_CaptureStatusCEU () == CAP_BUSY)
		{
			R_OS_TaskSleep (2);
		}
	}
}
// graphic buffer cursor_buffer is already defined in BSP/src/renesas/application/app_touchscreen/r_drawrectangle.c
volatile uint8_t *RZAFrameBuffers[2] =
{
    framebuffer[1],
    framebuffer[0],
};

static void IntCallbackFunc_LoVsync(vdc_int_type_t int_type)
{
    if (vsync_count > 0)
    {
        vsync_count--;
    }
}

static void Wait_Vsync(const int32_t wait_count)
{
    vsync_count = wait_count;
    while (vsync_count > 0)
    {
        vTaskDelay( 2 / portTICK_PERIOD_MS);
    }
}

/* Set / switch framebuffer */
void GrpDrv_SetFrameBuffer(void * ptr)
{
    if (draw_buffer_index == 1) {
        draw_buffer_index = 0;
    } else {
        draw_buffer_index = 1;
    }

    vdc_error_t error = R_RVAPI_GraphChangeSurfaceVDC(VDC_CHANNEL_0, VDC_LAYER_ID_2_RD, (void*)framebuffer[draw_buffer_index]);
    if (VDC_OK == error)
    {
         Wait_Vsync(1);
    }
}

void GRAPHIC_Clear(void)
{
    memset(framebuffer[draw_buffer_index], 0x0, FRAMEBUFFER_STRIDE * FRAMEBUFFER_HEIGHT);
}

void GRAPHIC_PutPixel(uint32_t x, uint32_t y, uint8_t color)
{
    for (int pixel = 0; pixel < DATA_SIZE_PER_PIC; ++pixel)
    {
        framebuffer[draw_buffer_index][(y * FRAMEBUFFER_STRIDE) + x * DATA_SIZE_PER_PIC + pixel] = color;
    }
}

void GRAPHIC_Rectangle( uint32_t begin_x, uint32_t begin_y, uint32_t width, uint32_t height, uint8_t color )
{
    uint32_t h = 0;
    uint32_t w = 0;

    /* new rectangle */
    h = begin_y;
    for (w = begin_x; w < (begin_x + width) ; w++)
    {
        GRAPHIC_PutPixel(w, h, color);
    }
    for (h = begin_y + 1; h < ((begin_y + height) - 1); h++)
    {
        GRAPHIC_PutPixel(begin_x, h, color);
        GRAPHIC_PutPixel(w, h, color);
    }
    for (w = begin_x; w < (begin_x + width); w++)
    {
        GRAPHIC_PutPixel(w, h, color);
    }
}

extern "C" void GRAPHIC_init_screen(void)
{
	os_task_t *p_os_task;
    vdc_error_t error;
    vdc_channel_t vdc_ch = VDC_CHANNEL_0;
    gr_surface_disp_config_t gr_disp_cnf;

    /***********************************************************************/
    /* display init (VDC output setting) */
    /***********************************************************************/
    {
        error = r_display_init (vdc_ch);
    }

    /***********************************************************************/
	/* Video init (CEU setting / VDC5 input setting) */
	/***********************************************************************/
	if (error == VDC_OK)
	{
	#if ( TARGET_BOARD == TARGET_BOARD_STREAM_IT2 )
			video_init_ceu ();
	#elif ( TARGET_BOARD == TARGET_BOARD_RSK )
			video_init_vdc(vdc_ch);
	#else
	#error ERROR: Invalid board defined.
	#endif
	}

#if ( TARGET_BOARD == TARGET_BOARD_STREAM_IT2 )
	/***********************************************************************/
	/* Camera init */
	/***********************************************************************/
	if (error == VDC_OK)
	{
	#if ( INPUT_SELECT == CAMERA_OV7670 )
			R_CAMERA_Ov7670Init(GRAPHICS_CAM_IMAGE_SIZE);
	#elif ( INPUT_SELECT == CAMERA_OV7740 )
			R_CAMERA_Ov7740Init(GRAPHICS_CAM_IMAGE_SIZE);
	#else
	#error ERROR: Invalid INPUT_SELECT.
	#endif
		}
	#endif

    if (error == VDC_OK)
    {
        error = R_RVAPI_InterruptEnableVDC(vdc_ch, VDC_INT_TYPE_S0_LO_VSYNC, 0, IntCallbackFunc_LoVsync);
    }
    /***********************************************************************/
    /* Graphic Layer 2 CLUT8 */
    /***********************************************************************/
    if (error == VDC_OK)
    {



        /* buffer clear */
        // Set frame buffer to black
        memset((void*)video_buffer, 0xF0, VIDEO_BUFFER_STRIDE * VIDEO_BUFFER_HEIGHT * VIDEO_BUFFER_NUM);

		gr_disp_cnf.layer_id         = VDC_LAYER_ID_0_RD;
		gr_disp_cnf.disp_area.hs_rel = DISP_AREA_HS;
		gr_disp_cnf.disp_area.hw_rel = DISP_AREA_HW;
		gr_disp_cnf.disp_area.vs_rel = DISP_AREA_VS;
		gr_disp_cnf.disp_area.vw_rel = DISP_AREA_VW;
		gr_disp_cnf.fb_buff          = &video_buffer[0];
		gr_disp_cnf.fb_stride        = VIDEO_BUFFER_STRIDE;
		//gr_disp_cnf.read_format      = VDC_GR_FORMAT_RGB565;
		gr_disp_cnf.read_format      = VDC_GR_FORMAT_YCBCR422;
		//gr_disp_cnf.read_ycc_swap    = VDC_GR_YCCSWAP_CBY0CRY1;
		gr_disp_cnf.read_ycc_swap    = VDC_GR_YCCSWAP_Y0CBY1CR;
		//gr_disp_cnf.read_swap        = VDC_WR_RD_WRSWA_32_16BIT;
		gr_disp_cnf.read_swap        = VDC_WR_RD_WRSWA_NON;
		gr_disp_cnf.clut_table = NULL;
		gr_disp_cnf.disp_mode        = VDC_DISPSEL_CURRENT;

		error = R_RVAPI_GraphCreateSurfaceVDC(vdc_ch, &gr_disp_cnf);
    }
#if 1
	if ( VDC_OK == error ) {

		/* buffer clear */
		// Set frame buffer to black
		memset((void*)&framebuffer[0], 0xF0, FRAMEBUFFER_STRIDE * FRAMEBUFFER_HEIGHT);
		memset((void*)&framebuffer[1], 0xF0, FRAMEBUFFER_STRIDE * FRAMEBUFFER_HEIGHT);

		uint32_t  clut_table[4] = {
				0x00000000, /* No.0 transparent color  */
				0xFF000000, /* No.1 black */
				0xFF00FF00, /* No.2 green */
				0xFFFF0000  /* No.3 red */
		};

#if (0) /* not use camera captured layer */
		gr_disp_cnf.layer_id         = VDC_LAYER_ID_0_RD;
#else   /* blend over camera captured image */
		gr_disp_cnf.layer_id         = VDC_LAYER_ID_2_RD;
#endif
		gr_disp_cnf.disp_area.hs_rel = 0;
		gr_disp_cnf.disp_area.hw_rel = FRAMEBUFFER_WIDTH;
		gr_disp_cnf.disp_area.vs_rel = 0;
		gr_disp_cnf.disp_area.vw_rel = FRAMEBUFFER_HEIGHT;
		gr_disp_cnf.fb_buff          = &framebuffer[0];
		gr_disp_cnf.fb_stride        = FRAMEBUFFER_STRIDE;
		//gr_disp_cnf.read_format      = VDC_GR_FORMAT_CLUT8;
#if defined FRAME_BUFFER_BITS_PER_PIXEL_16
		gr_disp_cnf.read_format      = VDC_GR_FORMAT_RGB565;
#elif defined FRAME_BUFFER_BITS_PER_PIXEL_32
		gr_disp_cnf.read_format      = VDC_GR_FORMAT_RGB888;
#endif
		//gr_disp_cnf.clut_table       = clut_table;
		gr_disp_cnf.read_ycc_swap    = VDC_GR_YCCSWAP_CBY0CRY1;
		gr_disp_cnf.read_swap        = VDC_WR_RD_WRSWA_32_16BIT;
#if (0) /* not use camera captured data */
		gr_disp_cnf.disp_mode        = VDC_DISPSEL_CURRENT;
#else   /* blend over camera captured image */
		gr_disp_cnf.disp_mode        = VDC_DISPSEL_BLEND;
#endif
		error = R_RVAPI_GraphCreateSurfaceVDC(vdc_ch, &gr_disp_cnf);

		GRAPHIC_Clear();

    }

	if (VDC_OK == error)
	{
		vdc_pd_disp_rect_t 	alpha_area;
		vdc_alpha_rect_t	var_alpha_rect;
		vdc_alpha_blending_rect_t alpha_blending_rect;

		/* Alpha blending in a rectangular area parameter ---------------------------*/
		alpha_area.vs_rel = DISP_AREA_VS;
		alpha_area.vw_rel = DISP_AREA_VW;
		alpha_area.hs_rel = DISP_AREA_HS;
		alpha_area.hw_rel = DISP_AREA_HW;
		alpha_blending_rect.gr_arc     = &alpha_area;		/* Alpha Blending Area of a rectangle */

		var_alpha_rect.gr_arc_coef      = (int16_t)0u;		/* Alpha coefficient */
		var_alpha_rect.gr_arc_def       = (uint8_t)0u;		/* Alpha default value */
		var_alpha_rect.gr_arc_rate      = (uint8_t)0u;		/* Alpha value add frame rate */
		var_alpha_rect.gr_arc_mul       = VDC5_OFF;			/* Multiply with current alpha */
		alpha_blending_rect.alpha_rect = &var_alpha_rect;	/* Alpha Blending Area in a rectangular area */

		/* Lower-layer plane in the scaler, on/off */
		alpha_blending_rect.scl_und_sel    = NULL; /* GR_VIN_SCL_UND_SEL */

		error = R_VDC_AlphaBlendingRect (VDC_CHANNEL_0, VDC_LAYER_ID_2_RD, VDC5_ON, &alpha_blending_rect);

	}
#endif

	if ( error == VDC_OK ) {
		vdc_gr_disp_sel_t    gr_disp_sel_tmp[VDC_GR_TYPE_NUM];
		vdc_start_t          start;

		/* Set layer read start process */
		gr_disp_sel_tmp[VDC_GR_TYPE_GR0] = VDC_DISPSEL_CURRENT;
#if 1
		gr_disp_sel_tmp[VDC_GR_TYPE_GR2] = VDC_DISPSEL_BLEND;
#else
		gr_disp_sel_tmp[VDC_GR_TYPE_GR2] = VDC_DISPSEL_IGNORED;
#endif
		gr_disp_sel_tmp[VDC_GR_TYPE_GR3] = VDC_DISPSEL_IGNORED;

		start.gr_disp_sel                 = gr_disp_sel_tmp;

		/* Start process */
		R_RVAPI_Start( vdc_ch, VDC_LAYER_ID_ALL, &start);
		//R_RVAPI_Start( vdc_ch, VDC_LAYER_ID_0_RD, &start);
	}
    /* Image Quality Adjustment */
    if (VDC_OK == error)
    {
        error = r_image_quality_adjustment(vdc_ch);
    }

    /* Enable signal output */
    if (VDC_OK == error)
    {
        /* Wait for register update */
        R_OS_TaskSleep(5);

        R_RVAPI_DispPortSettingVDC(vdc_ch, &VDC_LcdPortSetting);
    }

    p_os_task = R_OS_CreateTask("Camera Task",camera_task, NULL, R_OS_ABSTRACTION_PRV_DEFAULT_STACK_SIZE, TASK_BLINK_TASK_PRI);
}

void GRAPHIC_test(unsigned char color)
{
    GRAPHIC_Clear();
    for (int y = 0; y < FRAMEBUFFER_HEIGHT; ++y)
    {
        for (int x = 0; x < FRAMEBUFFER_WIDTH; ++x)
        {
            GRAPHIC_PutPixel(x, y, ((x * y) + color) & 0xff);
        }
    }
}

#ifdef   __cplusplus
}
#endif

/*****************************************************************************
 *
 * MODULE:             JN-AN-1220 ZLO Sensor Demo
 *
 * COMPONENT:          app_occupancy_buttons.c
 *
 * DESCRIPTION:        DK4 (DR1175/DR1199) Button Press detection (Implementation)
 *
 ****************************************************************************
 *
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on NXP products [NXP Microcontrollers such as JN5168, JN5169,
 * JN5179, JN5189].
 * You, and any third parties must reproduce the copyright and warranty notice
 * and any other legend of ownership on each copy or partial copy of the
 * software.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright NXP B.V. 2016-2018. All rights reserved
 *
 ***************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <jendefs.h>
#include "ZTimer.h"
#include "fsl_os_abstraction.h"
#include "ZQueue.h"
#include "app_main.h"
#include "dbg.h"
#include "app_events.h"
#include "app.h"
#include "app_pulses_buttons.h"
#include "fsl_iocon.h"
#include "fsl_gpio.h"
#include "fsl_gint.h"
#include "app_pulses_sensor_state_machine.h"
#include "app_main.h"
#if ((defined APP_NTAG_ICODE) && (APP_BUTTONS_NFC_FD != (0xff)))
#include "fsl_ntag.h"
#endif

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifndef DEBUG_APP_BUTTON
    #define TRACE_APP_BUTTON               FALSE
#else
    #define TRACE_APP_BUTTON               TRUE
#endif


/* Defines the number of bits sampled for a button debounce event. One sample taken for each bit set */
#define APP_BUTTON_SAMPLE_MASK          (0x3)

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
void gint_callback(void);
#if ((defined APP_NTAG_ICODE) && (APP_BUTTONS_NFC_FD != (0xff)))
void ntag_callback(ntag_field_detect_t fd, void *userData);
#endif

uint32 u32TimeButton;

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
#if (defined BUTTON_MAP_OM15082)
    #if ((defined APP_NTAG_ICODE) && (APP_BUTTONS_NFC_FD != (0xff)))
        PRIVATE uint8 s_u8ButtonDebounce[APP_BUTTONS_NUM] = { APP_BUTTON_SAMPLE_MASK,
                                                              APP_BUTTON_SAMPLE_MASK,
                                                             };
        PRIVATE uint32 u32PreviousDioState = APP_BUTTONS_DIO_MASK;
        PRIVATE uint8 s_u8ButtonState[APP_BUTTONS_NUM] = { 0,0};
        PRIVATE const uint8 s_u8ButtonDIOLine[APP_BUTTONS_NUM] =
        {
            APP_BOARD_SW0_PIN,
            APP_BOARD_SW1_PIN

        };
    #else
        PRIVATE uint8 s_u8ButtonDebounce[APP_BUTTONS_NUM] = { APP_BUTTON_SAMPLE_MASK,
                                                              APP_BUTTON_SAMPLE_MASK,
                                                             };
        PRIVATE uint32 u32PreviousDioState = APP_BUTTONS_DIO_MASK;
        PRIVATE uint8 s_u8ButtonState[APP_BUTTONS_NUM] = { 0,0};
        PRIVATE const uint8 s_u8ButtonDIOLine[APP_BUTTONS_NUM] =
        {
            APP_BOARD_SW0_PIN,
            APP_BOARD_SW1_PIN
        };
    #endif
#elif (defined BUTTON_MAP_OM15081)
    #if ((defined APP_NTAG_ICODE) && (APP_BUTTONS_NFC_FD != (0xff)))
        PRIVATE uint8 s_u8ButtonDebounce[APP_BUTTONS_NUM] = { APP_BUTTON_SAMPLE_MASK,
                                                              APP_BUTTON_SAMPLE_MASK };
        PRIVATE uint32 u32PreviousDioState = APP_BUTTONS_DIO_MASK;
        PRIVATE uint8 s_u8ButtonState[APP_BUTTONS_NUM] = { 0,0 };
        PRIVATE const uint8 s_u8ButtonDIOLine[APP_BUTTONS_NUM] =
        {
            APP_BOARD_SW0_PIN,
            APP_BUTTONS_NFC_FD
        };
    #else
        PRIVATE uint8 s_u8ButtonDebounce[APP_BUTTONS_NUM] = { APP_BUTTON_SAMPLE_MASK };
        PRIVATE uint32 u32PreviousDioState = APP_BUTTONS_DIO_MASK;
        PRIVATE uint8 s_u8ButtonState[APP_BUTTONS_NUM] = { 0 };
        PRIVATE const uint8 s_u8ButtonDIOLine[APP_BUTTONS_NUM] =
        {
            APP_BOARD_SW0_PIN
        };
    #endif
#else

#endif

PRIVATE volatile bool_t bDebouncing = FALSE;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: APP_bButtonInitialise
 *
 * DESCRIPTION:
 * Button Initialization
 *
 * PARAMETER: void
 *
 * RETURNS: bool
 *
 ****************************************************************************/
PUBLIC bool_t APP_bButtonInitialise(void)
{
    DBG_vPrintf(TRACE_APP_BUTTON, "\r\nBUTTON: APP_bButtonInitialise()");
#ifndef DONGLE

    /* Define the init structure for the input switch pins */
    gpio_pin_config_t switch_config = {
        kGPIO_DigitalInput,
    };
    uint8 u8Button;
    /* Loop through buttons */
    for (u8Button = 0; u8Button < APP_BUTTONS_NUM; u8Button++)
    {
        /* Valid pin ? */
        if (s_u8ButtonDIOLine[u8Button] < APP_NUMBER_OF_GPIO)
        {
#if APP_BUTTON_INVERT_MASK != 0
            /* Inverted operation ? */
            if (APP_BUTTON_INVERT_MASK & (1 << s_u8ButtonDIOLine[u8Button]))
            {
                /* Configure io mux for pull down operation */
                IOCON_PinMuxSet(IOCON, 0, s_u8ButtonDIOLine[u8Button], IOCON_FUNC0 | IOCON_MODE_PULLDOWN | IOCON_DIGITAL_EN);
            }
            /* Normal operation ? */
            else
#endif
            {
                /* Configure io mux for pull up operation */
                IOCON_PinMuxSet(IOCON, 0, s_u8ButtonDIOLine[u8Button], IOCON_FUNC0 | IOCON_MODE_PULLUP | IOCON_DIGITAL_EN);
            }
            /* Initialise GPIO use */
            GPIO_PinInit(GPIO, APP_BOARD_GPIO_PORT, s_u8ButtonDIOLine[u8Button], &switch_config);
        }
    }

    /* Initialize GINT0 */
    GINT_Init(GINT0);
    /* Setup GINT0 for edge trigger, "OR" mode */
    GINT_SetCtrl(GINT0, kGINT_CombineOr, kGINT_TrigEdge, gint_callback);
    /* Select pins & polarity for GINT0 */
    GINT_ConfigPins(GINT0, DEMO_GINT0_PORT, DEMO_GINT0_POL_MASK, DEMO_GINT0_ENA_MASK);
    /* Enable callback(s) */
    GINT_EnableCallback(GINT0);

    /* Using internal NTAG FD on JN518x ? */
    #if ((defined APP_NTAG_ICODE) && (APP_BUTTONS_NFC_FD != (0xff)))
    #if (APP_BUTTONS_NFC_FD ==  APP_INTERNAL_NTAG_FD_BIT)
    {
        /* Enable interrupt callback */
        ntag_config_t config;
        /* Get default config */
        NTAG_GetDefaultConfig(&config);
        /* Set callback function in config */
        config.callback = ntag_callback;
        /* Initialise ntag */
        NTAG_Init(&config);
    }
    #endif
    #endif

    /* Magic number in top of AOREG2 ? */
    if ((PMC->AOREG2 & 0xFE000000) == 0xAA000000)
    {
        /* Restore DIO state from AOREG2 */
        u32PreviousDioState = (PMC->AOREG2 & 0x01FFFFFF);
        DBG_vPrintf(TRACE_APP_BUTTON, "\r\nBUTTON: Load u32PreviousDioState = 0x%x", u32PreviousDioState);
    }
    else
    {
        /* Use default (all up) as prevoius state */
        u32PreviousDioState = APP_BUTTONS_DIO_MASK;
        DBG_vPrintf(TRACE_APP_BUTTON, "\r\nBUTTON: Default u32PreviousDioState = 0x%x", u32PreviousDioState);
    }

    /* If we came out of deep sleep; perform appropriate action as well based
       on button press.*/
    APP_cbTimerButtonScan(NULL);

    uint32 u32Buttons = APP_u32GetSwitchIOState();

    if (u32Buttons != APP_BUTTONS_DIO_MASK)
    {
        return TRUE;
    }
#endif
    return FALSE;
}


/****************************************************************************
 *
 * NAME: APP_u32GetSwitchIOState
 *
 * DESCRIPTION:
 * reads the IO state of any switch inputs, flips the bits of any switches that are
 * active high
 *
 * PARAMETER:
 *
 * RETURNS:
 *
 ****************************************************************************/
PUBLIC uint32 APP_u32GetSwitchIOState(void)
{
    uint32 u32ReadPinsInput;
    uint32 u32Return = -0xffffffff;
    #ifndef DONGLE
    {
        /* Read all GPIOs */
        u32ReadPinsInput = GPIO_PortRead(GPIO, APP_BOARD_GPIO_PORT);
        /* Using internal NTAG FD on JN518x ? */
        #if ((defined APP_NTAG_ICODE) && (APP_BUTTONS_NFC_FD != (0xff)))
        #if (APP_BUTTONS_NFC_FD == APP_INTERNAL_NTAG_FD_BIT)
        {
            /* Outside field ? */
            if (NTAG_PollFieldDetect() == kNTAG_FieldDetectOut)
            {
                /* Set bit for FD "button" */
                u32ReadPinsInput |= (1 << APP_BUTTONS_NFC_FD);
            }
        }
        #endif
        #endif
        /* Apply mask and inversions */
        u32Return = (u32ReadPinsInput & APP_BUTTONS_DIO_MASK) ^ APP_BUTTON_INVERT_MASK;
    }
    #endif
    //DBG_vPrintf(TRACE_APP_BUTTON, "APP_u32GetSwitchIOState()=0x%08x\r\n", u32Return);

    return u32Return;
}


/****************************************************************************
 *
 * NAME: gint_callback
 *
 * DESCRIPTION:
 *callback from the group io interrupt
 *
 * PARAMETER:
 *
 * RETURNS:
 *
 ****************************************************************************/
#ifndef DONGLE
void gint_callback(void)
{
    /* Take action for gint event */;
    uint32 u32IOStatus = APP_u32GetSwitchIOState();
    DBG_vPrintf(TRACE_APP_BUTTON, "GINT Pin Interrupt event detected %x\r\n", u32IOStatus);

// MJL not used in 6x/7x sensor    if (u32IOStatus != APP_BUTTONS_DIO_MASK)
    {
        /* disable edge detection until scan complete */
        GINT_DisableCallback(GINT0);
        /* Using internal NTAG FD on JN518x ? */
        #if ((defined APP_NTAG_ICODE) && (APP_BUTTONS_NFC_FD != (0xff)))
        #if (APP_BUTTONS_NFC_FD == APP_INTERNAL_NTAG_FD_BIT)
        {
            /* Disable interrupt callback */
            ntag_config_t config;
            /* Get default config */
            NTAG_GetDefaultConfig(&config);
            /* Initialise ntag */
            NTAG_Init(&config);
        }
        #endif
        #endif
        /* Run timer */
        //DBG_vPrintf(TRACE_APP_BUTTON, ";g");
        ZTIMER_eStop(u8TimerButtonScan);
        ZTIMER_eStart(u8TimerButtonScan, ZTIMER_TIME_MSEC(10));
    }
}
#endif

/****************************************************************************
 *
 * NAME: ntag_callback
 *
 * DESCRIPTION:
 * callback from the internal fd line
 *
 * PARAMETER:
 *
 * RETURNS:
 *
 ****************************************************************************/
#ifndef DONGLE
#if ((defined APP_NTAG_ICODE) && (APP_BUTTONS_NFC_FD != (0xff)))
void ntag_callback(ntag_field_detect_t fd, void *userData)
{
    /* Take action for gint event */;
    uint32 u32IOStatus = APP_u32GetSwitchIOState();
    DBG_vPrintf(TRACE_APP_BUTTON, "NTAG Pin Interrupt event detected %x fd_int=%d\r\n", u32IOStatus, fd);

// MJL not used in 6x/7x sensor    if (u32IOStatus != APP_BUTTONS_DIO_MASK)
    {
        /* disable edge detection until scan complete */
        GINT_DisableCallback(GINT0);
        /* Using internal NTAG FD on JN518x ? */
        #if (APP_BUTTONS_NFC_FD == APP_INTERNAL_NTAG_FD_BIT)
        {
            /* Disable interrupt callback */
            ntag_config_t config;
            /* Get default config */
            NTAG_GetDefaultConfig(&config);
            /* Initialise ntag */
            NTAG_Init(&config);
        }
        #endif
        /* Run timer */
        //DBG_vPrintf(TRACE_APP_BUTTON, ";n");
        ZTIMER_eStop(u8TimerButtonScan);
        ZTIMER_eStart(u8TimerButtonScan, ZTIMER_TIME_MSEC(10));
    }
}
#endif
#endif

/****************************************************************************
 *
 * NAME: APP_cbTimerButtonScan
 *
 * DESCRIPTION:
 * Timer callback to debounce the button presses
 *
 * PARAMETER:
 *
 * RETURNS:
 *
 ****************************************************************************/
PUBLIC void APP_cbTimerButtonScan(void *pvParam)
{
#if ((!defined DONGLE) && (!defined OM5578))
    /*
     * The DIO changed status register is reset here before the scan is performed.
     * This avoids a race condition between finishing a scan and re-enabling the
     * DIO to interrupt on a falling edge.
     */

    uint8 u8AllReleased = 0xff;
    unsigned int i;
    uint32 u32DIOState = APP_u32GetSwitchIOState();

    for (i = 0; i < APP_BUTTONS_NUM; i++)
    {
        uint8 u8Button = (uint8) ((u32DIOState >> s_u8ButtonDIOLine[i]) & 1);

        s_u8ButtonDebounce[i] <<= 1;
        s_u8ButtonDebounce[i] |= u8Button;
        u8AllReleased &= s_u8ButtonDebounce[i];

        if (0 == s_u8ButtonDebounce[i] && !s_u8ButtonState[i])
        {
            s_u8ButtonState[i] = TRUE;

            /*
             * button consistently depressed for 8 scan periods
             * so post message to application task to indicate
             * a button down event
             */
            APP_tsEvent sButtonEvent;
            sButtonEvent.eType = APP_E_EVENT_BUTTON_DOWN;
            sButtonEvent.uEvent.sButton.u8Button = i;
            u32TimeButton = OSA_TimeGetMsec();

            DBG_vPrintf(TRACE_APP_BUTTON, "Button DN=%d\r\n", i);

            if(ZQ_bQueueSend(&APP_msgAppEvents, &sButtonEvent) == FALSE)
            {
                DBG_vPrintf(TRACE_APP_BUTTON, "Button: Failed to post Event %d \r\n", sButtonEvent.eType);
            }
        }
        else if (0xff == s_u8ButtonDebounce[i] && s_u8ButtonState[i] != FALSE)
        {
            s_u8ButtonState[i] = FALSE;

            /*
             * button consistently released for 8 scan periods
             * so post message to application task to indicate
             * a button up event
             */
            u32TimeButton = OSA_TimeGetMsec() - u32TimeButton ;
			APP_tsEvent sButtonEvent;
			DBG_vPrintf(TRACE_APP_BUTTON, "\r\n----------------u32TimeButton: %d ---------------------\r\n", u32TimeButton);
			if ((u32TimeButton<5000) || (i != 1))
			{
				sButtonEvent.eType = APP_E_EVENT_BUTTON_UP;
			}else{
				sButtonEvent.eType = APP_E_EVENT_LEAVE_AND_RESET;
			}
			sButtonEvent.uEvent.sButton.u8Button = i;

			DBG_vPrintf(TRACE_APP_BUTTON, "Button UP=%d event :%d\r\n", i,sButtonEvent.eType);

			if(ZQ_bQueueSend(&APP_msgAppEvents, &sButtonEvent) == FALSE)
			{
				DBG_vPrintf(TRACE_APP_BUTTON, "Button: Failed to post Event %d \r\n", sButtonEvent.eType);
			}
        }
    }

    if (0xff == u8AllReleased)
    {
        /*
         * all buttons high so set dio to interrupt on change
         */
        DBG_vPrintf(TRACE_APP_BUTTON, "ALL UP\r\n", i);
        GINT_EnableCallback(GINT0);
        /* Using internal NTAG FD on JN518x ? */
        #if ((defined APP_NTAG_NWK) && (APP_BUTTONS_NFC_FD != (0xff)))
        #if (APP_BUTTONS_NFC_FD ==  APP_INTERNAL_NTAG_FD_BIT)
        {
            /* Enable interrupt callback */
            ntag_config_t config;
            /* Get default config */
            NTAG_GetDefaultConfig(&config);
            /* Set callback function in config */
            config.callback = ntag_callback;
            /* Initialise ntag */
            NTAG_Init(&config);
        }
        #endif
        #endif
        //DBG_vPrintf(TRACE_APP_BUTTON, ":t");
        ZTIMER_eStop(u8TimerButtonScan);
    }
    else
    {
        /*
         * one or more buttons is still depressed so continue scanning
         */
        //DBG_vPrintf(TRACE_APP_BUTTON, ";t");
        ZTIMER_eStop(u8TimerButtonScan);
        ZTIMER_eStart(u8TimerButtonScan, ZTIMER_TIME_MSEC(10));
    }
#endif
}



/****************************************************************************
 *
 * NAME: vActionOnButtonActivationAfterDeepSleep
 *
 * DESCRIPTION:
 * When we wake up, we have restarted so we need to manually check to see
 * what Dio woke us. Start the ButtonScanTask and disable wake interrupts
 *
 ****************************************************************************/
PUBLIC void vActionOnButtonActivationAfterDeepSleep(void)
{
    DBG_vPrintf(TRACE_APP_BUTTON, "\r\nBUTTON: vActionOnButtonActivationAfterDeepSleep()");
    /* disable edge detection until scan complete */
    GINT_DisableCallback(GINT0);
    /* Using internal NTAG FD on JN518x ? */
    #if ((defined APP_NTAG_ICODE) && (APP_BUTTONS_NFC_FD != (0xff)))
    #if (APP_BUTTONS_NFC_FD == APP_INTERNAL_NTAG_FD_BIT)
    {
        /* Disable interrupt callback */
        ntag_config_t config;
        /* Get default config */
        NTAG_GetDefaultConfig(&config);
        /* Initialise ntag */
        NTAG_Init(&config);
    }
    #endif
    #endif
    /* Make dummy call to timer callback to start debouncing */
    // MJL APP_cbTimerButtonScan(NULL);
    /* Initiate scanning */
    ZTIMER_teStatus eStatus;
    ZTIMER_eStop(u8TimerButtonScan);
    eStatus = ZTIMER_eStart(u8TimerButtonScan, ZTIMER_TIME_MSEC(10));
    DBG_vPrintf(TRACE_APP_BUTTON, "\r\nBUTTON: ZTIMER_eStart()=%d", eStatus);
}

/****************************************************************************
 *
 * NAME: vSaveDioStateBeforeDeepSleep
 *
 * DESCRIPTION:
 * Due to us going to sleep on a falling edge as well as a rising edge, we need
 * to save the Dio state into NVM so when we wake back up we know what edge we
 * had configured to wake us up.
 *
 ****************************************************************************/
PUBLIC void vSaveDioStateBeforeDeepSleep(void)
{
    /* Sace previous state to AOREG2 with magic number in upper bits */
    PMC->AOREG2 = (u32PreviousDioState | 0xAA000000);
    DBG_vPrintf(TRACE_APP_BUTTON, "\r\nBUTTON: Save u32PreviousDioState = 0x%x", u32PreviousDioState);
}

/****************************************************************************
 *
 * NAME: bGetPreSleepOccupancyState
 *
 * DESCRIPTION:
 * This function returns the last Dio edge of Switch 1 which determines what the last
 * Occupancy state was.
 *
 ****************************************************************************/
PUBLIC bool_t bGetPreSleepOccupancyState(void)
{
    DBG_vPrintf(TRACE_APP_BUTTON, "\r\nBUTTON: bGetPreSleepOccupancyState() = %08x, ", u32PreviousDioState & (1 << APP_BOARD_SW0_PIN));

    /* If previously the occupied DIO pin was low, return occupied, else unoccupied */
    if((u32PreviousDioState & (1 << APP_BOARD_SW0_PIN)) == 0)
    {
        DBG_vPrintf(TRACE_APP_BUTTON, "Occupied");
        return TRUE;
    }
    else
    {
        DBG_vPrintf(TRACE_APP_BUTTON, "Unoccupied");
        return FALSE;
    }
}

/****************************************************************************
 *
 * NAME: bButtonDebounceInProgress
 *
 * DESCRIPTION:
 * This function returns TRUE if the button debouncing routine is busy processing
 * a button press / release.
 *
 ****************************************************************************/
PUBLIC bool_t bButtonDebounceInProgress(void)
{
    return bDebouncing;
}
/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

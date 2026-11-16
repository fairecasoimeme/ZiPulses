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

uint32 u32TimeButton1;

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

/* Reed contact (SW0) tracking.
 * The pulse is counted on closure; the opening only re-arms the detection.
 * While awake, the group interrupt GINT0 is always armed with the polarity
 * opposite to the current reed level, so no transition is missed whatever
 * the length of the closure. While asleep, any level change wakes the chip
 * (PWR_vWakeUpConfig) and a scan runs at wake.
 * Before this fix (LiXee code), a closure longer than 20 scans (40 ms without
 * traces) stopped the scan without re-enabling the interrupt: a closure that
 * followed while the device was still awake was lost. It also kept the group
 * "active", so the link button was ignored while the reed stayed closed. */
#define SW0_INDEX                    0
#define SW1_INDEX                    1
#define REED_MASK                    (1 << APP_BOARD_SW0_PIN)
#define REED_CLOSED_CONFIRM_SAMPLES  3   /* closure stable for about 6 ms */
#define REED_OPEN_CONFIRM_SAMPLES    5   /* opening accepted after about 10 ms (bounce filter) */

/* RAM is retained during sleep: the state survives the warm starts */
PRIVATE bool_t bReedStateKnown = FALSE;  /* FALSE until the first scan after a cold start */
PRIVATE bool_t bReedClosed = FALSE;
PRIVATE uint8 u8ReedClosedRun = 0;       /* consecutive closed samples while closed */
PRIVATE uint8 u8ReedOpenRun = 0;         /* consecutive open samples while closed */

PRIVATE void vPostButtonEvent(APP_teEventType eType, uint8 u8Button)
{
    APP_tsEvent sButtonEvent;
    sButtonEvent.eType = eType;
    sButtonEvent.uEvent.sButton.u8Button = u8Button;
    DBG_vPrintf(TRACE_APP_BUTTON, "\r\nButton %d event %d\r\n", u8Button, eType);
    if (ZQ_bQueueSend(&APP_msgAppEvents, &sButtonEvent) == FALSE)
    {
        DBG_vPrintf(TRACE_APP_BUTTON, "Button: Failed to post Event %d \r\n", eType);
    }
}

/* Returns TRUE when the reed state is stable (nothing to confirm) */
PRIVATE bool_t bScanReed(uint32 u32DIOState)
{
    bool_t bLevelClosed = ((u32DIOState & REED_MASK) == 0);

    if (!bReedStateKnown)
    {
        /* Cold start: take the current level as reference, without counting */
        bReedStateKnown = TRUE;
        bReedClosed = bLevelClosed;
        u8ReedClosedRun = REED_CLOSED_CONFIRM_SAMPLES;
        u8ReedOpenRun = 0;
        DBG_vPrintf(TRACE_APP_BUTTON, "\r\nREED: initial state %s", bReedClosed ? "closed" : "open");
        return TRUE;
    }

    if (bReedClosed)
    {
        if (bLevelClosed)
        {
            u8ReedOpenRun = 0;
            if (u8ReedClosedRun < 0xFF)
            {
                u8ReedClosedRun++;
            }
        }
        else if (++u8ReedOpenRun >= REED_OPEN_CONFIRM_SAMPLES)
        {
            /* Opening confirmed: ready for the next pulse */
            bReedClosed = FALSE;
            u8ReedOpenRun = 0;
            u8ReedClosedRun = 0;
            DBG_vPrintf(TRACE_APP_BUTTON, "\r\nREED: open");
        }
    }
    else if (bLevelClosed)
    {
        /* Closure: one pulse, counted at once (the first sample is taken 20 ms
         * after the interrupt, past the closing bounce) */
        bReedClosed = TRUE;
        u8ReedClosedRun = 1;
        u8ReedOpenRun = 0;
        DBG_vPrintf(TRACE_APP_BUTTON, "\r\nREED: closed, pulse");
        vPostButtonEvent(APP_E_EVENT_BUTTON_DOWN, SW0_INDEX);
    }

    if (bReedClosed)
    {
        return (u8ReedOpenRun == 0) && (u8ReedClosedRun >= REED_CLOSED_CONFIRM_SAMPLES);
    }
    return TRUE;
}

/* Arms GINT0 so that it fires on the next reed transition (polarity opposite
 * to the current level) or on a press of the link button */
PRIVATE void vArmButtonInterrupt(void)
{
    uint32 u32Polarity = DEMO_GINT0_POL_MASK;   /* 0: active low */
    if ((APP_u32GetSwitchIOState() & REED_MASK) == 0)
    {
        /* Reed closed: wait for it to open */
        u32Polarity |= REED_MASK;
    }
    GINT_ConfigPins(GINT0, DEMO_GINT0_PORT, u32Polarity, DEMO_GINT0_ENA_MASK);
    GINT_ClrStatus(GINT0);
    GINT_EnableCallback(GINT0);
}

/* TRUE if the reed level no longer matches the tracked state while no scan
 * is running (transition missed between the end of a scan and the arming of
 * the interrupt). Checked before going to sleep. */
PUBLIC bool_t bAppReedScanNeeded(void)
{
    if (!bReedStateKnown || bDebouncing || (ZTIMER_eGetState(u8TimerButtonScan) == E_ZTIMER_STATE_RUNNING))
    {
        return FALSE;
    }
    return (((APP_u32GetSwitchIOState() & REED_MASK) == 0) != bReedClosed);
}

PUBLIC void vAppStartReedScan(void)
{
    GINT_DisableCallback(GINT0);
    bDebouncing = TRUE;
    ZTIMER_eStop(u8TimerButtonScan);
    ZTIMER_eStart(u8TimerButtonScan, ZTIMER_TIME_MSEC(2));
}

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

// MJL not used in 6x/7x sensor
    /* Any edge: closure, opening of the reed (all inputs high then) or link button */
    (void)u32IOStatus; /* only used by traces (TRACE=0) */
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
        ZTIMER_eStart(u8TimerButtonScan, ZTIMER_TIME_MSEC(20));
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
    DBG_vPrintf(TRACE_APP_BUTTON, "\r\n ------------------ BUTTON ----------------------- \r\n");

    uint32 u32DIOState = APP_u32GetSwitchIOState();
    bool_t bReedSettled;
    bool_t bSw1Settled;

    bDebouncing = TRUE;

    /* SW0: reed contact of the meter */
    bReedSettled = bScanReed(u32DIOState);

    /* SW1: link button (short press, 10 s press for leave and reset) */
    {
        uint8 u8Button = (uint8) ((u32DIOState >> s_u8ButtonDIOLine[SW1_INDEX]) & 1);
        s_u8ButtonDebounce[SW1_INDEX] <<= 1;
        s_u8ButtonDebounce[SW1_INDEX] |= u8Button;

        DBG_vPrintf(TRACE_APP_BUTTON, "\r\nButton 1 Int=%d u32DIOState=%d state=%d \r\n",
                    s_u8ButtonDebounce[SW1_INDEX], u32DIOState, s_u8ButtonState[SW1_INDEX]);

        if ((u8Button == 0) && !s_u8ButtonState[SW1_INDEX])
        {
            s_u8ButtonState[SW1_INDEX] = TRUE;
            u32TimeButton1 = OSA_TimeGetMsec();
            vPostButtonEvent(APP_E_EVENT_BUTTON_DOWN, SW1_INDEX);
        }
        else if (((s_u8ButtonDebounce[SW1_INDEX] & 0x03) == 0x03) && s_u8ButtonState[SW1_INDEX])
        {
            /* Released after two consecutive high samples (masked: a bounce while
             * pressed leaves other bits set, an exact 0x03 match would never come
             * and the scan would run forever, preventing sleep) */
            s_u8ButtonState[SW1_INDEX] = FALSE;
            u32TimeButton1 = OSA_TimeGetMsec() - u32TimeButton1;
            DBG_vPrintf(TRACE_APP_BUTTON, "\r\n----------------u32TimeButton1: %d ---------------------\r\n", u32TimeButton1);
            vPostButtonEvent((u32TimeButton1 < 5000) ? APP_E_EVENT_BUTTON_UP : APP_E_EVENT_LEAVE_AND_RESET, SW1_INDEX);
        }
        bSw1Settled = !s_u8ButtonState[SW1_INDEX] && ((s_u8ButtonDebounce[SW1_INDEX] & 0x03) == 0x03);
    }

    ZTIMER_eStop(u8TimerButtonScan);
    if (bReedSettled && bSw1Settled)
    {
        /* Nothing in progress: wait for the next transition on the interrupt */
        bDebouncing = FALSE;
        DBG_vPrintf(TRACE_APP_BUTTON, "SETTLED, reed %s\r\n", bReedClosed ? "closed" : "open");
        vArmButtonInterrupt();
    }
    else
    {
        /* Reed bouncing or opening, or button held: keep scanning */
        ZTIMER_eStart(u8TimerButtonScan, ZTIMER_TIME_MSEC(2));
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
    eStatus = ZTIMER_eStart(u8TimerButtonScan, ZTIMER_TIME_MSEC(20));
    DBG_vPrintf(TRACE_APP_BUTTON, "\r\nBUTTON: ZTIMER_eStart()=%d", eStatus);
    (void)eStatus; /* only used by traces (TRACE=0) */
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

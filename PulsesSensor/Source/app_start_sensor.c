/****************************************************************************
 *
 * MODULE:             JN-AN-1220 ZLO Sensor Demo
 *
 * COMPONENT:          app_start_sensot.c
 *
 * DESCRIPTION:        ZLO Sensor Application Initialisation and Startup
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
 * Copyright NXP B.V. 2016-2019. All rights reserved
 *
 ***************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <jendefs.h>
#include "app.h"
#include "fsl_wwdt.h"
#include "fsl_iocon.h"
#include "fsl_gpio.h"
#include "fsl_power.h"
#include "pin_mux.h"

#ifdef APP_LOW_POWER_API
#include "PWR_interface.h"
#else
#include "pwrm.h"
#endif

#include "pdum_nwk.h"
#include "pdum_apl.h"
#include "PDM.h"
#include "dbg.h"
#include "pdum_gen.h"
#include "zps_gen.h"
#include "zps_apl_af.h"
#include "AppApi.h"
#include "app_zlo_sensor_node.h"
#include "app_main.h"
#include "app_blink_led.h"
#include "app_pulses_buttons.h"
#include <string.h>
#include "bdb_api.h"
#include "fsl_os_abstraction.h"
#include "app_pulses_sensor_state_machine.h"
#include "fsl_wtimer.h"
#include "DebugExceptionHandlers_jn518x.h"
#ifdef APP_NTAG_ICODE
#include "ntag_nwk.h"
#include "app_ntag_icode.h"
#if (defined JENNIC_CHIP_FAMILY_JN518x)
#include "fsl_power.h"
#endif
#endif
#include "voltage_drv.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifndef DEBUG_START_UP
    #define TRACE_START FALSE
#else
    #define TRACE_START TRUE
#endif

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

PRIVATE void vInitialiseApp(void);
#ifdef SLEEP_ENABLE
    PRIVATE void vSetUpWakeUpConditions(void);
#endif

PRIVATE void vfExtendedStatusCallBack (ZPS_teExtendedStatus eExtendedStatus);
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
extern void *_stack_low_water_mark;

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
/**
 * Power manager Callbacks
 */
#ifdef APP_LOW_POWER_API
static void PreSleep(void);
static void Wakeup(void);
#else
static PWRM_DECLARE_CALLBACK_DESCRIPTOR(PreSleep);
static PWRM_DECLARE_CALLBACK_DESCRIPTOR(Wakeup);
#endif

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/


/****************************************************************************
 *
 * NAME: vAppMain
 *
 * DESCRIPTION:
 * Entry point for application from a cold start.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppMain(void)
{
    wwdt_config_t  config;

    DBG_vPrintf(TRACE_START, "\r\nSTART: vAppMain() ENTER");

    /* Initialise exception handlers for debugging */
    vDebugExceptionHandlersInitialise();

    /* Catch resets due to watchdog timer expiry. Comment out to harden code. */
    WWDT_GetDefaultConfig(&config);

    if (((PMC->RESETCAUSE) & PMC_RESETCAUSE_WDTRESET_MASK) == PMC_RESETCAUSE_WDTRESET_MASK)
    {
        DBG_vPrintf(TRACE_START, "\r\nSTART: Watchdog timer has reset device!");
#ifdef HALT_ON_EXCEPTION
        /* Enable the WWDT clock */
        CLOCK_EnableClock(kCLOCK_WdtOsc);
        RESET_PeripheralReset(kWWDT_RST_SHIFT_RSTn);
        WWDT_Deinit(WWDT);
        POWER_ClearResetCause();
        while (1);
#else
        /* Clear flag */
        PMC->RESETCAUSE = PMC_RESETCAUSE_WDTRESET_MASK;
#endif
    }

    /* initialise ROM based software modules */
    #ifndef JENNIC_MAC_MiniMacShim
    u32AppApiInit(NULL, NULL, NULL, NULL, NULL, NULL);
    #endif

    DBG_vPrintf(TRACE_START, "\r\nSTART: vAppMain() -> APP_vInitResources()");
    APP_vInitResources();

#if 0 //LJM
    /* Set IIC DIO lines to outputs */
    vAHI_DioSetDirection(0, IIC_MASK);
    vAHI_DioSetOutput(IIC_MASK, 0);
#endif

    DBG_vPrintf(TRACE_START, "\r\nSTART: vAppMain() -> APP_vInitialiseApp()");
    vInitialiseApp();

#if (defined APP_NTAG_ICODE)
    DBG_vPrintf(TRACE_START, "\r\nSTART: vAppMain() -> APP_bNtagPdmLoad()");
    /* Didn't start BDB using PDM data ? */
    if (FALSE == APP_bNtagPdmLoad())
#endif
    {
        DBG_vPrintf(TRACE_START, "\r\nSTART: vAppMain() -> BDB_vStart()");
        BDB_vStart();
        if (FALSE == APP_bNodeIsInRunningState()) BDB_eNsStartNwkSteering();
    }

#ifdef APP_NTAG_ICODE
#if (defined JENNIC_CHIP_FAMILY_JN518x)
    /* Not waking from deep sleep ? */
    if (0 == (POWER_GetResetCause() & RESET_WAKE_DEEP_PD))
#else
    /* Not waking from deep sleep ? */
    if (0 == (u16AHI_PowerStatus() & (1 << 11)))
#endif
    {
        DBG_vPrintf(TRACE_START, "\r\nSTART: vAppMain() -> APP_vNtagStart()");
        APP_vNtagStart(OCCUPANCYSENSOR_SENSOR_ENDPOINT);
    }
#endif

    /*switch to XTAL 32M for better timing accuracy*/
    SYSCON -> MAINCLKSEL    =  2;  /* 32 M XTAL */
    SystemCoreClockUpdate();
    OSA_TimeInit();
    DBG_vPrintf(TRACE_START, "\r\nSTART: vAppMain() EXIT");
}

/****************************************************************************
 *
 * NAME: vAppRegisterPWRCallbacks
 *
 * DESCRIPTION:
 * Power manager callback.
 * Called to allow the application to register  sleep and wake callbacks.
 *
 * PARAMETERS:      Name            RW  Usage
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
void vAppRegisterPWRCallbacks(void)
{
#ifdef APP_LOW_POWER_API
    PWR_RegisterLowPowerEnterCallback(PreSleep);
    PWR_RegisterLowPowerExitCallback(Wakeup);
#else
    PWRM_vRegisterPreSleepCallback(PreSleep);
    PWRM_vRegisterWakeupCallback(Wakeup);
#endif
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: vInitialiseApp
 *
 * DESCRIPTION:
 * Initialises Zigbee stack, hardware and application.
 *
 *
 * RETURNS:
 * void
 ****************************************************************************/
PRIVATE void vInitialiseApp(void)
{
    DBG_vPrintf(TRACE_START,"\r\nSTART: vInitialiseApp() ENTER");
    /*
     * Initialise JenOS modules. Initialise Power Manager even on non-sleeping nodes
     * as it allows the device to doze when in the idle task.
     * Parameter options: E_AHI_SLEEP_OSCON_RAMON or E_AHI_SLEEP_DEEP or ...
     */
    bDeepSleep = FALSE;
#ifdef APP_LOW_POWER_API
    (void) PWR_ChangeDeepSleepMode(PWR_E_SLEEP_OSCON_RAMON);
    PWR_Init();
    PWR_vForceRadioRetention(TRUE);
#else
    PWRM_vInit(E_AHI_SLEEP_OSCON_RAMON);
    PWRM_vForceRadioRetention(TRUE);
#endif

    DBG_vPrintf(TRACE_START,"\r\nSTART: vInitialiseApp() -> PDM_eInitialise()");
    PDM_eInitialise(1200, 63, NULL);

    /* Initialise Protocol Data Unit Manager */
    PDUM_vInit();

    ZPS_vExtendedStatusSetCallback(vfExtendedStatusCallBack);

    /* Initialise application */
    DBG_vPrintf(TRACE_START,"\r\nSTART: vInitialiseApp() -> APP_vInitialiseNode()");
    APP_vInitialiseNode();

    DBG_vPrintf(TRACE_START,"\r\nSTART: vInitialiseApp() EXIT, Tick Timer = %d", OSA_TimeGetMsec());
}

/****************************************************************************
 *
 * NAME: vfExtendedStatusCallBack
 *
 * DESCRIPTION:
 *
 * ZPS extended error callback .
 *
 * PARAMETERS:      Name            RW  Usage
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vfExtendedStatusCallBack (ZPS_teExtendedStatus eExtendedStatus)
{
    DBG_vPrintf(TRACE_START,"ERROR: Extended status %x\n", eExtendedStatus);
}

/****************************************************************************
 *
 * NAME: vSetUpWakeUpConditions
 *
 * DESCRIPTION:
 *
 * Set up the wake up inputs while going to sleep.
 *
 * PARAMETERS:      Name            RW  Usage
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vSetUpWakeUpConditions(void)
{

   /*
    * Set the DIO with the right edges for wake up
    * */
    /*Set the LED to inputs to reduce power consumption */
    /*the following pins are connected to LEDs hence drive them low*/
//    APP_vSetLED( LED2, 0);
 //   APP_vSetLED( LED3, 0);

    vSaveDioStateBeforeDeepSleep();

    DBG_vPrintf(TRACE_START, "\r\nSTART: vSetWakeUpConditions() Deep=%d, Buttons=%08x", bDeepSleep, APP_BUTTONS_DIO_MASK);
    if(bDeepSleep)
    {
        DBG_vPrintf(TRACE_START, ", Mask=%08x", APP_BUTTONS_DIO_MASK_FOR_DEEP_SLEEP& (~(1 << APP_BOARD_SW0_PIN)));
#ifdef APP_LOW_POWER_API
        PWR_vWakeUpConfig(APP_BUTTONS_DIO_MASK);
#else
        PWRM_vWakeUpConfig(APP_BUTTONS_DIO_MASK_FOR_DEEP_SLEEP & (~(1 << APP_BOARD_SW4_PIN)));
#endif
    }
    else
    {
        //DBG_vPrintf(TRACE_START, ", Mask=%08x", APP_BUTTONS_DIO_MASK & (~(1 << APP_BOARD_SW4_PIN)));
#ifdef APP_LOW_POWER_API
        PWR_vWakeUpConfig(APP_BUTTONS_DIO_MASK);
#else
        PWRM_vWakeUpConfig(APP_BUTTONS_DIO_MASK & (~(1 << APP_BOARD_SW4_PIN)));
#endif
    }
}

/****************************************************************************
 *
 * NAME: PreSleep
 *
 * DESCRIPTION:
 *
 * PreSleep call back by the power manager before the controller put into sleep.
 *
 * PARAMETERS:      Name            RW  Usage
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
#ifdef APP_LOW_POWER_API
static void PreSleep(void)
#else
PWRM_CALLBACK(PreSleep)
#endif
{
    DBG_vPrintf(TRACE_START,"\r\nSTART: PreSleep() ENTER, Deep = %d, Tick Timer = %d", bDeepSleep, OSA_TimeGetMsec());

    #if 1
    {
        uint32        u32WkSeconds;
        uint32        u32WkRead;
        WTIMER_status_t eWkStatus;

        eWkStatus = WTIMER_GetStatusFlags(WTIMER_TIMER0_ID);
        DBG_vPrintf(TRACE_START, "\r\nSTART: PWRM_CALLBACK(PreSleep), eWk0Status = %d", eWkStatus);
        if (eWkStatus == WTIMER_STATUS_RUNNING)
        {

            u32WkRead = WTIMER_ReadTimer(WTIMER_TIMER0_ID);
            u32WkSeconds = u32WkRead / u32GetWakeTimerCalibration();
            DBG_vPrintf(TRACE_START, " (%d / %ds)", u32WkRead, u32WkSeconds);
        }
        eWkStatus = WTIMER_GetStatusFlags(WTIMER_TIMER1_ID);
        DBG_vPrintf(TRACE_START, "\r\nSTART: PWRM_CALLBACK(PreSleep), eWk1Status = %d", eWkStatus);
        if (eWkStatus == WTIMER_STATUS_RUNNING)
        {

            u32WkRead = WTIMER_ReadTimer(WTIMER_TIMER1_ID);
            u32WkSeconds = u32WkRead / u32GetWakeTimerCalibration();
            DBG_vPrintf(TRACE_START, " (%d / %ds)", u32WkRead, u32WkSeconds);
        }
    }
    #endif


    /*Put off the LEDs Indicators to All off if the device is sleeping while the state is not running,
     * In running the state LED will be used to retain the occupied - unoccupied state.*/
    /*APP_vSetLED( LED2, 0);
    APP_vSetLED( LED3, 0);
    IOCON_PinMuxSet(IOCON, 0, BOARD_LED_D1_BIT, IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN | IOCON_IO_CLAMPING_NORMAL_MFIO);
    IOCON_PinMuxSet(IOCON, 0, BOARD_LED_D2_BIT, IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN | IOCON_IO_CLAMPING_NORMAL_MFIO);
    IOCON_PinMuxSet(IOCON, 0, BOARD_LED_D3_BIT, IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN | IOCON_IO_CLAMPING_NORMAL_MFIO);*/

    /* If the power mode is with RAM held do the following
     * else not required as the entry point will init everything*/
    if(!bDeepSleep)
    {
        /* Save the MAC settings (will get lost though if we don't preserve RAM) */
        vAppApiSaveMacSettings();
    }

    /* Set up wake up input */
    vSetUpWakeUpConditions();

    /* Put ZTimer module to sleep (stop tick timer) */
    ZTIMER_vSleep();

    DBG_vPrintf(TRACE_START,"\r\nSTART: PreSleep() EXIT, Deep = %d -------------------------------\r\n\r\n", bDeepSleep);

    /* Disable debug */
    DbgConsole_Deinit();

    /* Minimize GPIO power consumption */
    BOARD_SetPinsForPowerMode();
}

/****************************************************************************
 *
 * NAME: Wakeup
 *
 * DESCRIPTION:
 *
 * Wakeup call back by  power manager after the controller wakes up from sleep.
 *
 ****************************************************************************/
#ifdef APP_LOW_POWER_API
static void Wakeup(void)
#else
PWRM_CALLBACK(Wakeup)
#endif
{

    // NC! Don't debug before APP_vSetUpHardware()
    //DBG_vPrintf(TRACE_START, "\r\nSTART: PWRM_CALLBACK(Wakeup) ENTER, Woken up (CB)");
    //DBG_vPrintf(TRACE_START, ", powerStatus = 0x%04x", PMC->RESETCAUSE);
    /* If the power status is OK and RAM held while sleeping
     * restore the MAC settings
     * */
    if( !bDeepSleep)
    {
        // NC! APP_vSetUpHardware() before WTIMER_EnableInterrupts()
        // NC! Don't debug before APP_vSetUpHardware()
        //DBG_vPrintf(TRACE_START, "\r\nSTART: PWRM_CALLBACK(Wakeup) -> APP_vSetUpHardware(2)");
        APP_vSetUpHardware(2);
//        DBG_vPrintf(TRACE_START, "\r\nSTART: PWRM_CALLBACK(Wakeup) WARM");

        /* Enable interrupts for wake timer 0 */
        // NC! Only need to do this if using WK0 and want interrupt callbacks
        WTIMER_status_t InterruptStatus;
        InterruptStatus = WTIMER_GetStatusFlags(WTIMER_TIMER0_ID);
        if ( (InterruptStatus==WTIMER_STATUS_RUNNING) || (InterruptStatus==WTIMER_STATUS_EXPIRED) )
        {
//            DBG_vPrintf(TRACE_START, "\r\nSTART: PWRM_CALLBACK(Wakeup) -> WTIMER_EnableInterrupts()");
            WTIMER_EnableInterrupts(WTIMER_TIMER0_ID);
        }

        /* Restore MAC settings (turns radio on) */
        vAppApiRestoreMacSettings();
//        DBG_vPrintf(TRACE_START, "\r\nSTART: MAC settings restored");

        /* Define HIGH_POWER_ENABLE to enable high power module */
        #ifdef HIGH_POWER_ENABLE
            vAHI_HighPowerModuleEnable(TRUE, TRUE);
        #endif

//        DBG_vPrintf(TRACE_START, "\r\nSTART: Restarting, Tick Timer = %d", OSA_TimeGetMsec());
        ZTIMER_vWake();

        /* Ensure we stay awake long enough to handle any button events */
        ZTIMER_eStart(u8TimerMinWake, ZTIMER_TIME_MSEC(50));

        /* Initialise buttons */
        APP_bButtonInitialise();

        /* Read buttons */
//        DBG_vPrintf(TRACE_START, "\r\nSTART: PWRM_CALLBACK(Wakeup) -> vActionOnButtonActivationAfterDeepSleep() SHOULDN'T BE NEEDED");
        vActionOnButtonActivationAfterDeepSleep();

        /* Read voltage */
        //APP_vGetVoltageBattery();
    }
    else
    {
//        DBG_vPrintf(TRACE_START, "\r\nSTART: PWRM_CALLBACK(Wakeup) COLD");
    }

//    DBG_vPrintf(TRACE_START, "\r\nSTART: PWRM_CALLBACK(Wakeup) EXIT");
}

/****************************************************************************
 *
 * NAME: hardware_init
 *
 * DESCRIPTION:
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
void hardware_init(void)
{
    static bool_t bColdStarted = FALSE;

    if (FALSE == bColdStarted)
    {
        if(PMC->RESETCAUSE & ( PMC_RESETCAUSE_WAKEUPIORESET_MASK | PMC_RESETCAUSE_WAKEUPPWDNRESET_MASK ) )
        {
            bDeepSleep = TRUE;
        }
        APP_vSetUpHardware(1);
#ifdef APP_LOW_POWER_API
        PWR_vColdStart();
#else
        PWRM_vColdStart();
#endif
        bColdStarted = TRUE;
    }
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

/*****************************************************************************
 *
 * MODULE:             JN-AN-1220 ZLO Sensor Demo
 *
 * COMPONENT:          app_main.c
 *
 * DESCRIPTION:        ZLO Main event handler (Implementation)
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

#include <stdint.h>
#include "jendefs.h"
#include "app.h"
#include "fsl_wwdt.h"
#include "fsl_iocon.h"
#include "fsl_gpio.h"
#include "pin_mux.h"
#include "ZQueue.h"
#include "ZTimer.h"

#ifdef APP_LOW_POWER_API
#include "PWR_interface.h"
#else
#include "pwrm.h"
#endif

#include "portmacro.h"
#include "zps_apl_af.h"
#include "mac_vs_sap.h"
#include "dbg.h"
#include "app_main.h"
#include "app_pulses_buttons.h"
#include "app_events.h"
#include "app_zcl_sensor_task.h"
#include "PDM.h"
#include "app_zlo_sensor_node.h"
#include "app_nwk_event_handler.h"
#include "app_blink_led.h"
#include "App_PulsesSensor.h"
#include "app_sleep_handler.h"
#ifdef APP_NTAG_ICODE
#include "app_ntag_icode.h"
#endif
#include "temp_sensor_drv.h"
#include "voltage_drv.h"
#include "PDM_IDs.h"
#include "radio.h"
#if (ZIGBEE_USE_FRAMEWORK != 0)
#include "SecLib.h"
#include "RNG_Interface.h"
#include "MemManager.h"
#include "TimersManager.h"
#endif

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#ifndef DEBUG_APP
#define TRACE_APP   FALSE
#else
#define TRACE_APP   TRUE
#endif

#if (defined APP_NTAG_ICODE)
#define TIMER_QUEUE_SIZE             9
#else
#define TIMER_QUEUE_SIZE             8
#endif
#define MLME_QUEUE_SIZE              4
#define MCPS_QUEUE_SIZE              2
#define MCPS_DCFM_QUEUE_SIZE         4
#define ZPS_QUEUE_SIZE               1
#define ZCL_QUEUE_SIZE               1
#define APP_QUEUE_SIZE               4
#define BDB_QUEUE_SIZE               2
#if (defined APP_NTAG_ICODE)
#define APP_ZTIMER_STORAGE           6 /* NTAG: Added timer */
#else
#define APP_ZTIMER_STORAGE           5
#endif

#if (defined JENNIC_CHIP_FAMILY_JN517x)
#define NVIC_INT_PRIO_LEVEL_SYSCTRL (1)
#define NVIC_INT_PRIO_LEVEL_BBC     (7)
#endif

#define APP_WATCHDOG_STACK_DUMP TRUE

/* Interrupt priorities */
#define APP_BASE_INTERRUPT_PRIORITY (5)
#define APP_WATCHDOG_PRIOIRTY       (1)

/* Temperature ADC defines */
#define APP_ADC_BASE                       ADC0
#define APP_ADC_TEMPERATURE_SENSOR_CHANNEL 7U
#define APP_ADC_TEMPERATURE_DELAY_US       300
#define APP_ADC_TEMPERATURE_SAMPLES        8
#define TRACE_MAIN_RADIO                   TRUE
#define RADIO_TEMP_UPDATE                  TRUE

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
PUBLIC uint8 u8TimerButtonScan;
PUBLIC uint8 u8TimerMinWake;
PUBLIC uint8 u8TimerPoll;
PUBLIC uint8 u8TimerBlink;
PUBLIC uint8 u8TimerTick;

#if (defined APP_NTAG_ICODE)
PUBLIC uint8 u8TimerNtag;
#endif

PUBLIC tszQueue APP_msgZpsEvents;
PUBLIC tszQueue APP_msgZclEvents;
PUBLIC tszQueue APP_msgAppEvents;
PUBLIC tszQueue APP_msgBdbEvents;

PUBLIC bool_t APP_bPersistantPolling = FALSE;
PUBLIC volatile bool_t APP_bInitialised = FALSE;

#define WDT_CLK_FREQ CLOCK_GetFreq(kCLOCK_WdtOsc)
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

PRIVATE ZTIMER_tsTimer asTimers[APP_ZTIMER_STORAGE + BDB_ZTIMER_STORAGE];
#if (ZIGBEE_USE_FRAMEWORK == 0)
PRIVATE zps_tsTimeEvent asTimeEvent[TIMER_QUEUE_SIZE];
PRIVATE MAC_tsMcpsVsDcfmInd asMacMcpsDcfmInd[MCPS_QUEUE_SIZE];
PRIVATE MAC_tsMlmeVsDcfmInd asMacMlmeVsDcfmInd[MLME_QUEUE_SIZE];
PRIVATE ZPS_tsAfEvent asZpsStackEvent[ZPS_QUEUE_SIZE];
PRIVATE ZPS_tsAfEvent asZclStackEvent[ZCL_QUEUE_SIZE];
PRIVATE MAC_tsMcpsVsCfmData asMacMcpsDcfm[MCPS_DCFM_QUEUE_SIZE];
PRIVATE APP_tsEvent asAppEvent[APP_QUEUE_SIZE];
PRIVATE BDB_tsZpsAfEvent asBdbEvent[BDB_QUEUE_SIZE];
#endif

extern const uint8_t gUseRtos_c;

static volatile uint8_t wdt_update_count;

gpio_pin_config_t led_config = {
    kGPIO_DigitalOutput, 0,
};

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/


extern void zps_taskZPS(void);
extern void OSA_TimeInit(void);
extern void vAppMain(void);
extern uint32_t OSA_TimeGetMsec(void);

#if APP_WATCHDOG_STACK_DUMP
extern void ExceptionUnwindStack(uint32_t * pt);
#endif
void System_IRQHandler(void)
{
#ifdef WATCHDOG_ALLOWED
    uint32_t wdtStatus = WWDT_GetStatusFlags(WWDT);
    /* The chip should reset before this happens. For this interrupt to occur,
     * it means that the WD timeout flag has been set but the reset has not occurred  */
    if (wdtStatus & kWWDT_TimeoutFlag)
    {
        /* A watchdog feed didn't occur prior to window timeout */
        /* Stop WDT */
        WWDT_Disable(WWDT);
        WWDT_ClearStatusFlags(WWDT, kWWDT_TimeoutFlag);
        DBG_vPrintf(TRUE, "Watchdog timeout flag\r\n");
#if APP_WATCHDOG_STACK_DUMP
        /* 0x4015f28  0x32fc2    (Placed on stack by stack dump function)
         * 0x4015f2c  0xfffffff9 (Placed on stack by stack dump function)
         * 0x4015f30  0x0        R0
         * 0x4015f34  0x114ed    R1
         * 0x4015f38  0x0        R2
         * 0x4015f3c  0x40082100 R3
         * 0x4015f40  0x0        R12
         * 0x4015f44  0xe4d      Link Register (R14) (return address at time of exception)
         * 0x4015f48  0xe4c      Return address (PC at time of exception)
         * 0x4015f4c  0x41000000 PSR */
        ExceptionUnwindStack((uint32_t *) __get_MSP());
#endif
    }

    /* Handle warning interrupt */
    if (wdtStatus & kWWDT_WarningFlag)
    {
        /* A watchdog feed didn't occur prior to warning timeout */
        WWDT_ClearStatusFlags(WWDT, kWWDT_WarningFlag);
#if APP_WATCHDOG_STACK_DUMP
        if (wdt_update_count < 7)
#else
        if (wdt_update_count < 8)
#endif
        {
            /* Feed only for the first 8 warnings then allow for a WDT reset to occur */
            wdt_update_count++;
            WWDT_Refresh(WWDT);
            DBG_vPrintf(TRUE,"Watchdog warning flag %d\r\n", wdt_update_count);
        }
#if APP_WATCHDOG_STACK_DUMP
        else
        {
             DBG_vPrintf(TRUE,"Watchdog last warning %d\r\n", wdt_update_count);
             /* 0x4015f28  0x32fc2    (Placed on stack by stack dump function)
              * 0x4015f2c  0xfffffff9 (Placed on stack by stack dump function)
              * 0x4015f30  0x0        R0
              * 0x4015f34  0x114ed    R1
              * 0x4015f38  0x0        R2
              * 0x4015f3c  0x40082100 R3
              * 0x4015f40  0x0        R12
              * 0x4015f44  0xe4d      Link Register (R14) (return address at time of exception)
              * 0x4015f48  0xe4c      Return address (PC at time of exception)
              * 0x4015f4c  0x41000000 PSR */
             ExceptionUnwindStack((uint32_t *) __get_MSP());
        }
#endif
    }
#endif
}

/****************************************************************************
 *
 * NAME: APP_vMainLoop
 *
 * DESCRIPTION:
 * Main  execution loop
 *
 * RETURNS:
 * Never
 *
 ****************************************************************************/
void main_task (uint32_t parameter)
{

    /* e.g. osaEventFlags_t ev; */
    static uint8_t initialized = FALSE;

    if(!initialized)
    {
        /* place initialization code here... */
        /*myEventId = OSA_EventCreate(TRUE);*/
        initialized = TRUE;
#if (ZIGBEE_USE_FRAMEWORK != 0)
        RNG_Init();
        SecLib_Init();
        MEM_Init();
        TMR_Init();
#endif
        vAppMain();
    }

    APP_bInitialised |= TRUE;

    /* idle task commences on exit from OS start call */
    while (TRUE)
    {
        DBG_vPrintf(TRACE_APP, "ZPS\n\r");
        zps_taskZPS();

        DBG_vPrintf(TRACE_APP, "APP: Entering bdb_taskBDB\n");
        bdb_taskBDB();

        DBG_vPrintf(TRACE_APP, "TMR\n\r");
        ZTIMER_vTask();

        DBG_vPrintf(TRACE_APP, "ZLO\n\r");
        APP_taskSensor();

        DBG_vPrintf(TRACE_APP,"Action\r\n");
        APP_pulsesAction();

#ifdef WATCHDOG_ALLOWED
        /* Re-load the watch-dog timer. Execution must return through the idle
         * task before the CPU is suspended by the power manager. This ensures
         * that at least one task / ISR has executed with in the watchdog period
         * otherwise the system will be reset.
         */
        WWDT_Refresh(WWDT);
        wdt_update_count = 0;
#endif

       // DBG_vPrintf(1, "\r\nAPP: bFlagReset : %d\n",bFlagReset);
       // DBG_vPrintf(1, "\r\nAPP: u32countStart : %d\n",u32countStart);

		/* See if we are able to sleep or not */
		DBG_vPrintf(TRACE_APP, "APP: vAttemptToSleep\n");
		vAttemptToSleep();


	   /*
		 * suspends CPU operation when the system is idle or puts the device to
		 * sleep if there are no activities in progress
	   */
		#ifdef APP_LOW_POWER_API
				(void) PWR_EnterLowPower();
		#else
				PWRM_vManagePower();
		#endif
        if(!gUseRtos_c)
		{
			break;
		}
    }
}

/****************************************************************************
 *
 * NAME: APP_vSetUpHardware
 *
 * DESCRIPTION:
 * Set up interrupts
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_vSetUpHardware(uint8 u8Trace)
{
    wwdt_config_t config;
    uint32_t wdtFreq;
    /* Enable DMA access to RAM (assuming default configuration and MAC
     * buffers not in first block of RAM) */
    *(volatile uint32 *)0x40001000 = 0xE000F733;
    *(volatile uint32 *)0x40001004 = 0x109;
    /* Board pin, clock, debug console init */
    BOARD_InitHardware();
    DBG_vPrintf(TRUE, "\r\nTRUE: APP_vSetUpHardware(%d) ENTER", u8Trace);


    /* Initialise output LED GPIOs */
    GPIO_PinInit(GPIO, APP_BOARD_GPIO_PORT, APP_BASE_BOARD_LED1_PIN, &led_config);

#ifdef WATCHDOG_ALLOWED
    /* The WDT divides the input frequency into it by 4 */
    wdtFreq = WDT_CLK_FREQ/4 ;
    NVIC_EnableIRQ(WDT_BOD_IRQn);
    WWDT_GetDefaultConfig(&config);
    /* Replace default config values where required */
    /*
     * Set watchdog feed time constant to approximately 1s - 8 warnings
     * Set watchdog warning time to 512 ticks after feed time constant
     * Set watchdog window time to 1s
     */
    config.timeoutValue = wdtFreq * 1;
    config.warningValue = 512;
    config.windowValue = wdtFreq * 1;
    /* Configure WWDT to reset on timeout */
    config.enableWatchdogReset = true;
    config.clockFreq_Hz = WDT_CLK_FREQ;

    WWDT_Init(WWDT, &config);
    /* First feed starts the watchdog */
    WWDT_Refresh(WWDT);
#endif

    /* Initialise interrupt priorities */
    int   iAppInterrupt;
    for (iAppInterrupt = NotAvail_IRQn; iAppInterrupt < BLE_WAKE_UP_TIMER_IRQn; iAppInterrupt ++)
    {
        NVIC_SetPriority(iAppInterrupt, APP_BASE_INTERRUPT_PRIORITY);
    }
    NVIC_SetPriority(WDT_BOD_IRQn, APP_WATCHDOG_PRIOIRTY);

    SYSCON -> MAINCLKSEL       = 3;  /*32 MHz FRO */
    SystemCoreClockUpdate();
    OSA_TimeInit();
    APP_vInitLeds();

    DBG_vPrintf(TRUE, "\r\nTRUE: APP_vSetUpHardware(%d) EXIT", u8Trace);
}

/****************************************************************************
 *
 * NAME: APP_vInitResources
 *
 * DESCRIPTION:
 * Initialise resources (timers, queue's etc)
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_vInitResources(void)
{

    /* Initialise the Z timer module */
    ZTIMER_eInit(asTimers, sizeof(asTimers) / sizeof(ZTIMER_tsTimer));

    /* Create Z timers */
    ZTIMER_eOpen(&u8TimerButtonScan,    APP_cbTimerButtonScan,  NULL,   ZTIMER_FLAG_PREVENT_SLEEP);
    ZTIMER_eOpen(&u8TimerMinWake,       NULL,                   NULL,   ZTIMER_FLAG_PREVENT_SLEEP);
    ZTIMER_eOpen(&u8TimerPoll,          APP_cbTimerPoll,        NULL,   ZTIMER_FLAG_PREVENT_SLEEP);
    ZTIMER_eOpen(&u8TimerTick,          APP_cbTimerZclTick,     NULL,   ZTIMER_FLAG_PREVENT_SLEEP);
    ZTIMER_eOpen(&u8TimerBlink,         vAPP_cbBlinkLED,        NULL,   ZTIMER_FLAG_PREVENT_SLEEP);
#if (defined APP_NTAG_ICODE)
    ZTIMER_eOpen(&u8TimerNtag,          APP_cbNtagTimer,        NULL,   ZTIMER_FLAG_PREVENT_SLEEP);
#endif

    /* create all the queues*/
    #if (ZIGBEE_USE_FRAMEWORK == 0)
        ZQ_vQueueCreate(&APP_msgBdbEvents,      BDB_QUEUE_SIZE,           sizeof(BDB_tsZpsAfEvent),    (uint8*)asBdbEvent);
        ZQ_vQueueCreate(&APP_msgZpsEvents,      ZPS_QUEUE_SIZE,           sizeof(ZPS_tsAfEvent),       (uint8*)asZpsStackEvent);
        ZQ_vQueueCreate(&APP_msgZclEvents,      ZCL_QUEUE_SIZE,           sizeof(ZPS_tsAfEvent),       (uint8*)asZclStackEvent);
        ZQ_vQueueCreate(&APP_msgAppEvents,      APP_QUEUE_SIZE,           sizeof(APP_tsEvent),         (uint8*)asAppEvent);
        ZQ_vQueueCreate(&zps_msgMlmeDcfmInd,    MLME_QUEUE_SIZE,          sizeof(MAC_tsMlmeVsDcfmInd), (uint8*)asMacMlmeVsDcfmInd);
        ZQ_vQueueCreate(&zps_msgMcpsDcfmInd,    MCPS_QUEUE_SIZE,          sizeof(MAC_tsMcpsVsDcfmInd), (uint8*)asMacMcpsDcfmInd);
        ZQ_vQueueCreate(&zps_msgMcpsDcfm,       MCPS_DCFM_QUEUE_SIZE,     sizeof(MAC_tsMcpsVsCfmData), (uint8*)asMacMcpsDcfm);
        ZQ_vQueueCreate(&zps_TimeEvents,        TIMER_QUEUE_SIZE,         sizeof(zps_tsTimeEvent),     (uint8*)asTimeEvent);
    #else
        ZQ_vQueueCreate(&APP_msgBdbEvents,      BDB_QUEUE_SIZE,           sizeof(BDB_tsZpsAfEvent),    NULL);
        ZQ_vQueueCreate(&APP_msgZpsEvents,      ZPS_QUEUE_SIZE,           sizeof(ZPS_tsAfEvent),       NULL);
        ZQ_vQueueCreate(&APP_msgZclEvents,      ZCL_QUEUE_SIZE,           sizeof(ZPS_tsAfEvent),       NULL);
        ZQ_vQueueCreate(&APP_msgAppEvents,      APP_QUEUE_SIZE,           sizeof(APP_tsEvent),         NULL);
        ZQ_vQueueCreate(&zps_msgMlmeDcfmInd,    MLME_QUEUE_SIZE,          sizeof(MAC_tsMlmeVsDcfmInd), NULL);
        ZQ_vQueueCreate(&zps_msgMcpsDcfmInd,    MCPS_QUEUE_SIZE,          sizeof(MAC_tsMcpsVsDcfmInd), NULL);
        ZQ_vQueueCreate(&zps_msgMcpsDcfm,       MCPS_DCFM_QUEUE_SIZE,     sizeof(MAC_tsMcpsVsCfmData), NULL);
        ZQ_vQueueCreate(&zps_TimeEvents,        TIMER_QUEUE_SIZE,         sizeof(zps_tsTimeEvent),     NULL);
    #endif
}

PUBLIC void APP_vGetVoltagePourcentage()
{
	uint8_t voltage = sSensor.sPowerConfigServerCluster.u8BatteryVoltage;
	uint8_t pourcentage = 0;
	if (voltage>=32)
	{
		pourcentage=200;
	}else if ((voltage>=22) && (voltage<32)){
		pourcentage =(10 - (32 - voltage)) * 20;
	}else{
		pourcentage = 0;
	}
	DBG_vPrintf(1, "\r\n ----------Pourcentage : %d\r\n",pourcentage );
	sSensor.sPowerConfigServerCluster.u8BatteryPercentageRemaining = pourcentage;


}

PUBLIC void APP_vGetVoltageBattery()
{
	uint16 voltage = Get_BattVolt();
	DBG_vPrintf(1, "\r\n ----------VOLTAGE : %d\r\n",voltage );
	sSensor.sPowerConfigServerCluster.u8BatteryVoltage = (uint8_t)(voltage/ 100);


}

/****************************************************************************
 *
 * NAME: APP_vRadioTempUpdate
 *
 * DESCRIPTION:
 * Callback function from radio driver requesting write of calibration
 * settings to PDM
 *
 * RETURNS:
 * TRUE  - successful
 * FALSE - failed
 *
 ****************************************************************************/
PUBLIC void APP_vRadioTempUpdate(bool_t bLoadCalibration)
{
#if RADIO_TEMP_UPDATE
    bool_t bStatus = TRUE;

    /* Debug */
    DBG_vPrintf(TRACE_MAIN_RADIO, "\r\n%d: APP_vRadioTempUpdate(%d)",OSA_TimeGetMsec(), bLoadCalibration);
    /* Need to load calibration data ? */
    if (bLoadCalibration)
    {
        /* Read temperature and adc calibration parameter once. Store ADC calibration values into ADC register */
        bStatus = load_calibration_param_from_flash(APP_ADC_BASE);
        /* Debug */
        DBG_vPrintf(TRACE_MAIN_RADIO, "\r\n%d: load_calibration_param_from_flash() = %d", OSA_TimeGetMsec(), bStatus);
    }
    /* OK to read temperature */
    if (bStatus)
    {
        /* Temp in 128th degree */
        int32 i32Temp128th;
        /* Debug */
        DBG_vPrintf(TRACE_MAIN_RADIO, "\r\n%d: get_temperature() = ", OSA_TimeGetMsec());
        /* Read temperature (in 128th degree) */
        bStatus = get_temperature(APP_ADC_BASE, APP_ADC_TEMPERATURE_SENSOR_CHANNEL, APP_ADC_TEMPERATURE_DELAY_US, APP_ADC_TEMPERATURE_SAMPLES, &i32Temp128th);
        /* Debug */
        DBG_vPrintf(TRACE_MAIN_RADIO, "%d", bStatus);
        /* Success ? */
        if (bStatus)
        {
            /* Temp in half degree */
            int16 i16Temp2th = (int16)(i32Temp128th / 64);
            /* Debug */
            DBG_vPrintf(TRACE_MAIN_RADIO, ", i32Temp128th = %d, i16Temp2th = %d, real temperature : %d", i32Temp128th, i16Temp2th, ((i32Temp128th*100)/142));
            /* Pass to radio driver */
            vRadio_Temp_Update(i16Temp2th);
            sSensor.sTemperatureMeasurementServerCluster.i16MeasuredValue=((i32Temp128th*100)/142);
        }
    }
#endif
}

#if RADIO_TEMP_UPDATE
/****************************************************************************
 *
 * NAME: bRadioCB_WriteNVM
 *
 * DESCRIPTION:
 * Callback function from radio driver requesting write of calibration
 * settings to PDM
 *
 * RETURNS:
 * TRUE  - successful
 * FALSE - failed
 *
 ****************************************************************************/
PUBLIC bool_t bRadioCB_WriteNVM(uint8 *pu8DataBlock, uint16 u16DataLength)
{
    bool_t bReturn = FALSE;
    PDM_teStatus eStatus;

    eStatus = PDM_eSaveRecordData(PDM_ID_RADIO_SETTINGS, (void *)pu8DataBlock, u16DataLength);

    if (PDM_E_STATUS_OK == eStatus)
    {
        bReturn = TRUE;
    }

    /* Debug */
    DBG_vPrintf(TRACE_MAIN_RADIO, "\r\n%d: bRadioCB_WriteNVM(%d) = %d, eStatus = %d", OSA_TimeGetMsec(), u16DataLength, bReturn, eStatus);

    return bReturn;
}

/****************************************************************************
 *
 * NAME: u16RadioCB_ReadNVM
 *
 * DESCRIPTION:
 * Callback function from radio driver requesting read of calibration
 * settings from PDM
 *
 * RETURNS:
 * Number of bytes read
 *
 ****************************************************************************/
PUBLIC uint16 u16RadioCB_ReadNVM(uint8 *pu8DataBlock, uint16 u16DataLength)
{
    PDM_teStatus eStatus;
    uint16 u16BytesRead = 0;

    eStatus = PDM_eReadDataFromRecord(PDM_ID_RADIO_SETTINGS, (void *)pu8DataBlock, u16DataLength, &u16BytesRead);

    /* Debug */
    DBG_vPrintf(TRACE_MAIN_RADIO, "\r\n%d: u16RadioCB_ReadNVM(%d) = %d, eStatus = %d",OSA_TimeGetMsec(), u16DataLength, u16BytesRead, eStatus);

    return u16BytesRead;
}
#endif

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/


/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

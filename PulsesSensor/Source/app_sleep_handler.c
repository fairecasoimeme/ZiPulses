/*****************************************************************************
 *
 * MODULE:             JN-AN-1220 ZLO Sensor Demo
 *
 * COMPONENT:          app_sleep_handler.c
 *
 * DESCRIPTION:        ZLO Demo : Manages sleep configuration.
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
#include <string.h>
#include "app.h"
#include "dbg.h"
#include "ZTimer.h"
#include "app_main.h"

#ifdef APP_LOW_POWER_API
#include "PWR_interface.h"
#else
#include "pwrm.h"
#endif

#include "PDM.h"
#include "pdum_gen.h"
#include "app_common.h"
#include "zcl_options.h"
#include "app_zbp_utilities.h"
#include "app_sleep_handler.h"
#include "app_zcl_tick_handler.h"
#include "app_zcl_sensor_task.h"
#include "app_zlo_sensor_node.h"
#include "App_PulsesSensor.h"
#include "app_pulses_buttons.h"
#include "app_pulses_events.h"
#include "app_blink_led.h"
#include "app_nwk_event_handler.h"

#include "app_pulses_sensor_state_machine.h"

#include "app_events.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifndef DEBUG_SLEEP_HANDLER
    #define TRACE_SLEEP_HANDLER   FALSE
#else
    #define TRACE_SLEEP_HANDLER   TRUE
#endif

#if ZLO_MAX_REPORT_INTERVAL == 0
    #define MAXIMUM_TIME_TO_SLEEP 10 + 1
#else
    #define MAXIMUM_TIME_TO_SLEEP 60
#endif
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE uint8 u8NumberOfTimersTaskTimers(void);
PRIVATE void vScheduleSleep(void);
PRIVATE void vWakeCallBack(void);
PRIVATE void vStopAllTimers(void);
PRIVATE void vStopNonSleepPreventingTimers(void);
PRIVATE void vStartNonSleepPreventingTimers(void);
PRIVATE uint8 u8NumberOfNonSleepPreventingTimers(void);
PRIVATE void vPurgeTimerCallbacks(void);

PRIVATE uint16 u16FlagReportTimer=ZLO_MAX_REPORT_INTERVAL-1;
PRIVATE uint32 u32OldCounter=0;

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
#ifdef APP_LOW_POWER_API
extern uint8_t mLPMFlag;
#endif

extern bool_t bPreventTickTimerRestart;

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
#ifdef APP_LOW_POWER_API
PUBLIC PWR_tsWakeTimerEvent sWake;
#else
PUBLIC pwrm_tsWakeTimerEvent sWake;
#endif

uint8_t u8NbFailedTransmit;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME:        vAttemptToSleep
 *
 * DESCRIPTION: Checks to see if any software timers are running that may
 * prevent us from going to sleep. If there is none, if wake timer 0 is
 * running, schedule none deep sleep, if there is schedule deep sleep
 * which is checked if its enabled in vScheduleSleep.
 *
 ****************************************************************************/
/* Variables de sécurité pour forcer le sleep */
PRIVATE bool_t bSleepInProgress = FALSE;

PRIVATE uint8 u8ConsecutiveSleepFailures = 0;
PRIVATE uint8 u8SleepCheckFailures = 0;

#define MAX_CONSECUTIVE_SLEEP_FAILURES 10
#define MAX_SLEEP_CHECK_FAILURES 10

PUBLIC void vAttemptToSleep(void)
{
    uint16 u16ActivityCount;
    static uint8 u8ConsecutiveReportSleepAttempts = 0;
    static uint8 u8PollTimerRunningCount = 0;

    /* ===== NOUVEAU : Forcer l'arrêt du Poll si actif trop longtemps ===== */
	if (ZTIMER_eGetState(u8TimerPoll) == E_ZTIMER_STATE_RUNNING)
	{
		u8PollTimerRunningCount++;

		/* Arrêter après 30 cycles (~3 secondes à 100ms par cycle) */
		if (u8PollTimerRunningCount >= 30)
		{
			DBG_vPrintf(TRUE, "\r\n*** SLEEP: Poll timer timeout (%d cycles), FORCING STOP ***", u8PollTimerRunningCount);
			vStopPollTimerTask();
			u8PollTimerRunningCount = 0;

			/* Attendre un peu que l'arrêt prenne effet */
			volatile uint32 delay = 10000;
			while(delay--);
		}
	}
	else
	{
		u8PollTimerRunningCount = 0;
	}

#ifdef APP_LOW_POWER_API
    u16ActivityCount = mLPMFlag;
#else
    u16ActivityCount = PWRM_u16GetActivityCount();
#endif

    uint8 u8NonSleepPreventingTimers = u8NumberOfNonSleepPreventingTimers();
    uint8 u8TimersTaskTimers = u8NumberOfTimersTaskTimers();

    DBG_vPrintf(TRACE_SLEEP_HANDLER, "\r\nSLEEP: Act:%d NonSleep:%d Task:%d WD:%d CheckFail:%d",
                u16ActivityCount, u8NonSleepPreventingTimers, u8TimersTaskTimers,
                u16WatchdogAttemptToSleep, u8SleepCheckFailures);

    if ((u8TimersTaskTimers == 0) && (u8NonSleepPreventingTimers <= 1))
    {
        bSleepInProgress = TRUE;
        bPreventTickTimerRestart = TRUE;

        /* 1. Arrêter les timers */
        vStopNonSleepPreventingTimers();
        APP_vSetLED(LED1, 0);

        /* 2. Détection rapide du scénario "post-report" */
        bool_t bPostReportScenario = (u16ActivityCount >= 5);

        if (bPostReportScenario)
        {
            DBG_vPrintf(TRUE, "\r\nSLEEP: Post-report detected (Act=%d), fast track to force", u16ActivityCount);
            u8ConsecutiveReportSleepAttempts++;

            /* Si c'est la 2ème tentative après report, forcer directement */
            if (u8ConsecutiveReportSleepAttempts >= 2)
            {
                DBG_vPrintf(TRUE, "\r\n*** SLEEP: POST-REPORT FORCE (attempt %d) ***", u8ConsecutiveReportSleepAttempts);

                vStopAllTimers();
#ifdef APP_LOW_POWER_API
                mLPMFlag = 0;
#endif

                /* Courte pause seulement */
                volatile uint32 delay = 50000;
                while(delay--);

                bDeepSleep = FALSE;
                vScheduleSleep();

                uint32 u32WakeTicks = u32WakeTimerSecondsToTicks(MAXIMUM_TIME_TO_SLEEP, TRUE);
                vStartWakeTimer(u32WakeTicks);

                DBG_vPrintf(TRUE, "\r\nSLEEP: POST-REPORT FORCED - will wake in %d seconds", MAXIMUM_TIME_TO_SLEEP);

                /* Reset les compteurs */
                u8SleepCheckFailures = 0;
                u8ConsecutiveSleepFailures = 0;
                u16WatchdogAttemptToSleep = 0;
                u8ConsecutiveReportSleepAttempts = 0;  // Reset après succès

                bPreventTickTimerRestart = FALSE;
                bSleepInProgress = FALSE;
                return;
            }

            /* Première tentative : attente courte */
            volatile uint32 delay = 50000;
            while(delay--);
        }
        else
        {
            /* Reset du compteur si activité normale */
            u8ConsecutiveReportSleepAttempts = 0;

            /* Attente normale */
            volatile uint32 delay = 100000;
            while(delay--);
        }

<<<<<<< Updated upstream
        if (u16ActivityCount>2)
        {
        	DBG_vPrintf(TRACE_SLEEP_HANDLER , "\r\n------------------u16ActivityCount > 3 : \r\n");
        	/*PWR_teStatus result;
        	result = PWR_eRemoveActivity(&sWake);
        	DBG_vPrintf(TRACE_SLEEP_HANDLER , "\r\n------------------PWRM_eRemoveActivity : %d \r\n",result);

        	result = PWRM_eFinishActivity();
        	DBG_vPrintf(TRACE_SLEEP_HANDLER , "\r\n------------------PWRM_eRemoveActivity : %d \r\n",result);*/
=======
        /* 3. Revérifier l'activité */
#ifdef APP_LOW_POWER_API
        u16ActivityCount = mLPMFlag;
#else
        u16ActivityCount = PWRM_u16GetActivityCount();
#endif
        DBG_vPrintf(TRUE, "\r\nSLEEP: After wait, Act:%d", u16ActivityCount);
>>>>>>> Stashed changes

        /* 4. Vérifier si on peut dormir */
        int sleepCheck = PWR_CheckIfDeviceCanGoToSleep();
        DBG_vPrintf(TRACE_SLEEP_HANDLER, "\r\nSLEEP: PWR_CheckIfDeviceCanGoToSleep: %d", sleepCheck);

        if (sleepCheck <= 0)
        {
            u8SleepCheckFailures++;
            DBG_vPrintf(TRUE, "\r\nSLEEP: Check failed (count=%d)", u8SleepCheckFailures);

            /* FORÇAGE RAPIDE après seulement 3 échecs */
            if (u8SleepCheckFailures >= 3)
            {
                DBG_vPrintf(TRUE, "\r\n*** SLEEP: LEVEL 3 - FORCING SLEEP (fast track) ***");

                vStopAllTimers();

#ifdef APP_LOW_POWER_API
                mLPMFlag = 0;
#endif

                /* Courte pause */
                volatile uint32 delay = 50000;
                while(delay--);

                bDeepSleep = FALSE;
                vScheduleSleep();

                uint32 u32WakeTicks = u32WakeTimerSecondsToTicks(MAXIMUM_TIME_TO_SLEEP, TRUE);
                vStartWakeTimer(u32WakeTicks);

                DBG_vPrintf(TRUE, "\r\nSLEEP: FORCED - will wake in %d seconds", MAXIMUM_TIME_TO_SLEEP);

                u8SleepCheckFailures = 0;
                u8ConsecutiveSleepFailures = 0;
                u16WatchdogAttemptToSleep = 0;

                bPreventTickTimerRestart = FALSE;
                bSleepInProgress = FALSE;
                return;
            }

            /* Réautoriser et sortir */
            bPreventTickTimerRestart = FALSE;
            vStartNonSleepPreventingTimers();
            bSleepInProgress = FALSE;
            return;
        }

<<<<<<< Updated upstream
        /* Check if Wake timer 0 is running.*/
        /*if (( (SYSCON->WKT_STAT&SYSCON_WKT_STAT_WKT0_RUNNING_MASK) == SYSCON_WKT_STAT_WKT0_RUNNING_MASK))
        {
        	DBG_vPrintf(TRACE_SLEEP_HANDLER , "\r\nSLEEP: bDeepSleep FALSE");
            bDeepSleep = FALSE;
            vScheduleSleep();
        }
        else
        {
        	DBG_vPrintf(TRACE_SLEEP_HANDLER , "\r\nSLEEP: bDeepSleep TRUE");
        #ifdef CLD_OTA
            // OTA allowing deep sleep ?
            if (bOTADeepSleepAllowed())
            {
                // Set Flag as True
                bDeepSleep = TRUE;
            }
                // OTA not allowing deep sleep
            else
            {
                // Set Flag as False
                bDeepSleep = FALSE;
            }
        #else
            bDeepSleep = TRUE;
        #endif
            vScheduleSleep();
        }*/


		DBG_vPrintf(TRACE_SLEEP_HANDLER , "\r\nSLEEP: bDeepSleep FALSE");
		bDeepSleep = FALSE;
		uint32 u32WakeTicks;
=======
        /* sleepCheck > 0 : Peut dormir normalement */
        if (sleepCheck > 0)
        {
            u16WatchdogAttemptToSleep++;
            u8SleepCheckFailures = 0;
            u8ConsecutiveReportSleepAttempts = 0;  // Reset sur succès
>>>>>>> Stashed changes

            if (u16WatchdogAttemptToSleep > 20)
            {
                DBG_vPrintf(TRUE, "\r\nSLEEP: Too many attempts");
                u8ConsecutiveSleepFailures++;

                if (u8ConsecutiveSleepFailures >= MAX_CONSECUTIVE_SLEEP_FAILURES)
                {
                    DBG_vPrintf(TRUE, "\r\n!!! CRITICAL: Factory reset !!!");
                    APP_vFactoryResetRecords();
                    __disable_irq();
                    RESET_SystemReset();
                }
                else
                {
                    vStopAllTimers();
                    vStartNonSleepPreventingTimers();
                    u16WatchdogAttemptToSleep = 0;
                }

                bPreventTickTimerRestart = FALSE;
                bSleepInProgress = FALSE;
                return;
            }

            if (ZTIMER_eGetState(u8TimerButtonScan) != E_ZTIMER_STATE_RUNNING)
            {
                PWR_AllowDeviceToSleep();
            }

            DBG_vPrintf(TRACE_SLEEP_HANDLER, "\r\nSLEEP: Entering sleep mode (normal)");
            bDeepSleep = FALSE;

            vScheduleSleep();

            DBG_vPrintf(TRUE, "\r\nSLEEP: vScheduleSleep() completed");
            uint32 u32WakeTicks = u32WakeTimerSecondsToTicks(MAXIMUM_TIME_TO_SLEEP, TRUE);
            vStartWakeTimer(u32WakeTicks);

            u8ConsecutiveSleepFailures = 0;
        }

        bPreventTickTimerRestart = FALSE;
        bSleepInProgress = FALSE;
    }
    else
    {
        DBG_vPrintf(TRACE_SLEEP_HANDLER, "\r\nSLEEP: Cannot sleep - TaskTimers:%d NonSleep:%d",
                    u8TimersTaskTimers, u8NonSleepPreventingTimers);
    }
}



/****************************************************************************/
/***        Local Function                                                     ***/
/****************************************************************************/


/****************************************************************************
 *
 * NAME:        vPurgeTimerCallbacks
 *
 * DESCRIPTION: Force multiple stops of u8TimerTick to clear pending callbacks
 *
 ****************************************************************************/
PRIVATE void vPurgeTimerCallbacks(void)
{
    DBG_vPrintf(TRUE, "\r\nSLEEP: Purging timer callbacks...");
    volatile uint32 delay;
    /* Arrêter le timer plusieurs fois pour purger les callbacks en attente */
    for (uint8 i = 0; i < 5; i++)
    {
        if (ZTIMER_eGetState(u8TimerTick) != E_ZTIMER_STATE_STOPPED)
        {
            ZTIMER_eStop(u8TimerTick);
        }

        /* Petite pause entre chaque arrêt */
        delay = 1000;
        while(delay--);
    }

    /* Vérifier l'état final */
    ZTIMER_teState eState = ZTIMER_eGetState(u8TimerTick);
    DBG_vPrintf(TRUE, "\r\nSLEEP: u8TimerTick final state: %d", eState);

    /* Si TOUJOURS en marche, forcer un arrêt brutal */
    if (eState == E_ZTIMER_STATE_RUNNING)
    {
        DBG_vPrintf(TRUE, "\r\nSLEEP: !!! Timer still running, forcing brutal stop");
        vStopAllTimers();  // Arrêt de TOUS les timers

        delay = 10000;
        while(delay--);
    }
}



/****************************************************************************
 *
 * NAME:        u8NumberOfTimersTaskTimers
 *
 * DESCRIPTION: Checks to see if any timers are running that shouldn't be
 * interrupted by sleeping.
 *
 ****************************************************************************/
PRIVATE uint8 u8NumberOfTimersTaskTimers(void)
{
    uint8 u8NumberOfRunningTimers = 0;

    if (bButtonDebounceInProgress())
    {
        DBG_vPrintf(TRACE_SLEEP_HANDLER, "\r\nSLEEP: ButtonScaning");
        u8NumberOfRunningTimers++;
    }

    if (ZTIMER_eGetState(u8TimerButtonScan) == E_ZTIMER_STATE_RUNNING)
   {
	   DBG_vPrintf(TRACE_SLEEP_HANDLER, "\r\nTaskTimer: u8TimerButtonScan");
	   u8NumberOfRunningTimers++;
   }

    if (ZTIMER_eGetState(u8TimerPoll) == E_ZTIMER_STATE_RUNNING)
    {
        DBG_vPrintf(TRACE_SLEEP_HANDLER, "\r\nSLEEP: u8TimerPoll");
        u8NumberOfRunningTimers++;
    }

    if (ZTIMER_eGetState(u8TimerBlink) == E_ZTIMER_STATE_RUNNING)
    {
        DBG_vPrintf(TRACE_SLEEP_HANDLER, "\r\nSLEEP: APP_JoinBlinkTimer");
        u8NumberOfRunningTimers++;
    }

#if (defined APP_NTAG_ICODE)
    if (ZTIMER_eGetState(u8TimerNtag) == E_ZTIMER_STATE_RUNNING)
    {
//        DBG_vPrintf(TRACE_SLEEP_HANDLER, "\r\nSLEEP: APP_TimerNtag");
        u8NumberOfRunningTimers++;
    }
#endif

    return u8NumberOfRunningTimers;
}


/****************************************************************************
 *
 * NAME:        vScheduleSleep
 *
 * DESCRIPTION: If we have deep sleep enabled and we attempting to deep sleep
 * then re-initialise the power manager for deep sleep
 *
 ****************************************************************************/
PRIVATE void vScheduleSleep(void)
{
    uint32 u32WakeMs;

    DBG_vPrintf(TRACE_SLEEP_HANDLER , "\r\nSLEEP: vScheduleSleep()");

#ifdef DEEP_SLEEP_ENABLE
    if (bDeepSleep)
    {
#ifdef APP_LOW_POWER_API
        (void) PWR_ChangeDeepSleepMode(PWR_E_SLEEP_OSCOFF_RAMOFF);
        PWR_Init();
        PWR_vForceRadioRetention(FALSE);
#else
        PWRM_vInit(E_AHI_SLEEP_OSCOFF_RAMOFF);
        PWRM_vForceRadioRetention(FALSE);
#endif
        DBG_vPrintf(TRACE_SLEEP_HANDLER , "\r\nSLEEP: PWRM_vInit(E_AHI_SLEEP_OSCOFF_RAMOFF)");
    }
    else
    {
       #ifdef CLD_OTA
       if(eOTA_GetState() == OTA_DL_PROGRESS)
           u32WakeMs = SENSOR_OTA_SLEEP_IN_SECONDS * APP_PWRM_TICKS_PER_SECOND;
       else
       #endif
          // u32WakeMs = (MAXIMUM_TIME_TO_SLEEP - u32GetNumberOfZCLTicksSinceReport())*APP_PWRM_TICKS_PER_SECOND;
       u32WakeMs = MAXIMUM_TIME_TO_SLEEP * APP_PWRM_TICKS_PER_SECOND;
#ifdef APP_LOW_POWER_API
        PWR_eScheduleActivity(&sWake, u32WakeMs, vWakeCallBack);
#else
        PWRM_eScheduleActivity(&sWake, u32WakeMs, vWakeCallBack);
#endif
        DBG_vPrintf(TRACE_SLEEP_HANDLER , "\r\nSLEEP: PWRM_eScheduleActivity(%dms)", u32WakeMs);
    }
#else
    u32WakeMs = (MAXIMUM_TIME_TO_SLEEP - u32GetNumberOfZCLTicksSinceReport())*APP_PWRM_TICKS_PER_SECOND;
#ifdef APP_LOW_POWER_API
    PWR_eScheduleActivity(&sWake, u32WakeMs, vWakeCallBack);
#else
    PWRM_eScheduleActivity(&sWake, u32WakeMs, vWakeCallBack);
#endif
    DBG_vPrintf(TRACE_SLEEP_HANDLER , "\r\nSLEEP: PWRM_eScheduleActivity(%dms)", u32WakeMs);
#endif

}

PRIVATE void vStopAllTimers()
{

	ZTIMER_vStopAllTimers();

}
/****************************************************************************
 *
 * NAME:        vStopNonSleepPreventingTimers
 *
 * DESCRIPTION: The timers in this function should not stop us from sleep.
 * Stop the timers to reduce the activity count which will prevent sleep.
 *
 ****************************************************************************/
PRIVATE void vStopNonSleepPreventingTimers()
{
	DBG_vPrintf(TRACE_SLEEP_HANDLER , "\r\n------------------u8TimerTick : %d State : %d\r\n",u8TimerTick,ZTIMER_eGetState(u8TimerTick));
	DBG_vPrintf(TRACE_SLEEP_HANDLER , "\r\n------------------u8TimerPoll : %d State : %d\r\n",u8TimerPoll,ZTIMER_eGetState(u8TimerPoll));
	DBG_vPrintf(TRACE_SLEEP_HANDLER , "\r\n------------------u8TimerMinWake : %d State : %d\r\n",u8TimerMinWake,ZTIMER_eGetState(u8TimerMinWake));
	DBG_vPrintf(TRACE_SLEEP_HANDLER , "\r\n------------------u8TimerBlink : %d State : %d\r\n",u8TimerBlink,ZTIMER_eGetState(u8TimerBlink));

	/* Purger le timer Tick de manière agressive */
	vPurgeTimerCallbacks();

	if (ZTIMER_eGetState(u8TimerPoll) != E_ZTIMER_STATE_STOPPED)
		ZTIMER_eStop(u8TimerPoll);

	if (ZTIMER_eGetState(u8TimerMinWake) != E_ZTIMER_STATE_STOPPED)
		ZTIMER_eStop(u8TimerMinWake);

	if (ZTIMER_eGetState(u8TimerBlink) != E_ZTIMER_STATE_STOPPED)
		ZTIMER_eStop(u8TimerBlink);


}

/****************************************************************************
 *
 * NAME:        vStartNonSleepPreventingTimers
 *
 * DESCRIPTION: Start the timers that wont stop us in vAttemptToSleep
 *
 ****************************************************************************/
PRIVATE void vStartNonSleepPreventingTimers(void)
{
    if (ZTIMER_eGetState(u8TimerTick) != E_ZTIMER_STATE_RUNNING)
    {
        ZTIMER_eStart(u8TimerTick, ZCL_TICK_TIME);
    }
}

/****************************************************************************
 *
 * NAME:        u8NumberOfNonSleepPreventingTimers
 *
 * DESCRIPTION: Returns the number of timers that are running that we are
 * prepared to stop before going to sleep.
 *
 ****************************************************************************/
PRIVATE uint8 u8NumberOfNonSleepPreventingTimers(void)
{
    uint8 u8NumberOfRunningTimers = 0;

    if (ZTIMER_eGetState(u8TimerTick) == E_ZTIMER_STATE_RUNNING)
    {
        u8NumberOfRunningTimers++;
        DBG_vPrintf(TRACE_SLEEP_HANDLER, "\r\nu8NumberOfNonSleepPreventingTimers: u8TimerTick");
    }
    if (ZTIMER_eGetState(u8TimerMinWake) == E_ZTIMER_STATE_RUNNING)
	{
		u8NumberOfRunningTimers++;
		DBG_vPrintf(TRACE_SLEEP_HANDLER, "\r\nu8NumberOfNonSleepPreventingTimers: u8TimerMinWake");
	}
    if (ZTIMER_eGetState(u8TimerBlink) == E_ZTIMER_STATE_RUNNING)
	{
		u8NumberOfRunningTimers++;
		DBG_vPrintf(TRACE_SLEEP_HANDLER, "\r\nu8NumberOfNonSleepPreventingTimers: u8TimerBlink");
	}
    /*if (ZTIMER_eGetState(u8TimerPoll) == E_ZTIMER_STATE_RUNNING)
	{
		u8NumberOfRunningTimers++;
		DBG_vPrintf(TRACE_SLEEP_HANDLER, "\r\nu8NumberOfNonSleepPreventingTimers: u8TimerPoll");
	}
    if (ZTIMER_eGetState(u8TimerButtonScan) == E_ZTIMER_STATE_RUNNING)
   	{
   		u8NumberOfRunningTimers++;
   		DBG_vPrintf(TRACE_SLEEP_HANDLER, "\r\nu8NumberOfNonSleepPreventingTimers: u8TimerButtonScan");
   	}*/

    return u8NumberOfRunningTimers;
}

/****************************************************************************
 *
 * NAME: vWakeCallBack
 *
 * DESCRIPTION:
 * Wake up call back called upon wake up by the schedule activity event.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vWakeCallBack(void)
{
    DBG_vPrintf(TRACE_SLEEP_HANDLER, "\r\nSLEEP: vWakeCallBack()");
    /* Réautoriser le redémarrage du timer Tick */
    bPreventTickTimerRestart = FALSE;

#ifndef DEEP_SLEEP_ENABLE
    vUpdateZCLTickSinceSleep();
#endif

    u16WatchdogAttemptToSleep = 0;

	/* Reset le compteur d'échecs de sleep check si on se réveille normalement */
	if (u8SleepCheckFailures > 0)
	{
		u8SleepCheckFailures--;  // Décroissance progressive
		DBG_vPrintf(TRUE, "\r\nSLEEP: SleepCheckFailures decremented to %d", u8SleepCheckFailures);
	}

#ifdef CLD_OTA
    DBG_vPrintf(TRACE_SLEEP_HANDLER, "\r\nSLEEP: eOTA_GetState() : %d",eOTA_GetState());
    //if(eOTA_GetState() != OTA_IDLE)
    if(eOTA_GetState() >= OTA_QUERYIMAGE)
    {
        vStartPollTimer(POLL_TIME_FAST);
    }

    if(eOTA_GetState() == OTA_DL_PROGRESS)
    {
        /* Update for 1 second (1000ms) */
        vRunAppOTAStateMachine(SENSOR_OTA_SLEEP_IN_SECONDS*APP_PWRM_TICKS_PER_SECOND);
    }
    else
    {
        /* Update for normal sleep period */
        //vRunAppOTAStateMachine((MAXIMUM_TIME_TO_SLEEP - u32GetNumberOfZCLTicksSinceReport())*APP_PWRM_TICKS_PER_SECOND);
        vRunAppOTAStateMachine(MAXIMUM_TIME_TO_SLEEP * APP_PWRM_TICKS_PER_SECOND);
    }

    if(bOTA_IsWaitToUpgrade())
    {
        vStopPollTimerTask();
    }
#endif


    /*Start the u8TimerTick to continue the ZCL tasks */
    vStartNonSleepPreventingTimers();

    /* Increment report */
    u16FlagReportTimer++;

    DBG_vPrintf(1, "\r\n-----------u16FlagReportTimer : %d", u16FlagReportTimer);
    if (u16FlagReportTimer >= ZLO_MAX_REPORT_INTERVAL)
    {
    	u16FlagReportTimer=0;
    	APP_tsEvent sAppEvent;
		sAppEvent.eType = APP_E_EVENT_SEND_REPORT;
		if(ZQ_bQueueSend(&APP_msgAppEvents, &sAppEvent) == FALSE)
		{
			DBG_vPrintf(1, "\r\nEVENT PERIODIC SEND ERROR : %d", sAppEvent.eType);
		}
    }else{
		if (u32OldCounter != sDeviceCounter.counter)
		{
			DBG_vPrintf(1, "\r\n******************************send pulses *******************************************\r\n");
			u32OldCounter = sDeviceCounter.counter;

			APP_tsEvent sAppEvent;
			sAppEvent.eType = APP_E_EVENT_PERIODIC_REPORT;
			if(ZQ_bQueueSend(&APP_msgAppEvents, &sAppEvent) == FALSE)
			{
				DBG_vPrintf(1, "\r\nEVENT PERIODIC SEND ERROR : %d", sAppEvent.eType);
			}
		}
    }


    if (u8NbFailedTransmit>=5)
    {
    	DBG_vPrintf(1, "\r\nRESET after too many transmit failed\r\n");
    	vHandleButtonPressedEventRejoin(true);

    }



}


/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

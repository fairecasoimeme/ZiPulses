/*****************************************************************************
 *
 * MODULE:          JN-AN-1220 ZLO Sensor Demo
 *
 * COMPONENT:       app_nwk_event_handler.c
 *
 * DESCRIPTION:     ZLO Demo: Handles all network events like network join/leave
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
#include "dbg.h"
#include "app_events.h"
#include "ZTimer.h"
#include "app_main.h"
#include "App_PulsesSensor.h"
#include "app_common.h"
#include "app_zbp_utilities.h"
#include "app_nwk_event_handler.h"
#include "app_blink_led.h"
#include "PDM.h"
#include "pdum_gen.h"
#include "PDM_IDs.h"
#include "app_zlo_sensor_node.h"
#include "app_event_handler.h"
#ifdef CLD_OTA
#include "app_ota_client.h"
#endif
#include "MicroSpecific.h"
#include "app.h"
#include "app_reporting.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifdef DEBUG_NWK_EVENT_HANDLER
    #define TRACE_NWK_EVENT_HANDLER   TRUE
#else
    #define TRACE_NWK_EVENT_HANDLER  FALSE
#endif

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
PRIVATE uint32 u32PollTime = 0;
PRIVATE uint8 u8RejoinAttempts = 0;
PRIVATE uint16 u16RejoinBackoffWakes = 1;       /* réveils (60 s) avant le prochain rejoin */
PRIVATE uint16 u16WakesSinceRejoinFailure = 0;
#define APP_MAX_JOIN_ATTEMPTS        2          /* scans supplémentaires avant d'abandonner (non appairé) */
#define APP_REJOIN_BACKOFF_MAX_WAKES 32         /* ~32 min max entre deux rejoins */

PUBLIC bool_t bAppRejoinInProgress = FALSE;
PUBLIC bool_t bAppLeaveAndResetPending = FALSE;  /* appui 10 s : leave en cours puis factory reset */
PRIVATE bool_t bReportAtWindowEnd = FALSE;      /* rapport complet à la fin de la fenêtre de poll */

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: vStartPollTimer
 *
 * DESCRIPTION:
 * Function to start polling.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vStartPollTimer(uint32 u32PollInterval)
{
	ZPS_eAplZdoPoll();
	vStopPollTimerTask();
    u32PollTime = u32PollInterval;
    DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\nAPP Starting poll timer with interval %d", u32PollInterval);
    ZTIMER_eStart(u8TimerPoll, u32PollTime);
}

/****************************************************************************
 *
 * NAME: vStopPollTimerTask
 *
 * DESCRIPTION:
 * Function to stop polling.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vStopPollTimerTask()
{
    u32PollTime = 0;
    DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\nAPP Stopping poll timer");
    ZTIMER_eStop(u8TimerPoll);
}
/****************************************************************************
 *
 * NAME: APP_PollTask
 *
 * DESCRIPTION:
 * Poll Task for timed polling.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_cbTimerPoll( void * pvParam)
{
 //   ZPS_eAplZdoPoll();
//    DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\nAPP Poll Handler: Poll Sent, new poll time %d", u32PollTime);
  //  ZTIMER_eStart(u8TimerPoll, u32PollTime);

    /* Fenêtre de poll persistant : décomptée par le timer (donc en temps),
     * pas par les poll confirm qui peuvent s'enchaîner très vite */
    if (APP_bPersistantPolling)
    {
        if (u32countStart == 0)
        {
            DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\r\nPoll: Persistent window elapsed, stopping");
            APP_bPersistantPolling = FALSE;
            vStopPollTimerTask();
            if (bAppLeaveAndResetPending)
            {
                /* Pas de confirmation du leave : factory reset quand même */
                DBG_vPrintf(TRUE, "\r\nLEAVE: no confirm, forcing factory reset");
                vAppFinishLeaveAndReset();
            }
            if (bReportAtWindowEnd)
            {
                /* Fin d'appairage / rejoin : interview et binds terminés,
                 * envoyer index, batterie et température pour montrer que ça marche */
                bReportAtWindowEnd = FALSE;
                vSendAllReportToCoordinator();
                /* Rester éveillé 1 s le temps que les trames partent (et un éventuel retour) */
                vStartPersistentPollWindow(APP_REPORT_POLL_COUNT);
            }
            return;
        }
        u32countStart--;
    }
	vStartPollTimer(u32PollTime);
}

/****************************************************************************
 *
 * NAME: vStartPersistentPollWindow
 *
 * DESCRIPTION:
 * Keep the device awake and polling fast for u32Count POLL_TIME_FAST periods
 * so the coordinator can interview / bind / configure it.
 *
 ****************************************************************************/
PUBLIC void vStartPersistentPollWindow(uint32 u32Count)
{
    DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\r\nPoll: Persistent window start (%d)", u32Count);
    u32countStart = u32Count;
    APP_bPersistantPolling = TRUE;
    vStartPollTimer(POLL_TIME_FAST);
}

/****************************************************************************
 *
 * NAME: vExtendPersistentPollWindow
 *
 * DESCRIPTION:
 * Called on each message received from the network: if a persistent window
 * is running, make sure it lasts at least APP_PAIRING_POLL_EXTEND more periods.
 *
 ****************************************************************************/
PUBLIC void vRequestReportAtPollWindowEnd(void)
{
    bReportAtWindowEnd = TRUE;
}

PUBLIC void vExtendPersistentPollWindow(void)
{
    if (APP_bPersistantPolling && (u32countStart < APP_PAIRING_POLL_EXTEND))
    {
        u32countStart = APP_PAIRING_POLL_EXTEND;
    }
}

/****************************************************************************
 *
 * NAME: vHandlePollResponse
 *
 * DESCRIPTION:
 * Processes the poll response, Poll again if we have no ACK to force the Stack
 * to handle the failed poll count or poll to get the data.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE uint8 u8ConsecutivePollCount = 0;
#define MAX_CONSECUTIVE_POLLS 5

PUBLIC void vHandlePollResponse(ZPS_tsAfEvent* psStackEvent)
{
    DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\nAPP NWK Event Handler: Poll Response (status=%d)",
                psStackEvent->uEvent.sNwkPollConfirmEvent.u8Status);

    switch (psStackEvent->uEvent.sNwkPollConfirmEvent.u8Status)
    {
    case MAC_ENUM_SUCCESS:
        DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\r\nPoll: SUCCESS");

        /* SUCCESS : données reçues */
        u8ConsecutivePollCount = 0;

        /* Si c'est un poll persistant (pairing), re-poll : d'autres trames peuvent attendre */
        if (APP_bPersistantPolling)
        {
            DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\r\nPoll: Persistent mode, re-polling");
            vExtendPersistentPollWindow();
            ZPS_eAplZdoPoll();
        }
        else
        {
            /* Sinon (poll de rapport), arrêter */
            DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\r\nPoll: Normal mode, stopping");
            vStopPollTimerTask();
        }
        break;

    case MAC_ENUM_NO_ACK:
        DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\r\nPoll: NO_ACK");

        u8ConsecutivePollCount++;

        if (APP_bPersistantPolling)
        {
            /* Mode persistant : le timer de poll relancera, ne pas l'arrêter */
            break;
        }

        /* Re-poll avec limite */
        if (u8ConsecutivePollCount < MAX_CONSECUTIVE_POLLS)
        {
            DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\r\nPoll: Retrying (%d/%d)",
                       u8ConsecutivePollCount, MAX_CONSECUTIVE_POLLS);
            ZPS_eAplZdoPoll();
        }
        else
        {
            DBG_vPrintf(TRUE, "\r\nPoll: Max retries reached, stopping");
            u8ConsecutivePollCount = 0;
            vStopPollTimerTask();
        }
        break;

    case MAC_ENUM_NO_DATA:
        DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\r\nPoll: NO_DATA");

        /* NO_DATA : pas de données en attente */
        u8ConsecutivePollCount = 0;

        /* Si mode persistant (pairing), le timer de poll continue (pas de re-poll immédiat) */
        if (APP_bPersistantPolling)
        {
            DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\r\nPoll: Persistent mode, waiting next poll");
        }
        else
        {
            /* Sinon arrêter */
            DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\r\nPoll: Normal mode, stopping");
            vStopPollTimerTask();
        }
        break;

    default:
        DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\r\nPoll: Other status %d",
                   psStackEvent->uEvent.sNwkPollConfirmEvent.u8Status);
        u8ConsecutivePollCount = 0;
        if (!APP_bPersistantPolling)
        {
            vStopPollTimerTask();
        }
        break;
    }

    u16WatchdogAttemptToSleep = 0;

    /* La fenêtre de pairing est décomptée dans APP_cbTimerPoll */
    DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\r\nu32countStart: %d, APP_bPersistantPolling: %d",
               u32countStart, APP_bPersistantPolling);
}

/****************************************************************************
 *
 * NAME: APP_cbTimerRejoin
 *
 * DESCRIPTION:
 * Callback du timer de rejoin pour relancer une tentative
 *
 ****************************************************************************/
PUBLIC void APP_cbTimerRejoin(void *pvParam)
{
    DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\nRetrying rejoin...");

    /* Arrêter le polling pendant la tentative de rejoin */
    vStopPollTimerTask();

    /* Tenter un rejoin */
    if (ZPS_eAplZdoRejoinNetwork(FALSE) != ZPS_E_SUCCESS)
    {
        DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\nRejoin attempt failed");
        /* Si l'appel échoue immédiatement, programmer une nouvelle tentative */
        vHandleFailedRejoin();
    }
}

/****************************************************************************
 *
 * NAME: vHandleFailedToJoin
 *
 * DESCRIPTION:
 * If we have failed to join/rejoin the network stop the blink timer to
 * go back to sleep
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vHandleFailedToJoin(void)
{
    /* In case Find And Bind or Keep alive in progress stop it and make sure we go to sleep */
	APP_bPersistantPolling = FALSE;
	vStopPollTimerTask();
	bBDBJoinFailed = TRUE;
	bAppRejoinInProgress = FALSE;
	/* LED éteinte : on va dormir */
	vStopBlinkTimer();
	APP_vSetLED(LED1, 0);

	if (!APP_bNodeIsInRunningState())
	{
		/* Pas encore appairé : quelques scans seulement, puis on dort.
		 * Un appui sur le bouton relance la recherche (pas de reset en boucle) */
		DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\nAPP NWK Event Handler: Failed Join - Attempt %d/%d",
					u8RejoinAttempts + 1, APP_MAX_JOIN_ATTEMPTS);
		if (u8RejoinAttempts < APP_MAX_JOIN_ATTEMPTS)
		{
			u8RejoinAttempts++;
			BDB_eNsStartNwkSteering();
		}
		else
		{
			DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\nJoin given up, waiting for button press");
		}
	}
	else
	{
		/* Déjà appairé (parent / coordinateur absent) : pas de network steering
		 * (il ouvrirait le réseau sans rejoindre) ni de reset en boucle.
		 * On dort et le rejoin est retenté aux réveils avec un backoff. */
		DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\nAPP NWK Event Handler: Failed Rejoin, next try in %d wakes",
					u16RejoinBackoffWakes);
		u16WakesSinceRejoinFailure = 0;
	}
}

/****************************************************************************
 *
 * NAME: vHandleFailedRejoin
 *
 * DESCRIPTION:
 * If we have failed to rejoin the network we start the blink timer.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vHandleFailedRejoin(void)
{
	vHandleFailedToJoin();
}

/****************************************************************************
 *
 * NAME: vResetRejoinAttempts
 *
 * DESCRIPTION:
 * Réinitialiser les compteurs (succès, ou relance manuelle par le bouton)
 *
 ****************************************************************************/
PUBLIC void vResetRejoinAttempts(void)
{
    u8RejoinAttempts = 0;
    u16RejoinBackoffWakes = 1;
    u16WakesSinceRejoinFailure = 0;
}

/****************************************************************************
 *
 * NAME: vAppStartRejoin
 *
 * DESCRIPTION:
 * Rejoin du réseau connu (noeud déjà appairé)
 *
 ****************************************************************************/
PUBLIC void vAppStartRejoin(void)
{
    bAppRejoinInProgress = TRUE;
    sBDB.sAttrib.bbdbNodeIsOnANetwork = TRUE;
    BDB_vStart();
}

/****************************************************************************
 *
 * NAME: vRejoinBackoffOnWake
 *
 * DESCRIPTION:
 * Appelé à chaque réveil périodique : si le rejoin a échoué, le retenter
 * après 1, 2, 4 ... APP_REJOIN_BACKOFF_MAX_WAKES réveils pour économiser la pile
 *
 ****************************************************************************/
PUBLIC void vRejoinBackoffOnWake(void)
{
    if (!bBDBJoinFailed || bAppRejoinInProgress || !APP_bNodeIsInRunningState())
    {
        return;
    }

    u16WakesSinceRejoinFailure++;
    if (u16WakesSinceRejoinFailure < u16RejoinBackoffWakes)
    {
        return;
    }

    u16WakesSinceRejoinFailure = 0;
    if (u16RejoinBackoffWakes < APP_REJOIN_BACKOFF_MAX_WAKES)
    {
        u16RejoinBackoffWakes *= 2;
    }
    DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\nRejoin backoff: retrying (next in %d wakes)", u16RejoinBackoffWakes);
    vAppStartRejoin();
}

/****************************************************************************
 *
 * NAME: vHandleNetworkJoinEndDevice
 *
 * DESCRIPTION:
 * If we have joined a new network or rejoined, stop the timer tell the PIR
 * event handler.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vHandleNetworkJoinEndDevice(void)
{
    DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\nAPP NWK Event Handler: Network Join End Device");

    ZPS_eAplAibSetApsUseExtendedPanId( ZPS_u64NwkNibGetEpid(ZPS_pvAplZdoGetNwkHandle()) );

    /* Réinitialiser les compteurs de rejoin en cas de succès */
    vResetRejoinAttempts();

    /* Don't turn the timers off if we're in persistent polling mode */
    if(APP_bPersistantPolling != TRUE)
    {
        vStopPollTimerTask();
        vStopBlinkTimer();
    }



#ifdef CLD_OTA
    sDeviceDesc.eNodeState = E_RUNNING;
    /* Restart OTA if in Download State */
    if(eOTA_GetState() == OTA_DL_PROGRESS)
    {
        vRunAppOTAStateMachine(((SENSOR_OTA_SLEEP_IN_SECONDS + 1) * APP_PWRM_TICKS_PER_SECOND));
        vStartPollTimer(POLL_TIME_FAST);
    }
#endif

    vHandleNewJoinEvent();

#if (defined APP_NTAG_ICODE)
    sDeviceDesc.eNodeState = E_RUNNING;
    PDM_eSaveRecordData(PDM_ID_APP_SENSOR,
            &sDeviceDesc,
            sizeof(tsDeviceDesc));
    ZPS_vSaveAllZpsRecords();
#endif
}

/****************************************************************************
 *
 * NAME: vHandleNetworkLeave
 *
 * DESCRIPTION:
 * We have left the network so restart as factory new (not sure why we don't
 * restart the joining rather than restarting the whole device).
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vHandleNetworkLeave(ZPS_tsAfEvent* psStackEvent)
{
    DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\nAPP NWK Event Handler: Network Leave");
    if( psStackEvent->uEvent.sNwkLeaveIndicationEvent.u64ExtAddr == 0 &&
            (psStackEvent->uEvent.sNwkLeaveIndicationEvent.u8Rejoin == FALSE))
    {
        DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\nAPP NWK Event Handler: ZDO Leave" );
        vAppFinishLeaveAndReset();
    }
}

/****************************************************************************
 *
 * NAME: vAppStartLeaveAndReset
 *
 * DESCRIPTION:
 * Appui long (10 s) : quitter le réseau puis factory reset. Le device reste
 * éveillé (fenêtre de poll) jusqu'à la confirmation du leave, avec un timeout,
 * pour ne jamais rester à moitié réinitialisé (PDM effacée mais encore appairé).
 *
 ****************************************************************************/
PUBLIC void vAppStartLeaveAndReset(void)
{
    bAppLeaveAndResetPending = TRUE;

    if ((sDeviceDesc.eNodeState == E_RUNNING) &&
        (ZPS_eAplZdoLeaveNetwork(0UL, FALSE, FALSE) == ZPS_E_SUCCESS))
    {
        DBG_vPrintf(TRUE, "\r\nLEAVE: request sent, waiting for confirm");
        vStartPersistentPollWindow(APP_LEAVE_TIMEOUT_COUNT);
    }
    else
    {
        vAppFinishLeaveAndReset();
    }
}

/****************************************************************************
 *
 * NAME: vAppFinishLeaveAndReset
 *
 * DESCRIPTION:
 * Factory reset (toutes les données effacées si appui long) puis reboot
 *
 ****************************************************************************/
PUBLIC void vAppFinishLeaveAndReset(void)
{
    if (bAppLeaveAndResetPending)
    {
        PDM_vDeleteAllDataRecords();
    }
    APP_vFactoryResetRecords();
    MICRO_DISABLE_INTERRUPTS();
    RESET_SystemReset();
}

/****************************************************************************
 *
 * NAME: vHandleNetworkJoinAndRejoin
 *
 * DESCRIPTION:
 * Checks to see if we have joined a network. If we have, stop all the timers,
 * save the network state into PDM and attempt to sleep.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vHandleNetworkJoinAndRejoin(void)
{

    DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\nDEVICE_IN_NETWORK");
    ZTIMER_eStop(u8TimerPoll);
    vStopBlinkTimer();
    sDeviceDesc.eNodeState = E_RUNNING;

    PDM_eSaveRecordData(PDM_ID_APP_SENSOR,
                        &sDeviceDesc,
                        sizeof(tsDeviceDesc));
    ZPS_vSaveAllZpsRecords();


}

/****************************************************************************
 *
 * NAME: vHandleNetworkLeaveConfirm
 *
 * DESCRIPTION:
 * We have left the network so restart as factory new (not sure why we don't
 * restart the joining rather than restarting the whole device).
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vHandleNetworkLeaveConfirm(ZPS_tsAfEvent* psStackEvent)
{
    DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\nAPP NWK Event Handler: Network Leave Confirm Addr %016llx",
                psStackEvent->uEvent.sNwkLeaveConfirmEvent.u64ExtAddr);
    if ( psStackEvent->uEvent.sNwkLeaveConfirmEvent.u64ExtAddr == 0UL)
    {
        vAppFinishLeaveAndReset();
    }
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

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
PRIVATE uint32 u32RejoinBackoffTime = 10; // Par ex. 10 secondes
#define APP_MAX_REJOIN_ATTEMPTS 5
#define APP_REJOIN_BACKOFF_INITIAL 10000  // 10 secondes en ms
#define APP_REJOIN_BACKOFF_MAX 300000     // 5 minutes max

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
	vStartPollTimer(u32PollTime);
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

        /* Si c'est un poll persistant (pairing), re-poll */
        if (u32countStart > 0 || APP_bPersistantPolling)
        {
            DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\r\nPoll: Persistent mode, re-polling");
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

        /* Si mode persistant (pairing), continuer à poller */
        if (u32countStart > 0 || APP_bPersistantPolling)
        {
            DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\r\nPoll: Persistent mode, continuing");
            ZPS_eAplZdoPoll();
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
        vStopPollTimerTask();
        break;
    }

    u16WatchdogAttemptToSleep = 0;

    /* Gestion du compteur de pairing */
    DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\r\nu32countStart: %d, APP_bPersistantPolling: %d",
               u32countStart, APP_bPersistantPolling);

    if ((u32countStart == 0) && (APP_bPersistantPolling))
    {
        DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\r\nPoll: Pairing complete, stopping persistent polling");
        APP_bPersistantPolling = FALSE;
        vStopPollTimerTask();
    }
    else if (u32countStart > 0)
    {
        u32countStart--;
        DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\r\nPoll: Pairing countdown: %d remaining", u32countStart);
    }
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
    //DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\nAPP NWK Event Handler: Failed Rejoin");
    /* In case Find And Bind or Keep alive in progress stop it and make sure we go to sleep */
    /*vEventStopFindAndBind();
    APP_bPersistantPolling &= FALSE;
    bBDBJoinFailed = TRUE;*/
	//vEventStopFindAndBind();
	APP_bPersistantPolling &= FALSE;
	bBDBJoinFailed = TRUE;
    DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\nAPP NWK Event Handler: Failed Rejoin - Attempt %d/%d",
    	                u8RejoinAttempts + 1, APP_MAX_REJOIN_ATTEMPTS);
	if (u8RejoinAttempts < APP_MAX_REJOIN_ATTEMPTS)
	{

		/* Programmer une nouvelle tentative avec backoff exponentiel */
		DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\nScheduling rejoin in %d ms", u32RejoinBackoffTime);
		u8RejoinAttempts++;
		BDB_eNsStartNwkSteering();

	}
	else
	{
		DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\nMax rejoin attempts reached - going to reset");
		/* Après trop d'échecs, retour aux paramètres d'usine */
		RESET_SystemReset();
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
	/* Start Blink Timer to avoid sleeping */
	/*if(ZTIMER_eGetState(u8TimerBlink) != E_ZTIMER_STATE_RUNNING)
		vStartBlinkTimer(APP_JOINING_BLINK_TIME);*/
	APP_bPersistantPolling &= FALSE;
	bBDBJoinFailed = TRUE;
	DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\nAPP NWK Event Handler: Failed Rejoin - Attempt %d/%d",
						u8RejoinAttempts + 1, APP_MAX_REJOIN_ATTEMPTS);
	if (u8RejoinAttempts < APP_MAX_REJOIN_ATTEMPTS)
	{

		/* Programmer une nouvelle tentative avec backoff exponentiel */
		DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\nScheduling rejoin in %d ms", u32RejoinBackoffTime);
		u8RejoinAttempts++;
		BDB_eNsStartNwkSteering();

	}
	else
	{
		DBG_vPrintf(TRACE_NWK_EVENT_HANDLER, "\nMax rejoin attempts reached - going to reset");
		/* Après trop d'échecs, retour aux paramètres d'usine */
		RESET_SystemReset();
	}
}

/****************************************************************************
 *
 * NAME: vResetRejoinAttempts
 *
 * DESCRIPTION:
 * Réinitialiser les compteurs en cas de succès
 *
 ****************************************************************************/
PUBLIC void vResetRejoinAttempts(void)
{
    u8RejoinAttempts = 0;
    u32RejoinBackoffTime = APP_REJOIN_BACKOFF_INITIAL;

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
        APP_vFactoryResetRecords();
        MICRO_DISABLE_INTERRUPTS();
        RESET_SystemReset();
    }
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
        APP_vFactoryResetRecords();
        MICRO_DISABLE_INTERRUPTS();
        RESET_SystemReset();
    }
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

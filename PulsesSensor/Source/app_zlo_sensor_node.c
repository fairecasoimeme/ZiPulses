/*****************************************************************************
 *
 * MODULE:             JN-AN-1220 ZLO Sensor Demo
 *
 * COMPONENT:          app_zlo_sensor_node.c
 *
 * DESCRIPTION:        ZLO Demo : Stack <-> Occupancy Sensor App Interaction
 *                     (Implementation)
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
#include <AppApi.h>
#include "ZTimer.h"
#include "ZQueue.h"
#include "app_main.h"
#include "pdum_apl.h"
#include "PDM.h"
#include "dbg.h"
#include "zps_gen.h"
#include "app_common.h"
#include "app_blink_led.h"
#include "Groups.h"
#include "PDM_IDs.h"
#include "app.h"
#include "app_zlo_sensor_node.h"
#include "app_zcl_sensor_task.h"
#include "app_events.h"
#include "zcl_customcommand.h"
#include "app_pulses_sensor_state_machine.h"
#include "app_reporting.h"
#include "app_pulses_buttons.h"
#include "app_event_handler.h"
#include "app_nwk_event_handler.h"
#include "app_blink_led.h"
#include "bdb_api.h"
#ifdef APP_NTAG_ICODE
#include "app_ntag_icode.h"
#include "nfc_nwk.h"
#endif
#ifdef CLD_OTA
    #include "app_ota_client.h"
#endif
#include "MicroSpecific.h"
#include "voltage_drv.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifdef DEBUG_SENSOR_NODE
    #define TRACE_SENSOR_NODE   TRUE
#else
    #define TRACE_SENSOR_NODE   FALSE
#endif

#define WAKE_FROM_DEEP_SLEEP    (1<<11)




/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void app_vStartNodeFactoryNew(void);
PRIVATE void app_vRestartNode (void);
PRIVATE void vAppHandleZdoEvents( BDB_tsZpsAfEvent *psZpsAfEvent);
PRIVATE void vDeletePDMOnButtonPress(uint8 u8ButtonDIO);
PRIVATE void vAppHandleAfEvent( BDB_tsZpsAfEvent* psZpsAfEvent);
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
PUBLIC bool_t bDeepSleep;
PUBLIC tsDeviceDesc           sDeviceDesc;
PUBLIC tsDeviceCounter        sDeviceCounter;
PUBLIC bool_t bBDBJoinFailed = FALSE;
PUBLIC bool_t bFlagReset = FALSE;
PUBLIC uint32_t u32countStart=0;
PUBLIC uint16 u16WatchdogAttemptToSleep=0;


/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
uint8 u8NoQueryCount = 0;
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
#ifdef K32WMCM_APP_BUILD
/* Must be called before zps_eAplAfInit() */
void APP_SetHighTxPowerMode();

/* Must be called after zps_eAplAfInit() */
void APP_SetMaxTxPower();

#undef HIGH_TX_PWR_LIMIT
#define HIGH_TX_PWR_LIMIT 15	/* dBm */
/* High Tx power */
void APP_SetHighTxPowerMode()
{
	if (CHIP_IS_HITXPOWER_CAPABLE())
		vMMAC_SetTxPowerMode(TRUE);
}

void APP_SetMaxTxPower()
{
	if (CHIP_IS_HITXPOWER_CAPABLE())
		ZPS_eMacPlmeSet(PHY_PIB_ATTR_TX_POWER, HIGH_TX_PWR_LIMIT);
}
#endif

/****************************************************************************
 *
 * NAME: APP_vInitialiseNode
 *
 * DESCRIPTION:
 * Initialises the application related functions
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_vInitialiseNode(void)
{
    PDM_teStatus eStatusReportReload;
    DBG_vPrintf(TRACE_SENSOR_NODE, "\r\nNODE: APP_vInitialiseNode() ENTER");

    /* We need to get the previous state out off NVM and save it*/
    //sSensor.sOccupancySensingServerCluster.u8Occupancy = bGetPreSleepOccupancyState();
    //APP_vSetLED(LED1, sSensor.sOccupancySensingServerCluster.u8Occupancy);

#ifdef CLD_OTA
    vLoadOTAPersistedData();
#endif

    /* Restore any report data that is previously saved to flash */
    eStatusReportReload = eRestoreReports();
    uint16 u16ByteRead;
    PDM_eReadDataFromRecord(PDM_ID_APP_SENSOR,
                            &sDeviceDesc,
                            sizeof(tsDeviceDesc),
                            &u16ByteRead);

    sDeviceCounter.counter=0;
    sDeviceCounter.multiplier=1;
    sDeviceCounter.divisor=1;
    sDeviceCounter.unitMeasure=0;
    PDM_eReadDataFromRecord(PDM_ID_APP_COUNTER,
                                &sDeviceCounter,
                                sizeof(tsDeviceCounter),
                                &u16ByteRead);
	sSensor.sSimpleMeteringServerCluster.u48CurrentSummationDelivered = sDeviceCounter.counter;
	sSensor.sSimpleMeteringServerCluster.u24Multiplier = sDeviceCounter.multiplier;
	sSensor.sSimpleMeteringServerCluster.u24Divisor = sDeviceCounter.divisor;
	if (sDeviceCounter.multiplier>1)
	{
		sSensor.sSimpleMeteringServerCluster.u48CurrentTier1SummationDelivered = sDeviceCounter.multiplier * sDeviceCounter.counter;
	}
	if (sDeviceCounter.divisor>1)
	{
		sSensor.sSimpleMeteringServerCluster.u48CurrentTier1SummationDelivered = sDeviceCounter.counter / sDeviceCounter.divisor ;
	}
	if ((sDeviceCounter.divisor==1) && (sDeviceCounter.multiplier==1))
	{
		sSensor.sSimpleMeteringServerCluster.u48CurrentTier1SummationDelivered = sDeviceCounter.counter;
	}
	sSensor.sSimpleMeteringServerCluster.eUnitOfMeasure = sDeviceCounter.unitMeasure;

    APP_vRadioTempUpdate(TRUE);

	
#ifdef K32WMCM_APP_BUILD
    APP_SetHighTxPowerMode();
#endif

    /* Initialize ZBPro stack */
    ZPS_eAplAfInit();
    /*MCZB-919 : synchronize the state of ZPS with application*/
    if(sDeviceDesc.eNodeState == E_RUNNING && ZPS_u16MacPibGetPanId() == 0xFFFF)
    {
	 if(ZPS_psAplAibGetAib()->u64ApsUseExtendedPanid != 0)
	 {
	 	ZPS_vNwkNibSetExtPanId(ZPS_pvAplZdoGetNwkHandle(), ZPS_psAplAibGetAib()->u64ApsUseExtendedPanid);
		ZPS_eAplAfReInit();
	 }
    }

#ifdef K32WMCM_APP_BUILD
    APP_SetMaxTxPower();
#endif

    DBG_vPrintf(TRACE_SENSOR_NODE, "\r\nNODE: APP_vInitialiseNode() -> ZPS_eAplAfInit()");

    APP_ZCL_vInitialise();

    /* Set end device age out time to 11 days 9 hours & 4 mins */
    ZPS_bAplAfSetEndDeviceTimeout(ZED_TIMEOUT_16384_MIN);

    /*Load the reports from the PDM or the default ones depending on the PDM load record status*/
    if(eStatusReportReload !=PDM_E_STATUS_OK )
    {
        /*Load Defaults if the data was not correct*/
        vLoadDefaultConfigForReportable();
    }
    /*Make the reportable attributes */
    vMakeSupportedAttributesReportable();

    /* If the device state has been restored from flash, re-start the stack
     * and set the application running again.
     */
    sBDB.sAttrib.u32bdbPrimaryChannelSet = BDB_PRIMARY_CHANNEL_SET;
    sBDB.sAttrib.u32bdbSecondaryChannelSet = BDB_SECONDARY_CHANNEL_SET;
    BDB_tsInitArgs sInitArgs;
    sInitArgs.hBdbEventsMsgQ = &APP_msgBdbEvents;

    BDB_vInit(&sInitArgs);

    if (APP_bNodeIsInRunningState())
    {
        app_vRestartNode();
        sBDB.sAttrib.bbdbNodeIsOnANetwork = TRUE;
    }
    else
    {
        DBG_vPrintf(TRACE_SENSOR_NODE, "\r\nNODE: APP_vInitialiseNode() -> app_vStartNodeFactoryNew()");
        app_vStartNodeFactoryNew();
        sBDB.sAttrib.bbdbNodeIsOnANetwork = FALSE;
        vStartBlinkTimer(50);
    }

    /* Delete PDM if required */
    APP_bButtonInitialise();
    //vDeletePDMOnButtonPress(APP_BOARD_SW0_PIN);

    APP_vGetVoltageBattery();
    APP_vRadioTempUpdate(TRUE);

	bFlagReset = TRUE;
	u32countStart = u32WakeTimerSecondsToTicks(5, TRUE);
	DBG_vPrintf(1, "\r\n ----------u32countStart : %d\r\n",u32countStart );
    #ifdef PDM_EEPROM
        vDisplayPDMUsage();
    #endif

    DBG_vPrintf(TRACE_SENSOR_NODE, "\r\nNODE: APP_vInitialiseNode() -> APP_vInitialiseTasks()");
    APP_vInitialiseTasks();

    DBG_vPrintf(TRACE_SENSOR_NODE, "\r\nNODE: APP_vInitialiseNode() EXIT");
}


/****************************************************************************
 *
 * NAME: APP_vInitialiseTasks
 *
 * DESCRIPTION:
 * This is the main App Initialise.
 * Task that checks the power status and starts tasks based on those results
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_vInitialiseTasks(void)
{

    DBG_vPrintf(TRACE_SENSOR_NODE, "\r\nNODE: APP_vInitialiseTasks() ENTER, PowerStatus = %d", PMC->RESETCAUSE);

    if (APP_bNodeIsInRunningState())
    {
        DBG_vPrintf(TRACE_SENSOR_NODE, "\r\nNODE: APP_vInitialiseTasks() RUNNING -> vActionOnButtonActivationAfterDeepSleep()");
        vActionOnButtonActivationAfterDeepSleep();
    }
    else
    {
        DBG_vPrintf(TRACE_SENSOR_NODE, "\r\nNODE: APP_vInitialiseTasks() NOT RUNNING");
        /* We are factory new so start the blink timers*/
        //vStartBlinkTimer(APP_JOINING_BLINK_TIME);
    }
    DBG_vPrintf(TRACE_SENSOR_NODE, "\r\nNODE: APP_vInitialiseTasks() EXIT");
}

/****************************************************************************
 *
 * NAME: APP_vBdbCallback
 *
 * DESCRIPTION:
 * Callbak from the BDB
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_vBdbCallback(BDB_tsBdbEvent *psBdbEvent)
{

    switch(psBdbEvent->eEventType)
    {

    case BDB_EVENT_NONE:
        break;

    case BDB_EVENT_ZPSAF:                // Use with BDB_tsZpsAfEvent
        /* Not poll confirm ? */

        if (psBdbEvent->uEventData.sZpsAfEvent.sStackEvent.eType != ZPS_EVENT_NWK_POLL_CONFIRM)
        {
            DBG_vPrintf(TRACE_SENSOR_NODE, "\r\nNODE: APP_vBdbCallback(ZPSAF, %d)", psBdbEvent->uEventData.sZpsAfEvent.sStackEvent.eType);
        }
        vAppHandleAfEvent(&psBdbEvent->uEventData.sZpsAfEvent);
        break;

    case BDB_EVENT_INIT_SUCCESS:
        DBG_vPrintf(TRACE_SENSOR_NODE,"\r\nNODE: APP_vBdbCallback(INIT_SUCCESS)");
        break;

    case BDB_EVENT_FAILURE_RECOVERY_FOR_REJOIN: //Recovery on rejoin failure
        DBG_vPrintf(TRACE_SENSOR_NODE,"\r\nNODE: APP_vBdbCallback(FAILURE_RECOVERY_FOR_REJOIN)");
        vHandleFailedRejoin();
        break;

    case BDB_EVENT_REJOIN_FAILURE: // only for ZED
        DBG_vPrintf(TRACE_SENSOR_NODE, "\r\nNODE: APP_vBdbCallback(REJOIN_FAILURE)");
        vHandleFailedToJoin();
        break;

    case BDB_EVENT_REJOIN_SUCCESS: // only for ZED
        DBG_vPrintf(TRACE_SENSOR_NODE, "\r\nNODE: APP_vBdbCallback(REJOIN_SUCCESS)");
        bBDBJoinFailed = FALSE;
        vHandleNetworkJoinEndDevice();
        //test poll
		u32countStart=100;
		DBG_vPrintf(TRACE_SENSOR_NODE,"\r\nEVENT: vStartPersistantPolling()");
		APP_bPersistantPolling |= TRUE;
		vStartPollTimer(POLL_TIME_FAST);


        break;

    case BDB_EVENT_NWK_STEERING_SUCCESS:
        // go to running state
        DBG_vPrintf(TRACE_SENSOR_NODE,"\r\nNODE: APP_vBdbCallback(NWK_STEERING_SUCCESS)");
        bBDBJoinFailed = FALSE;
        vHandleNetworkJoinAndRejoin();
        //test poll
		u32countStart=100;
		DBG_vPrintf(TRACE_SENSOR_NODE,"\r\nEVENT: vStartPersistantPolling()");
		APP_bPersistantPolling |= TRUE;
		vStartPollTimer(POLL_TIME_FAST);

        break;

    case BDB_EVENT_NO_NETWORK:
        DBG_vPrintf(TRACE_SENSOR_NODE,"\r\nNODE: APP_vBdbCallback(NO_NETWORK)");
        vHandleFailedToJoin();
        break;

    case BDB_EVENT_NWK_JOIN_SUCCESS:
        DBG_vPrintf(TRACE_SENSOR_NODE,"\r\nNODE: APP_vBdbCallback(NWK_JOIN_SUCCESS)");
        break;

    case BDB_EVENT_APP_START_POLLING:
        DBG_vPrintf(TRACE_SENSOR_NODE,"\r\nNODE: APP_vBdbCallback(APP_START_POLLING)");
        /* Start fast seconds polling */
        vStartPollTimer(POLL_TIME_FAST);
        break;

    case BDB_EVENT_FB_HANDLE_SIMPLE_DESC_RESP_OF_TARGET:
        DBG_vPrintf(TRACE_SENSOR_NODE,"\r\nNODE: APP_vBdbCallback(FB_HANDLE_SIMPLE_DESC_RESP_OF_TARGET)");
        DBG_vPrintf(TRACE_SENSOR_NODE,"\r\nSimple descriptor %d %d %04x %04x %d \n",psBdbEvent->uEventData.psFindAndBindEvent->u8TargetEp,
                psBdbEvent->uEventData.psFindAndBindEvent->u16TargetAddress,
                psBdbEvent->uEventData.psFindAndBindEvent->u16ProfileId,
                psBdbEvent->uEventData.psFindAndBindEvent->u16DeviceId,
                psBdbEvent->uEventData.psFindAndBindEvent->u8DeviceVersion);
        break;

    case BDB_EVENT_FB_CHECK_BEFORE_BINDING_CLUSTER_FOR_TARGET:
        DBG_vPrintf(TRACE_SENSOR_NODE,"\r\nNODE: APP_vBdbCallback(FB_CHECK_BEFORE_BINDING_CLUSTER_FOR_TARGET), ClusterId=0x%s", psBdbEvent->uEventData.psFindAndBindEvent->uEvent.u16ClusterId);
        break;

    case BDB_EVENT_FB_CLUSTER_BIND_CREATED_FOR_TARGET:
        DBG_vPrintf(TRACE_SENSOR_NODE,"\r\nNODE: APP_vBdbCallback(FB_CLUSTER_BIND_CREATED_FOR_TARGET), ClusterId=0x%x", psBdbEvent->uEventData.psFindAndBindEvent->uEvent.u16ClusterId);
        break;

    case BDB_EVENT_FB_BIND_CREATED_FOR_TARGET:
        {
            DBG_vPrintf(TRACE_SENSOR_NODE,"\r\nNODE: APP_vBdbCallback(FB_BIND_CREATED_FOR_TARGET), TargetEp = %d", psBdbEvent->uEventData.psFindAndBindEvent->u8TargetEp);
            u8NoQueryCount = 0;
            uint8 u8Seq;
            tsZCL_Address sAddress;
            tsCLD_Identify_IdentifyRequestPayload sPayload;

            sPayload.u16IdentifyTime = 0;
            sAddress.eAddressMode = E_ZCL_AM_SHORT_NO_ACK;
            sAddress.uAddress.u16DestinationAddress = psBdbEvent->uEventData.psFindAndBindEvent->u16TargetAddress;

            eCLD_IdentifyCommandIdentifyRequestSend(psBdbEvent->uEventData.psFindAndBindEvent->u8InitiatorEp,
                                                    psBdbEvent->uEventData.psFindAndBindEvent->u8TargetEp,
                                                    &sAddress,
                                                    &u8Seq,
                                                    &sPayload);
        }
        break;

    case BDB_EVENT_FB_GROUP_ADDED_TO_TARGET:
        DBG_vPrintf(TRACE_SENSOR_NODE,"\r\nNODE: APP_vBdbCallback(FB_GROUP_ADDED_TO_TARGET)");
        break;

    case BDB_EVENT_FB_ERR_BINDING_TABLE_FULL:
        DBG_vPrintf(TRACE_SENSOR_NODE,"\r\nNODE: APP_vBdbCallback(FB_ERR_BINDING_TABLE_FULL)");
        break;

    case BDB_EVENT_FB_ERR_BINDING_FAILED:
        DBG_vPrintf(TRACE_SENSOR_NODE,"\r\nNODE: APP_vBdbCallback(FB_ERR_BINDING_FAILED)");
        break;

    case BDB_EVENT_FB_ERR_GROUPING_FAILED:
        DBG_vPrintf(TRACE_SENSOR_NODE,"\r\nNODE: APP_vBdbCallback(FB_ERR_GROUPING_FAILED)");
        break;

    case BDB_EVENT_FB_NO_QUERY_RESPONSE:
        DBG_vPrintf(TRACE_SENSOR_NODE,"\r\nNODE: APP_vBdbCallback(FB_NO_QUERY_RESPONSE)");
        //Example to stop further query repeating
        if(u8NoQueryCount >= 2)
        {
            u8NoQueryCount = 0;
            BDB_vFbExitAsInitiator();
        }
        else
        {
            u8NoQueryCount++;
        }
        break;

    case BDB_EVENT_FB_TIMEOUT:
        DBG_vPrintf(TRACE_SENSOR_NODE,"\r\nNODE: APP_vBdbCallback(FB_TIMEOUT)");
        break;

    default:
        DBG_vPrintf(TRACE_SENSOR_NODE,"\r\nNODE: APP_vBdbCallback(UNHANDLED = %d)", psBdbEvent->eEventType);
        break;

    }

}

/****************************************************************************
 *
 * NAME: APP_app_zlo_sensor_Task
 *
 * DESCRIPTION:
 * Checks to see what event has triggered the task to start and calls the
 * appropriate function. This handles App events, Stack events, timer activations
 * and manual activations.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_taskSensor(void)
{
    APP_tsEvent sAppEvent;
    sAppEvent.eType = APP_E_EVENT_NONE;

    /*Collect the application events*/
    if (ZQ_bQueueReceive(&APP_msgAppEvents, &sAppEvent) == TRUE)
    {
        DBG_vPrintf(TRACE_SENSOR_NODE, "\r\nNODE: APP_taskSensor() AppEvent=%d", sAppEvent.eType);
#if ((defined APP_NTAG_ICODE) && (APP_BUTTONS_NFC_FD != (0xff)))
        /* Is this a button event on field detect ? */
        if ( ( sAppEvent.eType == APP_E_EVENT_BUTTON_DOWN || sAppEvent.eType == APP_E_EVENT_BUTTON_UP )
               && ( sAppEvent.uEvent.sButton.u8Button == APP_E_BUTTONS_NFC_FD) )
        {
            /* Always pass this on for processing */
            vAppHandleAppEvent(sAppEvent);
        }
        /* Other event (handle as normal) ? */
        else
#endif
        {
        	vAppHandleAppEvent(sAppEvent);
        }
    }
}


/****************************************************************************
 *
 * NAME: vAppHandleAfEvent
 *
 * DESCRIPTION:
 * Handles AF Events.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vAppHandleAfEvent( BDB_tsZpsAfEvent *psZpsAfEvent)
{

#ifdef CLD_OTA
    if (psZpsAfEvent->sStackEvent.eType == ZPS_EVENT_APS_DATA_INDICATION)
    {
        if ((psZpsAfEvent->sStackEvent.uEvent.sApsDataIndEvent.eStatus == ZPS_E_SUCCESS) &&
                (psZpsAfEvent->sStackEvent.uEvent.sApsDataIndEvent.u8DstEndpoint == 0))
        {
            // Data Ind for ZDp Ep
            if (ZPS_ZDP_MATCH_DESC_RSP_CLUSTER_ID == psZpsAfEvent->sStackEvent.uEvent.sApsDataIndEvent.u16ClusterId)
            {
                DBG_vPrintf(TRACE_SENSOR_NODE, "\r\nNODE: ZPS_ZDP_MATCH_DESC_RSP_CLUSTER_ID");
                vHandleMatchDescriptor(&psZpsAfEvent->sStackEvent);
                if(APP_bPersistantPolling != TRUE)
                {
                    ZTIMER_eStop(u8TimerPoll);
                }
            } else if (ZPS_ZDP_IEEE_ADDR_RSP_CLUSTER_ID == psZpsAfEvent->sStackEvent.uEvent.sApsDataIndEvent.u16ClusterId) {
                vHandleIeeeAddressRsp(&psZpsAfEvent->sStackEvent);
                if(APP_bPersistantPolling != TRUE)
                {
                    ZTIMER_eStop(u8TimerPoll);
                }
            }
        }
    }
#endif
    if (psZpsAfEvent->u8EndPoint == app_u8GetSensorEndpoint())
    {
        DBG_vPrintf(TRACE_SENSOR_NODE, "\r\nNODE: vAppHandleAfEvent() -> APP_ZCL_vEventHandler()");
        APP_ZCL_vEventHandler( &psZpsAfEvent->sStackEvent);
    }
    else if (psZpsAfEvent->u8EndPoint == PULSESSENSOR_ZDO_ENDPOINT)
    {
        /* Not poll confirm ? */
        if (psZpsAfEvent->sStackEvent.eType != ZPS_EVENT_NWK_POLL_CONFIRM)
        {
            DBG_vPrintf(TRACE_SENSOR_NODE, "\r\nNODE: vAppHandleAfEvent() -> vAppHandleZdoEvents()");
        }
        vAppHandleZdoEvents( psZpsAfEvent);
    }

    /* Ensure Freeing of Apdus */
    if (psZpsAfEvent->sStackEvent.eType == ZPS_EVENT_APS_DATA_INDICATION)
    {
        PDUM_eAPduFreeAPduInstance(psZpsAfEvent->sStackEvent.uEvent.sApsDataIndEvent.hAPduInst);
    }
    else if ( psZpsAfEvent->sStackEvent.eType == ZPS_EVENT_APS_INTERPAN_DATA_INDICATION )
    {
        PDUM_eAPduFreeAPduInstance(psZpsAfEvent->sStackEvent.uEvent.sApsInterPanDataIndEvent.hAPduInst);
    }

}

/****************************************************************************
 *
 * NAME: vDeletePDMOnButtonPress
 *
 * DESCRIPTION:
 * PDM context clearing on button press
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vDeletePDMOnButtonPress(uint8 u8ButtonDIO)
{
    bool_t bDeleteRecords = FALSE;
    uint8 u8Status;
    uint32 u32Buttons = APP_u32GetSwitchIOState() & (1 << u8ButtonDIO);

    if (u32Buttons == 0)
    {
        bDeleteRecords = TRUE;
    }
    else
    {
        bDeleteRecords = FALSE;
    }
    /* If required, at this point delete the network context from flash, perhaps upon some condition
     * For example, check if a button is being held down at reset, and if so request the Persistent
     * Data Manager to delete all its records:
     * e.g. bDeleteRecords = vCheckButtons();
     * Alternatively, always call PDM_vDelete() if context saving is not required.
     */
    if(bDeleteRecords)
    {
    	/* wait for button release */
		while (u32Buttons == 0)
		{
			u32Buttons = APP_u32GetSwitchIOState() & (1 << u8ButtonDIO);
		}
        sBDB.sAttrib.bbdbNodeIsOnANetwork = FALSE;
        u8Status = ZPS_eAplZdoLeaveNetwork(0, FALSE,FALSE);
        if (ZPS_E_SUCCESS !=  u8Status )
        {
            /* Leave failed, probably lost parent, so just reset everything */
            DBG_vPrintf(TRACE_SENSOR_NODE,"\r\nNODE: vDeletePDMOnButtonPress() -> APP_vFactoryResetRecords()");
            APP_vFactoryResetRecords();
            MICRO_DISABLE_INTERRUPTS();
            RESET_SystemReset();
        }else {
        	DBG_vPrintf(1, "RESET: Sent Leave\r\n");
        }

    }
}



/****************************************************************************
 *
 * NAME: vAppHandleZdoEvents
 *
 * DESCRIPTION:
 * This is the main state machine which decides whether to call up the startup
 * or running function. This depends on whether we are in the network on not.
 *
 * PARAMETERS:
 * ZPS_tsAfEvent sAppStackEvent Stack event information.
 *
 ****************************************************************************/
PRIVATE void vAppHandleZdoEvents( BDB_tsZpsAfEvent *psZpsAfEvent)
{
#ifdef DEBUG_SENSOR_NODE
    //vDisplayNWKTransmitTable();
#endif

    /* Handle events depending on node state */
    switch (sDeviceDesc.eNodeState)
    {

    case E_STARTUP:
        vAppHandleStartup();
        break;

    case E_RUNNING:
        vAppHandleRunning( &(psZpsAfEvent->sStackEvent) );
        break;

    case E_JOINING_NETWORK:
        break;

    default:
        break;

    }

}

/****************************************************************************
 * NAME: app_vRestartNode
 *
 * DESCRIPTION:
 * Start the Restart the ZigBee Stack after a context restore from
 * the EEPROM/Flash
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void app_vRestartNode (void)
{

    /* The node is in running state indicates that
     * the EZ Mode state is as E_EZ_DEVICE_IN_NETWORK*/

    DBG_vPrintf(TRACE_SENSOR_NODE, "\r\nNODE: app_vRestartNode(), Non Factory New Start");

    ZPS_vSaveAllZpsRecords();
}


/****************************************************************************
 *
 * NAME: app_vStartNodeFactoryNew
 *
 * DESCRIPTION:
 * Start the ZigBee Stack for the first ever Time.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void app_vStartNodeFactoryNew(void)
{
    sBDB.sAttrib.u32bdbPrimaryChannelSet = BDB_PRIMARY_CHANNEL_SET;
    sBDB.sAttrib.u32bdbSecondaryChannelSet = BDB_SECONDARY_CHANNEL_SET;
    //BDB_eNsStartNwkSteering(); done later on  only if no OOB commissionning  ???

    sDeviceDesc.eNodeState = E_JOINING_NETWORK;
    /* Stay awake for joining */
    DBG_vPrintf(TRACE_SENSOR_NODE, "\r\nNODE: app_vStartNodeFactoryNew(), Factory New Start");
}

/****************************************************************************
 *
 * NAME: app_u8GetSensorEndpoint
 *
 * DESCRIPTION:
 * Return the application endpoint
 *
 * PARAMETER: void
 *
 * RETURNS: void
 *
 ****************************************************************************/
PUBLIC uint8 app_u8GetSensorEndpoint( void)
{
    return PULSESSENSOR_SENSOR_ENDPOINT;
}

/****************************************************************************
 *
 * NAME: APP_vFactoryResetRecords
 *
 * DESCRIPTION: reset application and stack to factory new state
 *              preserving the outgoing nwk frame counter
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_vFactoryResetRecords(void)
{
    sDeviceDesc.eNodeState = E_STARTUP;

    /* clear out the stack */
    DBG_vPrintf(TRACE_SENSOR_NODE, "\r\nCall clear stack");
    ZPS_vDefaultStack();
    ZPS_eAplAibSetApsUseExtendedPanId(0);
    ZPS_vSetKeys();

#ifdef CLD_OTA
    sDeviceDesc.bValid = FALSE;
    sDeviceDesc.u64IeeeAddrOfServer = 0;
    sDeviceDesc.u16NwkAddrOfServer = 0xffff;
    sDeviceDesc.u8OTAserverEP = 0xff;
#endif

    /* save everything */
    PDM_eSaveRecordData(PDM_ID_APP_SENSOR, &sDeviceDesc, sizeof(tsDeviceDesc));
    ZPS_vSaveAllZpsRecords();
}


PUBLIC bool_t APP_bNodeIsInRunningState(void)
{
    DBG_vPrintf(TRACE_SENSOR_NODE, "\r\nNODE: APP_bNodeIsInRunningState()=%d", sDeviceDesc.eNodeState);
    return (sDeviceDesc.eNodeState == E_RUNNING) ? TRUE:FALSE;
}

#ifdef CLD_OTA
/****************************************************************************
 *
 * NAME: eGetNodeState
 *
 * DESCRIPTION:
 * Returns the node state
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC teNodeState eGetNodeState(void)
{
    return sDeviceDesc.eNodeState;
}
/****************************************************************************
 *
 * NAME: sGetOTACallBackPersistdata
 *
 * DESCRIPTION:
 * returns persisted data
 *
 * RETURNS:
 * tsOTA_PersistedData
 *
 ****************************************************************************/

PUBLIC tsOTA_PersistedData sGetOTACallBackPersistdata(void)
{
    return sSensor.sCLD_OTA_CustomDataStruct.sOTACallBackMessage.sPersistedData;
}
#endif
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

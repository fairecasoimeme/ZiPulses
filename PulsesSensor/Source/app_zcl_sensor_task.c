/*****************************************************************************
 *
 * MODULE:             JN-AN-1220 ZLO Sensor Demo
 *
 * COMPONENT:          app_zcl_sensor_task.c
 *
 * DESCRIPTION:        ZCL Occupancy sensor Control Behaviour (Implementation)
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
#include "app_main.h"
#include "pdum_apl.h"
#include "pdum_gen.h"
#include "PDM.h"
#include "dbg.h"
#include "zps_apl_af.h"
#include "zps_apl_zdo.h"
#include "zps_apl_aib.h"
#include "zps_apl_zdp.h"
#include "mac_pib.h"
#include "zcl_options.h"
#include "zcl.h"
#include "app_common.h"
#include "app_zlo_sensor_node.h"
#include "app_zcl_tick_handler.h"
#include "app_zcl_sensor_task.h"
#include "App_PulsesSensor.h"
#include "app_reporting.h"
#include "app_blink_led.h"
#include "app_pulses_sensor_state_machine.h"
#include "fsl_wtimer.h"
#ifdef CLD_OTA
#include "OTA_private.h"
#endif

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifdef DEBUG_ZCL
    #define TRACE_ZCL   TRUE
#else
    #define TRACE_ZCL   FALSE
#endif

#ifdef DEBUG_SENSOR_TASK
    #define TRACE_SENSOR_TASK   TRUE
#else
    #define TRACE_SENSOR_TASK   FALSE
#endif

#define RADIO_TEMP_UPDATE_MS 300000


/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

PRIVATE void APP_ZCL_cbGeneralCallback(tsZCL_CallBackEvent *psEvent);
PRIVATE void APP_ZCL_cbEndpointCallback(tsZCL_CallBackEvent *psEvent);
#ifdef CLD_OTA
PRIVATE void APP_ZCL_OTASetBlockRequestTime(tsZCL_CallBackEvent *psCallBackEvent);
PRIVATE void APP_ZCL_OTASetUpgradeTime(tsZCL_CallBackEvent *psCallBackEvent);
#endif
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
extern tsDeviceDesc sDeviceDesc;
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
PRIVATE uint32 u32RadioTempUpdateMs = 0;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: APP_ZCL_vInitialise
 *
 * DESCRIPTION:
 * Initialises ZCL related functions
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_ZCL_vInitialise(void)
{
    teZCL_Status eZCL_Status;

    /* Initialise ZCL */
    eZCL_Status = eZCL_Initialise(&APP_ZCL_cbGeneralCallback, apduZCL);
    if (eZCL_Status != E_ZCL_SUCCESS)
    {
        DBG_vPrintf(TRACE_ZCL, "\r\nZCL: Error eZCL_Initialise returned %d", eZCL_Status);
    }

    /* Register ZLO EndPoint */
    eZCL_Status = eApp_ZLO_RegisterEndpoint(&APP_ZCL_cbEndpointCallback);
    if (eZCL_Status != E_ZCL_SUCCESS)
    {
            DBG_vPrintf(TRACE_SENSOR_TASK, "\r\nTASK: Error: eApp_ZLO_RegisterEndpoint:%d", eZCL_Status);
    }

    //DBG_vPrintf(TRACE_SENSOR_TASK, "\r\nTASK: Chan Mask %08x", ZPS_psAplAibGetAib()->pau32ApsChannelMask[0]);
    DBG_vPrintf(TRACE_SENSOR_TASK, "\r\nTASK: RxIdle TRUE");

    ZTIMER_eStart(u8TimerTick, ZCL_TICK_TIME);

    vAPP_ZCL_DeviceSpecific_Init();

#ifdef CLD_OTA
    vAppInitOTA();
#endif
}

PUBLIC void APP_cbTimerReport( void *pvParam)
{
	DBG_vPrintf(TRACE_SENSOR_TASK, "\r\n---------APP_cbTimerReport-----------\r\n");
}


/****************************************************************************
 *
 * NAME: ZCL_Task
 *
 * DESCRIPTION:
 * ZCL Task for the sensor
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_cbTimerZclTick( void *pvParam)
{
	static uint32 u32RadioTempUpdateMs = 0;
    /*
     * If the 1 second tick timer has expired, restart it and pass
     * the event on to ZCL
     */
	DBG_vPrintf(TRACE_SENSOR_TASK, "\r\n---------APP_cbTimerZclTick-----------\r\n");
    vAPP_ZCL_Tick();
    ZTIMER_eStart(u8TimerTick, ZCL_TICK_TIME);

    #if 1
    {
        uint32        u32WkSeconds;
        uint32        u32WkRead;
        WTIMER_status_t eWkStatus;

        eWkStatus = WTIMER_GetStatusFlags(WTIMER_TIMER0_ID);
        DBG_vPrintf(TRACE_SENSOR_TASK, "\r\nTASK: APP_cbTimerZclTick(), eWk0Status = %d", eWkStatus);
        if (eWkStatus == WTIMER_STATUS_RUNNING)
        {

            u32WkRead = WTIMER_ReadTimer(WTIMER_TIMER0_ID);
            u32WkSeconds = u32WkRead / u32GetWakeTimerCalibration();
            DBG_vPrintf(TRACE_SENSOR_TASK, " (%d / %ds)", u32WkRead, u32WkSeconds);
        }
        eWkStatus = WTIMER_GetStatusFlags(WTIMER_TIMER1_ID);
        DBG_vPrintf(TRACE_SENSOR_TASK, "\r\nTASK: APP_cbTimerZclTick(), eWk1Status = %d", eWkStatus);
        if (eWkStatus == WTIMER_STATUS_RUNNING)
        {

            u32WkRead = WTIMER_ReadTimer(WTIMER_TIMER1_ID);
            u32WkSeconds = u32WkRead / u32GetWakeTimerCalibration();
            DBG_vPrintf(TRACE_SENSOR_TASK, " (%d / %ds)", u32WkRead, u32WkSeconds);
        }
    }
    #endif

#ifdef CLD_OTA
     /* Update for 1 second (1000ms) */
     vRunAppOTAStateMachine(1000);
#endif

	u32RadioTempUpdateMs += 100; /* 100ms */

	/* Need to update radio temp ? */
    if(u32RadioTempUpdateMs >= RADIO_TEMP_UPDATE_MS)
    {
    	DBG_vPrintf(TRACE_SENSOR_TASK, "\r\nRESET TIMER");
        /* Reset timer counter */
        u32RadioTempUpdateMs = 0;
        /* Update radio temperature (not loading calibration) */
        APP_vRadioTempUpdate(FALSE);
    }

}
/****************************************************************************
 *
 * NAME: APP_vRadioTempUpdateMs
 *
 * DESCRIPTION:
 * Updates radio temperature timer (for use after sleeping)
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
 PUBLIC void APP_vRadioTempUpdateMs(uint32 u32Ms)
{
    /* Update timer */
    u32RadioTempUpdateMs += u32Ms;
}

/****************************************************************************
 *
 * NAME: APP_ZCL_vEventHandler
 *
 * DESCRIPTION:
 * Sends the stack event to the ZCL event handler.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_ZCL_vEventHandler(ZPS_tsAfEvent* psStackEvent)
{
    tsZCL_CallBackEvent sCallBackEvent;

    sCallBackEvent.pZPSevent = psStackEvent;
    sCallBackEvent.eEventType = E_ZCL_CBET_ZIGBEE_EVENT;
    vZCL_EventHandler(&sCallBackEvent);
}

PUBLIC void vAPP_ZCL_Tick(void)
{
    ZPS_tsAfEvent sStackEvent;
    tsZCL_CallBackEvent sCallBackEvent;
    sCallBackEvent.pZPSevent = &sStackEvent;

    vDecrementTickCount();
    sCallBackEvent.eEventType = E_ZCL_CBET_TIMER;
    vZCL_EventHandler(&sCallBackEvent);
}
/****************************************************************************
 *
 * NAME: APP_ZCL_cbGeneralCallback
 *
 * DESCRIPTION:
 * General callback for ZCL events
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void APP_ZCL_cbGeneralCallback(tsZCL_CallBackEvent *psEvent)
{

    switch (psEvent->eEventType)
    {

    case E_ZCL_CBET_UNHANDLED_EVENT:
        DBG_vPrintf(TRACE_ZCL, "\r\nZCL: EVT Unhandled Event");
        break;

    case E_ZCL_CBET_READ_ATTRIBUTES_RESPONSE:
        DBG_vPrintf(TRACE_ZCL, "\r\nZCL: EVT Read attributes response");
        break;

    case E_ZCL_CBET_READ_REQUEST:
        DBG_vPrintf(TRACE_ZCL, "\r\nZCL: EVT Read request");
        break;

    case E_ZCL_CBET_DEFAULT_RESPONSE:
        DBG_vPrintf(TRACE_ZCL, "\r\nZCL: EVT Default response");
        break;

    case E_ZCL_CBET_ERROR:
        DBG_vPrintf(TRACE_ZCL, "\r\nZCL: EVT Error");
        break;

    case E_ZCL_CBET_TIMER:
        break;

    case E_ZCL_CBET_ZIGBEE_EVENT:
        DBG_vPrintf(TRACE_ZCL, "\r\nZCL: EVT ZigBee");
        break;

    case E_ZCL_CBET_CLUSTER_CUSTOM:
        DBG_vPrintf(TRACE_ZCL, "\r\nZCL: EP EVT Custom");
        break;

    default:
        DBG_vPrintf(TRACE_ZCL, "\r\nZCL: Invalid event type");
        break;

    }
}

/****************************************************************************
 *
 * NAME: APP_ZCL_cbEndpointCallback
 *
 * DESCRIPTION:
 * Endpoint specific callback for ZCL events
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void APP_ZCL_cbEndpointCallback(tsZCL_CallBackEvent *psEvent)
{
	DBG_vPrintf(TRACE_ZCL, "\r\nZCL: EP EVT : %d",psEvent->eEventType);
    switch (psEvent->eEventType)
    {
    	case E_ZCL_CBET_REPORT_INDIVIDUAL_ATTRIBUTES_CONFIGURE:
        {
            tsZCL_AttributeReportingConfigurationRecord    *psAttributeReportingRecord= &psEvent->uMessage.sAttributeReportingConfigurationRecord;
            DBG_vPrintf(TRACE_ZCL, "\r\nZCL: Individual Configure attribute for Cluster = %d",psEvent->psClusterInstance->psClusterDefinition->u16ClusterEnum);
            DBG_vPrintf(TRACE_ZCL, "\r\nZCL: eAttributeDataType = %d",psAttributeReportingRecord->eAttributeDataType);
            DBG_vPrintf(TRACE_ZCL, "\r\nZCL: u16AttributeEnum = %d",psAttributeReportingRecord->u16AttributeEnum );
            DBG_vPrintf(TRACE_ZCL, "\r\nZCL: u16MaximumReportingInterval = %d",psAttributeReportingRecord->u16MaximumReportingInterval );
            DBG_vPrintf(TRACE_ZCL, "\r\nZCL: u16MinimumReportingInterval = %d",psAttributeReportingRecord->u16MinimumReportingInterval );
            DBG_vPrintf(TRACE_ZCL, "\r\nZCL: u16TimeoutPeriodField = %d",psAttributeReportingRecord->u16TimeoutPeriodField );
            DBG_vPrintf(TRACE_ZCL, "\r\nZCL: u8DirectionIsReceived = %d",psAttributeReportingRecord->u8DirectionIsReceived );
            DBG_vPrintf(TRACE_ZCL, "\r\nZCL: uAttributeReportableChange = %d",psAttributeReportingRecord->uAttributeReportableChange );
            DBG_vPrintf(TRACE_ZCL, "\r\nZCL: eZCL_Status = %d",psEvent->eZCL_Status );
            if (E_ZCL_SUCCESS == psEvent->eZCL_Status)
            {
                if(SE_CLUSTER_ID_SIMPLE_METERING == psEvent->psClusterInstance->psClusterDefinition->u16ClusterEnum)
                {
                	DBG_vPrintf(TRACE_ZCL, "\r\nZCL: vSaveReportableRecord" );
                    vSaveReportableRecord(SE_CLUSTER_ID_SIMPLE_METERING,psAttributeReportingRecord);
                }
                if(GENERAL_CLUSTER_ID_POWER_CONFIGURATION == psEvent->psClusterInstance->psClusterDefinition->u16ClusterEnum)
			    {
                	DBG_vPrintf(TRACE_ZCL, "\r\nZCL: vSaveReportableRecord" );
				   vSaveReportableRecord(GENERAL_CLUSTER_ID_POWER_CONFIGURATION,psAttributeReportingRecord);
			    }
            }
        }
        break;

    case E_ZCL_CBET_UNHANDLED_EVENT:
    case E_ZCL_CBET_READ_ATTRIBUTES_RESPONSE:
    case E_ZCL_CBET_READ_REQUEST:
    case E_ZCL_CBET_DEFAULT_RESPONSE:
    case E_ZCL_CBET_ERROR:
    case E_ZCL_CBET_TIMER:
    case E_ZCL_CBET_ZIGBEE_EVENT:
        DBG_vPrintf(TRACE_ZCL, "\r\nZCL: EP EVT No action");
        break;

    case E_ZCL_CBET_READ_INDIVIDUAL_ATTRIBUTE_RESPONSE:
        DBG_vPrintf(TRACE_SENSOR_TASK, "\r\nTASK: Read Attrib Rsp %d %02x", psEvent->uMessage.sIndividualAttributeResponse.eAttributeStatus,
            *((uint8*)psEvent->uMessage.sIndividualAttributeResponse.pvAttributeData));
        break;

    case E_ZCL_CBET_CLUSTER_CUSTOM:
        DBG_vPrintf(TRACE_ZCL, "\r\nZCL: EP EVT: Custom %04x", psEvent->uMessage.sClusterCustomMessage.u16ClusterId);

        switch (psEvent->uMessage.sClusterCustomMessage.u16ClusterId)
        {

			case GENERAL_CLUSTER_ID_BASIC:
			{
				tsCLD_BasicCallBackMessage *psCallBackMessage = (tsCLD_BasicCallBackMessage*)psEvent->uMessage.sClusterCustomMessage.pvCustomData;
				if (psCallBackMessage->u8CommandId == E_CLD_BASIC_CMD_RESET_TO_FACTORY_DEFAULTS )
				{
					DBG_vPrintf(TRACE_ZCL, "\r\nZCL: Basic Factory Reset Received");
					/* resetting the sensor structure back to zero*/
					memset(&sSensor,0,sizeof(tsZHA_BaseDevice));
					vAPP_ZCL_DeviceSpecific_Init();
					#ifdef CLD_OTA
						vAppInitOTA();
					#endif
					eApp_ZLO_RegisterEndpoint(&APP_ZCL_cbEndpointCallback);
				}
			}
			break;

			case GENERAL_CLUSTER_ID_IDENTIFY:
				{
					tsCLD_IdentifyCallBackMessage *psCallBackMessage = (tsCLD_IdentifyCallBackMessage*)psEvent->uMessage.sClusterCustomMessage.pvCustomData;

					if (psCallBackMessage->u8CommandId == E_CLD_IDENTIFY_CMD_IDENTIFY)
					{
						DBG_vPrintf(TRACE_ZCL, "\r\nZCL: EP E_CLD_IDENTIFY_CMD_IDENTIFY");
						/* provide callback to BDB handler for identify query response on initiator*/
						if(psEvent->psClusterInstance->bIsServer == FALSE)
						{
							tsBDB_ZCLEvent  sBDBZCLEvent;
							DBG_vPrintf(TRACE_ZCL, "\r\nZCL: CallBackBDB");
							sBDBZCLEvent.eType = BDB_E_ZCL_EVENT_IDENTIFY_QUERY;
							sBDBZCLEvent.psCallBackEvent = psEvent;
							BDB_vZclEventHandler(&sBDBZCLEvent);
						}
						else
						{
							if(sSensor.sIdentifyServerCluster.u16IdentifyTime == 0)
							{
								vStopBlinkTimer();
							}
						}

					}
					else if((psCallBackMessage->u8CommandId == E_CLD_IDENTIFY_CMD_TRIGGER_EFFECT) &&
							psEvent->psClusterInstance->bIsServer)
					{
						DBG_vPrintf(TRACE_ZCL, "\r\nZCL: Trigger Effect ID %d Vr %d",
								psCallBackMessage->uMessage.psTriggerEffectRequestPayload->eEffectId,
								psCallBackMessage->uMessage.psTriggerEffectRequestPayload->u8EffectVarient);
					}
				}
				break;

				#ifdef CLD_OTA
				case OTA_CLUSTER_ID:
				{
					 tsOTA_CallBackMessage *psCallBackMessage = (tsOTA_CallBackMessage *)psEvent->uMessage.sClusterCustomMessage.pvCustomData;
					 vHandleAppOtaClient(psCallBackMessage);
					 if((APP_bPersistantPolling != TRUE) &&
						   (((psCallBackMessage->eEventId == E_CLD_OTA_COMMAND_QUERY_NEXT_IMAGE_RESPONSE) || (psCallBackMessage->eEventId == E_CLD_OTA_INTERNAL_COMMAND_POLL_REQUIRED)) ||
							   (eOTA_GetState() == OTA_IDLE) || bOTA_IsWaitToUpgrade()))
					 {
						DBG_vPrintf(TRACE_ZCL, "\nAPP_ZCL: Stopped Poll");
						ZTIMER_eStop(u8TimerPoll);
					 }
				}
				break;
			   #endif

			case GENERAL_CLUSTER_ID_GROUPS:
				DBG_vPrintf(TRACE_ZCL, "\r\nZCL: for groups cluster");
				break;

			case 0x1000:
				DBG_vPrintf(TRACE_ZCL, "\r\nZCL: - for 0x1000");
				break;

			default:
				DBG_vPrintf(TRACE_ZCL, "\r\nZCL: for unknown cluster %d", psEvent->uMessage.sClusterCustomMessage.u16ClusterId);
				break;
			}
			break;

    case E_ZCL_CBET_CLUSTER_UPDATE:
        DBG_vPrintf(TRACE_ZCL, "\r\nZCL: Update Id %04x", psEvent->psClusterInstance->psClusterDefinition->u16ClusterEnum);
        if (psEvent->psClusterInstance->psClusterDefinition->u16ClusterEnum == GENERAL_CLUSTER_ID_IDENTIFY)
        {
            vAPP_ZCL_DeviceSpecific_UpdateIdentify();
        }
        break;
    case E_ZCL_CBET_WRITE_ATTRIBUTES:
    case E_ZCL_CBET_WRITE_INDIVIDUAL_ATTRIBUTE:
		if(SE_CLUSTER_ID_SIMPLE_METERING == psEvent->psClusterInstance->psClusterDefinition->u16ClusterEnum)
		{

			//PB avec psEvent->psClusterInstance->psClusterDefinition->psAttributeDefinition->u16AttributeEnum. Du coup, on modifie les variables du cluster qui doivent être modifiés
			sDeviceCounter.multiplier  = sSensor.sSimpleMeteringServerCluster.u24Multiplier;
			sDeviceCounter.unitMeasure = sSensor.sSimpleMeteringServerCluster.eUnitOfMeasure;
			sSensor.sSimpleMeteringServerCluster.u48CurrentTier1SummationDelivered = sDeviceCounter.multiplier * sDeviceCounter.counter;


		}

		break;

    default:
        DBG_vPrintf(TRACE_ZCL, "\r\nZCL: EP EVT Invalid event type");
        break;
    }

}

#ifdef CLD_OTA
/**************************************************************************//**
 *
 * @name vAppZcl_OTASetBlockRequestTime
 *
 * Description: Sends the OTA ticks to send block request.
 *
 * @param void
 *
 * @return void
 *
 ****************************************************************************/
PRIVATE void APP_ZCL_OTASetBlockRequestTime(tsZCL_CallBackEvent *psCallBackEvent)
{
    sSensor.sCLD_OTA_CustomDataStruct.sOTACallBackMessage.sPersistedData.u32RequestBlockRequestTime = 1;
    psCallBackEvent->uMessage.sTimerMessage.u32UTCTime = 2;
}

/**************************************************************************//**
 *
 * @name vAppZcl_OTASetUpgradeTime
 *
 * Description: Sends the OTA ticks to Upgrade.
 *
 * @param void
 *
 * @return void
 *
 ****************************************************************************/
PRIVATE void APP_ZCL_OTASetUpgradeTime(tsZCL_CallBackEvent *psCallBackEvent)
{
    psCallBackEvent->uMessage.sTimerMessage.u32UTCTime = 1;
    if(sSensor.sCLD_OTA_CustomDataStruct.sOTACallBackMessage.sPersistedData.u32RequestBlockRequestTime >= 1)
        sSensor.sCLD_OTA_CustomDataStruct.sOTACallBackMessage.sPersistedData.u32RequestBlockRequestTime -= 1;
}

/**************************************************************************//**
 *
 * @name vAppZcl_OTATick
 *
 * Description: Sends the OTA ticks to kick start the download process again.
 *
 * @param bWaitToUpgrade :Type bool_t
 *
 * @return void
 *
 ****************************************************************************/
PUBLIC void APP_ZCL_OTATick(bool_t bWaitToUpgrade)
{
    ZPS_tsAfEvent sStackEvent;
    tsZCL_CallBackEvent sCallBackEvent;
    sCallBackEvent.pZPSevent = &sStackEvent;
    sCallBackEvent.eEventType = E_ZCL_CBET_TIMER_MS;
    if(bWaitToUpgrade)
        APP_ZCL_OTASetUpgradeTime(&sCallBackEvent);
    else
        APP_ZCL_OTASetBlockRequestTime(&sCallBackEvent);

    eOtaTimeUpdate(app_u8GetSensorEndpoint(),FALSE,&sCallBackEvent);


}
#endif
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

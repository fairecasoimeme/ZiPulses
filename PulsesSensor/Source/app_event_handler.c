/*****************************************************************************
 *
 * MODULE:          JN-AN-1220 ZLO Sensor Demo
 *
 * COMPONENT:       app_event_handler.c
 *
 * DESCRIPTION:     ZLO Demo: Handles all the different type of Application events
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
#include "app_pulses_events.h"
#include "app_zlo_sensor_node.h"
#include "App_PulsesSensor.h"
#include "app_event_handler.h"
#include "app_reporting.h"
#include "app_blink_led.h"
#include "app_nwk_event_handler.h"
#include "bdb_api.h"
#include "app_main.h"
#include "zps_gen.h"
#if (defined APP_NTAG_ICODE)
#include "ntag_nwk.h"
#include "app_ntag_icode.h"
#endif

#include "MicroSpecific.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifdef DEBUG_EVENT_HANDLER
    #define TRACE_EVENT_HANDLER   TRUE
#else
    #define TRACE_EVENT_HANDLER   FALSE
#endif
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void vDioEventHandler(te_TransitionCode eTransitionCode);
PRIVATE void vEventStartFindAndBind(void);
PRIVATE void vStartPersistantPolling(void);
PRIVATE void vStopPersistantPolling(void);
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
extern const uint8 u8MyEndpoint;
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: vDioEventHandler
 *
 * DESCRIPTION:
 * Processes the Dio events like binding and occupancy. Any other events that
 * come through we immediately attempt to go to sleep as we have no process for
 * them.
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vDioEventHandler(te_TransitionCode eTransitionCode )
{
    ZPS_eAplZdoPoll();
    DBG_vPrintf(TRACE_EVENT_HANDLER, "\r\nEVENT: vDioEventHandler(%02x)", eTransitionCode);
    switch(eTransitionCode)
    {

    /* Fall through for the button presses as there will be a delayed action*/
    case SW0_BUTTON_PRESSED:

        break;

    case SW0_BUTTON_RELEASED:
    	vHandlePulseRisingEvent();
        break;

    case SW1_PRESSED:
    	vHandleButtonPressedEvent();
        break;

    case SW1_RELEASED:
    	//Get_BattVolt();
    	// APP_vRadioTempUpdate(FALSE);
    	 APP_vGetVoltageBattery();
    	 APP_vRadioTempUpdate(TRUE);
        //vHandleRisingEdgeEvent();
        break;

    case SW2_PRESSED:
        //vStartPersistantPolling();
        break;

    case SW3_PRESSED:
        //vStopPersistantPolling();
        break;

#if (defined APP_NTAG_ICODE)
    case FD_PRESSED:
    case FD_RELEASED:
        #if APP_NTAG_ICODE
            APP_vNtagStart(OCCUPANCYSENSOR_SENSOR_ENDPOINT);
        #endif
        break;
#endif

    case SW4_PRESSED:
        //vEventStartFindAndBind();
        break;

    case SW4_RELEASED:
        //vEventStopFindAndBind();
        break;
    case SW2_RELEASED:
    case SW3_RELEASED:

        break;

    default:
        break;

    }
}

/****************************************************************************
 *
 * NAME: vAppHandleAppEvent
 *
 * DESCRIPTION:
 * interprets the button press and calls the state machine.
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppHandleAppEvent(APP_tsEvent sButton)
{
    te_TransitionCode eTransitionCode=NUMBER_OF_TRANSITION_CODE;
    u16WatchdogAttemptToSleep=0;
    switch(sButton.eType)
    {

    case APP_E_EVENT_BUTTON_DOWN:
        DBG_vPrintf(TRACE_EVENT_HANDLER, "\r\nEVENT: vAppHandleAppEvent(BUTTON_DOWN, %d)", sButton.uEvent.sButton.u8Button);

        eTransitionCode = sButton.uEvent.sButton.u8Button;

        vDioEventHandler(eTransitionCode);
        break;

    case APP_E_EVENT_BUTTON_UP:
        DBG_vPrintf(TRACE_EVENT_HANDLER, "\r\nEVENT: vAppHandleAppEvent(BUTTON_UP, %d)", sButton.uEvent.sButton.u8Button);

        eTransitionCode = BUTTON_RELEASED_OFFSET | sButton.uEvent.sButton.u8Button;

        vDioEventHandler(eTransitionCode);
        break;

    case APP_E_EVENT_WAKE_TIMER:
        DBG_vPrintf(TRACE_EVENT_HANDLER, "\r\nEVENT: vAppHandleAppEvent(WAKE_TIMER)");
        vHandleWakeTimeoutEvent();
        break;

    case APP_E_EVENT_SEND_REPORT:
        DBG_vPrintf(TRACE_EVENT_HANDLER, "\r\nEVENT: vAppHandleAppEvent(SEND_REPORT)");
        vSendImmediateAllReport();
        break;

    case APP_E_EVENT_PERIODIC_REPORT:
        DBG_vPrintf(TRACE_EVENT_HANDLER, "\r\nEVENT: vAppHandleAppEvent(PERIODIC_REPORT)");
        break;

    case APP_E_EVENT_LEAVE_AND_RESET:
    	if (sDeviceDesc.eNodeState == E_RUNNING)
		{
			if (ZPS_eAplZdoLeaveNetwork( 0UL, FALSE, FALSE) != ZPS_E_SUCCESS )
			 {
				APP_vFactoryResetRecords();
				MICRO_DISABLE_INTERRUPTS();
				RESET_SystemReset();
			}
		}
		else
		{
			APP_vFactoryResetRecords();
			MICRO_DISABLE_INTERRUPTS();
			RESET_SystemReset();
		}
		PDM_vDeleteAllDataRecords();

    	break;
    default:
        break;
    }

}


/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: vEventStartFindAndBind
 *
 * DESCRIPTION:
 * Initiates the find and bind procedure, Starts a poll timer and the blink
 * timer.
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vEventStartFindAndBind(void)
{
    DBG_vPrintf(TRACE_EVENT_HANDLER,"\r\nEVENT: vEventStartFindAndBind()");
    sBDB.sAttrib.u16bdbCommissioningGroupID = 0xFFFF;
    vAPP_ZCL_DeviceSpecific_SetIdentifyTime(0xFF);
    BDB_eFbTriggerAsInitiator(u8MyEndpoint);
    vStartPollTimer(POLL_TIME);
    vStartBlinkTimer(APP_FIND_AND_BIND_BLINK_TIME);
}

/****************************************************************************
 *
 * NAME: vEventStopFindAndBind
 *
 * DESCRIPTION:
 * Stops the find and bind procedure and attempts to sleep.
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vEventStopFindAndBind(void)
{
    DBG_vPrintf(TRACE_EVENT_HANDLER,"\r\nEVENT: vEventStopFindAndBind()");
    vAPP_ZCL_DeviceSpecific_IdentifyOff();
    BDB_vFbExitAsInitiator();
    vStopBlinkTimer();
    vStopPollTimerTask();
    vHandleNewJoinEvent();
}

/****************************************************************************
 *
 * NAME: vStartPersistantPolling
 *
 * DESCRIPTION:
 * Starts the Poll timer which will in turn keep the device awake so it can
 * receive data from it's parent.
 *
 ****************************************************************************/
PRIVATE void vStartPersistantPolling(void)
{
    DBG_vPrintf(TRACE_EVENT_HANDLER,"\r\nEVENT: vStartPersistantPolling()");
    APP_bPersistantPolling |= TRUE;
    vStartPollTimer(POLL_TIME_FAST);
    vStartBlinkTimer(APP_KEEP_AWAKE_TIME);
}

/****************************************************************************
 *
 * NAME: vStopPersistantPolling
 *
 * DESCRIPTION:
 * Stops the poll timer which will allow the device to go back to sleep.
 *
 ****************************************************************************/
PRIVATE void vStopPersistantPolling(void)
{
    DBG_vPrintf(TRACE_EVENT_HANDLER,"\r\nEVENT: vStopPersistantPolling()");
    APP_bPersistantPolling &= FALSE;
    vStopPollTimerTask();
    vStopBlinkTimer();
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

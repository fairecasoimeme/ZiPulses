/*****************************************************************************
 *
 * MODULE:          JN-AN-1220 ZLO Sensor Demo
 *
 * COMPONENT:       app_sensor_state_machine.c
 *
 * DESCRIPTION:     ZLO Demo: Processes all stack events depending on it's state
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
#include "app_pulses_sensor_state_machine.h"
#include "App_PulsesSensor.h"
#include "app_nwk_event_handler.h"
#include "app_zbp_utilities.h"
#include "PDM.h"
#include "bdb_api.h"
#include "app_main.h"
#include "app_common.h"
#include "app_events.h"
#include "app_zlo_sensor_node.h"
#include "zps_gen.h"
#include "fsl_wtimer.h"
#include "PWR_Interface.h"
#ifdef APP_NTAG_ICODE
#include "app_ntag_icode.h"
#endif

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifdef DEBUG_SENSOR_STATE
    #define TRACE_SENSOR_STATE   TRUE
#else
    #define TRACE_SENSOR_STATE   FALSE
#endif

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
void WAKE_UP_TIMER0_IRQHandler(void);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
PRIVATE uint32 u32WtimerCalibration = APP_WAKE_TICKS_PER_SECOND;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: vAppHandleRunning
 *
 * DESCRIPTION:
 * Is called to handle stack events when we are currently in the network
 *
 ****************************************************************************/
PUBLIC void vAppHandleRunning(ZPS_tsAfEvent* psStackEvent)
{

    DBG_vPrintf(TRACE_SENSOR_STATE, "\r\nSTATE: vAppHandleRunning(%d)", psStackEvent->eType);

    switch(psStackEvent->eType)
    {

    case ZPS_EVENT_NWK_JOINED_AS_ENDDEVICE:
        vHandleNetworkJoinEndDevice();
        #ifdef APP_NTAG_ICODE
        {
            /* Not a rejoin ? */
            if (FALSE == psStackEvent->uEvent.sNwkJoinedEvent.bRejoin)
            {
                /* Write network data to tag */
                APP_vNtagStart(OCCUPANCYSENSOR_SENSOR_ENDPOINT);
            }
        }
        #endif
        break;

    case ZPS_EVENT_NWK_LEAVE_INDICATION:
        vHandleNetworkLeave( psStackEvent);
        break;

    case ZPS_EVENT_NWK_POLL_CONFIRM:
        vHandlePollResponse( psStackEvent);
        break;

    case ZPS_EVENT_NWK_LEAVE_CONFIRM:
        vHandleNetworkLeaveConfirm( psStackEvent);
        break;

    default:
        break;

    }

}

/****************************************************************************
 *
 * NAME: vAppHandleStartup
 *
 * DESCRIPTION:
 * Is called to handle stack events when we are currently not in the network
 *
 ****************************************************************************/
PUBLIC void vAppHandleStartup(void)
{
    DBG_vPrintf(TRACE_SENSOR_STATE, "\r\nSTATE: vAppHandleStartup()");
    BDB_eNsStartNwkSteering();
    sDeviceDesc.eNodeState = E_JOINING_NETWORK;
}

/****************************************************************************
 *
 * NAME: u32GetWakeTimerCalibration(
 *
 * DESCRIPTION:
 * Returns wake timer calibration value (ticks per second)
 *
 * RETURNS:
 * uint32 u32Ticks in native wake timer ticks
 *
 ****************************************************************************/
PUBLIC uint32 u32GetWakeTimerCalibration(void)
{
    return u32WtimerCalibration;
}

/****************************************************************************
 *
 * NAME: u32WakeTimerSecondsToTicks
 *
 * DESCRIPTION:
 * Calculates the ticks for the passed in time in seconds, with optional
 * calibration
 *
 * RETURNS:
 * uint32 u32Ticks in native wake timer ticks
 *
 ****************************************************************************/
PUBLIC uint32 u32WakeTimerSecondsToTicks(uint32 u32Seconds, bool_t bCalibrate)
{
    uint32 u32Ticks;

    // Calibrating ?
    if (bCalibrate)
    {
        u32WtimerCalibration = PWR_GetHk32kHandle()->freq32k;
    }
    // Calculate ticks
    u32Ticks = u32WtimerCalibration * u32Seconds;
    // Debug
    DBG_vPrintf(TRACE_SENSOR_STATE, "\r\nSTATE: u32WakeTimerSecondsToTicks(%d, %d [%d]) = %d", u32Seconds, bCalibrate, u32WtimerCalibration, u32Ticks);

    return (u32Ticks);
}


PUBLIC void vStopWakeTimer()
{
	//Stop timer
	WTIMER_StopTimer(WTIMER_TIMER0_ID);
	WTIMER_ClearStatusFlags(WTIMER_TIMER0_ID);
}
/****************************************************************************
 *
 * NAME: vStartWakeTimer
 *
 * DESCRIPTION:
 * Starts wake timer 0, time set in native wake timer units 32KHz on 8x
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vStartWakeTimer(uint32 u32WakeTicks)
{
	//Stop timer
	WTIMER_StopTimer(WTIMER_TIMER0_ID);
    DBG_vPrintf(TRACE_SENSOR_STATE, "\r\nSTATE: vStartWakeTimer(%dt / %ds)", u32WakeTicks, (u32WakeTicks/u32WtimerCalibration));
    // Enable wake timer interrupts
    WTIMER_EnableInterrupts(WTIMER_TIMER0_ID);
    // Run wake timer uncalibrated for now (set in 32KHz units)
    WTIMER_Init();
    WTIMER_StartTimer(WTIMER_TIMER0_ID, u32WakeTicks);
}


/****************************************************************************
 *
 * NAME: WAKE_UP_TIMER0_IRQHandler()
 *
 * DESCRIPTION:
 * Wake timer 0 interrupt handler
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
void WAKE_UP_TIMER0_IRQHandler(void)
{
    APP_tsEvent sButtonEvent;

    /* Stop the timer */
    WTIMER_StopTimer(WTIMER_TIMER0_ID);
    /* Clear status flags */
    WTIMER_ClearStatusFlags(WTIMER_TIMER0_ID);

    /* Post a message to the stack so we aren't handling events
     * in interrupt context
     */
    sButtonEvent.eType = APP_E_EVENT_WAKE_TIMER;
    ZQ_bQueueSend(&APP_msgAppEvents, &sButtonEvent);
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

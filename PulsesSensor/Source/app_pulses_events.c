/*
 * app_pulses_event.c
 *
 *  Created on: 8 nov. 2022
 *      Author: akila
 */
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include "dbg.h"
#include "app_sleep_handler.h"
#include "app_pulses_events.h"
#include "app_zlo_sensor_node.h"
#include "app_pulses_sensor_state_machine.h"
#include "app_blink_led.h"
#include "app_reporting.h"
#include "app.h"
#include "fsl_wtimer.h"
#include "voltage_drv.h"
#include "PDM_IDs.h"

extern bool bPulsesAction;


PUBLIC void vHandlePulseRisingEvent(void)
{
	bPulsesAction = true;
	sDeviceCounter.counter++;
	DBG_vPrintf(1, "\r\n********************************************************************************sDeviceCounter.multiplier :%d , sDeviceCounter.counter: %d //////////////////// \r\n", sDeviceCounter.multiplier,sDeviceCounter.counter);
	if (sDeviceCounter.multiplier>1)
	{
		sSensor.sSimpleMeteringServerCluster.u48CurrentTier1SummationDelivered = sDeviceCounter.multiplier * sDeviceCounter.counter;
	}
	if (sDeviceCounter.divisor>1)
	{
		sSensor.sSimpleMeteringServerCluster.u48CurrentTier1SummationDelivered =  sDeviceCounter.counter / sDeviceCounter.divisor;
	}
	if ((sDeviceCounter.divisor==1) && (sDeviceCounter.multiplier==1))
	{
		sSensor.sSimpleMeteringServerCluster.u48CurrentTier1SummationDelivered = sDeviceCounter.counter;
	}
	sSensor.sSimpleMeteringServerCluster.u48CurrentSummationDelivered = sDeviceCounter.counter;
	DBG_vPrintf(1, "\r\n********************************************************************************u48CurrentSummationDelivered : %d ////////////////////\r\n", sSensor.sSimpleMeteringServerCluster.u48CurrentSummationDelivered);

	//PDM_eSaveRecordData(PDM_ID_APP_COUNTER,&sDeviceCounter,sizeof(tsDeviceCounter));

	//APP_vGetVoltageBattery();

	APP_vSetLED(LED1, 1);

	//vSendImmediateIndexReport();


	//WTIMER_StopTimer(WTIMER_TIMER0_ID);
}


PUBLIC bool vHandleButtonPressedEventRejoin(bool ledON)
{
	DBG_vPrintf(1, "\r\n vHandleButtonPressedEventRejoin()\r\n");

	/*if(APP_bNodeIsInRunningState())
	{
		// TODO kick BDB for rejoin
		sBDB.sAttrib.bbdbNodeIsOnANetwork = TRUE;
		BDB_vStart();
		vStopBlinkTimer();
		APP_vSetLED(LED1, 1);
		return true;
	}
	else
	{
		vStartBlinkTimer(50);
		//Retrigger the network steering as sensor is not part of a network
		vAppHandleStartup();
		ZTIMER_eStop(u8TimerPoll);
		return false;
	}*/
	vStopBlinkTimer();
	APP_vSetLED(LED1, 1);
	BDB_vStart();

	/*if(bBDBJoinFailed)
	{
		if (ledON)
		{
			vStartBlinkTimer(50);
		}
		if(APP_bNodeIsInRunningState())
		{
			// TODO kick BDB for rejoin
			sBDB.sAttrib.bbdbNodeIsOnANetwork = TRUE;
			BDB_vStart();
		}
		else
		{
			//Retrigger the network steering as sensor is not part of a network
			vAppHandleStartup();
		}
		return false;
	}
	else
	{
		vStopBlinkTimer();
		APP_vSetLED(LED1, 1);
		return true;
	}*/

}

PUBLIC bool vHandleButtonPressedEvent(bool ledON)
{
	DBG_vPrintf(1, "\r\n vHandleButtonPressedEvent()\r\n");

	if(bBDBJoinFailed)
	{
		if (ledON)
		{
			vStartBlinkTimer(50);
		}
		if(APP_bNodeIsInRunningState())
		{
			// TODO kick BDB for rejoin
			sBDB.sAttrib.bbdbNodeIsOnANetwork = TRUE;
			BDB_vStart();
		}
		else
		{
			//Retrigger the network steering as sensor is not part of a network
			vAppHandleStartup();
		}
		return false;
	}
	else
	{
		vStopBlinkTimer();
		APP_vSetLED(LED1, 1);
		return true;
	}

}

PUBLIC void vHandleWakeTimeoutEvent(void)
{
	//APP_vGetVoltageBattery();
}

PUBLIC void vHandleNewJoinEvent(void)
{


}

/*
 * app_pulses_events.h
 *
 *  Created on: 8 nov. 2022
 *      Author: akila
 */

#ifndef PULSESSENSOR_SOURCE_APP_PULSES_EVENTS_H_
#define PULSESSENSOR_SOURCE_APP_PULSES_EVENTS_H_

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/
#include <jendefs.h>

PUBLIC void vHandlePulseRisingEvent(void);
PUBLIC void vHandleButtonPressedEvent(void);
PUBLIC void vHandleWakeTimeoutEvent(void);
PUBLIC void vHandleNewJoinEvent(void);

#endif /* PULSESSENSOR_SOURCE_APP_PULSES_EVENTS_H_ */

/*****************************************************************************
 *
 * MODULE:          JN-AN-1220 ZLO Sensor Demo
 *
 * COMPONENT:       app_blink_led.c
 *
 * DESCRIPTION:     ZLO Demo: Blinks an LED a number of pre defined ticks
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
#include "App_PulsesSensor.h"
#include "app_blink_led.h"
#include "ZTimer.h"
#include "app_main.h"
#include "app.h"
#include "fsl_iocon.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifdef DEBUG_BLINK_LED
    #define TRACE_BLINK_LED   TRUE
#else
    #define TRACE_BLINK_LED   FALSE
#endif
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
#define BLINK_LED   LED1
/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void vToggleLED(void);
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
PRIVATE bool_t bDIO1State = FALSE;
PRIVATE uint32 u32BlinkTickTime = 0;
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: APP_vInitLeds
 *
 * DESCRIPTION:
 * Initialises LED's
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_vInitLeds(void)
{
#if 0 //LJM
    vAHI_DioSetDirection(0, BOARD_LED_CTRL_MASK);
#else
    gpio_pin_config_t output_config = {kGPIO_DigitalOutput, 0};
    IOCON_PinMuxSet(IOCON, 0, BOARD_LED_D1_BIT, IOCON_FUNC0 | IOCON_DIGITAL_EN | IOCON_GPIO_MODE | IOCON_MODE_PULLDOWN);
    GPIO_PinInit(GPIO, 0, BOARD_LED_D1_BIT, &output_config);

    APP_vSetLED( LED1, 0);
#endif
}

/****************************************************************************
 *
 * NAME: APP_vSetLED
 *
 * DESCRIPTION:
 * Initialises LED's
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_vSetLED(uint8 u8Led, bool_t bOn)
{
#if 1 //LJM
    if ( u8Led == LED1)
    {
        if (bOn)
        {
            /* leds on dr1199 driven high for on */
            //vAHI_DioSetOutput(BOARD_LED_D1_PIN , 0);
            GPIO_PinWrite(GPIO, 0, BOARD_LED_D1_BIT, 1);
        }
        else
        {
            //vAHI_DioSetOutput(0, BOARD_LED_D1_PIN);
            GPIO_PinWrite(GPIO, 0, BOARD_LED_D1_BIT, 0);
        }
    }

#endif
}

/****************************************************************************
 *
 * NAME: APP_BlinkLED
 *
 * DESCRIPTION:
 * Toggles LED 2 and restarts the timer.
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAPP_cbBlinkLED( void *pvParams)
{
    DBG_vPrintf(TRACE_BLINK_LED, "\nAPP Blink LED: Task Started");
    ZTIMER_eStop(u8TimerBlink);
    vToggleLED();
    ZTIMER_eStart(u8TimerBlink, u32BlinkTickTime);
}

/****************************************************************************
 *
 * NAME: vStartBlinkTimer
 *
 * DESCRIPTION:
 * Starts the software timer and blinks it the desired amount of ticks. It also
 * stores the time so the Task can restart it with the defined time
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vStartBlinkTimer(uint32 u32Ticks)
{
    DBG_vPrintf(TRACE_BLINK_LED, "\nAPP Blink LED: Starting Blink Timer value = %d", u32Ticks);
    ZTIMER_eStop(u8TimerBlink);
    u32BlinkTickTime = u32Ticks;
    ZTIMER_eStart(u8TimerBlink, u32Ticks);
}

/****************************************************************************
 *
 * NAME: vStopBlinkTimer
 *
 * DESCRIPTION:
 * Stops the blink timer and turns the LED off
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vStopBlinkTimer(void)
{
    DBG_vPrintf(TRACE_BLINK_LED, "\nAPP Blink LED: Stopping Blink Timer");
    ZTIMER_eStop(u8TimerBlink);
    u32BlinkTickTime = 0;
    APP_vSetLED(BLINK_LED, FALSE);
}

/****************************************************************************/
/***        Local Function                                                ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: vToggleLED
 *
 * DESCRIPTION:
 * Changes the state of LED 2
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vToggleLED(void)
{
    DBG_vPrintf(TRACE_BLINK_LED, "\nAPP Blink LED: Toggle LED to %b", !bDIO1State);
    bDIO1State = !bDIO1State;
    APP_vSetLED(BLINK_LED, bDIO1State);
}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

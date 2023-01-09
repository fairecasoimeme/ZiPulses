/*****************************************************************************
 *
 * MODULE:          JN-AN-1220 ZLO Sensor Demo
 *
 * COMPONENT:       app_blink_led.h
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
#ifndef APP_BLINK_LED_H_
#define APP_BLINK_LED_H_

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/
#include <jendefs.h>

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define BOARD_LED_D1_BIT               (10)
#define BOARD_LED_D2_BIT               (6)
#define BOARD_LED_D3_BIT               (3)

#define BOARD_LED_D1_PIN               (1UL << BOARD_LED_D1_BIT)
#define BOARD_LED_D2_PIN               (1UL << BOARD_LED_D2_BIT)
#define BOARD_LED_D3_PIN               (1UL << BOARD_LED_D3_BIT)
#define BOARD_LED_CTRL_MASK            (BOARD_LED_D1_PIN | BOARD_LED_D2_PIN | BOARD_LED_D3_PIN)


/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

PUBLIC void vStartBlinkTimer(uint32 u32Ticks);
PUBLIC void vStopBlinkTimer(void);
PUBLIC void vAPP_cbBlinkLED( void *pvParams);
PUBLIC void APP_vSetLED(uint8 u8Led, bool_t bOn);
PUBLIC void APP_vInitLeds(void);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#endif /* APP_BLINK_LED_H_ */
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

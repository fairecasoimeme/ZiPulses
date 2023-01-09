/*
 * Copyright  2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_debug_console.h"
#include "fsl_clock.h"
#include "board.h"
#include "fsl_iocon.h"

/*******************************************************************************
 * Code
 ******************************************************************************/
#if (defined TCXO_32M_MODE_EN) && (TCXO_32M_MODE_EN != 0)
/* Table of load capacitance versus temperature for 32MHz crystal. Values below
   are for NDK NX2016SA 32MHz EXS00A-CS11213-6(IEC). Values are for temperatures
   from -40 to +130 in steps of 5 */
int32_t CLOCK_ai32MXtalIecLoadPfVsTemp_x1000[HW_32M_LOAD_VS_TEMP_SIZE] = {
    960,   1097,  1194,  1246,  1253,  1216,  1137,  1023,  /* -40, -35, ... -5 */
    879,   710,   523,   325,   122,   -81,   -277,  -464,  /* 0, 5, ... 35 */
    -637,  -794,  -933,  -1052, -1150, -1227, -1283, -1317, /* 40, 45, ... 75 */
    -1328, -1315, -1274, -1202, -1090, -930,  -709,  -409,  /* 80, 85, ... 115 */
    -9,    518,   1205};                                    /* 120, 125, 130 */
#endif

#if (defined TCXO_32k_MODE_EN) && (TCXO_32k_MODE_EN != 0)
/* Table of load capacitance versus temperature for 32kHz crystal. Values are
   for temperatures from -20 to +100 in steps of 20. *Note* values below are
   just for example */
int32_t CLOCK_ai32kXtalIecLoadPfVsTemp_x1000[HW_32k_LOAD_VS_TEMP_SIZE] = {960,   /* -20 */
                                                                          1097,  /*   0 */
                                                                          1194,  /*  20 */
                                                                          1246,  /*  40 */
                                                                          1253,  /*  60 */
                                                                          1216,  /*  80 */
                                                                          1137}; /* 100 */
#endif
/*******************************************************************************
 * Local Prototypes
 ******************************************************************************/
#ifdef CPU_K32W041AMK        
#define IOCON_QSPI_MODE_FUNC        (7U)
#define IOCON_PIO_DIGITAL_EN 0x80u    /*!<@brief Enables digital function */
#define IOCON_PIO_INV_DI 0x00u        /*!<@brief Input function is not inverted */
#define IOCON_PIO_INPFILT_OFF 0x0100u /*!<@brief Input filter disabled */
#define IOCON_PIO_OPENDRAIN_DI 0x00u  /*!<@brief Open drain is disabled */
#define IOCON_PIO_SSEL_DI 0x00u       /*!<@brief SSEL is disabled */

#define SPIFI_FAST_IO_MODE                                              \
    IOCON_PIO_FUNC(IOCON_QSPI_MODE_FUNC) |                              \
    /* No addition pin function */                                      \
    /* Fast mode, slew rate control is enabled */                       \
    IOCON_PIO_SLEW0(1) | IOCON_PIO_SLEW1(1) |                           \
    /* Input function is not inverted */                                \
    IOCON_PIO_INV_DI |                                                  \
    /* Enables digital function */                                      \
    IOCON_PIO_DIGITAL_EN |                                              \
    /* Input filter disabled */                                         \
    IOCON_PIO_INPFILT_OFF |                                             \
    /* Open drain is disabled */                                        \
    IOCON_PIO_OPENDRAIN_DI |                                            \
    /* SSEL is disabled */                                              \
    IOCON_PIO_SSEL_DI

#define LP_DIGITAL_PULLUP_CFG IOCON_PIO_FUNC(0) |\
                              IOCON_PIO_MODE(0) |\
                              IOCON_PIO_DIGIMODE(1)

#define SPIFI_FAST_IO_MODE_INACT   (SPIFI_FAST_IO_MODE | IOCON_PIO_MODE(2))
#define SPIFI_FAST_IO_MODE_PULLUP  (SPIFI_FAST_IO_MODE | IOCON_PIO_MODE(0))

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/


/*****************************************************************************
 * Local Prototypes
 ****************************************************************************/
static const iocon_group_t sfifi_io_cfg[] = {
    [0] = {
        .port = 0,
        .pin =  16,               /* SPIFI Chip Select */
        .modefunc = SPIFI_FAST_IO_MODE_PULLUP,
    },
    [1] = {
        .port = 0,
        .pin =  17,                 /*SPIFI DIO3 */
        .modefunc = SPIFI_FAST_IO_MODE_INACT,
    },
    [2] = {
        .port = 0,
        .pin =  18,                 /*SPIFI CLK */
        .modefunc = SPIFI_FAST_IO_MODE_INACT,
    },
    [3] = {
        .port = 0,
        .pin =  19,                 /*SPIFI DIO0 */
        .modefunc =SPIFI_FAST_IO_MODE_INACT,
    },
    [4] = {
        .port = 0,
        .pin =  20,                 /*SPIFI DIO2 */
        .modefunc = SPIFI_FAST_IO_MODE_INACT,
    },
    [5] = {
        .port = 0,
        .pin =  21,                 /*SPIFI DIO1 */
        .modefunc = SPIFI_FAST_IO_MODE_INACT,
    },
};
#endif

/*****************************************************************************
 * Local functions
 ****************************************************************************/

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/* Initialize debug console. */
status_t BOARD_InitDebugConsole(void)
{
    status_t result;

    uint32_t uartClkSrcFreq = BOARD_DEBUG_UART_CLK_FREQ;

    result =
        DbgConsole_Init(BOARD_DEBUG_UART_INSTANCE, BOARD_DEBUG_UART_BAUDRATE, BOARD_DEBUG_UART_TYPE, uartClkSrcFreq);

#ifndef RTL_SIMU_ON_ES2
    CLOCK_uDelay(500);
#endif

    return result;
}

/* Dummy functions, added to enable LowPower module to link */
void BOARD_DbgDiagEnable(void)
{
}
void BOARD_DbgLpIoSet(int pinid, int val)
{
    (void)(pinid);
    (void)(val);
}

/* MJL-REL10-SDK */
uint32_t BOARD_GetCtimerClock(CTIMER_Type *timer)
{
    return CLOCK_GetApbCLkFreq();
}

#ifdef CPU_K32W041AMK
void BOARD_InitSPIFI( void )
{
    uint32_t divisor;

    /* Enables the clock for the I/O controller block. 0: Disable. 1: Enable.: 0x01u */
    CLOCK_EnableClock(kCLOCK_Iocon);
    for (int i = 0; i < 6; i++)
    {
        if ((i==1 || i==4) && CHIP_USING_SPIFI_DUAL_MODE())
            continue;
        IOCON_PinMuxSet(IOCON,
                        sfifi_io_cfg[i].port,
                        sfifi_io_cfg[i].pin,
                        sfifi_io_cfg[i].modefunc);
    }
    RESET_SetPeripheralReset(kSPIFI_RST_SHIFT_RSTn);
    /* Enable clock to block and set divider */
    CLOCK_AttachClk(kMAIN_CLK_to_SPIFI);
    divisor = CLOCK_GetSpifiClkFreq() / BOARD_SPIFI_CLK_RATE;
    CLOCK_SetClkDiv(kCLOCK_DivSpifiClk, divisor ? divisor : 1, false);
    RESET_ClearPeripheralReset(kSPIFI_RST_SHIFT_RSTn);

}

void BOARD_SetSpiFi_LowPowerEnter(void)
{
#if 0
    /* Enables the clock for the I/O controller block. 0: Disable. 1: Enable.: 0x01u */
    CLOCK_EnableClock(kCLOCK_Iocon);
    /* Enables the clock for the GPIO0 module */
    CLOCK_EnableClock(kCLOCK_Gpio0);
    /* Turn all pins into analog inputs */
    for (int i = 0; i < 6; i++)
    {
        if ((i==1 || i==4) && CHIP_USING_SPIFI_DUAL_MODE())
            continue;
        /* Initialize GPIO functionality on each pin : all inputs */
        GPIO_PinInit(GPIO,
                     sfifi_io_cfg[i].port,
                     sfifi_io_cfg[i].pin,
                     &((const gpio_pin_config_t){kGPIO_DigitalInput, 1}));
        IOCON_PinMuxSet(IOCON,
                        sfifi_io_cfg[i].port,
                        sfifi_io_cfg[i].pin,
                        LP_DIGITAL_PULLUP_CFG);
    }
#endif
}
#endif

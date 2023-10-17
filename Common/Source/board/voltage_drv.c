/*
 *
 */
#include "dbg.h"
#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "fsl_adc.h"
#include "fsl_clock.h"
#include "voltage_drv.h"


#define ADC_Battery_SENSOR_CHANNEL 6

// Comment the define if you want to use specific calibration values or want to debug
#define LOAD_CAL_FROM_FLASH

static void update_ctrl0_adc_register(ADC_Type *base, uint8_t mode, uint8_t tsamp)
{
    // read the GPADC_CTRL0 register and update settings
    uint32_t read_reg = base->GPADC_CTRL0;

    // clear the "TEST" and "TSAMP" fields
    read_reg &= ~(ADC_GPADC_CTRL0_TEST_Msk | ADC_GPADC_CTRL0_GPADC_TSAMP_Msk);

    read_reg |= ((tsamp << ADC_GPADC_CTRL0_GPADC_TSAMP_Pos) & ADC_GPADC_CTRL0_GPADC_TSAMP_Msk);
    read_reg |= ((mode << ADC_GPADC_CTRL0_TEST_Pos) & ADC_GPADC_CTRL0_TEST_Msk);

    base->GPADC_CTRL0 = read_reg;
}

void Config_ADC(uint32_t delay_value)
{

	adc_conv_seq_config_t adcConvSeqConfigStruct;
	adc_config_t adcConfigStruct;

	adcConfigStruct.clockMode = kADC_ClockSynchronousMode; /* Using sync clock source. */
	adcConfigStruct.clockDividerNumber = 7;
	adcConfigStruct.resolution = kADC_Resolution12bit;
	adcConfigStruct.sampleTimeNumber = 0U;

	update_ctrl0_adc_register(ADC0, 0x0, 0x14);

	ADC_Init(ADC0, &adcConfigStruct);

	adcConvSeqConfigStruct.channelMask = (1U << ADC_Battery_SENSOR_CHANNEL);
	adcConvSeqConfigStruct.triggerMask = 2U;
	adcConvSeqConfigStruct.triggerPolarity = kADC_TriggerPolarityPositiveEdge;
	adcConvSeqConfigStruct.enableSingleStep = false;
	adcConvSeqConfigStruct.enableSyncBypass = true;
	adcConvSeqConfigStruct.interruptMode = kADC_InterruptForEachConversion;

	ADC_SetConvSeqAConfig(ADC0, &adcConvSeqConfigStruct);

	CLOCK_uDelay(delay_value);

	ADC_EnableConvSeqA(ADC0, true); /* Enable the conversion sequence A. */


}

uint16_t Get_BattVolt(void)
{

	Config_ADC(100);

	adc_result_info_t adcResultInfoStruct;
	uint32_t estimated_sum = 0;
	uint16_t voltage = 0;

	for (int i=0;i<13;i++)
	{
		ADC_DoSoftwareTriggerConvSeqA(ADC0);

		while (!ADC_GetChannelConversionResult(ADC0, ADC_Battery_SENSOR_CHANNEL, &adcResultInfoStruct))
		{
		}

		if (i>4)
		{
			PRINTF("adcResultInfoStruct.result = %d\r\n", adcResultInfoStruct.result);
			estimated_sum += adcResultInfoStruct.result;
		}
	}
	ADC_Deinit(ADC0);
	PRINTF("estimated_sum = %d\r\n", estimated_sum);
	voltage = (uint16_t)(estimated_sum / 8);
	PRINTF("voltage = %d\r\n", voltage);
	voltage = (uint16_t)adcResultInfoStruct.result;
	PRINTF("voltage = %d\r\n", voltage);


	return(voltage);
}

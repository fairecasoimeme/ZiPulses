/*****************************************************************************
 *
 * MODULE:             JN-AN-1220 ZLO Sensor Demo
 *
 * COMPONENT:          app_reporting.c
 *
 * DESCRIPTION:        ZLO Demo : OccupancySensor Report (Implementation)
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
#include <string.h>
#include "dbg.h"
#include "ZTimer.h"
#include "app_main.h"
#include "pdum_gen.h"
#include "PDM.h"
#include "app_common.h"
#include "PDM_IDs.h"
#include "zcl_options.h"
#include "app_reporting.h"
#include "App_PulsesSensor.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifndef DEBUG_REPORT
    #define TRACE_REPORT   FALSE
#else
    #define TRACE_REPORT   TRUE
#endif
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/*There is just one attributes at this point - Occupancy Attribute */

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
extern const uint8 u8MyEndpoint;
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
/*Just one reports for time being*/
PRIVATE tsReports asSavedReports[PULSES_NUMBER_OF_REPORTS];
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/


/****************************************************************************
 *
 * NAME: eRestoreReports
 *
 * DESCRIPTION:
 * Loads the reporting information from the EEPROM/PDM
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC PDM_teStatus eRestoreReports( void )
{
    /* Restore any report data that is previously saved to flash */
    uint16 u16ByteRead;
    PDM_teStatus eStatusReportReload = PDM_eReadDataFromRecord(PDM_ID_APP_REPORTS,
                                                              asSavedReports,
                                                              sizeof(asSavedReports),
                                                              &u16ByteRead);

    DBG_vPrintf(TRACE_REPORT, "\r\nREPORT: eRestoreReports()=%d", eStatusReportReload);
    /* Restore any application data previously saved to flash */

    return  (eStatusReportReload);
}

/****************************************************************************
 *
 * NAME: vMakeSupportedAttributesReportable
 *
 * DESCRIPTION:
 * Makes the attributes reportable for Occupancy attribute
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vMakeSupportedAttributesReportable(void)
{
    uint16 u16AttributeEnum;
    uint16 u16ClusterId;
    uint8 i;

    tsZCL_AttributeReportingConfigurationRecord*    psAttributeReportingConfigurationRecord;

    DBG_vPrintf(TRACE_REPORT, "\r\nREPORT: vMakeSupportedAttributesReportable()");
    for(i=0; i<PULSES_NUMBER_OF_REPORTS; i++)
    {
        u16AttributeEnum=asSavedReports[i].sAttributeReportingConfigurationRecord.u16AttributeEnum;
        u16ClusterId= asSavedReports[i].u16ClusterID;
        psAttributeReportingConfigurationRecord = &(asSavedReports[i].sAttributeReportingConfigurationRecord);
        DBG_vPrintf(TRACE_REPORT, "\r\nCluster %04x Attribute %04x Min %d Max %d IntV %d Direct %d Change %d",
                u16ClusterId,
                u16AttributeEnum,
                asSavedReports[i].sAttributeReportingConfigurationRecord.u16MinimumReportingInterval,
                asSavedReports[i].sAttributeReportingConfigurationRecord.u16MaximumReportingInterval,
                asSavedReports[i].sAttributeReportingConfigurationRecord.u16TimeoutPeriodField,
                asSavedReports[i].sAttributeReportingConfigurationRecord.u8DirectionIsReceived,
                asSavedReports[i].sAttributeReportingConfigurationRecord.uAttributeReportableChange.zint8ReportableChange);
        eZCL_CreateLocalReport( u8MyEndpoint, u16ClusterId, 0, TRUE, psAttributeReportingConfigurationRecord);
    }
}

/****************************************************************************
 *
 * NAME: vLoadDefaultConfigForReportable
 *
 * DESCRIPTION:
 * Loads a default configuration
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vLoadDefaultConfigForReportable(void)
{
    memset(asSavedReports, 0 ,sizeof(asSavedReports));
    int i;
    for (i=0; i<PULSES_NUMBER_OF_REPORTS; i++)
    {
        asSavedReports[i] = asDefaultReports[i];
    }

#if TRACE_REPORT

    DBG_vPrintf(TRACE_REPORT,"\r\nREPORT: vLoadDefaultConfigForReportable()");
    for(i=0; i <PULSES_NUMBER_OF_REPORTS; i++)
    {
        DBG_vPrintf(TRACE_REPORT,"\r\nCluster %04x Type %d Attr %04x Min %d Max %d IntV %d Direct %d Change %d",
                asSavedReports[i].u16ClusterID,
                asSavedReports[i].sAttributeReportingConfigurationRecord.eAttributeDataType,
                asSavedReports[i].sAttributeReportingConfigurationRecord.u16AttributeEnum,
                asSavedReports[i].sAttributeReportingConfigurationRecord.u16MinimumReportingInterval,
                asSavedReports[i].sAttributeReportingConfigurationRecord.u16MaximumReportingInterval,
                asSavedReports[i].sAttributeReportingConfigurationRecord.u16TimeoutPeriodField,
                asSavedReports[i].sAttributeReportingConfigurationRecord.u8DirectionIsReceived,
                asSavedReports[i].sAttributeReportingConfigurationRecord.uAttributeReportableChange.zuint8ReportableChange);
    }
#endif

    /*Save this Records*/
    PDM_eSaveRecordData(PDM_ID_APP_REPORTS,
                        asSavedReports,
                        sizeof(asSavedReports));

}


/****************************************************************************
 *
 * NAME: vSaveReportableRecord
 *
 * DESCRIPTION:
 * Loads a default configuration
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vSaveReportableRecord(  uint16 u16ClusterID,
                                    tsZCL_AttributeReportingConfigurationRecord* psAttributeReportingConfigurationRecord)
{
    int iIndex = 0;

    asSavedReports[iIndex].u16ClusterID=u16ClusterID;
    memcpy( &(asSavedReports[iIndex].sAttributeReportingConfigurationRecord),
            psAttributeReportingConfigurationRecord,
            sizeof(tsZCL_AttributeReportingConfigurationRecord) );

    DBG_vPrintf(TRACE_REPORT,"\r\nREPORT: vSaveReportableRecord()");
    DBG_vPrintf(TRACE_REPORT,"\r\nCluster %04x Type %d Attrib %04x Min %d Max %d IntV %d Direction %d Change %d",
            asSavedReports[iIndex].u16ClusterID,
            asSavedReports[iIndex].sAttributeReportingConfigurationRecord.eAttributeDataType,
            asSavedReports[iIndex].sAttributeReportingConfigurationRecord.u16AttributeEnum,
            asSavedReports[iIndex].sAttributeReportingConfigurationRecord.u16MinimumReportingInterval,
            asSavedReports[iIndex].sAttributeReportingConfigurationRecord.u16MaximumReportingInterval,
            asSavedReports[iIndex].sAttributeReportingConfigurationRecord.u16TimeoutPeriodField,
            asSavedReports[iIndex].sAttributeReportingConfigurationRecord.u8DirectionIsReceived,
            asSavedReports[iIndex].sAttributeReportingConfigurationRecord.uAttributeReportableChange.zuint8ReportableChange );

    /*Save this Records*/
    PDM_eSaveRecordData(PDM_ID_APP_REPORTS,
                        asSavedReports,
                        sizeof(asSavedReports));
}


PUBLIC void vSendImmediateIndexReport(void)
{
	PDUM_thAPduInstance myPDUM_thAPduInstance;
	tsZCL_Address sDestinationAddress;
	teZCL_Status eStatus;

	DBG_vPrintf(TRACE_REPORT, "\r\nREPORT: vSendImmediateReport()");

	/* get buffer to write the response in*/
	myPDUM_thAPduInstance = hZCL_AllocateAPduInstance();
	/* no buffers - return*/
	if(myPDUM_thAPduInstance == PDUM_INVALID_HANDLE)
	{
		DBG_vPrintf(TRACE_REPORT, "\r\nREPORT: hZCL_AllocateAPduInstance() == PDUM_INVALID_HANDLE, ERROR!");
	}
	/* Got buffer ? */
	else
	{
		/* Set the address mode to send to all bound device and don't wait for an ACK*/
		sDestinationAddress.eAddressMode = E_ZCL_AM_BOUND_NON_BLOCKING_NO_ACK; //MJL: was E_ZCL_AM_BOUND_NO_ACK;

		/* Attempt to send */
		eStatus = eZCL_ReportAttribute(&sDestinationAddress,
													SE_CLUSTER_ID_SIMPLE_METERING,
													0,
													1,
													1,
												   myPDUM_thAPduInstance);
		DBG_vPrintf(TRACE_REPORT, "\r\nREPORT: eZCL_ReportAllAttributes() = 0x%02x", eStatus);
		/* Sent the report with all attributes ? */
		if (E_ZCL_SUCCESS == eStatus)
		{
			DBG_vPrintf(TRACE_REPORT, ", SUCCESS");
		}
		else
		{
			DBG_vPrintf(TRACE_REPORT, ", FAILED");
			/* free buffer for failed send */
			PDUM_eAPduFreeAPduInstance(myPDUM_thAPduInstance);
		}

	}
}

/****************************************************************************
 *
 * NAME: vSendImmediateReport
 *
 * DESCRIPTION:
 * Method to send a report to all bound nodes.
 *
 ****************************************************************************/
PUBLIC void vSendImmediateAllReport(void)
{
    PDUM_thAPduInstance myPDUM_thAPduInstance;
    tsZCL_Address sDestinationAddress;
    teZCL_Status eStatus;

    DBG_vPrintf(TRACE_REPORT, "\r\nREPORT: vSendImmediateReport()");

    /* get buffer to write the response in*/
    myPDUM_thAPduInstance = hZCL_AllocateAPduInstance();
    /* no buffers - return*/
    if(myPDUM_thAPduInstance == PDUM_INVALID_HANDLE)
    {
        DBG_vPrintf(TRACE_REPORT, "\r\nREPORT: hZCL_AllocateAPduInstance() == PDUM_INVALID_HANDLE, ERROR!");
    }
    /* Got buffer ? */
    else
    {
        /* Set the address mode to send to all bound device and don't wait for an ACK*/
        sDestinationAddress.eAddressMode = E_ZCL_AM_BOUND_NON_BLOCKING_NO_ACK; //MJL: was E_ZCL_AM_BOUND_NO_ACK;

        /* Attempt to send */
    	eStatus = eZCL_ReportAttribute(&sDestinationAddress,
    	        									SE_CLUSTER_ID_SIMPLE_METERING,
													0,
    	                                            1,
    	                                            1,
    	                                           myPDUM_thAPduInstance);
        DBG_vPrintf(TRACE_REPORT, "\r\nREPORT: eZCL_ReportAllAttributes() = 0x%02x", eStatus);
        /* Sent the report with all attributes ? */
        if (E_ZCL_SUCCESS == eStatus)
        {
            DBG_vPrintf(TRACE_REPORT, ", SUCCESS");
        }
        else
        {
            DBG_vPrintf(TRACE_REPORT, ", FAILED");
            /* free buffer for failed send */
            PDUM_eAPduFreeAPduInstance(myPDUM_thAPduInstance);
        }
        APP_vGetVoltageBattery();
        eStatus = eZCL_ReportAttribute(&sDestinationAddress,
        											GENERAL_CLUSTER_ID_POWER_CONFIGURATION,
													E_CLD_PWRCFG_ATTR_ID_BATTERY_VOLTAGE,
													1,
													1,
												   myPDUM_thAPduInstance);
        DBG_vPrintf(TRACE_REPORT, "\r\nREPORT: eZCL_ReportAllAttributes() = 0x%02x", eStatus);
		/* Sent the report with all attributes ? */
		if (E_ZCL_SUCCESS == eStatus)
		{
			DBG_vPrintf(TRACE_REPORT, ", SUCCESS");
		}
		else
		{
			DBG_vPrintf(TRACE_REPORT, ", FAILED");
			/* free buffer for failed send */
			PDUM_eAPduFreeAPduInstance(myPDUM_thAPduInstance);
		}

		APP_vRadioTempUpdate(TRUE);
		eStatus = eZCL_ReportAttribute(&sDestinationAddress,
				 	 	 	 	 	 	 	 MEASUREMENT_AND_SENSING_CLUSTER_ID_TEMPERATURE_MEASUREMENT,
											 E_CLD_TEMPMEAS_ATTR_ID_MEASURED_VALUE,
												1,
												1,
											   myPDUM_thAPduInstance);
		DBG_vPrintf(TRACE_REPORT, "\r\nREPORT: eZCL_ReportAllAttributes() = 0x%02x", eStatus);
		/* Sent the report with all attributes ? */
		if (E_ZCL_SUCCESS == eStatus)
		{
			DBG_vPrintf(TRACE_REPORT, ", SUCCESS");
		}
		else
		{
			DBG_vPrintf(TRACE_REPORT, ", FAILED");
			/* free buffer for failed send */
			PDUM_eAPduFreeAPduInstance(myPDUM_thAPduInstance);
		}
}
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

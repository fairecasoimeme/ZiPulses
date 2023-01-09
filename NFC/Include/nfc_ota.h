/*****************************************************************************
 *
 * MODULE:      NFC
 *
 * COMPONENT:   nfc_ota.h
 *
 * AUTHOR:      Martin Looker
 *
 * DESCRIPTION: Common macros used by all NFC OTA processing
 *
 * $HeadURL: https://www.collabnet.nxp.com/svn/lprf_sware/Projects/Components/NFC/Tags/+v1017/Include/nfc_ota.h $
 *
 * $Revision: 93233 $
 *
 * $LastChangedBy: nxp29761 $
 *
 * $LastChangedDate: 2018-04-13 17:19:04 +0800 (周五, 2018-04-13) $
 *
 * $Id: nfc_ota.h 93233 2018-04-13 09:19:04Z nxp29761 $
 *
 ****************************************************************************
 *
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on NXP products [NXP Microcontrollers such as JN5168, JN5179].
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
 * Copyright NXP B.V. 2017-2018. All rights reserved
 *
 ***************************************************************************/
/*!
 * \file  nfc_ota.h
 *
 * \brief Common macros used by all NFC OTA processing
 *
 ***************************************************************************/
#ifndef NFC_OTA_H_
#define NFC_OTA_H_

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/
#include <jendefs.h>

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
/* SRAM size */
#define NFC_OTA_SRAM_SIZE                     64 /*!< Size of SRAM in bytes */

/* SRAM command values and flags */
#define NFC_OTA_CMD_NULL                    0x00 /*!< Null command value */
#define NFC_OTA_CMD_OTA_HEADER              0x01 /*!< OTA header command */
#define NFC_OTA_CMD_OTA_DATA                0x02 /*!< OTA data command */
#define NFC_OTA_CMD_FLAG_MORE               0x80 /*!< OTA more commands to follow flag */

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
#pragma GCC diagnostic ignored "-Wpacked"
#pragma GCC diagnostic ignored "-Wattributes"

/*! OTA Header Data */
typedef struct
{
    uint32 u32FileIdentifier;   /*!< OTA File Identifier */         //  4
    uint16 u16ManufacturerCode; /*!< OTA Manufacturer Code */       //  6
    uint16 u16ImageType;        /*!< OTA Image Type */              //  8
    uint32 u32FileVersion;      /*!< OTA File Version */            // 12
    uint16 u16StackVersion;     /*!< OTA Stack Version */           // 14
    uint32 u32TotalImage;       /*!< OTA Total Image Size */        // 22
    uint32 u32SavedImage;       /*!< OTA Saved Image Size */        // 26
} PACK tsNfcOtaHeader;

/*! OTA Header Command */
#define NFC_OTA_CMD_HEADER_SPARE_SIZE (NFC_OTA_SRAM_SIZE - 53)
typedef struct
{
    uint8          u8Cmd;       /*!< OTA Command ID */              //  1
    tsNfcOtaHeader  sActive;    /*!< OTA Active Image Header */     // 27
    tsNfcOtaHeader  sInactive;  /*!< OTA Inactive Image Header */   // 53
    uint8         au8Spare[NFC_OTA_CMD_HEADER_SPARE_SIZE];
} PACK tsNfcOtaCmdHeader;

/*! OTA Data Command */
#define NFC_OTA_CMD_DATA_DATA_SIZE (NFC_OTA_SRAM_SIZE - 5)
typedef struct
{
    uint8          u8Cmd;       /*!< OTA Command ID */              //  1
    uint32        u32Offset;    /*!< Offset of data within image */ //  5
                                /*!  Image data */                  // 64
    uint8         au8Data[NFC_OTA_CMD_DATA_DATA_SIZE];
} PACK tsNfcOtaCmdData;

#pragma GCC diagnostic pop

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#endif /* NFC_NWK_H_ */
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

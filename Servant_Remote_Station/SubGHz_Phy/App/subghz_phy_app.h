/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    subghz_phy_app.h
 * @author  MCD Application Team
 * @brief   Header of application of the SubGHz_Phy Middleware
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SUBGHZ_PHY_APP_H__
#define __SUBGHZ_PHY_APP_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "BME280.h"
#include <stdbool.h>
#include <stdint.h>
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

struct __attribute__((packed)) dataPacket {
  int32_t temperature;
  uint32_t pressure;
  uint32_t humidity;
  uint16_t seqNum;
  uint8_t nodeID;
};

struct __attribute__((packed)) ackPacket {
  uint16_t seqNum;
  uint8_t nodeID;
};

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/

/* USER CODE BEGIN EC */
extern bool packetACKED;
/* USER CODE END EC */

/* External variables --------------------------------------------------------*/
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Exported macros -----------------------------------------------------------*/
/* USER CODE BEGIN EM */
#define RF_FREQUENCY 915000000 // US ISM 915 MHz
#define TX_OUTPUT_POWER 14     // dBm
#define LORA_BANDWIDTH 0       // 125 kHz
#define LORA_SPREADING_FACTOR 7
#define LORA_CODINGRATE 1 // 4/5
#define LORA_PREAMBLE_LENGTH 8
#define LORA_SYMBOL_TIMEOUT 5
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define TX_BUFFER_SIZE 256
/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
/**
 * @brief  Init Subghz Application
 */
void SubghzApp_Init(void);

/* USER CODE BEGIN EFP */
void sendWeatherData(struct weatherData *data, uint8_t nodeID);
/* USER CODE END EFP */

#ifdef __cplusplus
}
#endif

#endif /*__SUBGHZ_PHY_APP_H__*/

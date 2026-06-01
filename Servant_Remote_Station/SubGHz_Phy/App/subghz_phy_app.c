/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    subghz_phy_app.c
 * @author  MCD Application Team
 * @brief   Application of the SubGHz_Phy Middleware
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

/* Includes ------------------------------------------------------------------*/
#include "subghz_phy_app.h"
#include "platform.h"
#include "sys_app.h"

/* USER CODE BEGIN Includes */
#include "../../Middlewares/Third_Party/SubGHz_Phy/radio_driver/radio.h"
#include <stdint.h>
#include <string.h>
/* USER CODE END Includes */

/* External variables
 * ---------------------------------------------------------*/
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* Radio events function pointer */
static RadioEvents_t RadioEvents;

/* USER CODE BEGIN PV */
static volatile bool txDone = true;
bool packetACKED = true;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/*!
 * @brief Function to be executed on Radio Tx Done event
 */
static void OnTxDone(void);

/**
 * @brief Function to be executed on Radio Rx Done event
 * @param  payload ptr of buffer received
 * @param  size buffer size
 * @param  rssi
 * @param  LoraSnr_FskCfo
 */
static void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi,
                     int8_t LoraSnr_FskCfo);

/**
 * @brief Function executed on Radio Tx Timeout event
 */
static void OnTxTimeout(void);

/**
 * @brief Function executed on Radio Rx Timeout event
 */
static void OnRxTimeout(void);

/**
 * @brief Function executed on Radio Rx Error event
 */
static void OnRxError(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Exported functions
 * ---------------------------------------------------------*/
void SubghzApp_Init(void) {
  /* USER CODE BEGIN SubghzApp_Init_1 */

  /* USER CODE END SubghzApp_Init_1 */

  /* Radio initialization */
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.RxDone = OnRxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  RadioEvents.RxTimeout = OnRxTimeout;
  RadioEvents.RxError = OnRxError;

  Radio.Init(&RadioEvents);

  /* USER CODE BEGIN SubghzApp_Init_2 */
  Radio.SetChannel(RF_FREQUENCY);

  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                    true, // CRC on
                    0, 0, LORA_IQ_INVERSION_ON,
                    300 // timeout ms
  );
  Radio.SetRxConfig(MODEM_LORA,
                    LORA_BANDWIDTH,        // bandwidth
                    LORA_SPREADING_FACTOR, // datarate / spreading factor
                    LORA_CODINGRATE,       // coding rate
                    0,                     // bandwidth AFC, unused for LoRa
                    LORA_PREAMBLE_LENGTH, LORA_SYMBOL_TIMEOUT,
                    LORA_FIX_LENGTH_PAYLOAD_ON, 0,
                    true, // CRC on
                    0,    // freq hop off
                    0,    // hop period
                    LORA_IQ_INVERSION_ON,
                    true // continuous RX
  );
  Radio.Rx(0);

  /* USER CODE END SubghzApp_Init_2 */
}

/* USER CODE BEGIN EF */

/* USER CODE END EF */

/* Private functions ---------------------------------------------------------*/
static void OnTxDone(void) {
  /* USER CODE BEGIN OnTxDone */
  txDone = true;
  Radio.Rx(0);
  /* USER CODE END OnTxDone */
}

static void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi,
                     int8_t LoraSnr_FskCfo) {
  /* USER CODE BEGIN OnRxDone */
  if (size != sizeof(struct ackPacket)) {
    printf("RX %u bytes (expected ACK %u), RSSI=%d\r\n", size,
           (unsigned)sizeof(struct ackPacket), rssi);
    Radio.Rx(0);
    return;
  }
  struct ackPacket receiveACK = {0};
  memcpy(&receiveACK, payload, sizeof(struct ackPacket));
  printf("ACK: seq=%u, node ID=%u\r\n", receiveACK.seqNum, receiveACK.nodeID);
  packetACKED = true;
  Radio.Rx(0);
  /* USER CODE END OnRxDone */
}

static void OnTxTimeout(void) {
  /* USER CODE BEGIN OnTxTimeout */
  txDone = true;
  Radio.Rx(0);
  /* USER CODE END OnTxTimeout */
}

static void OnRxTimeout(void) {
  /* USER CODE BEGIN OnRxTimeout */
  Radio.Rx(0);
  /* USER CODE END OnRxTimeout */
}

static void OnRxError(void) {
  /* USER CODE BEGIN OnRxError */
  Radio.Rx(0);
  /* USER CODE END OnRxError */
}

/* USER CODE BEGIN PrFD */
void sendWeatherData(struct weatherData *data, uint8_t nodeID,
                     RTC_HandleTypeDef *hrtc) {
  static uint16_t seqNum = 0;
  RTC_TimeTypeDef currTime = {0};
  RTC_DateTypeDef currDate = {0};
  uint32_t timestamp = 0;

  if (!txDone) {
    printf("Radio busy, skipping packet\r\n");
    return;
  }
  if (packetACKED) {
    if (HAL_RTC_GetTime(hrtc, &currTime, RTC_FORMAT_BIN) == HAL_OK) {
      timestamp = (currTime.Hours * 60 * 60) + (currTime.Minutes * 60) +
                  (currTime.Seconds);
    }
    // this value is not needed, only needs to be called for time to be accurate
    HAL_RTC_GetDate(hrtc, &currDate, RTC_FORMAT_BIN);
    seqNum++;
  }

  struct dataPacket weatherPacket = {.pressure = data->pressure,
                                     .temperature = data->temperature,
                                     .humidity = data->humidity,
                                     .seqNum = seqNum,
                                     .nodeID = nodeID,
                                     .timestamp = timestamp};

  txDone = false;
  packetACKED = false;
  printf("Sending...\r\n");
  Radio.Send((uint8_t *)&weatherPacket, sizeof(struct dataPacket));
}
/* USER CODE END PrFD */

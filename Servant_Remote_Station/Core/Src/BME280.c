#include "BME280.h"
#include "stm32wlxx_hal.h"
#include "stm32wlxx_hal_spi.h"
#include <stdint.h>

static inline void writeRegister(const struct BME280 *sensor, uint8_t *data,
                                 uint8_t *address) {
  sensor->CSPort->BSRR = (sensor->CSPin << 16);
  HAL_SPI_Transmit(sensor->SPIHandler, address, 1, 200);
  HAL_SPI_Transmit(sensor->SPIHandler, data, 1, 200);
  sensor->CSPort->BSRR = (sensor->CSPin);
}

static inline void readRegister(const struct BME280 *sensor, uint16_t size,
                                uint8_t *data, uint8_t *address) {
  sensor->CSPort->BSRR = (sensor->CSPin << 16);
  HAL_SPI_Transmit(sensor->SPIHandler, address, 1, 200);
  HAL_SPI_Receive(sensor->SPIHandler, data, size, 200);
  sensor->CSPort->BSRR = (sensor->CSPin);
}

void initBME280(struct BME280 *sensor, SPI_HandleTypeDef *SPIHandler,
                GPIO_TypeDef *CSPort, const uint16_t CSPin) {
  sensor->SPIHandler = SPIHandler;
  sensor->CSPort = CSPort;
  sensor->CSPin = CSPin;

  // set humidity to oversampling * 1
  uint8_t address = 0x72;
  uint8_t data = 0x1;
  writeRegister(sensor, &data, &address);
  // set temperature and pressure to oversampling * 1 and mode to forced
  address = 0x74;
  data = 0x25;
  writeRegister(sensor, &data, &address);
}

void readRawWeatherData(const struct BME280 *sensor,
                        struct rawWeatherData *rawWeatherData) {
  // Wake up into force mode to do a read once data is moved into registers
  uint8_t measuring = 0xFF;
  uint8_t address = 0x74;
  uint8_t forceModeAddress = 0x25;
  uint8_t timeout = 100;
  writeRegister(sensor, &forceModeAddress, &address);
  address = 0xF3;
  while (measuring & 0x8 && timeout--) {
    readRegister(sensor, 1, &measuring, &address);
    HAL_Delay(1);
  }

  uint8_t rawData[8] = {0};
  address = 0xF7;
  readRegister(sensor, 8, rawData, &address);
  rawWeatherData->rawHumidty =
      ((uint16_t)rawData[6] << 8) | ((uint16_t)rawData[7]);
  rawWeatherData->rawTemperature = ((uint32_t)rawData[3] << 12) |
                                   ((uint32_t)rawData[4] << 4) |
                                   ((uint32_t)rawData[5] >> 4);
  rawWeatherData->rawPressure = ((uint32_t)rawData[0] << 12) |
                                ((uint32_t)rawData[1] << 4) |
                                ((uint32_t)rawData[2] >> 4);
}

void readWeatherData(const struct BME280 *sensor, struct weatherData *data) {
  struct rawWeatherData rawData = {0};
  readRawWeatherData(sensor, &rawData);

  // The following code is from Bosch's documentation. I just had to edit it
  // and add some stuff. They have a lot of magic numbers and witchcraft
  // happening here

  /* Temperature Calibration */
  uint8_t digTArr[6] = {0};
  uint8_t address = 0x88;
  readRegister(sensor, 6, digTArr, &address);
  uint16_t dig_T1 = (digTArr[1] << 8) | digTArr[0];
  int16_t dig_T2 = (digTArr[3] << 8) | digTArr[2];
  int16_t dig_T3 = (digTArr[5] << 8) | digTArr[4];

  int32_t tVar1, tVar2, t_fine;
  tVar1 = ((((rawData.rawTemperature >> 3) - ((int32_t)dig_T1 << 1))) *
           ((int32_t)dig_T2)) >>
          11;
  tVar2 = (((((rawData.rawTemperature >> 4) - ((int32_t)dig_T1)) *
             ((rawData.rawTemperature >> 4) - ((int32_t)dig_T1))) >>
            12) *
           ((int32_t)dig_T3)) >>
          14;
  t_fine = tVar1 + tVar2;
  data->temperature = (t_fine * 5 + 128) >> 8;

  /* Pressure Calibration */
  uint8_t digPArr[18] = {0};
  address = 0x8E;
  readRegister(sensor, 18, digPArr, &address);
  uint16_t dig_P1 = (digPArr[1] << 8) | digPArr[0];
  int16_t dig_P2 = (digPArr[3] << 8) | digPArr[2];
  int16_t dig_P3 = (digPArr[5] << 8) | digPArr[4];
  int16_t dig_P4 = (digPArr[7] << 8) | digPArr[6];
  int16_t dig_P5 = (digPArr[9] << 8) | digPArr[8];
  int16_t dig_P6 = (digPArr[11] << 8) | digPArr[10];
  int16_t dig_P7 = (digPArr[13] << 8) | digPArr[12];
  int16_t dig_P8 = (digPArr[15] << 8) | digPArr[14];
  int16_t dig_P9 = (digPArr[17] << 8) | digPArr[16];

  int64_t p, pVar1, pVar2;
  pVar1 = ((int64_t)t_fine) - 128000;
  pVar2 = pVar1 * pVar1 * (int64_t)dig_P6;
  pVar2 = pVar2 + ((pVar1 * (int64_t)dig_P5) << 17);
  pVar2 = pVar2 + (((int64_t)dig_P4) << 35);
  pVar1 = ((pVar1 * pVar1 * (int64_t)dig_P3) >> 8) +
          ((pVar1 * (int64_t)dig_P2) << 12);
  pVar1 = (((((int64_t)1) << 47) + pVar1)) * ((int64_t)dig_P1) >> 33;
  if (pVar1 == 0) {
    data->pressure = 0;
    return; // avoid exception caused by division by zero
  }
  p = 1048576 - rawData.rawPressure;
  p = (((p << 31) - pVar2) * 3125) / pVar1;
  pVar1 = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
  pVar2 = (((int64_t)dig_P8) * p) >> 19;
  p = ((p + pVar1 + pVar2) >> 8) + (((int64_t)dig_P7) << 4);
  data->pressure = (uint32_t)p;

  /* Humidity Calibration */
  uint8_t dig_H1;
  address = 0xA1;
  readRegister(sensor, 1, &dig_H1, &address);
  uint8_t digHArr[7] = {0};
  address = 0xE1;
  readRegister(sensor, 7, digHArr, &address);
  int16_t dig_H2 = (digHArr[1] << 8) | digHArr[0];
  uint8_t dig_H3 = digHArr[2];
  int16_t dig_H4 = ((int16_t)((digHArr[3] << 4) | (digHArr[4] & 0x0F)));
  if (dig_H4 & 0x0800)
    dig_H4 |= 0xF000; // deal with sign bit
  int16_t dig_H5 = ((int16_t)((digHArr[5] << 4) | (digHArr[4] >> 4)));
  if (dig_H5 & 0x0800)
    dig_H5 |= 0xF000; // deal with sign bit
  int8_t dig_H6 = digHArr[6];
  int32_t v_x1_u32r;
  v_x1_u32r = (t_fine - ((int32_t)76800));
  v_x1_u32r =
      (((((rawData.rawHumidty << 14) - (((int32_t)dig_H4) << 20) -
          (((int32_t)dig_H5) * v_x1_u32r)) +
         ((int32_t)16384)) >>
        15) *
       (((((((v_x1_u32r * ((int32_t)dig_H6)) >> 10) *
            (((v_x1_u32r * ((int32_t)dig_H3)) >> 11) + ((int32_t)32768))) >>
           10) +
          ((int32_t)2097152)) *
             ((int32_t)dig_H2) +
         8192) >>
        14));
  v_x1_u32r =
      (v_x1_u32r -
       (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)dig_H1)) >>
        4));
  v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
  v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
  data->humidity = (uint32_t)(v_x1_u32r >> 12);
}

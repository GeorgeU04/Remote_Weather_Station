#ifndef BME280_H
#define BME280_H

#include "main.h"
#include <stdint.h>

struct BME280 {
  SPI_HandleTypeDef *SPIHandler;
  GPIO_TypeDef *CSPort;
  uint16_t CSPin;
};

struct rawWeatherData {
  uint32_t rawTemperature;
  uint32_t rawPressure;
  uint16_t rawHumidity;
};

struct weatherData {
  int32_t temperature;
  uint32_t pressure;
  uint32_t humidity;
};

void initBME280(struct BME280 *sensor, SPI_HandleTypeDef *SPIHandler,
                GPIO_TypeDef *CSPort, const uint16_t CSPin);
void readRawWeatherData(const struct BME280 *sensor,
                        struct rawWeatherData *data);
void readWeatherData(const struct BME280 *sensor, struct weatherData *data);

#endif // !BME280_H

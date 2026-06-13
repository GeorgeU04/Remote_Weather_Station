#ifndef MISC_H
#define MISC_H
#include <stdint.h>

uint8_t strToUint8(const char *str, uint8_t *ret);
uint8_t strToUint64(const char *str, uint64_t *ret);
uint8_t strToUint32(const char *str, uint32_t *ret);
#endif // !MISC_H

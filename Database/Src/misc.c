#include "../Inc/misc.h"
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

uint8_t strToUint8(const char *str, uint8_t *ret) {
  if (!str || !ret)
    return EXIT_FAILURE;
  char *end;
  uint64_t value = strtoul(str, &end, 10);

  if (errno == ERANGE || value > UINT8_MAX || *end != '\0') {
    return EXIT_FAILURE;
  }
  *ret = (uint8_t)value;
  return EXIT_SUCCESS;
}

uint8_t strToUint64(const char *str, uint64_t *ret) {
  if (!str || !ret)
    return EXIT_FAILURE;
  char *end;
  uint64_t value = strtoul(str, &end, 10);

  if (errno == ERANGE || *end != '\0') {
    return EXIT_FAILURE;
  }
  *ret = (uint64_t)value;
  return EXIT_SUCCESS;
}

uint8_t strToUint32(const char *str, uint32_t *ret) {
  if (!str || !ret)
    return EXIT_FAILURE;
  char *end;
  uint64_t value = strtoul(str, &end, 10);

  if (errno == ERANGE || *end != '\0' || value > UINT32_MAX) {
    return EXIT_FAILURE;
  }
  *ret = (uint32_t)value;
  return EXIT_SUCCESS;
}

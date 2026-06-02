#include "database.h"
#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <errno.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

const uint8_t MAX_NUM_SENSORS = 12;
uint32_t maxNumOfEntries = 0;
FILE *fptrArr[12] = {0};
uint32_t idxArr[12] = {0};
uint32_t recordCountArr[12] = {0};
uint8_t SetNumOfSensors = 0;

uint8_t initDatabase(uint8_t numOfSensors, uint64_t pageSize) {
  char fileName[14] = {0};
  memset(recordCountArr, 0, sizeof(recordCountArr));
  memset(idxArr, 0, sizeof(idxArr));
  memset(fptrArr, 0, sizeof(fptrArr));
  SetNumOfSensors = numOfSensors;
  for (size_t i = 0; i < numOfSensors; ++i) {
    if (snprintf(fileName, sizeof(fileName), "sensor_%zu.dat", i) < 0) {
      return EXIT_FAILURE;
    }
    fptrArr[i] = fopen(fileName, "wb+");
  }
  maxNumOfEntries = pageSize / sizeof(struct weatherData);

  return EXIT_SUCCESS;
}

uint8_t insert(uint8_t nodeID, const struct weatherData *data) {
  if (!data || nodeID >= SetNumOfSensors)
    return EXIT_FAILURE;
  uint32_t idx = idxArr[nodeID];
  FILE *fp = fptrArr[nodeID];
  uint32_t writeOffset = idx * sizeof(struct weatherData);
  if (fseek(fp, writeOffset, SEEK_SET) != 0)
    return EXIT_FAILURE;
  if (fwrite(data, sizeof(struct weatherData), 1, fp) != 1)
    return EXIT_FAILURE;
  fflush(fp);
  idxArr[nodeID] = (idx + 1) % maxNumOfEntries;
  if (recordCountArr[nodeID] != maxNumOfEntries)
    recordCountArr[nodeID]++;
  return EXIT_SUCCESS;
}

uint8_t insertN(uint8_t nodeID, uint32_t n, const struct weatherData *data) {
  if (!data || nodeID >= SetNumOfSensors)
    return EXIT_FAILURE;
  uint32_t idx = idxArr[nodeID];
  FILE *fp = fptrArr[nodeID];
  uint32_t writeOffset = idx * sizeof(struct weatherData);
  if (fseek(fp, writeOffset, SEEK_SET) != 0)
    return EXIT_FAILURE;
  if (fwrite(data, sizeof(struct weatherData), n, fp) != n)
    return EXIT_FAILURE;
  fflush(fp);
  idxArr[nodeID] = (idx + n) % maxNumOfEntries;
  if (recordCountArr[nodeID] + n < maxNumOfEntries)
    recordCountArr[nodeID] += n;
  else {
    recordCountArr[nodeID] = maxNumOfEntries;
  };
  return EXIT_SUCCESS;
}

uint8_t readLast(uint8_t nodeID, struct weatherData *data) {
  if (!data || nodeID >= SetNumOfSensors)
    return EXIT_FAILURE;
  if (recordCountArr[nodeID] == 0)
    return EXIT_FAILURE;
  FILE *fp = fptrArr[nodeID];
  fflush(fp);
  uint32_t idx = idxArr[nodeID] - 1;
  uint32_t readOffset = (idx % maxNumOfEntries) * sizeof(struct weatherData);
  if (fseek(fptrArr[nodeID], readOffset, SEEK_SET) != 0)
    return EXIT_FAILURE;
  if ((fread(data, sizeof(struct weatherData), 1, fptrArr[nodeID]) != 1))
    return EXIT_FAILURE;
  return EXIT_SUCCESS;
}

uint8_t readAll(uint8_t nodeID, uint32_t *recordsRead,
                struct weatherData *data) {
  if (!data || nodeID >= SetNumOfSensors || !recordsRead)
    return EXIT_FAILURE;
  *recordsRead = 0;
  if (recordCountArr[nodeID] == 0)
    return EXIT_FAILURE;
  FILE *fp = fptrArr[nodeID];
  uint32_t idx = idxArr[nodeID];
  uint32_t recordCount = recordCountArr[nodeID];
  fflush(fp);
  if (recordCount != maxNumOfEntries) {
    if (fseek(fptrArr[nodeID], 0, SEEK_SET) != 0)
      return EXIT_FAILURE;
    if ((fread(data, sizeof(struct weatherData), recordCount,
               fptrArr[nodeID]) != recordCount))
      return EXIT_FAILURE;
    *recordsRead = recordCount;
    // handle the wrap around
  } else {
    // first chunk
    uint32_t firstChunk = recordCount - idx;
    if (fseek(fptrArr[nodeID], idx * sizeof(struct weatherData), SEEK_SET) != 0)
      return EXIT_FAILURE;
    if ((fread(data, sizeof(struct weatherData), firstChunk, fptrArr[nodeID]) !=
         firstChunk))
      return EXIT_FAILURE;
    // second chunk
    if (fseek(fptrArr[nodeID], 0, SEEK_SET) != 0)
      return EXIT_FAILURE;
    if ((fread(data + firstChunk, sizeof(struct weatherData), idx,
               fptrArr[nodeID]) != idx))
      return EXIT_FAILURE;
    *recordsRead = recordCount;
  }
  return EXIT_SUCCESS;
}

uint8_t readN(uint8_t nodeID, uint32_t n, uint32_t *recordsRead,
              struct weatherData *data) {
  if (!data || nodeID >= SetNumOfSensors || !recordsRead)
    return EXIT_FAILURE;
  *recordsRead = 0;
  if (recordCountArr[nodeID] == 0)
    return EXIT_FAILURE;
  FILE *fp = fptrArr[nodeID];
  fflush(fp);
  if (n >= recordCountArr[nodeID]) {
    if (readAll(nodeID, recordsRead, data) != 0)
      return EXIT_FAILURE;
    return EXIT_SUCCESS;
  }
  uint32_t readOffset =
      (recordCountArr[nodeID] - n) * sizeof(struct weatherData);
  if (fseek(fptrArr[nodeID], readOffset, SEEK_SET) != 0)
    return EXIT_FAILURE;
  if ((fread(data, sizeof(struct weatherData), n, fptrArr[nodeID]) != n))
    return EXIT_FAILURE;
  *recordsRead = n;
  return EXIT_SUCCESS;
}

uint8_t createServer(int32_t *sock, int32_t *RXSock,
                     struct sockaddr_in address) {
  if (!sock || !RXSock)
    return EXIT_FAILURE;
  *sock = -1;
  *RXSock = -1;
  *sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (*sock == -1)
    return EXIT_FAILURE;
  int32_t opt = 1;
  setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  socklen_t addressSize = sizeof(address);
  if (bind(*sock, (struct sockaddr *)&address, addressSize) == -1)
    return EXIT_FAILURE;
  if (listen(*sock, 1) == -1)
    return EXIT_FAILURE;
  return EXIT_SUCCESS;
}

uint8_t acceptClient(int32_t sock, int32_t *RXSock,
                     struct sockaddr_in address) {
  if (!RXSock)
    return EXIT_FAILURE;
  socklen_t addressSize = sizeof(address);
  *RXSock =
      accept4(sock, (struct sockaddr *)&address, &addressSize, SOCK_NONBLOCK);
  if (*RXSock == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK)
      return EXIT_SUCCESS;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

uint8_t receiveCommand(int32_t RXsock, char *command, size_t commandSize) {
  ssize_t n = 0;
  if (!command || commandSize == 0)
    return EXIT_FAILURE;
  n = read(RXsock, command, commandSize - 1);
  // add extra reads if n != sizeof(buffer) -1
  if (n == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      command[0] = '\0';
      return EXIT_SUCCESS;
    } else {
      return EXIT_FAILURE;
    }
  }
  if (n == 0) {
    command[0] = '\0';
    return EXIT_FAILURE;
  }
  command[n] = '\0';
  return EXIT_SUCCESS;
}

uint8_t sendJSONData(int32_t RXsock, const char *data) { return EXIT_SUCCESS; }

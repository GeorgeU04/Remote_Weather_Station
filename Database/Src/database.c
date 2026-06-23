#include "../Inc/database.h"
#include "../Inc/misc.h"
#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <errno.h>
#include <inttypes.h>
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
  char fileName[19] = {0};
  memset(recordCountArr, 0, sizeof(recordCountArr));
  memset(idxArr, 0, sizeof(idxArr));
  memset(fptrArr, 0, sizeof(fptrArr));
  SetNumOfSensors = numOfSensors;
  for (size_t i = 0; i < numOfSensors; ++i) {
    if (snprintf(fileName, sizeof(fileName), "Data/sensor_%zu.dat", i) < 0) {
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

uint8_t sendJSONData(int32_t RXsock, char *data) {
  if (!data)
    return EXIT_FAILURE;
  char *identifier;
  char *nodeIDStr;
  uint8_t error = 0;
  uint32_t n = 0;
  identifier = strtok(data, " ");
  if (!identifier)
    return EXIT_FAILURE;

  if (strcmp("READ_N", identifier) == 0) {
    char *nStr;
    nodeIDStr = strtok(NULL, " ");
    if (!nodeIDStr)
      return EXIT_FAILURE;
    nStr = strtok(NULL, " \n");
    if (!nStr)
      return EXIT_FAILURE;
    error = strToUint32(nStr, &n);
    if (error) // fail point
      return EXIT_FAILURE;
  } else {
    nodeIDStr = strtok(NULL, " \n");
    if (!nodeIDStr)
      return EXIT_FAILURE;
  }
  uint8_t nodeID = 0;
  error = strToUint8(nodeIDStr, &nodeID);
  if (error)
    return EXIT_FAILURE;

  /* Handle READ_LAST */
  // Command: READ_LAST {NODE_ID}
  // JSON:
  // {
  //   "ok": true,
  //   "recordsRead": 1,
  //   "data": [{ "temperature": temp, "pressure": pres, "humidity": hum,
  //   "timeStamp": time }]
  // }

  uint32_t recordsRead = 0;
  if (strcmp(identifier, "READ_LAST") == 0) {
    struct weatherData wData = {0};
    error = readLast(nodeID, &wData);
    if (error) {
      recordsRead = 0;
      return EXIT_FAILURE;
    }
    recordsRead = 1;
    char buffer[128];
    snprintf(buffer, sizeof(buffer),
             "{\"ok\": true,\"recordsRead\": 1, \"data\": [{ \"temperature\": "
             "%" PRId32 ", \"pressure\": %" PRIu32 ", \"humidity\": %" PRIu32
             ", \"timeStamp\": %" PRIu32 " }]}",
             wData.temperature, wData.pressure, wData.humidity,
             wData.timeStamp);
    printf("%s\n", buffer);
    if (write(RXsock, buffer, strlen(buffer)) < 0)
      return EXIT_FAILURE;
  }
  /* Handle READ_ALL */
  // Command: READ_ALL {NODE_ID}
  // JSON:
  // {
  //   "ok": true,
  //   "recordsRead": 5,
  //   "data": [...]
  // }

  else if (strcmp(identifier, "READ_ALL") == 0) {
    struct weatherData *wData =
        malloc(sizeof(struct weatherData) * recordCountArr[nodeID]);
    if (!wData)
      return EXIT_FAILURE;
    error = readAll(nodeID, &recordsRead, wData);
    if (error) {
      free(wData);
      return EXIT_FAILURE;
    }
    if (recordsRead == 0)
      return EXIT_FAILURE;
    size_t bufferSize = sizeof(char) * recordsRead * 75 * 50;
    char *buffer = malloc(bufferSize);
    if (!buffer) {
      free(wData);
      return EXIT_FAILURE;
    }
    buffer[0] = '\0';
    uint32_t written = 0;
    written += snprintf(
        buffer, bufferSize,
        "{\"ok\": true,\"recordsRead\": %" PRIu32 ", \"data\": [", recordsRead);
    for (size_t i = 0; i < recordsRead - 1; ++i) {
      written += snprintf(buffer + written, bufferSize - written,
                          "{ \"temperature\": "
                          "%" PRId32 ", \"pressure\": %" PRIu32
                          ", \"humidity\": %" PRIu32 ", \"timeStamp\": %" PRIu32
                          " }, ",
                          wData[i].temperature, wData[i].pressure,
                          wData[i].humidity, wData[i].timeStamp);
    }
    snprintf(buffer + written, bufferSize - written,
             "{ \"temperature\": "
             "%" PRId32 ", \"pressure\": %" PRIu32 ", \"humidity\": %" PRIu32
             ", \"timeStamp\": %" PRIu32 " }]}",
             wData[recordsRead - 1].temperature,
             wData[recordsRead - 1].pressure, wData[recordsRead - 1].humidity,
             wData[recordsRead - 1].timeStamp);
    printf("%s\n", buffer);
    if (write(RXsock, buffer, strlen(buffer)) < 0) {
      free(wData);
      free(buffer);
      return EXIT_FAILURE;
    }
    free(buffer);
    free(wData);
  }
  /* Handle READ_N */
  // Command: READ_N {NODE_ID} {n}
  // JSON:
  // {
  //   ok: true,
  //   recordsRead: 5,
  //   data: [...]
  // }

  else if (strcmp(identifier, "READ_N") == 0) {
    struct weatherData *wData = malloc(sizeof(struct weatherData) * n);
    if (!wData)
      return EXIT_FAILURE;
    error = readN(nodeID, n, &recordsRead, wData);
    if (error) {
      free(wData);
      return EXIT_FAILURE;
    }
    if (recordsRead == 0)
      return EXIT_FAILURE;
    size_t bufferSize = sizeof(char) * recordsRead * 75 * 50;
    char *buffer = malloc(bufferSize);
    if (!buffer) {
      free(wData);
      return EXIT_FAILURE;
    }
    buffer[0] = '\0';
    uint32_t written = 0;
    written += snprintf(
        buffer, bufferSize,
        "{\"ok\": true,\"recordsRead\": %" PRIu32 ", \"data\": [", recordsRead);
    for (size_t i = 0; i < recordsRead - 1; ++i) {
      written += snprintf(buffer + written, bufferSize - written,
                          "{ \"temperature\": "
                          "%" PRId32 ", \"pressure\": %" PRIu32
                          ", \"humidity\": %" PRIu32 ", \"timeStamp\": %" PRIu32
                          " }, ",
                          wData[i].temperature, wData[i].pressure,
                          wData[i].humidity, wData[i].timeStamp);
    }
    snprintf(buffer + written, bufferSize - written,
             "{ \"temperature\": "
             "%" PRId32 ", \"pressure\": %" PRIu32 ", \"humidity\": %" PRIu32
             ", \"timeStamp\": %" PRIu32 " }]}",
             wData[recordsRead - 1].temperature,
             wData[recordsRead - 1].pressure, wData[recordsRead - 1].humidity,
             wData[recordsRead - 1].timeStamp);
    printf("%s\n", buffer);
    if (write(RXsock, buffer, strlen(buffer)) < 0) {
      free(buffer);
      free(wData);
      return EXIT_FAILURE;
    }
    free(wData);
    free(buffer);
  } else
    return EXIT_FAILURE;
  return EXIT_SUCCESS;
}

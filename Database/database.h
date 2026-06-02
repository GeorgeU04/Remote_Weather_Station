#ifndef DATABASE_H
#define DATABASE_H

#include <stddef.h>
#define _GNU_SOURCE
#include <netinet/in.h>
#include <stdint.h>

struct __attribute__((packed)) dataPacket {
  uint32_t timestamp;
  int32_t temperature;
  uint32_t pressure;
  uint32_t humidity;
  uint8_t seqNum;
  uint8_t nodeID;
};

struct weatherData {
  uint32_t timeStamp;
  int32_t temperature;
  uint32_t pressure;
  uint32_t humidity;
};

uint8_t initDatabase(uint8_t numOfSensors, uint64_t pageSize);
uint8_t insert(uint8_t nodeID, const struct weatherData *data);
uint8_t insertN(uint8_t nodeID, uint32_t n, const struct weatherData *data);
uint8_t readLast(uint8_t nodeID, struct weatherData *data);
uint8_t readAll(uint8_t nodeID, uint32_t *recordsRead,
                struct weatherData *data);
uint8_t readN(uint8_t nodeID, uint32_t n, uint32_t *recordsRead,
              struct weatherData *data);

uint8_t createServer(int32_t *sock, int32_t *RXSock,
                     struct sockaddr_in address);
uint8_t receiveCommand(int32_t RXsock, char *command, size_t commandSize);
uint8_t acceptClient(int32_t sock, int32_t *RXSock, struct sockaddr_in address);
uint8_t sendJSONData(int32_t RXsock, const char *data);

#endif // !DATABASE_H

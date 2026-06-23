#include "../Inc/database.h"
#include "../Inc/misc.h"
#include <fcntl.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

/*
 * Terminal Usage:
 * a.out [OPTIONS] [NUM_OF_SENSORS <= 12] [PAGE_SIZE] [Serial Port]
 * OPTIONS:
 * -B Page Size in Bytes
 * -K Page Size in Kilobytes
 * -M Page Size in Megabytes
 */

static uint8_t setUART(int32_t *serialPort, const char *port) {
  *serialPort = open(port, O_RDWR | O_NOCTTY);

  // Check for errors
  if (*serialPort < 0) {
    return EXIT_FAILURE;
  }
  struct termios portSettings = {0};
  tcgetattr(*serialPort, &portSettings);
  // Enable NON CANONICAL Mode for Serial Port Comm
  portSettings.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  // Turn OFF software based flow control (XON/XOFF).
  portSettings.c_iflag &= ~(IXON | IXOFF | IXANY);
  // Turn ON the receiver of the serial port (CREAD)
  portSettings.c_cflag |= CREAD | CLOCAL;
  // Turn OFF Hardware based flow control RTS/CTS
  portSettings.c_cflag &= ~CRTSCTS;
  // Disable parity bit
  portSettings.c_cflag &= ~PARENB;
  // Enable 1 stop bit
  portSettings.c_cflag &= ~CSTOPB;
  // Clear current character size mask
  portSettings.c_cflag &= ~CSIZE;
  // Set 8 bits for character size
  portSettings.c_cflag |= CS8;
  cfsetispeed(&portSettings, B115200);
  cfsetospeed(&portSettings, B115200);
  portSettings.c_cc[VMIN] = 20;  // Read at least 20 Bytes before returning
  portSettings.c_cc[VTIME] = 40; // Set timeout to 4 seconds, 40 * 100ms = 4s
  tcsetattr(*serialPort, TCSANOW, &portSettings);
  return EXIT_SUCCESS;
}

static uint8_t readUART(int32_t serialPort, struct weatherData *data,
                        uint8_t *nodeID) {
  struct dataPacket temp = {0};
  if (tcflush(serialPort, TCIOFLUSH) != 0)
    return EXIT_FAILURE;
  if ((read(serialPort, &temp, sizeof(struct dataPacket))) == -1)
    return EXIT_FAILURE;
  *nodeID = temp.nodeID;
  data->temperature = temp.temperature;
  data->humidity = temp.humidity;
  data->pressure = temp.pressure;
  data->timeStamp = temp.timestamp;
  return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {

  if (argc != 5) {
    fprintf(stderr,
            "Usage: %s [-B | -K | -M | -G] [NUM_OF_SENSORS <= 12] [PAGE_SIZE] "
            "[Serial Port]\n",
            argv[0]);
    return EXIT_FAILURE;
  }

  uint8_t error = 0;
  uint8_t val8;
  error = strToUint8(argv[2], &val8);
  if (error || val8 > 12) {
    fprintf(stderr, "[ERROR]: NUM_OF_SENSORS Must be a Valid Unsigned Integer "
                    "Less Than or Equal to 12\n");
    return EXIT_FAILURE;
  }
  if (val8 == 0) {
    fprintf(stderr, "[ERROR]: NUM_OF_SENSORS Must be Greater Than 0\n");
    return EXIT_FAILURE;
  }
  uint8_t numOfSensors = val8;

  uint64_t val64;
  error = strToUint64(argv[3], &val64);
  if (error) {
    fprintf(stderr,
            "[ERROR]: PAGE_SIZE Must be a Valid 64 bit Unsigned Integer\n");
    return EXIT_FAILURE;
  }
  if (val64 < sizeof(struct weatherData)) {
    fprintf(stderr, "[ERROR]: PAGE_SIZE Must be at least %zu Bytes\n",
            sizeof(struct weatherData));
    return EXIT_FAILURE;
  }
  uint64_t pageSize = val64;
  char *sizeFlag = argv[1];
  if (strcmp(sizeFlag, "-B") == 0) {
    ;
  } else if (strcmp(sizeFlag, "-K") == 0) {
    pageSize *= 1024;
  } else if (strcmp(sizeFlag, "-M") == 0) {
    pageSize *= 1024 * 1024;
  } else if (strcmp(sizeFlag, "-G") == 0) {
    pageSize *= 1024 * 1024 * 1024;
  } else {
    fprintf(stderr, "[ERROR]: Size Flag Must be -B, -K, -M, or ,-G\n");
    return EXIT_FAILURE;
  }

  const char *serialPortStr = argv[4];
  int32_t serialPort = 0;
  struct weatherData data = {0};
  uint8_t nodeID = 0;

  error = initDatabase(numOfSensors, pageSize);
  printf("Page Size: %luB\n", pageSize);
  if (error) {
    fprintf(stderr, "[ERROR]: Failed to Create Database\n");
    return EXIT_FAILURE;
  }

  error = setUART(&serialPort, serialPortStr);
  if (error) {
    fprintf(stderr, "[ERROR]: Failed to Initialize UART Port\n");
    return EXIT_FAILURE;
  }

  char buffer[32];
  int32_t sock = 0;
  int32_t RXSock = 0;
  in_port_t port = 9000;
  struct sockaddr_in address = {0};
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  address.sin_addr.s_addr = htonl(INADDR_ANY);

  error = createServer(&sock, &RXSock, address);
  if (error) {
    fprintf(stderr, "[ERROR]: Failed to Create Server Port\n");
    return EXIT_FAILURE;
  }
  while (1) {

    buffer[0] = '\0';
    /* Accept Client */
    if (RXSock == -1) {
      error = acceptClient(sock, &RXSock, address);

      if (error) {
        fprintf(stderr, "[ERROR]: Failed to Accept Client\n");
        continue;
      }
    }
    /* Receive Command */
    if (RXSock != -1) {
      error = receiveCommand(RXSock, buffer, sizeof(buffer));

      if (error) {
        close(RXSock);
        RXSock = -1;
        continue;
      }

      if (buffer[0] != '\0') {
        printf("Command: %s", buffer);
        // JSON Handling
        error = sendJSONData(RXSock, buffer);
        if (error) {
          fprintf(stderr, "[ERROR]: Failed to Send JSON Data\n");
          continue;
        }
        close(RXSock);
        RXSock = -1;
      }
    }
    /* Read UART */
    error = readUART(serialPort, &data, &nodeID);

    if (!error) {
      error = insert(nodeID, &data);

      if (!error) {
        printf("NodeID: %" PRIu8 "\nTimestamp: %" PRIu32
               "\nTemperature: %" PRId32 "\nHumidity: %" PRIu32
               "\nPressure: %" PRIu32 "\n",
               nodeID, data.timeStamp, data.temperature, data.humidity,
               data.pressure);
      }
    } else
      fprintf(stderr, "[ERROR]: Failed to Read UART\n");
  }
  return EXIT_SUCCESS;
}

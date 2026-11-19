#ifndef CLIENT_DEFINE_H
#define CLIENT_DEFINE_H

#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <ifaddrs.h>
#include <inttypes.h>
#include <stdarg.h>
#include <sys/time.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define CLIENT_DISCOVERY_PORT       37020
#define CLIENT_DISCOVERY_ATTEMPTS   10
#define CLIENT_DISCOVERY_INTERVAL  10
#define CLIENT_DISCOVERY_TIMEOUT    2
#define CLIENT_MAX_FRAME_SIZE       (64U * 1024U)
#define CLIENT_OUTPUT_SIZE          (48U * 1024U)
#define CLIENT_TEST_FILENAME        "test.txt"
#define CLIENT_TEST_LINE            "hello world 1 ... 2"
#define CLIENT_DISCOVERY_REQUEST    "GET_INFO_DISCOVER_V1"
#define CLIENT_DISCOVERY_RESPONSE   "GET_INFO_SERVER_V1"

#endif /* CLIENT_DEFINE_H */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#if defined WIN32
#include <winsock.h>
#else
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#define closesocket close
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <stdint.h>   // NECESSARIO PER uint32_t

// Porta di default
#define SERVER_PORT 56700

// Dimensioni pacchetti
#define REQUEST_SIZE   (1 + 64)
#define RESPONSE_SIZE  (4 + 1 + 4)

// Status codes
#define STATUS_OK 0
#define STATUS_CITY_NOT_FOUND 1
#define STATUS_INVALID_REQUEST 2

// Struttura richiesta client
typedef struct {
    char type;
    char city[64];
} weather_request_t;

// Struttura risposta server
typedef struct {
    unsigned int status;
    char type;
    float value;
} weather_response_t;

// Case insensitive
#ifndef WIN32
int strcasecmp(const char *s1, const char *s2);
#else
#define strcasecmp _stricmp
#endif

#endif

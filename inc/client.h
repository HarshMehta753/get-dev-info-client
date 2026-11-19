#ifndef CLIENT_H
#define CLIENT_H

#include "define.h"

/* Discover the server, establish TCP connection and receive the client ID. */
int client_init(void);

/* Process framed commands from the server until the connection closes. */
int client_process(void);

/* Close the active TCP connection. */
void client_cleanup(void);

#endif /* CLIENT_H */

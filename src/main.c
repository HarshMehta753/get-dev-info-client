#include "client.h"
#include <signal.h>

int main(void)
{
    signal(SIGPIPE, SIG_IGN);
    printf("Client starting. Discovering server...\n");

    if (client_init() != 0) {
        fprintf(stderr, "Error: client initialization failed.\n");
        return EXIT_FAILURE;
    }

    printf("Connected. Waiting for server commands.\n");
    int result = client_process();
    client_cleanup();
    return (result == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

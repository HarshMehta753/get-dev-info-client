#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "client.h"

#include <netdb.h>
#include <time.h>

static int g_tcp_socket = -1;
static int32_t g_client_id = -1;

static int send_all(int fd, const void *data, size_t length)
{
    const unsigned char *ptr = data;
    size_t sent = 0;
    while (sent < length) {
        ssize_t n = send(fd, ptr + sent, length - sent, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) return -1;
        sent += (size_t)n;
    }
    return 0;
}

static int recv_all(int fd, void *data, size_t length)
{
    unsigned char *ptr = data;
    size_t received = 0;
    while (received < length) {
        ssize_t n = recv(fd, ptr + received, length - received, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) return -1;
        received += (size_t)n;
    }
    return 0;
}

/* TCP protocol: a 32-bit network-byte-order payload length, followed by bytes. */
static int send_frame(int fd, const char *payload, size_t length)
{
    if (length == 0 || length > CLIENT_MAX_FRAME_SIZE) return -1;
    uint32_t network_length = htonl((uint32_t)length);
    if (send_all(fd, &network_length, sizeof(network_length)) != 0) return -1;
    return send_all(fd, payload, length);
}

static int recv_frame(int fd, char **payload_out)
{
    uint32_t network_length;
    *payload_out = NULL;
    if (recv_all(fd, &network_length, sizeof(network_length)) != 0) return -1;

    uint32_t length = ntohl(network_length);
    if (length == 0 || length > CLIENT_MAX_FRAME_SIZE) {
        errno = EMSGSIZE;
        return -1;
    }

    char *payload = calloc((size_t)length + 1U, 1U);
    if (payload == NULL) return -1;
    if (recv_all(fd, payload, length) != 0) {
        free(payload);
        return -1;
    }
    payload[length] = '\0';
    *payload_out = payload;
    return (int)length;
}

static int discover_server(struct sockaddr_in *server_address)
{
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        perror("UDP socket");
        return -1;
    }

    int enable = 1;
    if (setsockopt(udp_socket, SOL_SOCKET, SO_BROADCAST, &enable, sizeof(enable)) < 0) {
        perror("SO_BROADCAST");
        close(udp_socket);
        return -1;
    }

    struct timeval timeout = {
        .tv_sec = CLIENT_DISCOVERY_TIMEOUT,
        .tv_usec = 0
    };
    if (setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("SO_RCVTIMEO");
        close(udp_socket);
        return -1;
    }

    struct sockaddr_in broadcast_address;
    memset(&broadcast_address, 0, sizeof(broadcast_address));
    broadcast_address.sin_family = AF_INET;
    broadcast_address.sin_port = htons(CLIENT_DISCOVERY_PORT);
    broadcast_address.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    for (int attempt = 1; attempt <= CLIENT_DISCOVERY_ATTEMPTS; ++attempt) {
        struct timespec attempt_start;
        clock_gettime(CLOCK_MONOTONIC, &attempt_start);
        printf("Server discovery attempt %d/%d...\n", attempt, CLIENT_DISCOVERY_ATTEMPTS);
        if (sendto(udp_socket, CLIENT_DISCOVERY_REQUEST,
                   sizeof(CLIENT_DISCOVERY_REQUEST) - 1U, 0,
                   (struct sockaddr *)&broadcast_address,
                   sizeof(broadcast_address)) < 0) {
            perror("UDP broadcast");
        } else {
            char response[128];
            struct sockaddr_in peer;
            socklen_t peer_length = sizeof(peer);
            ssize_t received = recvfrom(udp_socket, response, sizeof(response) - 1U, 0,
                                        (struct sockaddr *)&peer, &peer_length);
            if (received > 0) {
                response[received] = '\0';
                unsigned int tcp_port = 0;
                char magic[64] = {0};
                if (sscanf(response, "%63s %u", magic, &tcp_port) == 2 &&
                    strcmp(magic, CLIENT_DISCOVERY_RESPONSE) == 0 &&
                    tcp_port > 0 && tcp_port <= 65535U) {
                    memset(server_address, 0, sizeof(*server_address));
                    server_address->sin_family = AF_INET;
                    server_address->sin_addr = peer.sin_addr;
                    server_address->sin_port = htons((uint16_t)tcp_port);
                    char ip[INET_ADDRSTRLEN] = {0};
                    inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof(ip));
                    printf("Discovered server at %s:%u\n", ip, tcp_port);
                    close(udp_socket);
                    return 0;
                }
                fprintf(stderr, "Ignoring invalid discovery response from server.\n");
            }
        }

        if (attempt < CLIENT_DISCOVERY_ATTEMPTS) {
            /* Keep a full 10 seconds between attempt start times, even if an
             * invalid response arrives before the UDP receive timeout. */
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            time_t elapsed_seconds = now.tv_sec - attempt_start.tv_sec;
            long elapsed_nanoseconds = now.tv_nsec - attempt_start.tv_nsec;
            if (elapsed_nanoseconds < 0) {
                --elapsed_seconds;
                elapsed_nanoseconds += 1000000000L;
            }
            long remaining_nanoseconds =
                (long)CLIENT_DISCOVERY_INTERVAL * 1000000000L -
                ((long)elapsed_seconds * 1000000000L + elapsed_nanoseconds);
            if (remaining_nanoseconds > 0) {
                struct timespec delay = {
                    .tv_sec = remaining_nanoseconds / 1000000000L,
                    .tv_nsec = remaining_nanoseconds % 1000000000L
                };
                while (nanosleep(&delay, &delay) < 0 && errno == EINTR) {}
            }
        }
    }

    close(udp_socket);
    fprintf(stderr, "No server discovered after %d attempts.\n", CLIENT_DISCOVERY_ATTEMPTS);
    return -1;
}

int client_init(void)
{
    struct sockaddr_in server_address;
    if (discover_server(&server_address) != 0) return -1;

    g_tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (g_tcp_socket < 0) {
        perror("TCP socket");
        return -1;
    }

    if (connect(g_tcp_socket, (struct sockaddr *)&server_address,
                sizeof(server_address)) < 0) {
        perror("TCP connect");
        client_cleanup();
        return -1;
    }

    /* Compatibility with the current server's initial native int client-ID message. */
    int32_t client_id = 0;
    if (recv_all(g_tcp_socket, &client_id, sizeof(client_id)) != 0) {
        fprintf(stderr, "Server disconnected before sending client ID.\n");
        client_cleanup();
        return -1;
    }
    g_client_id = client_id;
    printf("TCP connection established. Client ID: %" PRId32 "\n", g_client_id);
    return 0;
}

static int append_output(char *output, size_t capacity, size_t *used,
                         const char *format, ...)
{
    if (*used >= capacity) return -1;
    va_list args;
    va_start(args, format);
    int n = vsnprintf(output + *used, capacity - *used, format, args);
    va_end(args);
    if (n < 0 || (size_t)n >= capacity - *used) return -1;
    *used += (size_t)n;
    return 0;
}

static int command_host_info(char *output, size_t capacity)
{
    size_t used = 0;
    char hostname[256] = "unknown";
    if (gethostname(hostname, sizeof(hostname) - 1U) != 0) {
        snprintf(hostname, sizeof(hostname), "unavailable");
    }
    if (append_output(output, capacity, &used, "Hostname: %s\nIPv4 addresses:\n", hostname) != 0)
        return -1;

    struct ifaddrs *interfaces = NULL;
    if (getifaddrs(&interfaces) != 0) return -1;
    for (struct ifaddrs *it = interfaces; it != NULL; it = it->ifa_next) {
        if (it->ifa_addr == NULL || it->ifa_addr->sa_family != AF_INET ||
            (it->ifa_flags & IFF_UP) == 0 || (it->ifa_flags & IFF_LOOPBACK) != 0)
            continue;
        char ip[INET_ADDRSTRLEN] = {0};
        struct sockaddr_in *addr = (struct sockaddr_in *)it->ifa_addr;
        if (inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip)) != NULL) {
            if (append_output(output, capacity, &used, "%s: %s\n", it->ifa_name, ip) != 0) {
                freeifaddrs(interfaces);
                return -1;
            }
        }
    }
    freeifaddrs(interfaces);
    if (used == 0) snprintf(output, capacity, "Hostname: %s\nNo active non-loopback IPv4 address found.\n", hostname);
    return 0;
}

static int command_create_file(char *output, size_t capacity)
{
    FILE *file = fopen(CLIENT_TEST_FILENAME, "w");
    if (file == NULL) {
        snprintf(output, capacity, "Unable to create %s: %s", CLIENT_TEST_FILENAME, strerror(errno));
        return -1;
    }
    int failed = fprintf(file, "%s\n", CLIENT_TEST_LINE) < 0;
    if (fclose(file) != 0) failed = 1;
    if (failed) {
        snprintf(output, capacity, "Failed writing %s", CLIENT_TEST_FILENAME);
        return -1;
    }
    snprintf(output, capacity, "Created %s and wrote: %s", CLIENT_TEST_FILENAME, CLIENT_TEST_LINE);
    return 0;
}

static int command_delete_line(char *output, size_t capacity)
{
    FILE *input = fopen(CLIENT_TEST_FILENAME, "r");
    if (input == NULL) {
        if (errno == ENOENT) {
            snprintf(output, capacity, "%s does not exist; nothing to delete.", CLIENT_TEST_FILENAME);
            return 0;
        }
        snprintf(output, capacity, "Unable to open %s: %s", CLIENT_TEST_FILENAME, strerror(errno));
        return -1;
    }

    char temporary[] = ".test.txt.tmp.XXXXXX";
    int temp_fd = mkstemp(temporary);
    if (temp_fd < 0) {
        fclose(input);
        snprintf(output, capacity, "Unable to create temporary file: %s", strerror(errno));
        return -1;
    }
    FILE *out = fdopen(temp_fd, "w");
    if (out == NULL) {
        close(temp_fd);
        unlink(temporary);
        fclose(input);
        snprintf(output, capacity, "Unable to open temporary file: %s", strerror(errno));
        return -1;
    }

    char *line = NULL;
    size_t line_capacity = 0;
    ssize_t line_length;
    bool removed = false;
    int failed = 0;
    while ((line_length = getline(&line, &line_capacity, input)) != -1) {
        size_t compare_length = (size_t)line_length;
        while (compare_length > 0 && (line[compare_length - 1] == '\n' || line[compare_length - 1] == '\r'))
            --compare_length;
        if (!removed && compare_length == strlen(CLIENT_TEST_LINE) &&
            memcmp(line, CLIENT_TEST_LINE, compare_length) == 0) {
            removed = true;
            continue;
        }
        if (fwrite(line, 1U, (size_t)line_length, out) != (size_t)line_length) {
            failed = 1;
            break;
        }
    }
    if (ferror(input)) failed = 1;
    free(line);
    fclose(input);
    if (fclose(out) != 0) failed = 1;

    if (failed || rename(temporary, CLIENT_TEST_FILENAME) != 0) {
        int saved_errno = errno;
        unlink(temporary);
        snprintf(output, capacity, "Failed updating %s: %s", CLIENT_TEST_FILENAME, strerror(saved_errno));
        return -1;
    }
    snprintf(output, capacity, removed ? "Deleted the matching line from %s." :
             "Matching line was not present in %s; file left unchanged.", CLIENT_TEST_FILENAME);
    return 0;
}

static int command_delete_file(char *output, size_t capacity)
{
    if (unlink(CLIENT_TEST_FILENAME) == 0) {
        snprintf(output, capacity, "Deleted %s.", CLIENT_TEST_FILENAME);
        return 0;
    }
    if (errno == ENOENT) {
        snprintf(output, capacity, "%s does not exist; nothing to delete.", CLIENT_TEST_FILENAME);
        return 0;
    }
    snprintf(output, capacity, "Unable to delete %s: %s", CLIENT_TEST_FILENAME, strerror(errno));
    return -1;
}

static int command_list_downloads(char *output, size_t capacity)
{
    const char *home = getenv("HOME");
    if (home == NULL || home[0] == '\0') {
        snprintf(output, capacity, "HOME environment variable is not set.");
        return -1;
    }
    char path[1024];
    int n = snprintf(path, sizeof(path), "%s/Downloads", home);
    if (n < 0 || (size_t)n >= sizeof(path)) {
        snprintf(output, capacity, "Downloads path is too long.");
        return -1;
    }
    DIR *directory = opendir(path);
    if (directory == NULL) {
        snprintf(output, capacity, "Unable to open %s: %s", path, strerror(errno));
        return -1;
    }
    size_t used = 0;
    if (append_output(output, capacity, &used, "Contents of %s:\n", path) != 0) {
        closedir(directory);
        return -1;
    }
    struct dirent *entry;
    while ((entry = readdir(directory)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        if (append_output(output, capacity, &used, "%s\n", entry->d_name) != 0) {
            closedir(directory);
            snprintf(output, capacity, "Output exceeded the supported size limit.");
            return -1;
        }
    }
    closedir(directory);
    if (used == 0) snprintf(output, capacity, "No entries found in %s.", path);
    return 0;
}

static int execute_command(const char *command, char *output, size_t capacity)
{
    memset(output, 0, capacity);
    if (strcmp(command, "HOST_INFO") == 0) return command_host_info(output, capacity);
    if (strcmp(command, "CREATE_FILE") == 0) return command_create_file(output, capacity);
    if (strcmp(command, "DELETE_LINE") == 0) return command_delete_line(output, capacity);
    if (strcmp(command, "DELETE_FILE") == 0) return command_delete_file(output, capacity);
    if (strcmp(command, "LIST_DOWNLOADS") == 0) return command_list_downloads(output, capacity);
    snprintf(output, capacity, "Unsupported command: %s", command);
    return -1;
}

int client_process(void)
{
    for (;;) {
        char *command = NULL;
        int frame_length = recv_frame(g_tcp_socket, &command);
        if (frame_length < 0) {
            if (errno != 0) perror("Receiving command");
            fprintf(stderr, "Server disconnected or sent an invalid frame.\n");
            return -1;
        }

        /* Commands are an allowlisted token, not arbitrary shell input. */
        char output[CLIENT_OUTPUT_SIZE] = {0};
        int status = execute_command(command, output, sizeof(output));
        free(command);

        size_t response_capacity = sizeof(output) + 96U;
        char *response = calloc(response_capacity, 1U);
        if (response == NULL) {
            fprintf(stderr, "Unable to allocate command response.\n");
            return -1;
        }
        int response_length = snprintf(response, response_capacity,
                                       "RESULT %" PRId32 " %s\n%s",
                                       g_client_id, status == 0 ? "SUCCESS" : "FAILURE", output);
        if (response_length < 0 || (size_t)response_length >= response_capacity) {
            free(response);
            fprintf(stderr, "Command response exceeded the supported size.\n");
            return -1;
        }
        int send_result = send_frame(g_tcp_socket, response, (size_t)response_length);
        free(response);
        if (send_result != 0) {
            perror("Sending command result");
            return -1;
        }
    }
}

void client_cleanup(void)
{
    if (g_tcp_socket >= 0) {
        close(g_tcp_socket);
        g_tcp_socket = -1;
    }
}

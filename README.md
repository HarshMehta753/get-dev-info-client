# get_info client

Standalone C client. It discovers the server over IPv4 UDP broadcast, then uses TCP for command/result communication. No server IP or port is passed on the command line.

## Build

```sh
cmake -S . -B build
cmake --build build
```

Run:

```sh
./build/client
```

## UDP discovery protocol (server support required)

The client broadcasts the following exact ASCII payload to UDP port `37020`:

```text
GET_INFO_DISCOVER_V1
```

The server must reply to the client's UDP source address/port with:

```text
GET_INFO_SERVER_V1 <tcp_port>
```

Example:

```text
GET_INFO_SERVER_V1 8080
```

The client takes the server IPv4 address from the UDP response's source address and the TCP port from the response payload. It makes up to 10 attempts, immediately on startup and then at 10-second intervals. Each attempt waits up to 2 seconds for a response; the remaining 8 seconds complete the interval. Total discovery duration is approximately 90 seconds if no server responds.

## TCP protocol contract for the future server implementation

The client retains the current initial client-ID handshake: immediately after TCP accept, the server sends one native-endian 32-bit integer client ID. This is followed by framed messages:

- Every frame is a 4-byte unsigned payload length in network byte order, followed by exactly that many payload bytes.
- Server-to-client command payloads are one of the exact command tokens below.
- Client-to-server result payloads use `RESULT <client_id> SUCCESS\n<output>` or `RESULT <client_id> FAILURE\n<error/output>`.

Commands supported by this client:

| Command token | Action |
|---|---|
| `HOST_INFO` | Return hostname and active non-loopback IPv4 addresses. |
| `CREATE_FILE` | Create/overwrite `test.txt` in the client's current working directory with `hello world 1 ... 2` and a newline. |
| `DELETE_LINE` | Remove the first exact matching `hello world 1 ... 2` line from `test.txt`. Missing file/line is treated as a successful no-op. |
| `DELETE_FILE` | Delete `test.txt`; a missing file is treated as a successful no-op. |
| `LIST_DOWNLOADS` | Return entries in `$HOME/Downloads`. |

Only these allowlisted command tokens are accepted. The client deliberately does not execute arbitrary shell commands received from the network. Directory listing and host information are implemented using POSIX APIs rather than shelling out.

## Network notes

- UDP broadcast discovery normally works only within the same broadcast domain/VLAN. Routers generally do not forward broadcast packets.
- Allow inbound UDP port `37020` on the server and the advertised TCP port through the server firewall.
- The server should reply to the source address/port of the discovery datagram (not to a hard-coded client port).
- Multiple network interfaces, Wi-Fi client isolation, VLANs, VPNs, and host firewalls can prevent discovery.
- This repository contains only the client. Until the server implements the UDP response and framed TCP protocol described above, discovery/command exchange will not interoperate with the old server implementation.

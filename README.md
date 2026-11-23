🚀 get_info-client

A tiny C client that finds its server automatically, gets its identity, and follows instructions. No need to introduce them manually!

This project implements dynamic server discovery and server-client communication using C socket programming.

The client discovers an available server within the same Local Area Network (LAN) using UDP broadcast. Once the server is discovered, it establishes a TCP connection, receives a unique Client ID, and waits for commands from the server.

Based on the received command, the client performs the requested task and sends the result back to the server.

In simple words: Find the server → Get an ID → Receive commands → Do the work → Report back.

📋 Prerequisites

Before getting started, make sure your system has the following:

C Compiler: GCC or Clang.
CMake: Version 3.10 or later (or the minimum version specified in CMakeLists.txt).
Build Tools: Make or another supported CMake build tool.
Operating System: Linux or another POSIX-compatible environment.
Network: Client and server should be connected to the same LAN for UDP broadcast discovery.

Check whether the required tools are installed:

sh
gcc --version
cmake --version
make --version


If these commands return version information, you're good to go!

🛠️ Build Instructions

First, clone the repository:

sh
git clone <repository-url>


Navigate to the project directory:

sh
cd get_info-client


Create the build directory and configure the project:

sh
cmake -S . -B build


Compile the client:

sh
cmake --build build


If everything goes well, congratulations! Your client is ready. 🎉

▶️ Run the Client

Start the client using:

sh
./build/client


That's it! No need to provide a server IP address or port.

The client will automatically search for a server on the local network.

🔍 How Server Discovery Works

The client uses IPv4 UDP broadcast to discover an available server.

Here is how the discovery process works:

Client starts and broadcasts a discovery packet.
Server listens for incoming discovery packets.
Server receives the discovery request and responds with its TCP port.
Client extracts the server's IP address and TCP port from the response.
Client establishes a TCP connection with the discovered server.
UDP Discovery Protocol

The client broadcasts the following exact ASCII message to UDP port 37020:

Client → Server

text
GET_INFO_DISCOVER_V1


The server must respond to the client's UDP source address and port with:

Server → Client

text
GET_INFO_SERVER_V1 <tcp_port>


Example:

text
GET_INFO_SERVER_V1 8080


The client automatically identifies the server's IPv4 address from the UDP response's source address and uses the TCP port provided in the response.

Discovery Retry Mechanism

Don't worry if the server doesn't respond immediately. The client is patient (well, up to a point!).

Maximum discovery attempts: 10
First attempt: Immediately after startup.
Retry interval: 10 seconds.
Response timeout per attempt: 2 seconds.
Total discovery duration: Approximately 90 seconds if no server responds.

If the server is unavailable, the client continues retrying according to this mechanism.

🔌 TCP Communication Protocol

Once the server is discovered, the client establishes a TCP connection.

The communication follows this sequence:

Server accepts the TCP connection.
Server sends a unique Client ID as a native-endian 32-bit integer.
Server sends commands to the client using framed TCP messages.
Client executes the requested task.
Client sends the execution result back to the server.
Message Framing

Each TCP message follows this format:

Field	Description
Payload Length	4-byte unsigned integer in network byte order
Payload	Actual message content

The payload length specifies the number of bytes in the message.

Client Result Format

After executing a command, the client sends the result in one of the following formats:

Successful execution:

text
RESULT <client_id> SUCCESS
<output>


Failed execution:

text
RESULT <client_id> FAILURE
<error/output>

⚙️ Supported Commands

The server can send the following commands to the client:

Command	Description
HOST_INFO	Retrieves the hostname and active non-loopback IPv4 addresses.
CREATE_FILE	Creates or overwrites test.txt in the client's current working directory with hello world 1 ... 2 followed by a newline.
DELETE_LINE	Removes the first exact matching hello world 1 ... 2 line from test.txt.
DELETE_FILE	Deletes test.txt from the current working directory.
LIST_DOWNLOADS	Lists entries inside the user's $HOME/Downloads directory.
Important Notes
CREATE_FILE overwrites the existing test.txt file.
DELETE_LINE treats a missing file or matching line as a successful no-op.
DELETE_FILE also treats a missing file as a successful no-op.
Only predefined and allowlisted command tokens are accepted.
Arbitrary shell commands received over the network are not executed.
Host information and directory listing use POSIX APIs rather than shell commands.
🌐 Network Requirements

For successful communication between the client and server, keep the following points in mind:

Both devices should generally be connected to the same LAN/broadcast domain.
The server must listen for UDP discovery requests on port 37020.
The server's advertised TCP port must be allowed through its firewall.
The server must respond to the source address and source port of the client's UDP discovery packet.
UDP broadcast packets are generally not forwarded by routers.
Wi-Fi client isolation, VLAN separation, VPNs, multiple network interfaces, and firewall restrictions may interfere with discovery.

Troubleshooting tip: If the client cannot find the server, first check your network connection and firewall settings. Sometimes the network is the problem, not your code. 🙂

🖥️ Server Compatibility

This repository contains only the client-side implementation.

The server must implement the UDP discovery response and framed TCP communication protocol documented above.

The server must also send the initial Client ID handshake immediately after accepting the TCP connection.

Until the server supports this protocol, the client will not interoperate with the older server implementation.

📌 Project Workflow
text
       CLIENT STARTS
             |
             v
    UDP BROADCAST DISCOVERY
             |
             v
     SERVER RESPONDS
             |
             v
    EXTRACT SERVER IP/PORT
             |
             v
       TCP CONNECTION
             |
             v
      RECEIVE CLIENT ID
             |
             v
       WAIT FOR COMMAND
             |
             v
       EXECUTE COMMAND
             |
             v
      SEND RESULT TO SERVER
             |
             v
           DONE!

🎯 Project Objective

The primary objective of this project is to implement automatic server discovery and client identification within a Local Area Network using C socket programming.

It eliminates the need for manually configuring server IP addresses and enables centralized command execution and result collection through TCP communication.

Find it. Connect to it. Identify it. Get the job done! 🚀
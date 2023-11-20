# ISA Project - TFTP Client + Server

### Author: Lukáš Zavadil (xzavad20)
### Created: 20.11. 2023
### Project description:
TFTP client and server implementation in C based on RFC 1350, RFC 2347, RFC 2348 and RFC 2349. Development and testing was done on reference Nix environment. The project is structured into folders that wrap certain parts of it, **bin** for executable binaries, **include** for header files, **obj** for object files and **src** for source code files. The project is compiled using Makefile's **make** command, which generates two executable binaries, tftp-client and tftp-server, inside **bin** folder. There is also **manual.pdf**, which contains a detailed description of the project and its implementation.
### Usage:
- **Server:** ```./bin/tftp-server -p 6969 root_dir```
- **Client Read:** ```./bin/tftp-client -p 6969 -h 127.0.0.1 -t client_dir/client_file.txt -f server_file.txt```
- **Client Write:** ```./bin/tftp-client -p 6969 -h 127.0.0.1 -t server_file.txt < client_file.txt```
### Limitations:
The timeout option was not implemented, server will accept it and retrun OACK packet, but it will not affect the program.
### List of files:
- **tftp-server.c**
- **tftp-server.h**
- **tftp-client.c**
- **tftp-client.h**
- **utils.c**
- **utils.h**
- **Makefile**
- **README.md**
- **manual.pdf**
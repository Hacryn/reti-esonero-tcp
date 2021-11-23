/*
 ============================================================================
 Name        : main.c
 Author      : Enrico Ciciriello - Simone Summo
 Version     : 0.1
 Copyright   : MIT
 Description : Online calculator (client)
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "protocol.h"

#if defined WIN32
#include <winsock.h>
#else
#define closesocket close
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#define BUFFSIZE (64)

int initializeWSA();
void errormsg(const char* msg);
int userinteraction(int mySocket);
void extractop(cpack *pack, const char* s);

int main(int argc, char *argv[]) {
    struct sockaddr_in sad;
    unsigned long addr;
    int mySocket;
    int returnv;
    int port;

    if (initializeWSA() == -1) {
    	return -1;
    }

    // Arguments extraction
    if(argc < 2) {
        addr = inet_addr("127.0.0.1");
        port = PORT;
    } else if (argc < 3) {
        addr = inet_addr(argv[1]);

        if(addr == INADDR_NONE) {
            errormsg("Address invalid, default address selected");
            addr = inet_addr("127.0.0.1");
        }
    } else if (argc == 3) {
        addr = inet_addr(argv[1]);
        port = atoi(argv[2]);

        if(addr == INADDR_NONE) {
            errormsg("Address invalid, default address selected");
            addr = inet_addr("127.0.0.1");
        }

        if(port == 0) {
            errormsg("Port invalid, default port selected");
            port = PORT;
        }
    } else {
        errormsg("Too many arguments, retry with 2 argument (an address and a port)");
        return -1;
    }

    // Socket creation
	mySocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (mySocket < 0) {
		errormsg("Socket creation failed, please retry");
		return -1;
	}

    // Address configuration
    sad.sin_family = AF_INET;
    sad.sin_addr.s_addr = addr;
    sad.sin_port = htons(port);

    // Server connection
    if (connect(mySocket, (struct sockaddr_in*) &sad, sizeof(sad)) < 0) {
		errormsg("Connection failed, check if server is online and available");
		closesocket(mySocket);
		return -1;
	}

    // Interaction with the user
    returnv = userinteraction(mySocket);
    closesocket(mySocket);
    return returnv;
}

int userinteraction(int mySocket) {
    int active = 1;
    int errorc = 0;
    char s[BUFFSIZE];
    cpack sendp;
    spack recvp;

    while (active) {
        // If the client failed to send/receive for 3 or more times the app close itself
        if(errorc >= 3) {
            errormsg("Communication failed with the server, closing the app");
            return -1;
        }
        
        // Read operation from the user
        printf("Insert operation: ");
        scanf("%64[^\n]", s);
        fflush(stdin);
        fflush(stdout);
        extractop(&sendp, s);
        // Mark the closing of the connection 
        if (sendp.operation == '=') {
            active = 0;
        }

        // Send the package to the server
        if (sendp.operation != 'i') {
            sendp.operand1 = htonl(sendp.operand1);
            sendp.operand2 = htonl(sendp.operand2);

            if (send(mySocket, &sendp, sizeof(sendp), 0) < 0) {
                errormsg("Failed to send operation to the server, please check and retry");
                errorc++;
            } else {
                // Receive the result from the server
                if (recv(mySocket, &recvp, sizeof(recvp), 0) < 0) {
                    errormsg("Failed to receive operation from the server, please check and retry");
                    errorc++;
                } else {
                    errorc = 0;
                    recvp.error = ntohl(recvp.error);
                    recvp.result = ntohl(recvp.result);

                    // If error, inform the user
                    if(recvp.error != 0) {
                        switch (recvp.error) {
                            case 1:
                                errormsg(ERROR1);
                                break;
                            case 2:
                                errormsg(ERROR2);
                                break;
                            default:
                                errormsg("Unknown error");
                        }
                    // Otherwise print the result
                    } else {
                        printf("Result: %d\n", recvp.result);
                    }
                }
            }
        } else {
            errormsg("Operation format invalid, check the format is correct");
        }
    }

    return 0;
}

void extractop(cpack *pack, const char* s) {
	int op1, op2;
	char op;

	if (s[0] == '=') {
		pack->operation = s[0];
		return;
	}

	if (strlen(s) >= 5) {
        if (sscanf(s, "%c %d %d", &op, &op1, &op2) < 3) {
        	pack->operation = 'i';
        } else if (op != '+' && op != '-' && op != '=' && op != 'x' && op != '/') {
        	pack->operation = 'i';
        } else {
        	pack->operation = op;
        	pack->operand1 = op1;
        	pack->operand2 = op2;
        }
    } else {
    	pack->operation = 'i';
    }
} 

int initializeWSA() {
#if defined WIN32
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0) {
		errormsg("StartUp failed");
		return -1;
	}
#endif
	return 0;
}

void errormsg(const char* msg) {
    printf("Error: %s\n", msg);
}

/*
 ============================================================================
 Name        : main.c
 Author      : Enrico Ciciriello -  Simone Summo
 Version     :
 Copyright   : MIT
 Description : Online calculator (server)
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "calculator.h"
#include "protocol.h"

#if defined WIN32
#include <winsock.h>
#else
#define closesocket close
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#define QLEN 5
#define BUFFLENGTH 64

void errorhandler(const char *errorMessage);
void clearwinsock();
int handleClient(const int serverSocket, const struct sockaddr_in *sad, const int clientSocket, const struct sockaddr_in *cad);

int main(int argc, char *argv[]){
	int port;
	if(argc > 1){
		port = atoi(argv[1]);

		if(port < 1 || port > 65535){
			printf("bad port number %s \n", argv[1]);
			return -1;
		}
	} else {
		port = PORT;
	}

	// Initialize WSA
	#if defined WIN32
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if(iResult != 0){
		errorhandler("Error at WSAStartup(). \n");
		return -1;
	}
	#endif

	// Socket creation
	int mySocket;
	mySocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(mySocket < 0){
		errorhandler("Socket creation failed.");
		clearwinsock();
		return -1;
	}

	// Address configuration
	struct sockaddr_in sad;
	sad.sin_family = AF_INET;
	sad.sin_addr.s_addr = inet_addr("127.0.0.1");
	sad.sin_port = htons(port);
	if(bind(mySocket, (struct sockaddr_in *) &sad, sizeof(sad)) < 0){
		errorhandler("bind() failed. \n");
		closesocket(mySocket);
		clearwinsock();
		return -1;
	}

	// Socket configuration for listening
	if(listen(mySocket, QLEN) < 0){
		errorhandler("listen() failed. \n");
		closesocket(mySocket);
		clearwinsock();
		return -1;
	}

	// Accepting a client
	struct sockaddr_in cad;
	int clientSocket;
	int clientLen;
	printf("Waiting for a client to connect... \n");
	while(1){
		clientLen = sizeof(cad);
		if((clientSocket = accept(mySocket, (struct sockaddr *) &cad, &clientLen)) < 0){
			errorhandler("accept() failed. \n");
			// Closing connection
			closesocket(clientSocket);
			clearwinsock();
			return 0;
		}
		if(handleClient(mySocket, &sad, clientSocket, &cad) < 0){
			errorhandler("error in client handling");
			closesocket(clientSocket);
		}
	}
}

// Print error message
void errorhandler(const char *errorMessage){
	printf("%s\n", errorMessage);
}

// Closing WSA
void clearwinsock(){
	#if defined WIN32
	WSACleanup();
	#endif
}

int handleClient(const int serverSocket, const struct sockaddr_in *sad, const int clientSocket, const struct sockaddr_in *cad) {
	printf("Connection established with %s:%d \n", inet_ntoa(cad->sin_addr), ntohs(cad->sin_port));

	cpack rcv;
	spack snd;

	int result;

	while(1){
		// Take data from client
		if(recv(clientSocket, &rcv, sizeof(rcv), 0) < 0){
			errorhandler("Failed to receive operation and operators");
			return -1;
		} else {
			// Conversion data from network to host long
			rcv.operand1 = ntohl(rcv.operand1);
			rcv.operand2 = ntohl(rcv.operand2);

			switch(rcv.operation){
				// Addition
				case '+':
					result = add(rcv.operand1, rcv.operand2);
					snd.result = result;
					snd.error = 0;
					break;
				// Subtraction
				case '-':
					result = sub(rcv.operand1, rcv.operand2);
					snd.result = result;
					snd.error = 0;
					break;
				// Multiplication
				case 'x':
					result = mult(rcv.operand1, rcv.operand2);
					snd.result = result;
					snd.error = 0;
					break;
				// Division
				case '/':
					if(rcv.operand2 == 0){
						snd.result = 0;
						snd.error = 2;
					} else {
						result = division(rcv.operand1, rcv.operand2);
						snd.result = result;
						snd.error = 0;
					}
					break;
				// Closing connection with client
				case '=':
					printf("Connection closed with %s:%d \n", inet_ntoa(cad->sin_addr), ntohs(cad->sin_port));
					closesocket(clientSocket);
					return 0;
					break;
				// Unrecognized symbol
				default:
					snd.error = 1;
					snd.result = 0;
			}
		}
		
		// Conversion data from host to network long
		snd.result = htonl(snd.result);
		snd.error = htonl(snd.error);

		// Sending results to client
		if(send(clientSocket, &snd, sizeof(snd), 0) < 0) {
			errorhandler("Failed to send result to the client");
			return -1;
		}
	}
}

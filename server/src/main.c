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

#define QLEN 6 		// size of request queue
#define BUFFLENGTH 64

void errorhandler(char *errorMessage);
void clearwinsock();
int handleClient(const int serverSocket, const struct sockaddr_in *sad, const int clientSocket, const struct sockaddr_in *cad);

int main(int argc, char *argv[]){
	int port;
	if(argc > 1){
		port = atoi(argv[1]);
	} else {
		port = PORT;
		if(port < 0){
			printf("bad port number %s \n", argv[1]);
			return 0;
		}
	}

	// INIZIALIZZAZIONE PROGRAMMA WINDOWS
	#if defined WIN32
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if(iResult != 0){
		errorhandler("Error at WSAStartup(). \n");
		return -1;
	}
	#endif

	// CREAZIONE SOCKET
	int mySocket;
	mySocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(mySocket < 0){
		errorhandler("Socket creation failed.");
		clearwinsock();
		return -1;
	}

	// ASSEGNAZIONE INDIRIZZO
	struct sockaddr_in sad;
	sad.sin_family = AF_INET;
	sad.sin_addr.s_addr = inet_addr("127.0.0.1");
	sad.sin_port = htons(PORT);
	if(bind(mySocket, (struct sockaddr_in *) &sad, sizeof(sad)) < 0){
		errorhandler("bind() failed. \n");
		closesocket(mySocket);
		clearwinsock();
		return -1;
	}

	// SETTAGGIO DELLA SOCKET ALL'ASCOLTO
	if(listen(mySocket, QLEN) < 0){
		errorhandler("listen() failed. \n");
		closesocket(mySocket);
		clearwinsock();
		return -1;
	}

	// ACCETTARE UNA NUOVA CONNESSIONE
	struct sockaddr_in cad;
	int clientSocket;
	int clientLen;
	printf("Waiting for a client to connect...");
	while(1){
		clientLen = sizeof(cad);
		if((clientSocket = accept(mySocket, (struct sockaddr *) &cad, &clientLen)) < 0){
			errorhandler("accept() failed. \n");
			// CHIUSURA DELLA CONNESSIONE
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

// STAMPA MESSAGGI DI ERRORE
void errorhandler(char *errorMessage){
	printf("%s", errorMessage);
}

// CHIUSURA DI WINSOCK32
void clearwinsock(){
	#if defined WIN32
	WSACleanup();
	#endif
}

int handleClient(const int serverSocket, const struct sockaddr_in *sad, const int clientSocket, const struct sockaddr_in *cad){
	printf("Connection established with %s", inet_ntoa(cad->sin_addr));

	cpack rcv;
	spack snd;

	int result;

	while(1){
		if(recv(clientSocket, &rcv, sizeof(rcv), 0) < 0){
			errorhandler("Failed to receive operation and operators, please check and retry.");
			return -1;
		} else {
			rcv.operation = ntohl(rcv.operation);
			rcv.operand1 = ntohl(rcv.operand1);
			rcv.operand2 = ntohl(rcv.operand2);

			switch(rcv.operation){
				case '+':
					result = add(rcv.operand1, rcv.operand2);
					snd.result = result;
					snd.error = 0;
					break;
				case '-':
					result = sub(rcv.operand1, rcv.operand2);
					snd.result = result;
					snd.error = 0;
					break;
				case '*':
					result = mult(rcv.operand1, rcv.operand2);
					snd.result = result;
					snd.error = 0;
					break;
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
				case '=':
					closesocket(clientSocket);
					return 0;
					break;
				default:
					snd.error = 1;
					snd.result = 0;
					break;
			}
		}
	}

	snd.result = htonl(snd.result);
	snd.error = htonl(snd.error);

	if(send(serverSocket, &snd, sizeof(snd), 0) < 0){
		errorhandler("Failed to send result to the client, please check and retry");
		return -1;
	}
}

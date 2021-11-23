/*
 * protocol.h
 *
 *  Created on: 23 nov 2021
 *      Author: Enrico Ciciriello
 */

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

// list of values
#define PORT (60000)

// list of the errors
#define ERROR1 "Operation invalid"
#define ERROR2 "Division for 0 (/0)"

/*
 * The client send this package to the server
 * operation: 	tell which operation the server must do
 * operand*: 	the two operands of the operation
 */
typedef struct client_package {
	char operation;
	int operand1;
	int operand2;
} cpack;

/*
 * The server respond to the client with this package
 * result: 	the result of the operation
 * error: 	if 0 then none error occurred in the operation
 * 			else an error occurred and the error code point to an error message
 */

typedef struct server_package {
	int result;
	int error;
} spack;


#endif /* PROTOCOL_H_ */

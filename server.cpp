//Name: Sunny Gurung
//NetID: sg2094

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h> //for inet_addr()
#include <netinet/in.h>
#include <iostream>
#include <stdlib.h> //for strtol
#include <errno.h> //for error messages
#include <time.h> //for random function
#include <unistd.h> //for close()

int main(int argc, char* argv[]) {
	char message[10]; //incoming message
	int random_port;	//random port
	char r_port[6];	//random port as char array
	FILE *file; 		//file to store received message
		
	
	//checking to see the right usage
	if (argc != 2) {
		printf("%s \n", "Usage: ./server <port number>");
		
		exit(1);
	}
	
	char *no_string; //for use in conversion of string to long int
	errno = 0;
	long converted= strtol(argv[1], &no_string, 10);	//converting to long int
	int port_number;		//for holding the port number
	
	//if the port number entered is valid
	if(errno != 0 || *no_string != '\0' || (converted < 1024 || converted > 65535)){
	std::cout << "Error in port number!!! Please enter between 1024 and 65535. \n";
	exit(1);
	}
	else {
	port_number = converted;
	}
	
	//performing socket()
	int socket_number;
	socket_number = socket(AF_INET, SOCK_STREAM, 0); 	// IPV4, TCP sock stream, TCP transfer protocol
	
	if(socket_number ==-1) 
	{ std::cout << "Socket Error: " << strerror(errno) << " \n";
	  exit(1);
	}
	
	
	struct sockaddr_in server_address;	//structure for server address
	
	server_address.sin_family = AF_INET;	//IPv4
	server_address.sin_port = htons(port_number);	//port number
	memset(server_address.sin_zero, '\0', sizeof server_address.sin_zero);	//setting sin_zero to 0
	server_address.sin_addr.s_addr = htonl(INADDR_ANY); 	//for binding to all local interfaces
	
	//performing bind()
	if(bind(socket_number, (struct sockaddr*) &server_address, sizeof(server_address))==-1){
	std::cout << "Binding Error: " << strerror(errno) << " \n";
	exit(1);
	}
	
	//performing listen() setting backlog to 5
	if(listen(socket_number, 5)==-1){
	std::cout << "Listening Error: " << strerror(errno) << " \n";
	exit(1);
	} 
	
	//getting ready for accepting
	struct sockaddr_storage client_address;		//for holding client address
	socklen_t addr_size = sizeof client_address;		//size of client address
	int accept_socket;
	
	//accepting
	
	accept_socket = accept(socket_number, (struct sockaddr*)&client_address, &addr_size); //accepting socket number
	if(accept_socket==-1){
	std::cout << "Accepting Error: " << strerror(errno) << " \n";
	exit(1);
	}
	
	//receiving the message
	if(recv(accept_socket, message, 512, 0)==-1){
	std::cout << "Recieving Error: " << strerror(errno) << " \n";
	exit(1);
	}else{
	std::cout << "Handshake detected.";
	}
	
	//generating random socket number
	srand(time(0));	//randomizing the random
	random_port = (rand() % 64512 + 1024);	//seleting random
	sprintf(r_port, "%d", random_port);	//changing random port to char array for sending
	
	
	//sending random socket number generated
	int sent_length = send(accept_socket, r_port, sizeof r_port, 0);
	if(sent_length != sizeof(r_port))
		std::cout << "Error :: Message Length: " << sizeof(r_port) << " Transmitted Length: " << sent_length << std::endl;
		
	//outputting the random port
	std::cout << "Selected the random port " << random_port << "." << std::endl;
	
	
	close(socket_number);
	close(accept_socket);
	
	//stage 2
	
	//getting new socket
	socket_number = socket(AF_INET, SOCK_DGRAM, 0); 	// IPV4, TCP sock stream, TCP transfer protocol
	
	if(socket_number ==-1) 
	{ std::cout << "Socket Error: " << strerror(errno) << " \n";
	  exit(1);
	}
	
	server_address.sin_port = htons(random_port);	//selecting new port number

	//performing another bind()
	if(bind(socket_number, (struct sockaddr*) &server_address, sizeof(server_address))==-1){
	std::cout << "Binding Error: " << strerror(errno) << " \n";
	exit(1);
	}
	
	
	memset(&message, '\0', sizeof message); //clearing any message
	
	
	//receive from client and send back uppercase message to client
	
	char bold_message[5];		//for sending uppercase messages back to client
	bool end_of_file = false;	//detecting end of file
		
	file = fopen("dataReceived.txt", "w");	//creating or re-writing existing file
	int len_eof;		//stores the last integer when end of file
	
	//loop until end of file is detected
	while(!end_of_file) {
	
	//receiving four bytes of data
	if((recvfrom(socket_number, message, 4, 0, (struct sockaddr *)&client_address, &addr_size)) == -1){
	std::cout << "Error receiving!" << std::endl;
	}
	
	//iterating through the whole message
	for(int i=0; i<4; i++){
	bold_message[i]= toupper(message[i]);	//converting to uppercase
		if(message[i] == '\0')	{	//checking ending of message
		end_of_file = true;	//setting end_of_file to be true
		len_eof = i;
		}
	}
	
	if(end_of_file){	//checking to see end of file
	fwrite(message, 1, len_eof, file); //only write until end of file
	}
	else{
	fwrite(message, 1, 4, file);	//write the whole four characters
	}
	
	//sending the converted uppercase message
	sendto(socket_number, bold_message, 4, 0, (struct sockaddr *)&client_address, addr_size);
	}
	
	fclose(file);	//closing the file
	close(socket_number);	//closing the socket
	
	return 0; 
}	

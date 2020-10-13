/****************************************************
 Roshan KC (rk942)
 Data Communication
 Professor - Maxwell Young
 Programming assigment 1: Client.cpp to demonstrate 
 TCP and UDP connection
 Due Date - September 15, 2020
****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>
#include <fstream>
#include <ctime>
#include <time.h>
#include <sys/select.h>
#include <sys/unistd.h>
#include <sys/fcntl.h>
#include <errno.h>
#include "packet.h"
#include "packet.cpp"

using namespace std;

int main(int argc, char *argv[])
{
	//output the usage when length is not 4
	if (argc != 4) {
		printf("%s \n", "Usage: ./client localhost <port number> <filename>");
		exit(1);
	}

	struct hostent *s;
	s = gethostbyname(argv[1]);
	int portno;
	struct sockaddr_in server;
	socklen_t slen = sizeof(server);
	portno = atoi(argv[2]);

	bzero((char *)&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(portno);

	bcopy((char *)s->h_addr,
		  (char *)&server.sin_addr.s_addr,
		  s->h_length);

	// udp method to transfer file

	int udpSocket = 0;
	if ((udpSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		std::cout << "Socket Error: " << strerror(errno) << " \n";
		exit(1);
	}

	server.sin_port = htons(portno); //new randomport to communicate

	FILE *file; //holding the file to read
	file = fopen(argv[3], "r"); // filename

	if (file == NULL)
	{
		std::cout << "Error opening file!" << std::endl;
		exit(1);
	}


	char Datatosend[8][30]; //array of 7 char*
	

	int type = 0;
	int seqnum = 0;
	int length;
	int ackseq = -1;
	int window = 7;
	int sendbase = 0;
	int timeout;
	int nextseqnum = 1;
	int outPacket = 0;
	int loc;

	ofstream seqnumfile;
	seqnumfile.open("clientseqnum.log");

	ofstream acknumfile;
	acknumfile.open("clientack.log");

	struct timeval tval;
	tval.tv_sec = 2;
	tval.tv_usec = 0;

	int sockettime = setsockopt(udpSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tval, sizeof(tval));
	if (sockettime < 0)
	{

		perror("SetSockopt error\n");
	}

	printf("\n------------------------\n");
	while (1)
	{
		memset(sentData, 0, sizeof(sentData));
		memset(chartosend, 0, sizeof(chartosend));
		memset(receivedData, 0, sizeof(receivedData));

		// if outpacket less than 7 (form 0 - 6)
		if (!readFile.eof() && outPacket < 7)
		{
			type = 1;

			readFile.read(chartosend, sizeof(chartosend));

			length = strlen(chartosend); //length of the character to be sent

			packet filepacket(type, seqnum, length, chartosend);

			filepacket.serialize((char *)sentData);

			sendto(udpSocket, sentData, sizeof(sentData), 0, (struct sockaddr *)&server, sizeof(server));
			filepacket.printContents();

			seqnumfile << seqnum << "\n"; //write in a file

			//next seq number and outpacket
			nextseqnum = (seqnum + 1) % 8;
			outPacket = outPacket + 1;

			printf("SB: %d\n", sendbase);
			printf("NS: %d\n", nextseqnum);
			printf("Number of outstanding packets: %d\n", outPacket);
			printf("--------------------------\n");
			// Increment
			seqnum = (seqnum + 1) % 8;
		}
		
	}

	readFile.close();
	acknumfile.close();
	seqnumfile.close();

	close(udpSocket);

	return 0;
}

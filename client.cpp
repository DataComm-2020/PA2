/****************************************************
 Roshan KC (rk942)
 Data Communication
 Professor - Maxwell Young
 Programming assigment 1: Client.cpp to demonstrate 
 TCP and UDP connection
 Due Date - September 15, 2020
****************************************************/

#include <stdlib.h>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <fstream>
#include <arpa/inet.h>
#include "packet.h"
#include <math.h>
#include <time.h>
#include <unistd.h>
#include "packet.cpp"

using namespace std;

int main(int argc, char *argv[])
{
	//output the usage when length is not 4
	if (argc != 4)
	{
		printf("%s \n", "Usage: ./client localhost <port number> <filename>");
		//exit(1);
	}

	struct hostent *s;
	s = gethostbyname(argv[1]);
	int portNo;
	struct sockaddr_in server;
	socklen_t slen = sizeof(server);
	portNo = atoi(argv[2]);

	bzero((char *)&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(portNo);

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

	server.sin_port = htons(portNo); //new randomport to communicate

	FILE *file;					//holding the file to read
	file = fopen(argv[3], "r"); // filename
	FILE *seqNumLog;			//holding the log file descriptor
	FILE *ackLog;
	fd_set fileDescriptors;
	FD_ZERO(&fileDescriptors);
	FD_SET(udpSocket, &fileDescriptors);

	if (file == NULL)
	{
		cout << "Error opening file " << argv[3] << " !" << endl;
		exit(1);
	}

	int type = 0;
	int seqnum = 0;
	char data[30];
	int length = sizeof(data);
	char dataPacket[7][37];
	int nextSeqNum = 1;
	int base = 1;
	int N = 7;
	packet pack;
	seqNumLog = fopen("clientseqnum.log", "w"); //creating or re-writing existing file
	ackLog = fopen("clientack.log", "w");

	if (seqNumLog == NULL)
	{
		std::cout << "Error opening file clientseqnum.log!" << std::endl;
		exit(1);
	}

	if (ackLog == NULL)
	{
		std::cout << "Error opening file clientack.log!" << std::endl;
		exit(1);
	}

	struct timeval timer;
	timer.tv_sec = 0;
	timer.tv_usec = 20;

	char strSeqNumLog[4];
	char receivedData[42];

	bool endOfFilereached = false;

	while (!endOfFilereached)
	{
		while (nextSeqNum < base + N)
		{
			memset(&data, 0, sizeof(data));
			memset(&dataPacket[seqnum], 0, sizeof(dataPacket[seqnum]));
			int readCount = fread(data, 1, length, file);
			cout << readCount << endl;
			if (readCount == length)
			{
				pack = packet(type, seqnum, length, data);
				pack.serialize(dataPacket[seqnum]);
				cout << 1 << endl;
				//send the message to server
				sendto(udpSocket, dataPacket[seqnum], sizeof(dataPacket[seqnum]), 0, (struct sockaddr *)&server, sizeof(server));
				cout << "-----------------------------------------" << endl;
				pack.printContents();

				memset(&strSeqNumLog, 0, sizeof(strSeqNumLog));

				sprintf(strSeqNumLog, "%d\n", pack.getSeqNum());
				fwrite(strSeqNumLog, 1, sizeof(strSeqNumLog), seqNumLog);
				seqnum = ++seqnum % 8;
				nextSeqNum++;
			}
			else
			{

				data[readCount] = '\0';
				pack = packet(type, seqnum, readCount, data);
				pack.serialize(dataPacket[seqnum]);

				//send the message to server
				sendto(udpSocket, dataPacket[seqnum], sizeof(dataPacket[seqnum]), 0, (struct sockaddr *)&server, sizeof(server));
				pack.printContents();

				sprintf(strSeqNumLog, "%d\n", pack.getSeqNum());
				fwrite(strSeqNumLog, 1, sizeof(strSeqNumLog), seqNumLog);
				seqnum = ++seqnum % 8;
				nextSeqNum++;
				endOfFilereached = true;

				pack = packet(3, seqnum, 0, NULL);
				pack.serialize(dataPacket[seqnum]);

				//send the message to server
				sendto(udpSocket, dataPacket[seqnum], sizeof(dataPacket[seqnum]), 0, (struct sockaddr *)&server, sizeof(server));
				pack.printContents();

				sprintf(strSeqNumLog, "%d\n", pack.getSeqNum());
				fwrite(strSeqNumLog, 1, sizeof(strSeqNumLog), seqNumLog);
				seqnum = seqnum++ % 8;
				nextSeqNum++;
				endOfFilereached = true;
				break;
			}
		}

		int setTimer = select(udpSocket, &fileDescriptors, NULL, NULL, &timer);
		if (setTimer > 0)
		{
			recvfrom(udpSocket, receivedData, sizeof(receivedData), 0, (struct sockaddr *)&server, (socklen_t *)sizeof(server));
			pack.deserialize(receivedData);
			pack.printContents();
			sprintf(strSeqNumLog, "%d\n", pack.getSeqNum());
			fwrite(strSeqNumLog, 1, sizeof(strSeqNumLog), ackLog);
			base = pack.getSeqNum() + 1;
		}
		else if (setTimer < 0)
		{
			std::cout << "Socket Error: " << strerror(errno) << " \n";
			exit(1);
		}
		else
		{
			cout << "\n**********Timeout Occured*******************" << endl;
			cout << "Resending all packets....." << endl;
			for (int i = 0; i < N && dataPacket[i] != 0; ++i)
			{

				sendto(udpSocket, dataPacket[i], sizeof(dataPacket[i]), 0, (struct sockaddr *)&server, sizeof(server));
				packet pack2(0, 0, 0, 0);
				pack2.deserialize(dataPacket[i]);

				pack.printContents();
				sprintf(strSeqNumLog, "%d\n", pack.getSeqNum());
				fwrite(strSeqNumLog, 1, sizeof(strSeqNumLog), seqNumLog);
			}
		}
	}
	fclose(seqNumLog);
	fclose(ackLog);
	close(udpSocket);

	return 0;
}

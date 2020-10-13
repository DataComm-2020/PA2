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

	//server address
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

	//file descriptor
	FILE *file;					//holding the file to read
	file = fopen(argv[3], "r"); // filename
	FILE *seqNumLog;			//holding the log file descriptor
	FILE *ackLog;

	//setting up for select
	fd_set fileDescriptors;
	FD_ZERO(&fileDescriptors);
	FD_SET(udpSocket, &fileDescriptors);

	seqNumLog = fopen("clientseqnum.log", "w"); //creating or re-writing existing file
	ackLog = fopen("clientack.log", "w");

	//file opened correctly
	if (file == NULL)
	{
		cout << "Error opening file " << argv[3] << " !" << endl;
		exit(1);
	}

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

	int type = 0;
	int seqnum = -1;
	char data[7][31];

	for (int j = 0; j < 7; j++)
	{
		memset(data[j], '\0', sizeof(data[j]));
	}

	int length = 30;
	char dataPacket[42];
	memset(dataPacket, '\0', sizeof(dataPacket));

	int nextSeqNum = 1;
	int base = 1;
	int N = 7;
	packet pack = packet(0, 0, 0, NULL);

	struct timeval timer;

	char strSeqNumLog[4];
	char receivedData[42];

	bool endOfFilereached = false;

	while (!endOfFilereached)
	{
		while (nextSeqNum < base + N)
		{
			seqnum++;
			seqnum = seqnum % 8;

			memset(&dataPacket, 0, sizeof(dataPacket));
			int readCount = fread(data[seqnum], 1, length, file);

			if (readCount == length)
			{
				pack = packet(type, seqnum, length, data[seqnum]);
				pack.serialize(dataPacket);

				//send the message to server
				sendto(udpSocket, dataPacket, sizeof(dataPacket), 0, (struct sockaddr *)&server, sizeof(server));
				cout << "-----------------------------------------" << endl;
				pack.printContents();
				memset(&strSeqNumLog, 0, sizeof(strSeqNumLog));

				sprintf(strSeqNumLog, "%d\n", pack.getSeqNum());
				fwrite(strSeqNumLog, 1, sizeof(strSeqNumLog), seqNumLog);
				nextSeqNum++;
			}
			else
			{

				data[seqnum][readCount] = '\0';
				pack = packet(type, seqnum, readCount, data[seqnum]);
				pack.serialize(dataPacket);

				//send the message to server
				sendto(udpSocket, dataPacket, sizeof(dataPacket), 0, (struct sockaddr *)&server, sizeof(server));
				pack.printContents();

				sprintf(strSeqNumLog, "%d\n", pack.getSeqNum());
				fwrite(strSeqNumLog, 1, sizeof(strSeqNumLog), seqNumLog);

				nextSeqNum++;
				endOfFilereached = true;

				seqnum++;
				seqnum = seqnum % 8;
				pack = packet(3, seqnum, 0, NULL);
				pack.serialize(dataPacket);

				//send the message to server
				sendto(udpSocket, dataPacket, sizeof(dataPacket), 0, (struct sockaddr *)&server, sizeof(server));
				pack.printContents();

				sprintf(strSeqNumLog, "%d\n", pack.getSeqNum());
				fwrite(strSeqNumLog, 1, sizeof(strSeqNumLog), seqNumLog);

				nextSeqNum++;
				endOfFilereached = true;
				break;
			}
		}
		timer.tv_sec = 2;
		timer.tv_usec = 0;
		int setTimer = select(udpSocket, &fileDescriptors, NULL, NULL, &timer);
		int expectedAck < 0;
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

			for (int i = (seqnum - 6) % 8; i < seqnum + 1; ++i)
			{
				if (data[i] != nullptr)
				{
					pack = packet(type, i, length, data[i]);

					pack.serialize(dataPacket);

					//send the message to server
					sendto(udpSocket, dataPacket, sizeof(dataPacket), 0, (struct sockaddr *)&server, sizeof(server));
					cout << "-----------------------------------------" << endl;
					pack.printContents();

					memset(&strSeqNumLog, 0, sizeof(strSeqNumLog));

					sprintf(strSeqNumLog, "%d\n", pack.getSeqNum());
					fwrite(strSeqNumLog, 1, sizeof(strSeqNumLog), seqNumLog);
				}
			}
		}
	}
	fclose(seqNumLog);
	fclose(ackLog);
	close(udpSocket);
}

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
	char data[8][31];

	for (int j = 0; j < 8; j++)
	{
		memset(data[j], '\0', sizeof(data[j]));
	}

	int length = 30;
	char dataPacket[42];
	memset(dataPacket, '\0', sizeof(dataPacket));

	int nextSeqNum = 1;
	int base = 0;
	int N = 7;
	packet pack = packet(0, 0, 0, NULL);

	struct timeval timer;
	//setting up for select
	fd_set fileDescriptors;
	FD_ZERO(&fileDescriptors);
	FD_SET(udpSocket, &fileDescriptors);

	char strSeqNumLog[4];
	char receivedData[42];

	bool endOfFileReached = false;
	int outstandingPacket = 0;
	cout << "-----------------------------------------" << endl;
	while (!endOfFileReached)
	{
		while (outstandingPacket < 7)
		{

			seqnum++;
					seqnum = seqnum % 8;
			type = 1;

			memset(&dataPacket, 0, sizeof(dataPacket));
			int readCount = fread(data[seqnum], 1, length, file);
			if (!endOfFileReached)
			{
				if (readCount == length)
				{
					pack = packet(type, seqnum, length, data[seqnum]);
					pack.serialize(dataPacket);

					//send the message to server
					sendto(udpSocket, dataPacket, sizeof(dataPacket), 0, (struct sockaddr *)&server, sizeof(server));

					outstandingPacket = (outstandingPacket + 1) % 8;
					nextSeqNum = (seqnum + 1) % 8;

					cout << outstandingPacket << endl;
					pack.printContents();
					cout << "SB: " << base << endl;
					cout << "NS: " << nextSeqNum << endl;
					cout << "Number of Outstanding Packets: " << outstandingPacket << endl;
					cout << "-----------------------------------------" << endl;
					memset(&strSeqNumLog, 0, sizeof(strSeqNumLog));

					sprintf(strSeqNumLog, "%d\n", pack.getSeqNum());
					fwrite(strSeqNumLog, 1, sizeof(strSeqNumLog), seqNumLog);

					
				}
				else
				{
					if (readCount == 0 && endOfFileReached == false)
					{
						endOfFileReached = true;
					}
					else
					{
						data[seqnum][readCount] = '\0';
						pack = packet(type, seqnum, readCount, data[seqnum]);
						pack.serialize(dataPacket);

						//send the message to server
						sendto(udpSocket, dataPacket, sizeof(dataPacket), 0, (struct sockaddr *)&server, sizeof(server));
						pack.printContents();
						cout << "SB: " << base << endl;
						cout << "NS: " << nextSeqNum << endl;
						cout << "Number of Outstanding Packets: " << outstandingPacket + 1 << endl;
						memset(&strSeqNumLog, 0, sizeof(strSeqNumLog));

						sprintf(strSeqNumLog, "%d\n", pack.getSeqNum());
						fwrite(strSeqNumLog, 1, sizeof(strSeqNumLog), seqNumLog);
						outstandingPacket++;
						nextSeqNum++;
						endOfFileReached = true;
					}
				}
			}
			else
			{
				//send end of file message to the server

				seqnum++;
				seqnum = seqnum % 8;

				pack = packet(3, seqnum, 0, NULL);
				pack.serialize(dataPacket);

				sendto(udpSocket, dataPacket, sizeof(dataPacket), 0, (struct sockaddr *)&server, sizeof(server));
				pack.printContents();
				cout << "SB: " << base << endl;
				cout << "NS: " << nextSeqNum << endl;
				cout << "Number of Outstanding Packets: " << outstandingPacket + 1 << endl;

				sprintf(strSeqNumLog, "%d\n", pack.getSeqNum());
				fwrite(strSeqNumLog, 1, sizeof(strSeqNumLog), seqNumLog);
				outstandingPacket = (outstandingPacket+1)%8;
				nextSeqNum++;
				nextSeqNum = nextSeqNum % 8;
				break;
			}
		}
		if (!endOfFileReached)
		{
			timer.tv_sec = 2;
			timer.tv_usec = 0;
			int m = udpSocket;
			int setTimer = select(m + 1, &fileDescriptors, NULL, NULL, &timer);

			if (setTimer > 0)
			{
				int c = recvfrom(udpSocket, receivedData, sizeof(receivedData), 0, (struct sockaddr *)&server, &slen);
				if (c < 0)
				{

					perror("Receiving error\n");
				}
				packet pack(0, 0, 0, NULL);

				pack.deserialize((char *)receivedData);
				pack.printContents();
				int ackseq = pack.getSeqNum();
				if (ackseq >= base || (ackseq < (base + N) % 8 && ackseq >= 0))
				{
					base = (pack.getSeqNum() + 1) % 8;

					if (nextSeqNum > pack.getSeqNum())
					{
						outstandingPacket = (nextSeqNum - base) % 8;
					}
					else
					{
						outstandingPacket = (N - (abs(nextSeqNum - ackseq))) % 8;
					}
				}
				printf("SB: %d\n", base);
				printf("NS: %d\n", nextSeqNum);
				printf("Number of outstanding packets: %d\n", outstandingPacket);
				printf("------------------------------------------------------\n");
				sprintf(strSeqNumLog, "%d\n", pack.getSeqNum());
				fwrite(strSeqNumLog, 1, sizeof(strSeqNumLog), ackLog);
				
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
	}
	fclose(seqNumLog);
	fclose(ackLog);
	close(udpSocket);
}

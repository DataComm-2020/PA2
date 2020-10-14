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

#include "packet.h"
#include "packet.cpp"

using namespace std;

int main(int argc, char *argv[])
{

  int portno;
  struct sockaddr_in client, server;
  socklen_t clen = sizeof(client);
  socklen_t slen = sizeof(server);

  // create socket first
  portno = atoi(argv[1]);
  server.sin_family = AF_INET;         // server byte order
  server.sin_addr.s_addr = INADDR_ANY; // current host ip address
  server.sin_port = htons(portno);     //port into network byte order

  // udp method to send file

  /* Stage 2: Transcation using UDP sockes below */
  int udp_socket = 0;
  if ((udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) // socket(int domain, int type, int protocol)
    cout << "Error in socket creation.\n";
  server.sin_port = htons(portno);

  if ((bind(udp_socket, (struct sockaddr *)&server, sizeof(server))) < 0)
  {
    perror("binding");
    return -1;
  }

  ofstream outfile;
  outfile.open(argv[2]); // opens a file

  ofstream arrivalfile;
  arrivalfile.open("arrival.log");

  char payload[37];
  char data[30];
  char ack[42];

  int type = 1;
  int seqnum = 0;
  int acktype = 0;
  int expectseq = 0;

  packet filepacket(type, seqnum, sizeof(data), data);


  while (1){

    memset(payload, 0, sizeof(payload));
    memset(data, 0, sizeof(data));
    memset(ack, 0, sizeof(ack));

    if (recvfrom(udp_socket, payload, sizeof(payload), 0, (struct sockaddr *)&client, &clen) == -1)
    {
      perror("Receiving\n");
    }

    filepacket.deserialize((char *)payload);
    type = filepacket.getType();
    seqnum = filepacket.getSeqNum();

    arrivalfile << seqnum << "\n";
    printf("\n------------------------------\n");
    filepacket.printContents();

    printf("\nExpecting Rn: %d\n", expectseq);
    printf("sn: %d\n", seqnum);

    
    if (expectseq == seqnum)
    {
      
      if (filepacket.getType() == 3)
      {

        acktype = 2;
        packet ackpacket(acktype, seqnum, 0, 0);
        ackpacket.serialize((char *)ack);
        if (sendto(udp_socket, ack, sizeof(ack), 0, (struct sockaddr *)&client, sizeof(client)) == -1)
        {
          perror("Sending Error");
        }

        ackpacket.printContents();

        printf("----------------------------\n");
        break;
      }
      // EOT packet
     
        outfile << filepacket.getData();

        packet ackpacket(acktype, seqnum, 0, 0);
        ackpacket.serialize((char *)ack);
        if (sendto(udp_socket, ack, sizeof(ack), 0, (struct sockaddr *)&client, sizeof(client)) == -1)
       {
         printf("Sending Error\n");
        }

        ackpacket.printContents();


        expectseq = (expectseq + 1) % 8;
      
    }

    else
    {

      packet ackpacket(acktype, expectseq, 0, 0);
      ackpacket.serialize((char *)ack);
      sendto(udp_socket, ack, sizeof(ack), 0, (struct sockaddr *)&client, sizeof(client));
      ackpacket.printContents();
      printf("---------------------------\n");
    
    }
  }

  outfile.close(); //close the file

  close(udp_socket); // close the socket
}
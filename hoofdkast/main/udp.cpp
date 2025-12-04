/*
 * Udp.cpp
 *
 *  Created on: May 16, 2021
 *      Author: dig
 */

 #include <sys/socket.h>
 #include <arpa/inet.h>
 #include <netinet/in.h>
 #include "include/udp.h"
 
 int udpSend(char *mssg, int port) {
     int sockfd;
     int opt;
     struct sockaddr_in servaddr;
 
     if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
         return -1;
     }
     memset(&servaddr, 0, sizeof(servaddr));
     servaddr.sin_family = AF_INET;
     servaddr.sin_port = htons(port);
     servaddr.sin_addr.s_addr =INADDR_NONE;// INADDR_ANY;
 
     setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST ,&opt, sizeof(opt));
 
     sendto(sockfd, mssg, strlen(mssg), 0, (const struct sockaddr*) &servaddr, sizeof(servaddr));
     close(sockfd);
     return 0;
 }
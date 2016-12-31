/***************
* Samir Yuja sjy14b
* Due 11/29/2016
* PROJECT 2
* COP 5570
***************/

/******
* UTIL.CPP: contains wrapper functions.
* Perform error checking internally.
* Modified from wrapsock.c provided by the UNP book
*******/


#ifndef _UTIL_H
#define _UTIL_H

#define LISTENQ 15
#define MAXBUFLEN 256

#include <string>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>

  using namespace std;

/****************************
**INIT SOCKADDR STRUCT
*****************************/
void initSockAddrIn(sockaddr_in &servaddr, int portnum, in_addr_t ip = 0);

//returns the IP in dotted decimal from the struct
string getIP(struct sockaddr_in &serv_addr);
//returns the portnumber
int getPort(struct sockaddr_in &serv_addr);
/****************************
**ESTABLISHING CONNECTION
*****************************/
int Socket(int family, int type, int protocol);
void Bind(int fd, const struct sockaddr *sa, socklen_t salen);
void Listen(int fd, int backlog = LISTENQ);
int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr);
void Connect(int fd, const struct sockaddr *sa, socklen_t salen);

/****************************
**SENDING AND RECEIVING
*****************************/
void Write(int fd, const char* buf, size_t count);
int Read(int fd, char* buf, size_t count);

/****************************
**MULTIPLEXING
*****************************/
int Select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
       struct timeval *timeout);

/****************************
**GETTING INFORMATION
*****************************/
//HACK: remove if not needed
void Getpeername(int fd, struct sockaddr *sa, socklen_t *salenptr);

//HACK: remove if not needed
void Getsockname(int fd, struct sockaddr *sa, socklen_t *salenptr);

void Setsockopt(int fd, int level,
   int optname, const void *optval, socklen_t optlen);

#endif

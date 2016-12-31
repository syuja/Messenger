/***************
* Samir Yuja sjy14b
* Due 11/29/2016
* PROJECT 2
* COP 5570
***************/


#include "Util.h"

/****************************
**INIT SOCKADDR STRUCT
*****************************/
void initSockAddrIn(sockaddr_in &servaddr, int portnum,in_addr_t ip){
  //memset((void *) &servaddr, 0, sizeof(servaddr));
  if(ip == 0) ip = INADDR_ANY;
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(portnum);
  servaddr.sin_addr.s_addr = ip;
}

string getIP(struct sockaddr_in &serv_addr){
  char ip_add[INET_ADDRSTRLEN];

  inet_ntop(AF_INET, &serv_addr.sin_addr, ip_add, INET_ADDRSTRLEN);
  return ip_add;
}


int getPort(struct sockaddr_in &serv_addr){
  return ntohs(serv_addr.sin_port);
}

/****************************
**ESTABLISHING CONNECTION
*****************************/

int Socket(int family, int type, int protocol){
	int	n;
	if ((n = socket(family, type, protocol)) < 0){
    perror("Util Socket error: ");
    exit(1);
  }
	return(n);
}

void Bind(int fd, const struct sockaddr *sa, socklen_t salen){
	if (bind(fd, sa, salen) < 0){
		perror("Util Bind error: ");
    exit(1);
  }
}

//Listen wrapper
void Listen(int fd, int backlog){
	if (listen(fd, LISTENQ) < 0){
		perror("Util Listen error: ");
    exit(1);
  }
}

//imported from Unix Programming wrapsock.c
int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr){
	int	n;

  again:
  	if ((n = accept(fd, sa, salenptr)) < 0) {
  #ifdef	EPROTO
  		if (errno == EPROTO || errno == ECONNABORTED)
  #else
  		if (errno == ECONNABORTED)
  #endif
  			goto again;
  		else{
  			perror("Util Accept error: ");
        exit(1);
      }
	}
	return(n);
}


void Connect(int fd, const struct sockaddr *sa, socklen_t salen){
	if (connect(fd, sa, salen) < 0){
		perror("Util Connect error: ");
    exit(1);
  }
}

/****************************
**SENDING AND RECEIVING
*****************************/


void Write(int fd, const char* buf, size_t count){
  if(write(fd, buf, count) != count){
      perror("Util Write error: ");
  }
}

int Read(int fd, char* buf, size_t count){
  int n;
  if((n=read(fd, buf, count))< 0 ){
    perror("Util Read error: ");
  }
  return n;
}

/****************************
**MULTIPLEXING
*****************************/

int Select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
       struct timeval *timeout){
	int	n;

	if ((n = select(nfds, readfds, writefds, exceptfds, timeout)) < 0){
		perror("Util Select error: ");
    exit(1);
  }
	return(n);		/* can return 0 on timeout */
}

/****************************
**GETTING INFORMATION
*****************************/
//HACK: remove if not needed
void Getpeername(int fd, struct sockaddr *sa, socklen_t *salenptr){
	if (getpeername(fd, sa, salenptr) < 0){
		perror("Util Getpeername error: ");
    exit(1);
  }
}


void Getsockname(int fd, struct sockaddr *sa, socklen_t *salenptr){
	if (getsockname(fd, sa, salenptr) < 0){
		perror("Util Getsockname error: ");
    exit(1);
  }
}


void Setsockopt(int fd, int level,
   int optname, const void *optval, socklen_t optlen){
	if (setsockopt(fd, level, optname, optval, optlen) < 0)
		perror("Util Setsockopt error: ");
}

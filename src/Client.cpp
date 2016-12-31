/***************
* Samir Yuja sjy14b
* Due 11/29/2016
* PROJECT 2
* COP 5570
***************/

#include "Client.h"
#include "Util.h"


#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <string> //for stoi
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>


/****************************
**CONSTRUCTOR/DESTRUCTOR
*****************************/

//checks that ConfigFile exists, calls read
Client::Client(string ConfigFile){
  //check that it exists
  struct stat buf;

  //never registered so empty
  uname = "";
  pwd = "";
  connectfd = -1;

  if(lstat(ConfigFile.c_str(), &buf) < 0){
    fprintf(stderr, "Server Construct Error: %s '%s'\n", strerror(errno),
     ConfigFile.c_str());
    exit(1);
  }

  this->ConfigFile = ConfigFile;

  //read config file
  readConfigFile(this->ConfigFile);
}


Client::~Client(){
  //close all open sockets
  clientExit();
}


/****************************
**LISTEN FOR FRIENDS
*****************************/

//creates listening socket
void Client:: startClient(){
  struct sockaddr_in cli_addr;
  //get any portnumber
  initSockAddrIn(cli_addr, PORTNUM);

  //create socket
  listenfd = Socket(AF_INET, SOCK_STREAM, 0);

  //reuseaddr
  socklen_t yes = 1;
  Setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

  //bind it
  Bind(listenfd, (struct sockaddr *)&cli_addr, sizeof(cli_addr));

  //listen
  struct sockaddr_in sin;
  Listen(listenfd);

  //save IP and port
  cli_loc_port = listenfd;
  getSocketInfo(cli_loc_port, cli_loc_ip);
  printClientListening();
}

/****************************
**CONNECT TO SERVER AND LOGIN
*****************************/

//connects to the server
void Client:: connectClient(){
  struct sockaddr_in cli_addr;
  //get any portnumber

  cli_addr.sin_family = AF_INET;
  cli_addr.sin_port = htons(servport);
  cli_addr.sin_addr.s_addr = inet_addr(servhost.c_str());

  //socket
  this-> connectfd = Socket(AF_INET, SOCK_STREAM,0);

  //connect
  Connect(this->connectfd, (struct sockaddr*)&cli_addr, sizeof(cli_addr));

  //save client's ip and port
  cli_port = connectfd;
  //gets the client's ip and port
  cli_port = getPort(cli_addr);
  cli_ip = getIP(cli_addr);
  printClientServerConn();
}

//attempts to login, if successful sends location info
void Client::userLogin(){
  string name_pwd = "";
  string location = "";
  int n = 0;
  char buf[MAXBUFLEN];

  //repeatedly ask login info
  while(1){
    //concatenate
    name_pwd = uname + " " + pwd;

    //send pwd and uname to server
    Write(connectfd, name_pwd.c_str(), MAXBUFLEN-1);

    memset(&buf, 0, sizeof(MAXBUFLEN));
    //read response 200 or 500
    n = Read(connectfd, buf, MAXBUFLEN-1);
    buf[n] = '\0';
    string str_buf(buf);
    if(str_buf == "500"){
      //ask for password again
      getLogin();
      continue;
    }
    else if(str_buf == "200"){
      //success: send location
      location = cli_loc_ip + " " + to_string(cli_loc_port);
      Write(connectfd, location.c_str(), MAXBUFLEN-1);
      break;
    }
    else{
      //server send a either 200 or 500?
    }
  }
}

/************************************
**CONNECT TO SERVER AND LOGIN HELPERS
*************************************/

//ask for username and password
void Client::getLogin(){
  cout << "Server Login" << endl;
  cout << "User name: ";
  getline(cin, this->uname);
  cout << "Password: ";
  getline(cin, this->pwd);
  cout << endl;

}

//get ip and port from socket
void Client::getSocketInfo(int &fd, string &ip){
  struct sockaddr_in sin;
  memset(&sin, 0, sizeof(sin));
  socklen_t len = sizeof(sin);

  Getsockname(fd, (struct sockaddr*) &sin, &len);
  fd = getPort(sin);
  ip = getIP(sin);
}


/****************************
**SELECT (server, chat, stdin)
*****************************/

//handles all incoming messages and connections
void Client::clientSelect(){
  //two sets of commands 1) before login  and another 2) after login
  isLoggedIn = false;
  string user_in = "";
  char buf[MAXBUFLEN];
  int n = 0;

  maxfd = (listenfd > STDIN_FILENO) ? listenfd : STDIN_FILENO;

  //select setup
  fd_set tmp_set;
  FD_ZERO(&readset);
  addToSet(listenfd);
  addToSet(STDIN_FILENO);

  //HACK: only print once
  printMainMenu();


  //use select here instead ==> stdin or received from server
  while(1){
    //make a copy of select
    changedSet = false;
    tmp_set = readset;
    select(maxfd+1, &tmp_set, NULL, NULL, NULL);


    //if stdin is set (user entered input)
    if(FD_ISSET(STDIN_FILENO, &tmp_set)){
      //read user input
      getline(cin, user_in);
      clientOptions(user_in);
    }


    if(isLoggedIn){
      //listen for server messages
      if(FD_ISSET(connectfd, &tmp_set)){
        //reset buf
        memset(&buf, 0, sizeof(buf));
        n = Read(connectfd, buf, MAXBUFLEN-1);
        buf[n] = '\0';
        if(n == 0){
          cerr << "Client: Server closed connection" << endl;
          close(connectfd);
          isLoggedIn = false;
          //remove server from readset
          removeFromSet(connectfd);
        }
        serverMessage(buf);
      }
      //check for message from friends
      for(int i = 0; i < friends.size(); ++i){
        //friend has active socket
        if(friends[i].f_sockfd != -1){
          //friend's socket is set
          if(FD_ISSET(friends[i].f_sockfd , &tmp_set)){
            chatReceivedFriend(i);
          }
        }
      }

    //listen for incoming chat requests
    if(FD_ISSET(listenfd, &tmp_set)){
      //received new connection
      chatReceived(listenfd);
     }
     }
    }
  }

//add to readset, update maxfd
void Client::addToSet(int fd){

  allSockFds.push_back(fd);
  FD_SET(fd, &readset);
  maxfd = (fd > maxfd)? fd : maxfd;

}

//remove from FD_SET readset, update maxfd
void Client::removeFromSet(int fd){
  FD_CLR(fd, &readset);
  vector<int>::iterator it = allSockFds.begin();
  for( ; it != allSockFds.end(); ++it){
    if(*it == fd){
      allSockFds.erase(it);
      break;
    }
  }
  //new max
  maxfd = max(maxfd, *max_element(allSockFds.begin(),
						allSockFds.end()));
}


/****************************
**SELECT HANDLERS
*****************************/
//handle user input
void Client::clientOptions(string input){
  string choice;
  cout << ">";
  //input split
  stringstream ss(input);
  ss >> choice;

  if(!isLoggedIn){
    if(choice == "r" || choice == "l"){
      connectClient();
      if(uname == "" && pwd == "")
        getLogin();
      userLogin();
      isLoggedIn = true;
      //reset friends online when loggin in
      friends.clear();
      //add new connectfd to the set
      addToSet(connectfd);
    }
    //exit
    else if(choice == "exit"){
      clientExit();
    }
    else{
      cout << "Please Enter Valid Choice!" << endl;
      cout << "(client not logged in)" << endl;
    }
  }
  else{
    //logout
    if(choice == "logout"){
      Write(connectfd, "logout", MAXBUFLEN-1);
      //close all friend connections
      for(int i = 0; i < friends.size(); ++i){
        if(friends[i].f_sockfd != -1){
          close(friends[i].f_sockfd);
          removeFromSet(friends[i].f_sockfd);
          friends[i].f_sockfd = -1;
          friends[i].isOnline = false;
        }
        friends[i].isOnline = false;
      }
      close(connectfd);
      removeFromSet(connectfd);
      isLoggedIn = false;
    }
    //invite
    else if(choice == "i"){
      //send the server the message
      Write(connectfd, input.c_str(), MAXBUFLEN-1);
    }
    //accept invite
    else if(choice == "ia"){
        Write(connectfd, input.c_str(), MAXBUFLEN-1);
    }
    //message friend
    else if(choice == "m"){
      //client tries to send new message
      chatConnect(input);
    }
    else{
      cout << "Please Enter Valid Choice!" << endl;
      cout << "(client already logged in)" << endl;
    }

  }
  return;
}

//handle messages from the server
void Client::serverMessage(string mssg){
  //mssg may contain multiple commands

  //check if multiple messages are merged
  size_t pos = 0;
  if((pos = mssg.find_first_of(":"))!= string::npos){
    if(mssg.find_first_of(":", pos+1)!=string::npos){
      cout << "Client: received concatenated message = " << mssg << endl;
    }
  }

  stringstream ss((mssg));
  string token;
  getline(ss, token, ':');

  //contacts
  if(token == "contactsOn"){
    while(getline(ss, token, ';')){
      if(token == " ") break;
      addFriends(token);
    }
    printFriends();
  }
  //loggedIn
  else if(token == "loggedIn"){
    getline(ss, token, ';');
    addFriends(token);
    printFriends();

  }
  //loggedOut
  else if(token == "loggedOut"){
    getline(ss, token, ';');
    //isOnline = false, closes(friend_sockfd) and removes from readset
    removeFriends(token);
    printFriends();
  }
  //invite or invite accept
  else if(token == "i"){
    cout << "Received invite: " << mssg << endl;
  }
  else if(token == "ia"){
    cout << "Accepted Invite: " << mssg << endl;
  }
  else{
    cout << "serverMessage: error invalid message from Server" << endl;
  }
}

//sending message "m friend hellofriend2"
void Client::chatConnect(string mssg){
    //preprocess message
    stringstream ss(mssg);
    string token;
    string mssg2send;
    string t_mssg;

    //split the message
    ss >> token;
    //prepare message to send
    mssg2send += token + " " + uname + " ";
    ss >> token;

    getline(ss, t_mssg, '\n');
    mssg2send += t_mssg;


   int f_ind = getIndexFromName(token);
   if(f_ind == -1){
     cerr << "Client Chatconnect: no such contact " << token << endl;
     return;
    }
   if(friends[f_ind].isOnline == false){
     //CLIENT: remove from set???
     cerr << "Client Chatconnect: message undeliverable friend '"
     << token << "' is offline"<<endl;
     //remove from set?
     return;
   }

    //get friend socket
    int friendfd = getSockByName(token);

    if(friendfd == -1){
      //create a new socket
      friendfd = Socket(AF_INET, SOCK_STREAM, 0);
      //save it
      friends[f_ind].f_sockfd = friendfd;
      //add it to the readset
      addToSet(friendfd);

      //initialize cli_addr
      struct sockaddr_in cli_addr;
      initSockAddrIn(cli_addr, friends[f_ind].f_port,
        inet_addr(friends[f_ind].f_ip.c_str()));

      //connect
      Connect(friendfd, (struct sockaddr*)&cli_addr, sizeof(cli_addr));
      //able to send so friend is online!
      friends[f_ind].isOnline = true;
      }
      //writing message
      Write(friendfd, mssg2send.c_str(), MAXBUFLEN-1);
}

//incoming connection; process incoming message
void Client::chatReceived(int acceptedFd){
  struct sockaddr_in recaddr;
  int *sockptr;
  socklen_t len = sizeof(recaddr);
  char buf[MAXBUFLEN];

  //accept new connection
  acceptedFd = Accept(acceptedFd, (struct sockaddr*) &recaddr, &len);

  //read the message
  int n = 0;
  memset(&buf, 0, sizeof(buf));
  n= Read(acceptedFd, buf, MAXBUFLEN-1);
  buf[n] = '\0';

  //process the message
  string token;
  string mssg;
  istringstream ss(buf);
  //discard m
  ss >> token;
  //save sender name <== modified by server
  ss >> token;
  getline(ss, mssg, '\n');

  //find friend in contacts
  int ind = getIndexFromName(token);

  if(ind == -1){
    cerr << "Client chatReceived: message dropped no such contact " <<
    token << endl;
    return;
  }
  //if is friend then print!
  cout << token << " >>" << mssg << endl;

  //mark friend as active and save their sockfd
  friends[ind].isOnline = true;
  friends[ind].f_sockfd = acceptedFd;
  addToSet(acceptedFd);
}

//known friend messaged us
void Client::chatReceivedFriend(int ind){
  char buf[MAXBUFLEN];
  memset(&buf, 0, sizeof(buf));

  int n= Read(friends[ind].f_sockfd, buf, MAXBUFLEN-1);
  buf[n] = '\0';
  if(n == 0){
    //friend closed the connection!!

    //isonline to false, close that sock_fd
    close(friends[ind].f_sockfd);
    removeFromSet(friends[ind].f_sockfd);
    friends[ind].isOnline = false;
    friends[ind].f_sockfd = -1;
    return;
  }

  //process the message
  string token;
  string mssg;
  istringstream ss(buf);
  //discard m
  ss >> token;
  //save sender name <== modified by server
  ss >> token;
  getline(ss, mssg, '\n');

  cout << token << " >>" << mssg << endl;
}

//close all sockets
void Client::clientExit(){
  cerr << "Client Exit: closing = " << listenfd << endl << connectfd << endl;
  close(listenfd);
  close(connectfd);

  for(int i = 0; i < friends.size(); ++i){
    if(friends[i].f_sockfd != -1){
      cerr << friends[i].f_sockfd << endl;
      close(friends[i].f_sockfd);
    }
  }
  exit(0);
}

/****************************
**SELECT HELPERS
*****************************/

//return sockfd given friend name
int Client::getSockByName(string token){
  for(int i = 0; i < friends.size(); ++i){
    if(token == friends[i].f_name){
      return friends[i].f_sockfd;
    }
  }
  return -1;
}

//return friends index given name
int Client::getIndexFromName(string token){
  for(int i = 0; i < friends.size(); ++i){
    if(token == friends[i].f_name)
      return i;
  }
  return -1;
}

//get index given socket
int Client::getIndexFromSock(int sockfd){
  for(int i = 0; i < friends.size(); ++i){
    if(friends[i].f_sockfd == sockfd){
      return i;
    }
  }
  return -1;
}

/****************************
**PROCESS SERVER MESSAGES
*****************************/

//add "bob 0.0.0.0 51000" to the friends list
void Client::addFriends(string friend_info){
  struct Friend tempF;
  stringstream ss(friend_info);
  string token;

  //get friend info
  ss >> token;
  tempF.f_name = token;
  ss >> token;
  tempF.f_ip = token;
  ss >> token;
  tempF.f_port = stoi(token);

  tempF.f_sockfd = -1;
  tempF.isOnline = true;

  //check if friend just logging back in
  for(int i = 0; i < friends.size(); ++i){
    if(tempF.f_name == friends[i].f_name){
      //if login again, simply update
      friends[i].f_ip = tempF.f_ip;
      friends[i].f_port = tempF.f_port;
      friends[i].isOnline = true;
      friends[i].f_sockfd = -1;
      return;
    }
  }
  //else new friend
  friends.push_back(tempF);
}

//friend logged out, close socket and remove from fd_set
void Client::removeFriends(string friend_info){
  istringstream ss(friend_info);
  string name, ip, port;
  //get info of friend to log out
  ss >> name; ss >> ip; ss >> port;

  //set offline
  for(int i = 0; i < friends.size(); ++i){
    if(friends[i].f_name == name){
      if(friends[i].isOnline) friends[i].isOnline = false;
      if(friends[i].f_port > 0) friends[i].f_port = 0;
      //if open close it
      if(friends[i].f_sockfd > 0){
        close(friends[i].f_sockfd);
        removeFromSet(friends[i].f_sockfd);
      }
      break;
    }
  }
}

/****************************
**READING CONFIGURATION FILES
*****************************/

void Client::readConfigFile(string filename){
  string line = "";
  //open the file
  ifstream ConfigFILE(filename);
  if(!ConfigFILE.is_open()){
    fprintf(stderr, "Client: could not open configfile\n");
    exit(1);
  }

  //get servhost
  getline(ConfigFILE, line);
  if(line == ""){
    fprintf(stderr, "readConfigFile Error: empty ConfigFILE\n");
    exit(1);
  }
  //convert fully qualified domain to dotted decimal
  string full_dom = line.substr(line.find_first_of(":",0)+1, string::npos);
  struct hostent *he =  gethostbyname(full_dom.c_str());
  if(he == NULL){
    cerr << "Client readconfigfile: hostent is NULL" << endl;
    exit(1);
  }
  //saving in host order; when connecting will be network order
  this->servhost = inet_ntoa(*(struct in_addr*)he->h_addr_list[0]);

  //get servport
  getline(ConfigFILE, line);
  if(line == ""){
    fprintf(stderr, "readConfigFile Error: empty ConfigFILE\n");
    exit(1);
  }
  this->servport = stoi(line.substr(line.find_first_of(":",0)+1, string::npos));
}

/****************************
**PRINTING
*****************************/

void Client::printMainMenu(){
  cout << "Client Menu: " << endl;
  cout << "'l' for login " << endl;
  cout << "'i potential_friend [message]' for invite " << endl;
  cout << "'ia inviter_name [message]' for invite accept " << endl;
  cout << "'r' for register " << endl;
  cout << "'logout' for logout " << endl;
  cout << "'exit' for logout " << endl;
}

void Client::printClientListening(){
  cout << "Client Listening On: " << endl;
  cout << "IP = " << cli_loc_ip << " Port = " << cli_loc_port << endl;
}

void Client::printClientServerConn(){
  cout << "Client Connected On: " << endl;
  cout << "IP = " << cli_ip << " Port = " << cli_port << endl;
}

void Client::printFriends(){
  cout << "Friends: " << endl;

   for(int i = 0; i < friends.size(); ++i){
    cout << friends[i].f_name << " " << friends[i].f_ip <<
     " " << friends[i].f_port << " sock:" << friends[i].f_sockfd <<
     " online: " << friends[i].isOnline << endl;
  }

}

/***************
* Samir Yuja sjy14b
* Due 11/29/2016
* PROJECT 2
* COP 5570
***************/


#ifndef _CLIENT_H
#define _CLIENT_H

#include <vector>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <signal.h>
#include <sys/types.h>
#include <netdb.h>

#include <arpa/inet.h>

  using namespace std;

#define PORTNUM 51000


class Client{
  public:
    Client(string ConfigFile);     //Constructor
    ~Client();

    //Select stdin,server,chat
    void clientSelect();
    //Server messaged
    void serverMessage(string mssg);
    //User input: login, register, exit, etc
    void clientOptions(string input);


    //Friend Chat Connection
    void startClient();            //start listening for friends
    void getSocketInfo(int &fd, string &ip);  //calls Getsockname

    //EXIT <== signal handler calls this!
    void clientExit();

  private:
    //client chat and client server connection
    int servport, cli_port;
    string servhost, cli_ip;           //dotted decimal server ip
    int listenfd, connectfd, maxfd;
    fd_set readset;

    //clients location
    int cli_loc_port;
    string cli_loc_ip;

    //login info
    bool isLoggedIn;
    bool changedSet;
    string uname, pwd;

    //READ config file
    string ConfigFile;
    void readConfigFile(string filename);

    //CHAT
    //friend struct
    struct Friend{
      string f_name;
      string f_ip;
      int f_port;
      int f_sockfd;
      bool isOnline;
    };

    //save friend info
    vector<struct Friend> friends;


    //LOGIN/REGISTER HELPERS
    void connectClient();           //CONNECT to server
    void getLogin();                 //get user and pwd
    void userLogin();               //connect to the server

    //SELECT helpers
    vector <int> allSockFds;
    void addToSet(int fd);
    void removeFromSet(int fd);
    int getSockByName(string name);
    int getIndexFromName(string name);
    int getIndexFromSock(int sockfd);

    //SERVER Message Helpers
    void addFriends(string friend_info);
    void removeFriends(string friend_info);


    //CHAT
    void chatConnect(string mssg);
    void chatReceived(int acceptedFd);
    void chatReceivedFriend(int ind);



    //PRINT functions
    void printClientListening();
    void printClientServerConn();
    void printMainMenu();
    void printFriends();

};

#endif

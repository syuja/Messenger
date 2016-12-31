/***************
* Samir Yuja sjy14b
* Due 11/29/2016
* PROJECT 2
* COP 5570
***************/

#ifndef _SERVER_H
#define _SERVER_H

#include <vector>
#include <string>
#include <iostream>
#include <pthread.h>
#include <signal.h>
#include <ctime>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define CLIENTCONFIG "cconfig"
#define USERINFO "user_info"

  using namespace std;


class Server{
  public:
    Server(string UserFileName, string ConfigFile); //Constructor
    ~Server();                                      //Close sockfd

    void startServer();

    //THREADS
    static void *processClientHelper(void *arg);  //thread 1 client
    void processClient(void *arg);                //thread 1 client MUTEX

    //LOGIN client
    bool canLogRegClient(int sockfd, string uname_pwd); //mutex

    //HANDLE client requests
    void clientMenu(int clientfd);

    //LOCATION
    void sendLoc(int clientfd);         //sends friends my location
    void sendFriendsLoc(int clientfd);  //send client their friends location

    //SIGNAL HANDLER
    void serverExit();
    static void thread_handler(int signo);


  //private members
  private:

    static Server *server;
    //FILENAMES
    string UserInfoFile, ConfigFile;

    //SERVER config
    int portnum, listenfd;
    string domain_name;
    string server_ip;

    //COUNT threads and users
    vector<pthread_t> thread_ids;
    int total_online_users;
    //mutex for total online users
    pthread_mutex_t total_online_users_lock;
    pthread_mutex_t users_lock;
    pthread_mutex_t invites_lock;

    //Accepting Connection
    //holds name, pwd, location, socket, contacts and if active
    struct User{
      string uname;
      string pwd;
      string ip;
      int sockfd;
      int port;
      vector<string> contacts;
      bool isActive;
    };

    vector<struct User> users;

    struct Invite{
      string inviter;
      bool invReceived;
      string iaccepter;
      bool accReceived;
      clock_t invT;
    };

    vector<struct Invite> invites;


    //INVITE/ACCEPT INVITE
    void invite(int clientfd, string istr);
    void inviteAccept(int clientfd, string iastr);
    void addNewFriends(int i);

    //HELPER: Reading Files
    void readUserInfoFile(string filename);
    void getName(string UF_line, struct User &new_user);
    void getPSSWD(string UF_line, struct User &new_user);
    void getContactList(string UF_line, struct User &new_user);

    void readConfigFile(string filename);
    void setPortNum(int portnum);

    //HELPER: Sending Location
    void saveCliLoc(string loc, int clientfd); //MUTEX required
    vector<int> getFriendSockets(int i);
    vector<string> getFriendsLocations(int i);
    int getIndexFromSock(int clientfd);
    int getIndexFromName(string token);
    int getSockByName(string user_n);


    //HELPER: Menu: invite, accept invite, log out
    void logOut(int clientfd);

    //Write files
    void writeUserInfo();
    void writeForClient(sockaddr_in &serv_addr);

    //PRINT
    void printUsers();                              //print accounts
    void printActiveUsers();                        //count active users
    void printInvites();

};

struct pthreadARG{
  Server *S;
  int sock;
};

#endif

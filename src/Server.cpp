/***************
* Samir Yuja sjy14b
* Due 11/29/2016
* PROJECT 2
* COP 5570
***************/

#include "Server.h"
#include "Util.h"

#include <iostream>
#include <fstream>
#include <string> //for stoi
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
  using namespace std;

//threadsignal handler
void Server::thread_handler(int signo){
  //exit, so mainthread can do pthread_join
   pthread_exit(NULL);
  }

/****************************
**CONSTRUCTOR/DESTRUCTOR
*****************************/

Server::Server(string UserFileName, string ConfigFile){
  //Check that the files exist
  struct stat buf;
  total_online_users_lock = PTHREAD_MUTEX_INITIALIZER;
  users_lock = PTHREAD_MUTEX_INITIALIZER;
  invites_lock = PTHREAD_MUTEX_INITIALIZER;
  total_online_users = 0;

  if(lstat(UserFileName.c_str(), &buf) < 0){
    fprintf(stderr, "Server Construct Error: %s '%s'\n", strerror(errno),
     UserFileName.c_str());
    exit(1);
  }
  if(lstat(UserFileName.c_str(), &buf) < 0){
    fprintf(stderr, "Server Construct Error: %s '%s'\n", strerror(errno),
     ConfigFile.c_str());
    exit(1);
  }
  UserInfoFile = UserFileName;
  this->ConfigFile = ConfigFile;

  //Read Each Into the Structures
  readUserInfoFile(UserInfoFile);
  readConfigFile(this->ConfigFile);
}

Server::~Server(){
  //close each Socket in users
  serverExit();
}

void Server::startServer(){
  struct sockaddr_in serv_addr, recaddr;
  int rec_sock;
  pthread_t tid;
  //initialize serv_addr
  initSockAddrIn(serv_addr, portnum);

  //socket
  listenfd = Socket(AF_INET, SOCK_STREAM, 0);

  //turn on SO_REUSEADDR
  socklen_t yes = 1;
  Setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

  //bind it
  Bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

  //Save IP
  this->server_ip = getIP(serv_addr);

  //write to ip and port for client
  writeForClient(serv_addr);

  //PRINT for client
  cout << "Server Domain = " << domain_name <<
  " Port = " << ntohs(serv_addr.sin_port) << endl;

  //listen
  Listen(listenfd);

  for(; ;){
    int *sockptr;
    char dest[INET_ADDRSTRLEN];
    socklen_t len = sizeof(recaddr);
    tid = 0;

    rec_sock = Accept(listenfd, (struct sockaddr*) &recaddr, &len);
    cout << "Connected Remote Client IP = " <<
    inet_ntop(AF_INET, &recaddr.sin_addr, dest, INET_ADDRSTRLEN) <<
    " Portnum = " << ntohs(recaddr.sin_port) << endl;

    //call thread to handle the connection

    struct pthreadARG *ARG =
    (struct pthreadARG*) malloc(sizeof(struct pthreadARG));
    ARG->S = this;
    ARG->sock = rec_sock;
    pthread_create(&tid,NULL, &Server::processClientHelper, (void*) ARG);
    thread_ids.push_back(tid);
  }
}

//write full domain name and port to cconfig for client
void Server::writeForClient(sockaddr_in &serv_addr){
  string svhost = "servhost:";
  string svport  = "servport:" + to_string(portnum);

  char hostname[512];
  hostname[511] = '\0';
  gethostname(hostname, 511);

  struct hostent *h = gethostbyname(hostname);
  if(h == NULL){
    cerr << "Server writeforclient: hostent null" << endl;
    exit(1);
  }
  svhost += h->h_name;
  domain_name = h->h_name;
  ofstream CC_file(CLIENTCONFIG);
  if(!CC_file.is_open()){
    cerr << "Server: writeforclient could not open" << endl;
    exit(1);
  }
  CC_file << svhost << endl;
  CC_file << svport << endl;
}

/****************************
**HANDLE CLIENT CONNECTION
*****************************/

//static starting function
void * Server::processClientHelper(void *arg){
  //register a signal handler for the thread!
  struct sigaction thread_sig;
  thread_sig.sa_handler = thread_handler;
  sigemptyset(&thread_sig.sa_mask);
  thread_sig.sa_flags = 0;
  sigaction(SIGUSR1, &thread_sig, 0);

  //call member function!
  struct pthreadARG ARG = *((struct pthreadARG*) arg);
  static_cast<Server*>(ARG.S)->processClient(arg);
  return NULL;
}

//actual starting function for threads
void Server::processClient(void *arg){
  //obtain mutex of shared variables
  char buf[MAXBUFLEN];
  int n = 0;
  char s200[] = "200";
  char s500[] = "500";

  //get arguments
  struct pthreadARG locARG = *((struct pthreadARG*) arg);
  int clientfd = locARG.sock;
  free(arg);

  //read read either login or registration
  while(1){
    //first read username and pwd
    memset(&buf,0,sizeof(buf));
    while((n =Read(clientfd, buf, MAXBUFLEN-1))>0){
        buf[n]='\0';
        break;
      }
      //attempt to login in or register user
      if(canLogRegClient(clientfd,buf)){
        //Write 200
        Write(clientfd, s200, MAXBUFLEN-1);

        pthread_mutex_lock(&total_online_users_lock);
          total_online_users++;
        pthread_mutex_unlock(&total_online_users_lock);

        break;
      }
      else{
        //Write 500
        Write(clientfd, s500, MAXBUFLEN-1);
       }
       memset(&buf,0,sizeof(buf));
    }
    //get location

    while(1){
      memset(&buf, 0, sizeof(buf));
      if((n=Read(clientfd, buf, MAXBUFLEN-1))>0){
          buf[n] = '\0';
          saveCliLoc(string(buf),clientfd);
          //send location to all friends
          sendLoc(clientfd);
          sendFriendsLoc(clientfd);
          break;
      }
  }
  //send friend locations to all + listen for requests
  clientMenu(clientfd);
}

//login or register the client
bool Server::canLogRegClient(int sockfd, string uname_pwd){
  string usern, upwd;
  bool isTaken = false;

  //split user name and password
  size_t space_pos = uname_pwd.find_first_of(" ", 0);
  usern = uname_pwd.substr(0, space_pos);
  upwd = uname_pwd.substr(space_pos+1, string::npos);
  char dest[INET_ADDRSTRLEN];

  //LOGIN
  //check if user name exists and pwd matches
  pthread_mutex_lock(&users_lock);
  for(int i = 0; i < users.size(); ++i){
     if(users[i].uname == usern){
      isTaken = true;
      if(users[i].pwd == upwd){
         //login success
         users[i].isActive = true;
         users[i].sockfd = sockfd;
         pthread_mutex_unlock(&users_lock);
         return true;
       }
     }
  }
  pthread_mutex_unlock(&users_lock);

  //REGISTER
  //check if user name already exists
  if(isTaken){
       return false;
     }
  //can register
   struct User new_user;
   new_user.uname = usern;
   new_user.pwd = upwd;
   new_user.isActive = true;
   new_user.sockfd = sockfd;

   pthread_mutex_lock(&users_lock);
    users.push_back(new_user);
   pthread_mutex_unlock(&users_lock);

   return true;
}


//send location to friends, also sending name
void Server::sendLoc(int clientfd){
  //what is client's index?
  int i = getIndexFromSock(clientfd);

  //will mutex lock
  vector<int> friends_sd = getFriendSockets(i);

  if(friends_sd.empty()){
    //no friends so just return
    return;
  }

  //loggedin <== format to friends
  string location = "loggedIn: " + users[i].uname + " " + users[i].ip;
  location += " " + to_string(users[i].port) + "; ";

  //send it to friends
  for(int j = 0; j < friends_sd.size(); ++j){
        Write(friends_sd[j], location.c_str(), MAXBUFLEN-1);
      }
}

//send friends loc to new user
void Server::sendFriendsLoc(int clientfd){
  int i = getIndexFromSock(clientfd);
  vector<string> friends_loc = getFriendsLocations(i);

  //if no friends are active then just return
  if(friends_loc.empty()){
    //no friends so just return
     return;
   }
  //store all friends locations
  string cont_loc = "contactsOn: " ;

  for(int j = 0; j < friends_loc.size(); ++j){
    cont_loc += friends_loc[j];
  }
  Write(clientfd, cont_loc.c_str(), MAXBUFLEN-1);
}



/**********************************
**HANDLE CLIENT CONNECTION HELPERS
**********************************/

//save the clients location
void Server::saveCliLoc(string loc, int clientfd){
  string ip;
  int port, i;

  //split loc "ip port" into "ip" and int(port)
  size_t space_pos = loc.find_first_of(" ", 0);
  ip = loc.substr(0, space_pos+1);
  port = stoi(loc.substr(space_pos+1, string::npos));


  //save it
  i = getIndexFromSock(clientfd);
  struct sockaddr_in client_st;
  memset(&client_st, 0, sizeof(client_st));
  int len = sizeof(client_st);
  getpeername(clientfd, (struct sockaddr*) &client_st, (socklen_t *)&len);
  pthread_mutex_lock(&users_lock);
    users[i].ip = inet_ntoa(client_st.sin_addr);
    users[i].port = port;
    cout << "Client Listening on IP = " << users[i].ip << " Port = " <<
     users[i].port << endl;
  pthread_mutex_unlock(&users_lock);
  cout << "Users Online : " << total_online_users << endl;
}


//get sockets of active friends
vector<int> Server::getFriendSockets(int i){
  //holds all friends sd
  vector<int> friends_sd;

  pthread_mutex_lock(&users_lock);
  for(int k = 0; k < users[i].contacts.size(); ++k){
    for(int j = 0; j < users.size(); ++j){
      if(users[i].contacts[k] == users[j].uname){
        if(users[j].isActive){
          //add to vector
          friends_sd.push_back(users[j].sockfd);
        }
      }
    }
  }
  pthread_mutex_unlock(&users_lock);

  return friends_sd;
}

//returns locations of friends for user[i]
vector<string> Server::getFriendsLocations(int i){
  vector<string> friend_locations;

  pthread_mutex_lock(&users_lock);
  for(int k = 0; k < users[i].contacts.size(); ++k){
    for(int j = 0; j < users.size(); ++j){
      if(users[i].contacts[k] == users[j].uname){
        if(users[j].isActive){
          string l =
          users[j].uname+" "+users[j].ip+" "+to_string(users[j].port) + "; ";
            friend_locations.push_back(l);
            break;
           }
        }
     }//end first for
  }//end second for
  pthread_mutex_unlock(&users_lock);
  return friend_locations;
}

//return index of client given client's socket
int Server::getIndexFromSock(int clientfd){
  int ind = -1;

  pthread_mutex_lock(&users_lock);
  for(int i = 0; i < users.size(); ++i){
    //if socket matches, then return
    if(users[i].sockfd == clientfd){
      ind = i;
      break;
    }
  }
  pthread_mutex_unlock(&users_lock);
  return ind;
}

//returns sockfd given name of user
int Server::getSockByName(string user_n){
  int ind = -1;
  pthread_mutex_lock(&users_lock);
  for(int i = 0; i < users.size(); ++i){
    if(users[i].uname == user_n){
      if(users[i].isActive == true){
        //found
        ind =  users[i].sockfd;
        break;
      }
      else{
        //found but not online
        break;
      }
    }
  }
  pthread_mutex_unlock(&users_lock);
  return ind;
}

/**********************************
**CLIENT MENU
**********************************/

void Server::clientMenu(int clientfd){
  //save online users
  char buf[MAXBUFLEN];
  string buf_str;
  string choice;
  int n;

  while(1){
    //read
    memset(&buf, 0, sizeof(buf));
    if((n = Read(clientfd, buf, MAXBUFLEN-1))>0){
      //act based on command
      buf[n]= '\0';
    }
    //client logged out!
    else if(n == 0){
      //client has nothing to send currently
      logOut(clientfd);
      continue;
    }
    buf_str = buf;
    choice = buf_str.substr(0, buf_str.find_first_of(" ",0));


    //i potential_friend [mssg]
    if(choice == "i"){
      invite(clientfd, buf);
      //printInvites();
    }
    //ia inviter_name [mssg]
    else if(choice == "ia"){
      inviteAccept(clientfd, buf);
      // printInvites();
    }
    //logout
    else if(buf_str == "logout"){
      logOut(clientfd);
    }
    //please enter a valid command
    else{
      cerr << "Server clientMenu: invalid mssg: " << buf_str << endl;
    }

    memset(&buf, 0, MAXBUFLEN);
    buf_str.clear();
    choice.clear();
    //exit then remove location, unactive, close socketfd
  }
}

/**********************************
**CLIENT MENU HELPERS
**********************************/

//invite: change name and send out! (save in struct)
void Server::invite(int clientfd, string istr){
  //get information
  struct Invite new_invite;
  string orig_str = istr;

  //Process the message
  //returns name
  istr =  istr.substr(istr.find_first_of(" ",0)+1, string::npos);
  size_t pos = istr.find_first_of(" ",0);
  new_invite.iaccepter = istr.substr(0,pos);
  //check for blank message
  string mssg;
  if(pos == string::npos){
    mssg = "";
  }else{
    mssg = istr.substr(pos,string::npos);
  }

  //change the name before sending!
  new_invite.inviter = users[getIndexFromSock(clientfd)].uname;
  istr = "i: " + new_invite.inviter +" "+mssg;


  new_invite.invReceived = true;
  new_invite.accReceived = false;
  new_invite.invT = clock();

  //check if it already exists
  pthread_mutex_lock(&invites_lock);

  bool isFound = false;
  for(int i = 0; i < invites.size(); ++i){
    //if found don't save duplicate
    if(invites[i].inviter == new_invite.inviter &&
       invites[i].iaccepter == new_invite.iaccepter){
         invites[i].invT = clock();
         isFound = true;
         break;
    }
  }
  //else save new invite
  if(!isFound)
    invites.push_back(new_invite);

  pthread_mutex_unlock(&invites_lock);

  //get socket of accepter ==> not active then just return
  int tmp_sock = getSockByName(new_invite.iaccepter);
  if(tmp_sock == -1){
    cerr << "invite: user not active" << endl;
    return;
  }

  //if online pass it forward
  Write(tmp_sock, istr.c_str(), MAXBUFLEN-1);

}

//inviteAccept: notify both, and save contacts
void Server::inviteAccept(int clientfd, string iastr){
  //get the information
  struct Invite new_invite;

  //process the message
  string orig_str = iastr;
  iastr =  iastr.substr(iastr.find_first_of(" ",0)+1, string::npos);
  size_t pos = iastr.find_first_of(" ",0);
  new_invite.inviter = iastr.substr(0,pos);

  pos = iastr.find_first_of(" ", pos);
  if(pos == string::npos){
    iastr = "ia: " + users[getIndexFromSock(clientfd)].uname +" ";
  }
  else{
    iastr = "ia: "+ users[getIndexFromSock(clientfd)].uname;
    iastr += " " +iastr.substr(pos, string::npos);
  }


  new_invite.iaccepter = users[getIndexFromSock(clientfd)].uname;
  new_invite.invReceived = false;
  new_invite.accReceived = true;
  new_invite.invT = clock();

  //check that invite exists, else discard
  //check if it already exists
  pthread_mutex_lock(&invites_lock);

  bool isFound = false;
  int i;
  for(i = 0; i < invites.size(); ++i){
    if(invites[i].inviter == new_invite.inviter &&
       invites[i].iaccepter == new_invite.iaccepter &&
      invites[i].invReceived == true){
        invites[i].accReceived = true;
         isFound = true;
         break;
    }
  }
  //else discard!
  if(!isFound){
    cerr << "inviteAccept: dropping ia, no original invite" << endl;
    return;
  }
  pthread_mutex_unlock(&invites_lock);

  if(isFound){
    //get socket of inviter
    int tmp_sock = getSockByName(new_invite.inviter);
    if(tmp_sock == -1){
      cerr << "inviteAccept: user not online" << endl;
      return;
    }

    //if online pass it forward
    Write(tmp_sock, iastr.c_str(), MAXBUFLEN-1);
    addNewFriends(i);
  }
}

//sends logged in so that friends add each other
void Server::addNewFriends(int i){
  //add friends to each other's lists
  int f1_ind = getIndexFromName(invites[i].inviter);
  int f2_ind = getIndexFromName(invites[i].iaccepter);

  users[f1_ind].contacts.push_back(users[f2_ind].uname);
  users[f2_ind].contacts.push_back(users[f1_ind].uname);
  //loggedIn: alice 127.0.0.1 61185;
  string loggedin1 = "loggedIn: " + users[f1_ind].uname + " " +users[f1_ind].ip;
  loggedin1 += " " + to_string(users[f1_ind].port) + "; ";

  //notify first friend
  Write(users[f2_ind].sockfd, loggedin1.c_str(), MAXBUFLEN-1);

  //prepare second messaage
  string loggedin2 = "loggedIn: " + users[f2_ind].uname + " " + users[f2_ind].ip;
  loggedin2 += " " + to_string(users[f2_ind].port) + "; ";

  //notify second frined
  Write(users[f1_ind].sockfd, loggedin2.c_str(), MAXBUFLEN-1);
}

//return index given name!
int Server::getIndexFromName(string token){
  for(int i = 0; i < users.size(); ++i){
    if(token == users[i].uname)
      return i;
  }
  return -1;
}

//close connection, notify friends
void Server::logOut(int clientfd){
  int i = getIndexFromSock(clientfd);
  string loggedOut = "";
  vector <int> friends_sd;

  //print thread and info
  pthread_t t = pthread_self();
  int in = getIndexFromSock(clientfd);
  cerr << "Server: Logged Out " << users[in].uname <<" IP= " << users[in].ip <<
   " Port = " << users[in].port << " tid = "<< t << endl;

  //MUTEX
  pthread_mutex_lock(&users_lock);
  //prepare string: notify friends
  loggedOut = "loggedOut: "+users[i].uname+" "+users[i].ip + " ";
  loggedOut += to_string(users[i].port) + ";";
  //mark as unactive
  users[i].isActive = false;
  //remove location
  users[i].ip = "";
  users[i].port = 0;
  pthread_mutex_unlock(&users_lock);

  //logged out: name ip port to friends
  friends_sd = getFriendSockets(getIndexFromSock(clientfd));
  for(int j = 0; j < friends_sd.size(); ++j){
      Write(friends_sd[j], loggedOut.c_str(), MAXBUFLEN-1);
  }

  //close connection
  close(clientfd);

  pthread_mutex_lock(&total_online_users_lock);
    total_online_users--;
    cout << "Users Online : " << total_online_users << endl;
  pthread_mutex_unlock(&total_online_users_lock);

  //exit thread
  pthread_exit(NULL);
}

/****************************
**EXIT (also helps signal handler)
*****************************/

//kill threads, join them and close sockets
void Server:: serverExit(){
  //close all connection
  cerr << "serverExit: closing thread id = " << endl;
  for(int i = 0; i < thread_ids.size(); ++i){
     cerr << thread_ids[i] << " " << endl;
    if(pthread_kill(thread_ids[i], SIGUSR1) != 0){
      perror("serverExit pthread_kill: ");
    }
  }
  cerr << "serverExit: joining thread id = " << endl;
  for(int i = 0; i < thread_ids.size(); ++i){
   cerr << thread_ids[i] << " " << endl;
    pthread_join(thread_ids[i], NULL);
  }

  close(listenfd);

  //no need for mutex, threads have been killed
    cerr << "serverExit: closing socket = ";
  for(int i = 0; i < users.size(); ++i){
    if(users[i].sockfd != -1){
      cerr  << users[i].sockfd << " ";
      close(users[i].sockfd);
    }
  }
  cerr << endl;
  cerr << "serverExit: Writing " << USERINFO << endl;
  writeUserInfo();
}

void Server::writeUserInfo(){
  //save all user info to file
  ofstream CC_file(USERINFO);
  if(!CC_file.is_open()){
    fprintf(stderr, "Server: readUserInfoFile could not open\n");
    exit(1);
  }
  for(int i = 0; i < users.size(); ++i){
      CC_file << users[i].uname << "|" <<
      users[i].pwd << "|";
      for(int j = 0; j < users[i].contacts.size(); ++j){
        CC_file << users[i].contacts[j];
        if(j != users[i].contacts.size()-1){
          //no ; for last contact
          CC_file << ";";
        }
      }
      CC_file << endl;
  }
}

/****************************
**READING CONFIGURATION FILES
*****************************/

//retrieve all acounts into users
void Server::readUserInfoFile(string filename){
  //struct User new_user;
  //memset(&new_user, 0, sizeof(new_user));
  string line;

  //open UserFileName
  ifstream UserInfo(filename);
  if(!UserInfo.is_open()){
    cerr << "Server: readUserInfoFile could not open" << endl;
    UserInfo.close();
    exit(1);
  }

  //error possible buffer overflow
  while(getline(UserInfo, line)){
    //get username, pwd and contacts from each line
    struct User new_user;
    getName(line, new_user);
    getPSSWD(line, new_user);
    getContactList(line, new_user);
    new_user.isActive = false;
    new_user.sockfd = -1;
    users.push_back(new_user);
    // memset(&new_user, 0, sizeof(new_user));
  }
  UserInfo.close();
}


//read the config file and save the portnumber
void Server::readConfigFile(string filename){
  string line = "";
  //open the file
  ifstream ConfigFILE(filename);
  if(!ConfigFILE.is_open()){
    fprintf(stderr, "Server: readConfigFile could not open\n");
    ConfigFILE.close();
    exit(1);
  }
  getline(ConfigFILE, line);
  //get the portnumber
  if(line == ""){
    fprintf(stderr, "readConfigFile Error: empty ConfigFILE\n");
    ConfigFILE.close();
    exit(1);
  }
  setPortNum(stoi(line.substr(line.find_first_of(":",0)+1, string::npos)));
  ConfigFILE.close();
}


/*************************************
**READING CONFIGURATION FILES HELPERS
*************************************/

//retrieve username from user_info
void Server::getName(string UF_line, struct User &new_user){
  //get bob| and push bob
  size_t name_end = UF_line.find_first_of("|", 0);
  string username = UF_line.substr(0, name_end);
  new_user.uname = username;
}

//retrieve password from user_info
void Server::getPSSWD(string UF_line, struct User &new_user){
  //get |bob123| and push
  string pwd = "";
  size_t pwd_begin = UF_line.find_first_of("|", 0);
  size_t pwd_end = UF_line.find_first_of("|", pwd_begin+1);
  pwd = UF_line.substr(pwd_begin+1, pwd_end-pwd_begin-1);
  new_user.pwd = pwd;

}

//retrieve contact list from user_info
void Server::getContactList(string UF_line, struct User &new_user){
  //for each friend push them back ==> bob;alice;fred
  size_t pos_bar = UF_line.find_last_of("|");
  size_t pos_col = UF_line.find_first_of(";", pos_bar);


  //get all contacts
  while( pos_col != string::npos){
    new_user.contacts.push_back(UF_line.substr(pos_bar+1, pos_col-pos_bar-1));
    pos_bar = pos_col;
    pos_col = UF_line.find_first_of(";", pos_bar+1);
  }
  //get very last contact
  new_user.contacts.push_back(UF_line.substr(pos_bar+1));
}

//check portnum and set
void Server::setPortNum(int portnum){
  if(portnum > -1 and portnum < 65535){
    this->portnum = portnum;
   }
   else{
     fprintf(stderr, "setPortNum invalid portnumber");
     exit(1);
   }
}


/****************************
**Printing
*****************************/

void Server::printUsers(){
  for(int i = 0; i < users.size(); ++i){
    cout << "User: " << endl <<
    "uname = " << users[i].uname << " pwd = " << users[i].pwd <<endl <<
    "ip = " << users[i].ip << " port = " << users[i].port << " socket = " <<
    users[i].sockfd << endl << "isActive = " << users[i].isActive <<
    " contacts : ";

    for(int j = 0; j < users[i].contacts.size(); ++j){
      cout << users[i].contacts[j] << " ";
    }
    cout << endl;
  }
}

void Server::printActiveUsers(){
  int count = 0;
  for(int i = 0; i < users.size(); ++i){
    if(users[i].isActive == 1){
      cout << "name = " << users[i].uname << endl;
      count++;
    }
  }
  cout << "total active users = " << count;
}

void Server::printInvites(){
  cout << "Printing Invites: " << endl;
  for(int i = 0; i < invites.size(); ++i){
    cout << i << " inviter = " << invites[i].inviter << " " <<
    invites[i].invReceived<<   " receiver= " <<
    invites[i].iaccepter << " "<< invites[i].accReceived << endl;
  }
}

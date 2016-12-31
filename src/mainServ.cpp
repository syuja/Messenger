/***************
* Samir Yuja sjy14b
* Due 11/29/2016
* PROJECT 2
* COP 5570
***************/

#include "Util.h"
#include "Server.h"


#include <iostream>
  using namespace std;

static Server *server_ptr;


/****************************
**SIGNAL HANDLER
*****************************/
void signal_handler(int signo){
    server_ptr->serverExit();
  }


int main(int argc, char *argv[]){

  if(argc < 3){
    cout << "Usage: ./server.x user_info_file configration_file" <<
     endl;
    exit(1);
  }

  //Server
  Server S1(argv[1], argv[2]);
  server_ptr = &S1;

  //SIGNAL HANDLER
  struct sigaction ctr_c_struct;
  ctr_c_struct.sa_handler = signal_handler;
  sigemptyset(&ctr_c_struct.sa_mask);
  ctr_c_struct.sa_flags = 0;
  sigaction(SIGINT, &ctr_c_struct, 0);

  //Create Listening Socket and Display IP and Port
  S1.startServer();


  return 0;
}

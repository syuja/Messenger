/***************
* Samir Yuja sjy14b
* Due 11/29/2016
* PROJECT 2
* COP 5570
***************/

#include "Util.h"
#include "Client.h"


#include <iostream>
  using namespace std;


static Client *client_ptr;

/****************************
**SIGNAL HANDLER
*****************************/
void signal_handler(int signo){
    client_ptr->clientExit();
  }

int main(int argc, char *argv[]){
  if(argc < 2){
    cout << "Usage: ./client.x configration_file" <<
     endl;
    exit(1);
  }

  //instantiate client
  Client C1(argv[1]);

  client_ptr = &C1;

  //SIGNAL HANDLER
  struct sigaction ctr_c_struct;
  ctr_c_struct.sa_handler = signal_handler;
  sigemptyset(&ctr_c_struct.sa_mask);
  ctr_c_struct.sa_flags = 0;
  sigaction(SIGINT, &ctr_c_struct, 0);

  //Create Listening Socket
  C1.startClient();
  //present menu to user
  C1.clientSelect();



  return 0;
}

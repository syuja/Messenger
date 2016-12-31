# Messenger  
### Starting  
First run `make`.  
To start the server run:  
		`./server.x user_info config_file`   
To start clients run:   
		`./client.x cconfig`  

Once the client has started, the user can log in, register, invite friends, accept invites, log out  
or exit.  

The server uses pthreads to handle incoming connections.   
The client uses `select` to multiplex incoming connections/messages.  

----  
This project is based on a Concurrent, Parallel and Distributed Programming [assignment](CPD_Proj2.pdf).    
	

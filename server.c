/*
** server.c -- a stream socket server demo
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "3522"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold
#define MAXDATASIZE 100

enum COMMANDS{LIST, CHECK, DISPLAY, DOWNLOAD};

void sigchld_handler(int s)
{
	(void)s; // quiet unused variable warning

	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;

   ////////NETWORK SETUP////////////
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

   /////////////////////////////////////

	printf("server: waiting for connections...\n");

   //file descriptor for the parent-child pipes
   //fd[0] is for child of main loop
   //fd[1] is for child of child
   int fd[2][2];

   //100-byte buffer
   char readbuffer[MAXDATASIZE];

   //memset(readbuffer, 3, sizeof(readbuffer));

	while(1) {  // main accept() loop
      int lastPacket = 0;

      ///////ACCEPT INCOMING CONNECTION///////////////
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);
      ///////////////////////////////////////////////

      //create a child process 
      //with a pipe fd[1] connecting parent and child
		pipe(fd[1]);
      if (!fork()) {
			close(sockfd); // child doesn't need the listener
         //fill the buffer with the sent data from the client
         if(recv(new_fd, readbuffer, MAXDATASIZE, 0) == -1){
            perror("recv");
            exit(1);
         }
         //printf("server: got %s\n", readbuffer);

         char filename[MAXDATASIZE];
         
         //the first (and only) character sent from client to server is the command
         switch(readbuffer[0]){
            //list
            case 'l':
               //create a child of the child process
               //with a pipe fd[0] connecting child and subchild
               pipe(fd[0]);
               if(!fork()){   
                  //set the pipe as the output for programs instead of the terminal
                  dup2(fd[0][1], STDOUT_FILENO);
                  //execute ls
                  //output is piped to parent
                  execlp("/bin/ls", "ls", (char*)NULL);
			         exit(0);
               }
               //once the subchild is done
               wait(NULL);
               //copy the output of ls to the buffer
               read(fd[0][0], readbuffer, 100);
            break;
            //check
            case 'c':
               //clear the filename buffer (in case theres junk data in there)
               memset(filename, 0, sizeof(filename));
               //fill the filename buffer with the filename
               //as given in the argument of the client command
               for(int i = 2; readbuffer[i] != '\0' && readbuffer[i] != '\n'; i++){
                  filename[i - 2] = readbuffer[i];
               }
               //check if the file exists in the directory
               int fileExists = access(filename, F_OK);
               printf("filename is %s and is %d\n", filename, fileExists);
               //if it does (note access returns 0 if the file DOES exist)
               if(!fileExists){
                  //clear the buffer and write that the file exists
                  memset(readbuffer, 0, sizeof(readbuffer));
                  strcpy(readbuffer, "File ");
                  strcat(readbuffer, filename);
                  strcat(readbuffer, " exists");
               }
               //otherwise
               //(the file does not exist in the directory)
               else{
                  //clear the buffer and write that the file was not found
                  memset(readbuffer, 0, sizeof(readbuffer));
                  strcpy(readbuffer, "File ");
                  strcat(readbuffer, filename);
                  strcat(readbuffer, " not found");
               }
            break;
            //display
            case 'p':
            //download
            case 'd':
            //in both cases, the server side is identical
               //clear the filename buffer (in case theres junk data in there)
               memset(filename, 0, sizeof(filename));
               //fill the filename buffer with the filename
               //as given in the argument of the client command
               for(int i = 2; readbuffer[i] != '\0' && readbuffer[i] != '\n'; i++){
                  filename[i - 2] = readbuffer[i];
               }
               //if the file exists
               if(!access(filename, F_OK)){
                  //create a child of the child process
                  //with a pipe fd[0] connecting child and subchild
                  pipe(fd[0]);
                  //and then link the subchild->child pipe with the 
                  //child->parent pipe, bypassing the manual writing from child to parent
                  dup2(fd[1][1], fd[0][1]);
                  if(!fork()){   
                     //set the subchild->child pipe as the output for programs instead of the terminal
                     dup2(fd[0][1], STDOUT_FILENO);
                     //execute cat
                     //output is piped to parent
                     execlp("/bin/cat", "cat", filename, (char*)NULL);
			            exit(0);
                  }
                  //once the subchild is done
                  wait(NULL);
                  //end the child early. the pipe already has the buffer we want.
                  close(new_fd);
                  exit(0);
               }
               //if the file does not exist
               else{
                  //clear the buffer and write that the file was not found
                  memset(readbuffer, 0, sizeof(readbuffer));
                  strcpy(readbuffer, "File ");
                  strcat(readbuffer, filename);
                  strcat(readbuffer, " not found");
               }
            break;
         }
         //write the buffer to the parent-child pipe
         write(fd[1][1], readbuffer, 100);
         close(new_fd);
         exit(0);
		}
      //once the child is done
      wait(NULL);

      //NOTE: packets are 100 bytes. the first 99 are data and the last is always NULL
      //continuously transmit packets until the last one is read
      while(!lastPacket){
         //read the buffer that it sent back to the parent
         read(fd[1][0], readbuffer, 99);
         
         //the last packet is when the readbuffer ends with a NULL
         if(readbuffer[98] == '\0'){
            lastPacket = 1;
         }

         //printf("sending %s to client\n", readbuffer);
         //create a child
         if(!fork()){
            //send the packet to the client
			   if (send(new_fd, readbuffer, 100, 0) == -1)
				   perror("send");
            exit(0);
         }
         //once the child is done
         wait(NULL);
         
         /*
         for(int i = 0; i < 100; i++){
            printf("readbuffer[%d]=%c %d\n", i, readbuffer[i], readbuffer[i]);
         }
         */
      
         //clear packet
         memset(readbuffer, 0, sizeof(readbuffer));
     
      }
		close(new_fd);  // parent doesn't need this
      
   }

	return 0;
}


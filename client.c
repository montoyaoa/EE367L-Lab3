/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "3522" // the port client will be connecting to 

#define MAXDATASIZE 100 // max number of bytes we can get at once 

enum COMMANDS{LIST, CHECK, DISPLAY, DOWNLOAD};



// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc != 2) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}

   //100-char buffer to hold input commands
   //we only care about the first char
   char command[MAXDATASIZE];
   //infinite loop
   while(1){
      //prompt the user for input
      printf("Command (type 'h' for help): ");
      //store the input to the command buffer
      fgets(command, sizeof(command), stdin);

      //the first char in the command is the command type
      switch(command[0]){
         //list
         case 'l':
            ///////////////////////////socket setup///////////////////////////////////
            memset(&hints, 0, sizeof hints);
	         hints.ai_family = AF_UNSPEC;
	         hints.ai_socktype = SOCK_STREAM;

	         if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
		         fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		         return 1;
	         }

	         // loop through all the results and connect to the first we can
	         for(p = servinfo; p != NULL; p = p->ai_next) {
		         if ((sockfd = socket(p->ai_family, p->ai_socktype,
				      p->ai_protocol)) == -1) {
			         perror("client: socket");
			         continue;
		         }

		         if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			         perror("client: connect");
			         close(sockfd);
			         continue;
		         }

		         break;
	         }

	         if (p == NULL) {
		         fprintf(stderr, "client: failed to connect\n");
		         return 2;
	         }

	         inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			      s, sizeof s);
	         //printf("client: connecting to %s\n", s);

	         freeaddrinfo(servinfo); // all done with this structure
            //////////////////////////////////////////////////////////////////////////
            
            //send the list command to the server
            if((numbytes = send(sockfd, command, 1, 0)) != 1){
               perror("list");
               exit(1);
            }

            //recieve the server reply
	         if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
	            perror("recv");
	            exit(1);
	         }
            
            //parse and display the server reply
            buf[numbytes] = '\0';
            if(numbytes != 0){
	            printf("%s\n",buf);
               close(sockfd);
            }
         break;
         //check
         case 'c':
            //check that the command is entered correctly
            if(command[1] == ' '){
               ///////////////////////////socket setup///////////////////////////////////
               memset(&hints, 0, sizeof hints);
               hints.ai_family = AF_UNSPEC;
               hints.ai_socktype = SOCK_STREAM;

               if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
                  fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
                  return 1;
               }

               // loop through all the results and connect to the first we can
               for(p = servinfo; p != NULL; p = p->ai_next) {
                  if ((sockfd = socket(p->ai_family, p->ai_socktype,
                     p->ai_protocol)) == -1) {
                     perror("client: socket");
                     continue;
                  }

                  if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                     perror("client: connect");
                     close(sockfd);
                     continue;
                  }

                  break;
               }

               if (p == NULL) {
                  fprintf(stderr, "client: failed to connect\n");
                  return 2;
               }

               inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
                  s, sizeof s);
               //printf("client: connecting to %s\n", s);

               freeaddrinfo(servinfo); // all done with this structure
               //////////////////////////////////////////////////////////////////////////
                  
               //send the check command to the server
               if((numbytes = send(sockfd, command, MAXDATASIZE, 0)) == -1){
                  perror("check");
                  exit(1);
               }

               //recieve the server reply
	            if ((numbytes = recv(sockfd, buf, MAXDATASIZE, 0)) == -1) {
	               perror("recv");
	               exit(1);
	            }

               //parse and display the server reply
               if(numbytes != 0){
	               printf("%s\n",buf);
                  close(sockfd);
               }
            }
            else{
               printf("invalid command format\n");
            }
         break;
         //display
         case 'p':
            //check that the command is entered correctly
            if(command[1] == ' '){
               ///////////////////////////socket setup///////////////////////////////////
               memset(&hints, 0, sizeof hints);
               hints.ai_family = AF_UNSPEC;
               hints.ai_socktype = SOCK_STREAM;

               if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
                  fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
                  return 1;
               }

               // loop through all the results and connect to the first we can
               for(p = servinfo; p != NULL; p = p->ai_next) {
                  if ((sockfd = socket(p->ai_family, p->ai_socktype,
                     p->ai_protocol)) == -1) {
                     perror("client: socket");
                     continue;
                  }

                  if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                     perror("client: connect");
                     close(sockfd);
                     continue;
                  }

                  break;
               }

               if (p == NULL) {
                  fprintf(stderr, "client: failed to connect\n");
                  return 2;
               }

               inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
                  s, sizeof s);
               //printf("client: connecting to %s\n", s);

               freeaddrinfo(servinfo); // all done with this structure
               //////////////////////////////////////////////////////////////////////////

               //send the display command to the server
               if((numbytes = send(sockfd, command, MAXDATASIZE, 0)) == -1){
                  perror("display");
                  exit(1);
               }

               int lastPacket = 0;

               while(!lastPacket){
                  if ((numbytes = recv(sockfd, buf, MAXDATASIZE, 0)) == -1) {
	                  perror("recv");
	                  exit(1);
	               }
                  buf[99] = '\0';
                  
                  if(numbytes != 0){
                     printf("%s", buf);
                  }
                  if(buf[98] == '\0'){
                     lastPacket = 1;
                  }
	              memset(buf, 0, sizeof(buf)); 
               }
               close(sockfd);
               printf("\n");
            }
            else{
               printf("invalid command format\n");
            }
         break;
         //download
         case 'd':
            //check that the command is entered correctly
            if(command[1] == ' '){
               /*
               char filename[100];
               memset(filename, 0, sizeof(filename));
               for(int i = 2; command[i] != '\0' && command[i] != '\n'; i++){
                  filename[i - 2] = command[i];
               }

               if(!access(filename, F_OK)){
               }
               else{
               }
               */
               //TODO: Implement file download from server
            }
            else{
               printf("invalid command format\n");
            }
         break;
         //quit
         case 'q':
            close(sockfd);
            return 0;
            break;
         //help
         case 'h':
            printf("command list:\n");
            printf("l: list\n");
            printf("c: check <filename>\n");
            printf("p: display <filename>\n");
            printf("d: download <filename>\n");
            printf("q: quit\n");
            printf("h: help\n");
            break;
         //non-recognized command
         default:
            printf("invalid command\n");
      }

      //clear the command buffer 
      memset(command, 0, sizeof(command));
   }
	
   close(sockfd);

	return 0;
}


/*
 * proxy.c - A Simple Sequential Web proxy
 *
 * Course Name: 14:332:456-Network Centric Programming
 * Assignment 2
 * Student Name:______________________
 * 
 * IMPORTANT: Give a high level description of your code here. You
 * must also provide a header comment at the beginning of each
 * function that describes what that function does.
 */ 

#include "csapp.h"
/*
 * Globals
 */

FILE * fp;

pthread_mutex_t connectionmutex, filemutex = PTHREAD_MUTEX_INITIALIZER;

struct sockaddr_in clientaddr;	

struct arg_struct{
	int clientfd;
	struct sockaddr_in clientaddr;	
};

/*
 * Function prototypes
 */
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, int size);
void * thread_Handler(void * args);

/* 
 * main - Main routine for the proxy program 
 */
int main(int argc, char **argv)
{
	
	/* Check arguments */
    if (argc != 3) {
		fprintf(stderr, "Usage: %s <port number> <'t' for multithreaded version, 'p' for mulitprocess version>\n", argv[0]);
		exit(0);
    }
	if (strcmp(argv[2],"p") == 0){
		int port;
		int listenfd;
		int serverfd;
		int clientfd;
		int clientlen;
		char buffer[MAXBUF];
		
		int flags = O_RDWR | O_APPEND | O_CREAT;
		int fd = open("proxy.log", flags, S_IRWXU);
		rio_t rio;
		
		port = atoi(argv[1]);
	
		//socket(),bind(),listen() wrapper defined in csapp.c
		listenfd = Open_listenfd(port);
		
		//initiate child process
		pid_t pid;
		struct flock lock;
		//start the loop
		while(1){
			clientlen = sizeof(clientaddr);
			
			clientfd = Accept(listenfd, (SA*) &clientaddr, (socklen_t*) &clientlen);
			
			if((pid = fork()) == 0){
				//printf("enter child\n");
				Rio_readinitb(&rio,clientfd);
				Rio_readlineb(&rio,buffer,MAXBUF);
				
						
				//parse request
				char command[MAXLINE];
				char url[MAXLINE];
				char protocol[MAXLINE];
						
				sscanf(buffer, "%s %s %s", command, url, protocol);
								
				//extracting port number and domain name from url
				char * domainName;
				int serverport = 80;
				
				char * tempString = &url[0];
				char * tempString2;
				
				//we need to check if url has protocol exlicitly stated, if not skip if block
				if(strstr(tempString, "//") != NULL){
					tempString = strstr(tempString, "/");
					//remove the first 2 slashes  
					tempString += 2;
					tempString2 = strstr(tempString, "/");
					//remove the trailing slash
				}
				
				*tempString2 = 0;

				//if there is no ":", then tempString is just the domain name
				if((strstr(tempString, ":")) == NULL){

					domainName = tempString;
				}
				else{

					domainName = strtok(tempString, ":");
					tempString2 = strtok(NULL,":");
					serverport = atoi(tempString2);
				}
				printf("recieved: %s %s %s\n", command, url, protocol);
				printf("Server domain: %s\n", domainName);
				printf("Server port: %d\n\n", serverport);
				
				//now we send the request to the server
				serverfd = Open_clientfd(domainName,serverport);

				Rio_writen(serverfd, buffer, MAXBUF);

				
				int n;
				
				n = (int) Rio_readn(serverfd,buffer,MAXLINE);

				Rio_writen(clientfd,buffer,n);
				bzero(buffer, MAXBUF);
				
				//Set up to write to log
				char * logentry = (char *) Malloc(sizeof(char));
				format_log_entry(logentry, &clientaddr, url, n);
				
				//Wait for file to be unlocked by another process and lock
				lock.l_type = F_WRLCK;
				fcntl(fd,F_SETLKW,&lock);
				
				//write to end of file
				lseek(fd,0,SEEK_END);
				write(fd,logentry,strlen(logentry));
				printf("logged: %s\n\n", logentry);
				
				//unlock log file
				lock.l_type = F_UNLCK;
				fcntl(fd,F_SETLKW,&lock);
				
				free(logentry);
				close(clientfd);
				close(serverfd);
				exit(0);
			}
			
		}
	}
    if (strcmp(argv[2],"t")==0){
		int port;
		int listenfd;
		int clientfd;
		int clientlen;
		
		fp = Fopen("proxy.log","a");
		
		port = atoi(argv[1]);
	
		//socket(),bind(),listen() wrapper defined in csapp.c
		listenfd = Open_listenfd(port);
		
		pthread_t tid;
		//start the loop
		while(1){
			clientlen = sizeof(clientaddr);
			
			pthread_mutex_lock(&connectionmutex);
			clientfd = Accept(listenfd, (SA*) &clientaddr, (socklen_t*) &clientlen);
			pthread_mutex_unlock(&connectionmutex);
			struct arg_struct args;
			args.clientfd = clientfd;
			args.clientaddr = clientaddr;
			
			Pthread_create(&tid, NULL, thread_Handler, (void *) &args);
			Pthread_detach(tid);
		}
	}
	else{
		fprintf(stderr, "Usage: third argument must be 't' for multithreaded version or 'p' for mulitprocess version\n");
		exit(0);
	}
    exit(0);
    
}

/*
 * format_log_entry - Create a formatted log entry in logstring. 
 * 
 * The inputs are the socket address of the requesting client
 * (sockaddr), the URI from the request (uri), and the size in bytes
 * of the response from the server (size).
 */
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, 
		      char *uri, int size)
{
    
    time_t now;
    char time_str[MAXLINE];
    unsigned long host;
    unsigned char a, b, c, d;
	
    /* Get a formatted time string */
    now = time(NULL);
    
    strftime(time_str, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));
	
    /* 
     * Convert the IP address in network byte order to dotted decimal
     * form. Note that we could have used inet_ntoa, but chose not to
     * because inet_ntoa is a Class 3 thread unsafe function that
     * returns a pointer to a static variable (Ch 13, CS:APP).
     */

      
    host = ntohl(sockaddr->sin_addr.s_addr);
    
    a = host >> 24;
    b = (host >> 16) & 0xff;
    c = (host >> 8) & 0xff;
    d = host & 0xff;
	

    /* Return the formatted log entry string */
    sprintf(logstring, "%s: %d.%d.%d.%d %s %d\n", time_str, a, b, c, d, uri, size);
    
}

void * thread_Handler(void * args)
{
	struct arg_struct * arguments = args;
	int clientfd = arguments->clientfd;
	struct sockaddr_in address = arguments->clientaddr;	
	int serverfd;
	char buffer[MAXBUF];
	
	rio_t rio;
	
	Rio_readinitb(&rio,clientfd);
	Rio_readlineb(&rio,buffer,MAXBUF);
				
						
	//parse request
	char command[MAXLINE];
	char url[MAXLINE];
	char protocol[MAXLINE];
						
	sscanf(buffer, "%s %s %s", command, url, protocol);
								
	//extracting port number and domain name from url
	char * domainName;
	int serverport = 80;
				
	char * tempString = &url[0];
	char * tempString2;
				
	//we need to check if url has protocol exlicitly stated, if not skip if block
	if(strstr(tempString, "//") != NULL){
		tempString = strstr(tempString, "/");
		//remove the first 2 slashes  
		tempString += 2;
		tempString2 = strstr(tempString, "/");
		//remove the trailing slash
	}
				
	*tempString2 = 0;
	//if there is no ":", then tempString is just the domain name
	if((strstr(tempString, ":")) == NULL){

		domainName = tempString;
	}
	else{

		domainName = strtok(tempString, ":");
		tempString2 = strtok(NULL,":");
		serverport = atoi(tempString2);
	}
	printf("recieved: %s %s %s\n", command, url, protocol);
	printf("Server domain: %s\n", domainName);
	printf("Server port: %d\n\n", serverport);
				
	//now we send the request to the server
	serverfd = Open_clientfd(domainName,serverport);

	Rio_writen(serverfd, buffer, MAXBUF);

				
	int n;
				
	n = (int) Rio_readn(serverfd,buffer,MAXLINE);

	Rio_writen(clientfd,buffer,n);
	bzero(buffer, MAXBUF);
		
	char * logentry = (char *) Malloc(sizeof(char));
	
	pthread_mutex_lock(&filemutex);
	
	
	format_log_entry(logentry, &address, url, n);
			
	//write to log
	fprintf(fp,logentry,strlen(logentry));
	printf("logged: %s\n", logentry);
		
	fflush(fp);
	pthread_mutex_unlock(&filemutex);
	close(serverfd);
	close(clientfd);
	
	//Pthread_detach(Pthread_self());
	Pthread_exit(NULL);
	return 0;
}

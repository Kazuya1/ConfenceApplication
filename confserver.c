/*--------------------------------------------------------------------*/
/* conference server */

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

extern char * recvtext(int sd);
extern int sendtext(int sd, char *msg);

extern int startserver();
/*--------------------------------------------------------------------*/

struct arg{
    int fd;
    int* clients;
};

pthread_mutex_t lock;

void handle_thread(void* argument){
    /*
     FILL HERE
     wait using select() for
     messages from existing clients and
     connect requests from new clients
     */
    struct arg* tmp = (struct arg*)argument;
    int fd = tmp->fd;
    int i;
    
    pthread_mutex_lock(&lock);
    for(i=0;i<FD_SETSIZE;i++){
        if(tmp->clients[i]==-1){
            tmp->clients[i]=fd;
            break;
        }
    }
    pthread_mutex_unlock(&lock);
    
    /* look for messages from live clients */
    char * clienthost; /* host name of the client */
    ushort clientport; /* port number of the client */
    
    
    /*
     FILL HERE:
     figure out client's host name and port
     using getpeername() and gethostbyaddr()
     */
    struct sockaddr_in sa;
    unsigned int salen = sizeof(struct sockaddr_in);
    if(getpeername(fd, (struct sockaddr*)&sa, &salen)==-1){
        perror("getpeername");
        exit(1);
    }
    struct hostent* ret = gethostbyaddr((void*)&sa.sin_addr.s_addr, 4, AF_INET);
    clienthost = ret->h_name;
    clientport = ntohs(sa.sin_port);
    char* msg = NULL;
    /* read the message */
    while(1){
        msg = recvtext(fd);
        if (!msg) {
            /* disconnect from client */
            printf("admin: disconnect from '%s(%hu)'\n", clienthost,
                   clientport);
            
            pthread_mutex_lock(&lock);
            for(i=0;i<FD_SETSIZE;i++){
                if(tmp->clients[i]==fd){
                    tmp->clients[i]=-1;
                    break;
                }
            }
            pthread_mutex_unlock(&lock);
            
            /* close the socket */
            close(fd);
            break;
        } else {
            /*
             FILL HERE
             send the message to all live clients
             except the one that sent the message
             */
            for(i=0;i<FD_SETSIZE;i++){
                if(tmp->clients[i]!=-1&&tmp->clients[i]!=fd){
                    sendtext(tmp->clients[i], msg);
                }
            }
            
            
            /* display the message */
            printf("%s(%hu): %s", clienthost, clientport, msg);
            
            /* free the message */
            free(msg);
        }
    }
    pthread_exit(NULL);
}



/*--------------------------------------------------------------------*/
int fd_isset(int fd, fd_set *fsp) {
    return FD_ISSET(fd, fsp);
}
/* main routine */
int main(int argc, char *argv[]) {
    pthread_mutex_init(&lock, NULL);
    
    
    int servsock; /* server socket descriptor */
    
    /* check usage */
    if (argc != 1) {
        fprintf(stderr, "usage : %s\n", argv[0]);
        exit(1);
    }
    
    /* get ready to receive requests */
    servsock = startserver();
    if (servsock == -1) {
        perror("Error on starting server: ");
        exit(1);
    }
    
    /*
     FILL HERE:
     init the set of live clients
     */
    int client[FD_SETSIZE];
    int i;
    for (i = 0; i < FD_SETSIZE; i++) client[i] = -1;
        
    /* receive requests and process them */
    while (1) {
        pthread_t thread_id;
        /*
         FILL HERE:
         accept a new connection request
         */
        struct sockaddr_in sa;
        unsigned int salen = sizeof(struct sockaddr);
        int csd = accept(servsock,(struct sockaddr*)&sa,&salen);
        
        /* if accept is fine? */
        if (csd != -1) {
            char * clienthost; /* host name of the client */
            ushort clientport; /* port number of the client */
            /*
             FILL HERE:
             figure out client's host name and port
             using gethostbyaddr() and without using getpeername().
             */
            
            struct hostent* ret = gethostbyaddr((const void*)&sa.sin_addr, sizeof(struct in_addr), AF_INET);
            if(ret==NULL){
                perror("gethostbyaddr");
                exit(1);
            }
            clienthost = ret->h_name;
            clientport = ntohs(sa.sin_port);
            printf("admin: connect from '%s' at '%hu'\n", clienthost,
                   clientport);
            struct arg argument;
            argument.fd = csd;
            argument.clients = client;
            if(pthread_create(&thread_id,NULL,(void *)(&handle_thread),(void *)(&argument)) == -1){
                fprintf(stderr,"pthread_create error!\n");
                break;
            }
        } else {
            perror("accept");
            exit(0);
        }
    }
    return 0;
}
/*--------------------------------------------------------------------*/


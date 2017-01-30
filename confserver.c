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

extern char * recvtext(int sd);
extern int sendtext(int sd, char *msg);

extern int startserver();
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
int fd_isset(int fd, fd_set *fsp) {
    return FD_ISSET(fd, fsp);
}
/* main routine */
int main(int argc, char *argv[]) {
    int servsock; /* server socket descriptor */
    fd_set tmp;
    fd_set livesdset; /* set of live client sockets */
    int livesdmax; /* largest live client socket descriptor */
    
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
    int maxindex = -1;
    int i;
    for(i=0;i<FD_SETSIZE;i++) client[i] = -1;

    livesdmax = servsock;
    FD_ZERO(&livesdset);
    FD_SET(servsock, &livesdset);
        
    /* receive requests and process them */
    while (1) {
        int frsock; /* loop variable */
        tmp = livesdset;
        /*
         FILL HERE
         wait using select() for
         messages from existing clients and
         connect requests from new clients
         */
        if(select(livesdmax+1, &tmp, NULL, NULL, NULL)==-1){
            perror("select");
            exit(1);
        }
        /* look for messages from live clients */
        for (frsock = 3; frsock <= livesdmax; frsock++) {
            /* skip the listen socket */
            /* this case is covered separately */
            if (frsock == servsock)
                continue;
            
            if (fd_isset(frsock, &tmp)) {
                char * clienthost; /* host name of the client */
                ushort clientport; /* port number of the client */
                /*
                 FILL HERE:
                 figure out client's host name and port
                 using getpeername() and gethostbyaddr()
                 */
                struct sockaddr_in sa;
                unsigned int salen = sizeof(struct sockaddr_in);
                if(getpeername(frsock, (struct sockaddr*)&sa, &salen)==-1){
                    perror("getpeername");
                    exit(1);
                }
                struct hostent* ret = gethostbyaddr((void*)&sa.sin_addr.s_addr, 4, AF_INET);
                clienthost = ret->h_name;
                clientport = ntohs(sa.sin_port);
                char* msg = NULL;
                /* read the message */
                msg = recvtext(frsock);
                if (!msg) {
                    /* disconnect from client */
                    printf("admin: disconnect from '%s(%hu)'\n", clienthost,
                           clientport);
                    
                    /*
                     FILL HERE:
                     remove this guy from the set of live clients
                     */
                    FD_CLR(frsock, &livesdset);
                    for(i=0;i<FD_SETSIZE;i++){
                        if(frsock==client[i]){
                            client[i] = -1;
                        }
                    }
                    if(frsock==livesdmax){
                        livesdmax = servsock;
                        for(i=0;i<FD_SETSIZE;i++){
                            if(client[i]>livesdmax){
                                livesdmax = client[i];
                            }
                        }
                    }
                    
                    /* close the socket */
                    close(frsock);
                } else {
                    /*
                     FILL HERE
                     send the message to all live clients
                     except the one that sent the message
                     */
                    for(i=0;i<FD_SETSIZE;i++){
                        if(client[i]>0&&client[i]!=frsock){
                            sendtext(client[i], msg);
                        }
                    }
                    
                    /* display the message */
                    printf("%s(%hu): %s", clienthost, clientport, msg);
                    
                    /* free the message */
                    free(msg);
                }
            }
        }
        
        /* look for connect requests */
        if (fd_isset(servsock, &tmp)) {
            
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
                
                /*
                 FILL HERE:
                 add this guy to set of live clients
                 */
                for(i=0;i<FD_SETSIZE;i++){
                    if(client[i]<0){
                        client[i] = csd;
                        break;
                    }
                }
                if(i==FD_SETSIZE){
                    printf("Too many clients\n");
                    exit(1);
                }
                FD_SET(csd,&livesdset);
                if(i>maxindex) maxindex = i;
                if(csd>livesdmax) livesdmax = csd;
            } else {
                perror("accept");
                exit(0);
            }
        }
    }
    return 0;
}
/*--------------------------------------------------------------------*/


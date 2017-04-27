/*--------------------------------------------------------------------*/
/* conference client */

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

#define MAXMSGLEN  1024

extern char * recvtext(int sd);
extern int sendtext(int sd, char *msg);

extern int hooktoserver(char *servhost, ushort servport);
/*--------------------------------------------------------------------*/

void handle_thread(void* sock_fd){
    int sock = *((int*)sock_fd);
    while (1) {
        char msg[MAXMSGLEN];
        if (!fgets(msg, MAXMSGLEN, stdin))
            exit(0);
        sendtext(sock, msg);
        printf(">");
        fflush(stdout);
    }
}

/*--------------------------------------------------------------------*/
int main(int argc, char *argv[]) {
    int sock;
    int retval;
    
    /* check usage */
    if (argc != 3) {
        fprintf(stderr, "usage : %s <servhost> <servport>\n", argv[0]);
        exit(1);
    }
    
    /* get hooked on to the server */
    sock = hooktoserver(argv[1], atoi(argv[2]));
    if (sock == -1) {
        perror("Error: ");
        exit(1);
    }
    pthread_t thread_id;
    if(pthread_create(&thread_id,NULL,(void *)(&handle_thread),(void *)(&sock)) == -1){
        fprintf(stderr,"pthread_create error!\n");
        exit(1);
    }
    
    /* keep talking */
    while (1) {
        char *msg;
        msg = recvtext(sock);
        if (!msg) {
            /* server killed, exit */
            fprintf(stderr, "error: server died\n");
            exit(1);
        }
        /* display the message */
        printf(">>> %s", msg);
        /* free the message */
        free(msg);
        printf(">");
        fflush(stdout);
    }
    return 0;
}
/*--------------------------------------------------------------------*/


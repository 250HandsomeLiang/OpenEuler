#include "kernel/types.h"
#include "user.h"
int main(int argc ,char* argv[]){
    if(argc!=1){
        printf("pingpong error,it only needs one argument");
        exit(-1);
    }
    int p1[2];//0 read channel,1 write channel "ping"
    int p2[2];//"pong"
    pipe(p1);
    pipe(p2);
    char buffer[16];
    if(fork()==0){
        //child
        close(p1[1]);
        read(p1[0],buffer,16);
        int pid=getpid();//get child pid
        printf("%d: received %s\n",pid,buffer);
        close(p1[0]);

        close(p2[0]);
        write(p2[1],"pong",16);
        close(p2[1]);
    }else{
        //parent
        close(p1[0]);
        write(p1[1],"ping",16);
        close(p1[1]);

        close(p2[1]);
        read(p2[0],buffer,16);
        int pid=getpid();
        printf("%d: received %s\n",pid,buffer);
        close(p2[0]);
    }
    exit(0);   
}
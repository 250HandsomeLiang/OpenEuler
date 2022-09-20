#include "kernel/types.h"
#include "user.h"
void findPrime(int i,int fd[],int a);
int main(int argc,char* argv[]){
 if(argc!=1){
    printf("primes error,it should only have one argument");
    exit(-1);
 }  
    int fd[2]={0,0};
    findPrime(0,fd,0);
    exit(0);
}
void findPrime(int i,int fd[],int a){
    //i initial flag,fd file description ,a prime 
    //main process
    int p[2];
    char num[3];
    pipe(p);
    if(fork()==0){
        //child
        close(p[1]);
        char q[3];
        if(read(p[0],q,3)){
            printf("prime %d\n",atoi(q));
            findPrime(1,p,atoi(q));//create new process
        }
        close(p[0]);
    }
    else{
        //parent
        if(i==0){
            close(p[0]);
            for(int i=2;i<=35;i++){
                if(i>=10){
                    num[2]='\0';
                    num[1]=i%10+48;
                    num[0]=i/10+48;
                
                }else{
                    num[0]=i+48;
                    num[1]=num[2]='\0';
                }
                write(p[1],num,3);
            }
            close(p[1]);
        }else{
            close(p[0]);// to right
            close(fd[1]);// from left
            while (read(fd[0],num,3))
            {
                if((atoi(num))%a!=0){
                    write(p[1],num,3);
                }
            }
            close(fd[0]);
            close(p[1]);    
        }
        wait(0);
    }
    exit(0);
}


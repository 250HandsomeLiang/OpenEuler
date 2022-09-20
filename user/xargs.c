#include "kernel/types.h"
#include "user.h"
#include "kernel/param.h"
int main(int argc,char* argv[]){
    if(argc<2){
        printf("error\n");
        exit(-1);
    }
    char *buf[MAXARG];
    int i=0;
    for( i=0;i<argc;i++){
        buf[i]=argv[i];
    }
    buf[i]=0;
    char input[512];
    int p[2];

    while (read(0,input,512))
    {   
        pipe(p);
        if(fork()==0){
            close(p[1]);
            char data[512];
            read(p[0],data,512);
            int count=i;
            int len=0;
            char str[512];
            //spilt string by \n
            for(int t=0;t<strlen(data);t++){
                if((data[t]=='\n'&&t>0)||t==strlen(data)-1){
                    buf[count]=(char*)malloc(sizeof(char)*512);
                    strcpy(buf[count],str);
                    count++;
                    for(int i=0;i<512;i++){
                        str[i]='\0';
                    }
                    len=0;
                }else if(data[t]!='n'){
                    str[len]=data[t];
                    len++;
                }
            }
            
            char *inst[MAXARG];
            for(int j=1;j<=count-1;j++){
                inst[j-1]=buf[j];
            }
            inst[count-1]=0;
            exec(buf[1],inst);
            exit(0);   
        }
        else{
            close(p[0]);
            write(p[1],input,512);
            wait(0);
        }
    }
    close(p[1]);
    exit(0);
}
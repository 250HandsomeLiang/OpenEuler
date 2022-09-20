#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char*
fmtname(char *path)
{
  char *p;
  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  return p;
}
void
find(char *path,char *dest)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;
  char *dest_name;
  char *buf_name;
  if((fd = open(path, 0)) < 0){
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE:
    return;
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("find: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    dest_name=fmtname(dest);
    while(read(fd, &de, sizeof(de)) == sizeof(de)){//read file
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;//change the buf content
      if(stat(buf, &st) < 0){
        printf("find: cannot stat %s\n", buf);
        continue;
      }
      buf_name=fmtname(buf);
      if(!strcmp(buf_name,dest_name)){
        printf("%s/%s\n",path,dest);
      }else if(st.type==T_DIR&&strcmp(buf_name,".")&&strcmp(buf_name,"..")){
            //file name
            printf("path:%s\n",buf);
            char path_name[512];
            strcpy(path_name,buf);
            find(path_name,dest);
      }
    }
    break;
  }
  close(fd);
}


int
main(int argc, char *argv[])
{
  int i;

  if(argc < 2){
    printf("error,find should have three argument\n");
    exit(-1);
  }
  for(i=1; i<argc-1; i++)
    find(argv[i],argv[argc-1]);
  exit(0);
}

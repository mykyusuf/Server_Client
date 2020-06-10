#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <signal.h>
//#include <sys/inotify.h>

//#define EVENT_SIZE  ( sizeof (struct inotify_event) )
//#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

#define MAX 1024
#define SA struct sockaddr

int is_dir(const char *path);
int is_regular_file(const char *path);
int is_link(const char *path);
int is_pipe(const char *path);
void postOrderApply(char* path,int sockfd);
void signalHandler(int sig);
char* notify(char *notify_dir);
int sockfd;

long int size_func(char *path);

int copy_file(char sourcePath[],char destPath[]);

void func(int sockfd,char* path);

int main(int argc,char** argv)
{
    int connfd;
    struct sockaddr_in servaddr, cli;
    char cwd[256];
    char *ip=argv[2];
    int port=atoi(argv[3]);
    
    signal(SIGINT,signalHandler);
    signal(SIGTSTP,signalHandler);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));
    
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(ip);
    servaddr.sin_port = htons(port);
    
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        printf("connection with the server failed...\n");
        
        exit(0);
    }
    else{
        printf("connected to the server..\n");
        strcpy(cwd,argv[1]);
        func(sockfd,cwd);
        postOrderApply(cwd,sockfd);
        
        while(1){
            
            signal(SIGINT,signalHandler);
            signal(SIGTSTP,signalHandler);
            //strcpy(cwd,notify(cwd));
            //printf("MODİFİED ::: %s\n",cwd);
            //func(sockfd,cwd);
            
        }
        
    }
    
    close(sockfd);
    
}
void signalHandler(int sig){
    
    close(sockfd);
    
    printf("Exit by Signal!\n");
    exit(1);
    
}

void postOrderApply(char* path,int sockfd){
    
    struct dirent *de;
    char cwd[256];
    char old[256];
    int size1=0;
    int size2=0;
    int size3=0;
    int totalsize=0;
    int sum=0;
    
    strcpy(cwd,path);
    DIR *dr = opendir(cwd);
    strcpy(old,cwd);
    struct stat statbuf;
    if (dr == NULL)
    {
        
        printf("Could not open current directory -> %s",cwd);
        
    }
    
    else{
        while ((de = readdir(dr)) != NULL){
            
            
            strcat(cwd,"/");
            strcat(cwd,de->d_name);
            if(is_regular_file(cwd) && !is_link(cwd) && !is_pipe(cwd)){
                stat(cwd,&statbuf);
                
                func(sockfd,cwd);
                usleep(10000);
                
                //printf("%s\n",cwd);
                
            }
            else if(is_dir(cwd)){
                if(strcmp(de->d_name,".")!=0 && strcmp(de->d_name,"..")!=0){
                    stat(cwd,&statbuf);
                    //printf("%s\n",cwd);
                    
                    func(sockfd,cwd);
                    usleep(100);
                    postOrderApply(cwd,sockfd);
                    
                    
                }
            }

            strcpy(cwd,old);
        }
        
    }
    
    closedir(dr);
}
int is_regular_file(const char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}
int is_dir(const char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISDIR(path_stat.st_mode);
}
int is_link(const char *path)
{
    struct stat path_stat;
    lstat(path, &path_stat);
    return S_ISLNK(path_stat.st_mode);
}
int is_pipe(const char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISFIFO(path_stat.st_mode);
}
long int size_func(char *path){
    struct stat statbuf;
    if(stat(path, &statbuf) == -1){
        perror("Failed to get file status");
        return -1;
    }
    if(S_ISREG(statbuf.st_mode) > 0){
        return statbuf.st_size;
    }
    if(S_ISDIR(statbuf.st_mode) > 0){
        return 0;
    }
    return 0;
}
int copy_file(char sourcePath[],char destPath[]){
    
    char chr[size_func(sourcePath)];
    
    int fd, size, fdWrite, sizeWrite;
    fd=open(sourcePath,O_RDONLY);
    
    fdWrite=open(destPath,O_WRONLY|O_CREAT|O_TRUNC,0644);
    size=read(fd,chr,size_func(sourcePath));
    
    sizeWrite=write(fdWrite, chr, sizeof(chr));
    
    chr[size]='\0';
    
    
    close(fd);
    close(fdWrite);
    return 0;
    
}
void func(int sockfd,char* path)
{
    char buff[MAX];
    
    bzero(buff, sizeof(buff));
    strcpy(buff,path);
    
    if(is_dir(buff)){
        
        char *p="dir";
        
        write(sockfd,p,strlen(p));
        //printf("yollanan : %s\n",p);
        usleep(100);
        write(sockfd, buff, strlen(buff));
        //printf("yollanan : %s\n",buff);
        bzero(buff, sizeof(buff));
        usleep(100);
        
        
    }
    else if(is_regular_file(buff)){
        
        char *p="reg";
        char chr[1024];
        int fd;
        long long int size;
        
        write(sockfd,p,strlen(p));
        //printf("yollanan : %s\n",p);
        usleep(100);
        
        write(sockfd, buff, strlen(buff));
        //printf("yollanan : %s\n",buff);
        
        usleep(100);
        
        char str[20];
        size=size_func(buff);
        sprintf(str, "%lld", size);
        write(sockfd,str, strlen(str));
        
        usleep(100);
        
        fd=open(buff,O_RDONLY);
        
        for(int i=0;i<size/1024;i++){
            read(fd,chr,1024);
            write(sockfd,chr,1024);
            memset(chr,0,1024);
            usleep(100);
        }
        
        if(size%1024>0){
            read(fd,chr,(size%1024));
            write(sockfd,chr,(size%1024));
            memset(chr,0,1024);
            
        }
        close(fd);
        
        bzero(buff, sizeof(buff));
        
    }
    
}
/*
char* notify(char *notify_dir){
    int length, i = 0;
    int fd;
    int wd;
    char buffer[EVENT_BUF_LEN];
    fd = inotify_init();
    
    if ( fd < 0 ) {
        perror( "inotify_init" );
    }
    
    wd = inotify_add_watch( fd, notify_dir, IN_CREATE | IN_DELETE );
    length = read( fd, buffer, EVENT_BUF_LEN );
    if ( length < 0 ) {
        perror( "read" );
    }
    char  temp[PATH_MAX];
    strcpy(temp,notify_dir);
    while ( i < length ) {     struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];     if ( event->len ) {
        if ( event->mask & IN_CREATE ) {
            if ( event->mask & IN_ISDIR ) {
                strcat(temp,"/");
                strcat(temp,event->name);
                printf( "New directory %s created.\n", temp );
                char *temp_path=temp;
                return temp_path;
            }
            else {
                strcat(temp,"/");
                strcat(temp,event->name);
                printf( "New file %s created.\n", temp );
                char *temp_path=temp;
                return temp_path;
                
            }
        }
        else if ( event->mask & IN_DELETE ) {
            if ( event->mask & IN_ISDIR ) {
                strcat(temp,"/");
                strcat(temp,event->name);
                printf( "Directory %s deleted.\n", temp );
                char *temp_path=temp;
                return temp_path;
                
            }
            else {
                strcat(temp,"/");
                strcat(temp,event->name);
                printf( "File %s deleted.\n", temp );
                char *temp_path=temp;
                return temp_path;
                
            }
        }
    }
        i += EVENT_SIZE + event->len;
    }
    inotify_rm_watch( fd, wd );
    
    close( fd );
    return NULL;
    
}*/

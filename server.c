#include<stdio.h>
#include<string.h>    //strlen
#include<stdlib.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
#include<pthread.h> //for threading , link with lpthread
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>

void *connection_handler(void *);
int is_dir(const char *path);
int is_regular_file(const char *path);
int is_link(const char *path);
int is_pipe(const char *path);
void signalHandler(int sig);
int socket_desc , client_sock;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

int main(int argc , char *argv[])
{
    int c;
    struct sockaddr_in server , client;
    int t_size=0,port;
    
    mkdir(argv[1],0777);
    t_size=atoi(argv[2]);
    port=atoi(argv[3]);
    
    signal(SIGINT,signalHandler);
    signal(SIGTSTP,signalHandler);
    
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( port );

    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {

        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");

    listen(socket_desc , 3);

    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
    
    pthread_t thread_id[t_size];
    int i=0;
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        puts("Connection accepted");
        if(i>(t_size-1)){
            for(int j=0;j<i;j++){
                pthread_join(thread_id[j] , NULL);
            }
            i=0;
        }
        
        if( pthread_create( &thread_id[i] , NULL ,  connection_handler , &client_sock) < 0)
        {
            perror("could not create thread");
            return 1;
        }
        else{
            i++;
        }
        
        puts("Handler assigned");
    }
    
    
    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }
    
    
    close(socket_desc);
    close(client_sock);
    
    return 0;
}

void *connection_handler(void *socket_desc)
{
    int sock = *(int*)socket_desc;
    int read_size;
    char client_message[1024];
    FILE *f;
    FILE *fp;
    signal(SIGINT,signalHandler);
    signal(SIGTSTP,signalHandler);

    while( (read_size = recv(sock , client_message , 1024 , 0)) > 0 )
    {
        
        pthread_mutex_lock(&lock);

        client_message[read_size]='\0';
        
        //printf("%s",client_message);
        
        if(!strcmp(client_message,"dir")){
            
            memset(client_message, 0, 1024);
            
            read_size=recv(sock , client_message , 1024 , 0);
            client_message[read_size]='\0';
            
            char x[1024];
            getcwd(x,sizeof(x));
            strcat(x,"/serverside/");
            strcat(x,client_message);
            fp = fopen ("serverside/log.txt", "a");
            fprintf(fp, "%s olusturuldu\n", x);
            fclose(fp);
            //printf("%s\n",x);
            mkdir(x,0777);
            
            memset(client_message, 0, 1024);
            memset(x, 0, 1024);
            
        }
        
        else if(!strcmp(client_message,"reg")){
            
            int fd;
            long long int size=0,len;
            memset(client_message, 0, 1024);
            
            read_size=recv(sock , client_message , 1024 , 0);
            client_message[read_size]='\0';
            
            char x[1024];
            getcwd(x,sizeof(x));
            strcat(x,"/serverFolder/");
            strcat(x,client_message);
            fp = fopen ("serverFolder/log.txt", "a");
            fprintf(fp, "%s olusturuldu\n", x);
            fclose(fp);
            //printf("%s\n",x);
            memset(client_message, 0, 1024);
            
            read_size=recv(sock , client_message , 1024 , 0);
            len = strlen(client_message);
            
            for(int i=0; i<len; i++){
                size = size * 10 + ( client_message[i] - '0');
            }
            
            //printf("*** %lld ***\n",size);
            
            memset(client_message, 0, 1024);


            
            fd=open(x,O_WRONLY| O_CREAT |O_TRUNC,0644);

            for(int i=0;i<size/1024;i++){
                recv(sock , client_message , 1024 , 0);
                write(fd, client_message, 1024);
                memset(client_message, 0, 1024);
                
            }
            if(size%1024>0){

                recv(sock , client_message , (size%1024) , 0);
                write(fd, client_message, (size%1024));
                
            }
            close(fd);

            memset(client_message, 0, 1024);
            
            memset(x, 0, 1024);
            
        }
        
        
        memset(client_message, 0, 1024);
        
        pthread_mutex_unlock(&lock);
    
    }
    if(read_size == 0)
    {
        puts("Client disconnected");
        fflush(stdout);
    }
    else if(read_size == -1)
    {
        perror("recv failed");
    }

    pthread_exit(NULL);
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
void signalHandler(int sig){
    close(socket_desc);
    close(client_sock);
    printf("Exit by Signal!\n");
    exit(1);
}

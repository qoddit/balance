#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#define SERVER_BUF 8192
#define MAX_BODY (100U*1024U*1024U)
static volatile sig_atomic_t stop_flag=0;
static void on_sig(int s){(void)s;stop_flag=1;}
static void usage(void){fprintf(stderr,"balance-server --bind HOST:PORT --data DIR [--token TOKEN]\n");}
static int safe_id(const char*s){if(!s||!*s||strlen(s)>128U)return 0;for(size_t i=0;s[i];++i)if(!((s[i]>='a'&&s[i]<='z')||(s[i]>='A'&&s[i]<='Z')||(s[i]>='0'&&s[i]<='9')||s[i]=='-'||s[i]=='_'))return 0;return 1;}
static int sendall(int fd,const char*b,size_t n){while(n){ssize_t w=send(fd,b,n,MSG_NOSIGNAL);if(w<=0)return -1;b+=w;n-=(size_t)w;}return 0;}
static void reply(int fd,int code,const char*body){char h[512];const char*msg=code==200?"OK":code==201?"Created":code==404?"Not Found":code==401?"Unauthorized":"Bad Request";int n=snprintf(h,sizeof(h),"HTTP/1.1 %d %s\r\nContent-Length: %zu\r\nConnection: close\r\nContent-Type: application/octet-stream\r\n\r\n",code,msg,strlen(body));sendall(fd,h,(size_t)n);sendall(fd,body,strlen(body));}
int main(int argc,char**argv){const char*bind_host="127.0.0.1";int port=8080;const char*data="./balance-server-data";const char*token=NULL;for(int i=1;i<argc;++i){if(strcmp(argv[i],"--bind")==0&&i+1<argc){char tmp[256];snprintf(tmp,sizeof(tmp),"%s",argv[++i]);char*c=strrchr(tmp,':');if(!c){usage();return 2;}*c='\0';bind_host=strdup(tmp);port=atoi(c+1);}else if(strcmp(argv[i],"--data")==0&&i+1<argc)data=argv[++i];else if(strcmp(argv[i],"--token")==0&&i+1<argc)token=argv[++i];else{usage();return 2;}}
    mkdir(data,0700);signal(SIGINT,on_sig);signal(SIGTERM,on_sig);int s=socket(AF_INET,SOCK_STREAM,0);if(s<0)return 1;int one=1;setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&one,sizeof(one));struct sockaddr_in a={0};a.sin_family=AF_INET;a.sin_port=htons((uint16_t)port);if(inet_pton(AF_INET,bind_host,&a.sin_addr)!=1){fprintf(stderr,"bind host must be IPv4 literal in this minimal server; use reverse proxy for DNS/TLS\n");return 2;}if(bind(s,(struct sockaddr*)&a,sizeof(a))<0||listen(s,32)<0)return 1;
    while(!stop_flag){int fd=accept(s,NULL,NULL);if(fd<0){if(errno==EINTR)continue;break;}char b[SERVER_BUF+1];ssize_t n=recv(fd,b,sizeof(b)-1,0);if(n<=0){close(fd);continue;}b[n]='\0';char method[16],path[256];if(sscanf(b,"%15s %255s",method,path)!=2){reply(fd,400,"bad request");close(fd);continue;}char *auth=strstr(b,"Authorization:");if(token){char expected[768];snprintf(expected,sizeof(expected),"Authorization: Bearer %s",token);if(!auth||strncmp(auth,expected,strlen(expected))!=0){reply(fd,401,"unauthorized");close(fd);continue;}}
        if(strncmp(path,"/v1/blob/",9)!=0){reply(fd,404,"not found");close(fd);continue;}const char*id=path+9;if(!safe_id(id)){reply(fd,400,"bad id");close(fd);continue;}char filepath[1024];snprintf(filepath,sizeof(filepath),"%s/%s.blob",data,id);
        char *cl=strstr(b,"Content-Length:");size_t len=0;if(cl)len=(size_t)strtoull(cl+15,NULL,10);if(len>MAX_BODY){reply(fd,400,"too large");close(fd);continue;}char *body=strstr(b,"\r\n\r\n");size_t got=body?(size_t)(n-(body+4-b)):0;body=body?body+4:NULL;
        if(strcmp(method,"PUT")==0){int out=open(filepath,O_WRONLY|O_CREAT|O_TRUNC,0600);if(out<0){reply(fd,500,"io");close(fd);continue;}if(body&&got)write(out,body,got);size_t left=len>got?len-got:0;while(left){char buf[8192];ssize_t r=recv(fd,buf,left>sizeof(buf)?sizeof(buf):left,0);if(r<=0)break;write(out,buf,(size_t)r);left-=(size_t)r;}fsync(out);close(out);if(left==0)reply(fd,201,"stored");else reply(fd,400,"truncated");}
        else if(strcmp(method,"GET")==0){
            int in=open(filepath,O_RDONLY);
            if(in<0){reply(fd,404,"not found");close(fd);continue;}
            struct stat st;
            if(fstat(in,&st)!=0){close(in);reply(fd,500,"io");close(fd);continue;}
            char h[256];
            int hn=snprintf(h,sizeof(h),"HTTP/1.1 200 OK\r\nContent-Length: %lld\r\nConnection: close\r\nContent-Type: application/octet-stream\r\n\r\n",(long long)st.st_size);
            sendall(fd,h,(size_t)hn);
            char buf[8192];
            ssize_t r;
            while((r=read(in,buf,sizeof(buf)))>0) sendall(fd,buf,(size_t)r);
            close(in);
        } else {
            reply(fd,400,"unsupported");
        }
        close(fd);
    }close(s);return 0;}

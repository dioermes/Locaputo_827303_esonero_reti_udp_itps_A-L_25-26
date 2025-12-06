#include "protocol.h"

#if defined WIN32
#include <winsock.h>
#endif

void usage(const char *p) {
    printf("Uso: %s [-s server] [-p port] -r \"type city\"\n",p);
}

void capitalizza(char *s) {
    int cap=1;
    for(int i=0;s[i];i++) {
        if(s[i]==' ') cap=1;
        else if(cap) { s[i]=toupper(s[i]); cap=0; }
        else s[i]=tolower(s[i]);
    }
}

int main(int argc,char *argv[]) {
#if defined WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2),&wsa);
#endif

    char server_name[256]="localhost";
    int port=SERVER_PORT;
    weather_request_t req={0};
    int found=0;

    for(int i=1;i<argc;i++) {
        if(strcmp(argv[i],"-s")==0 && i+1<argc) strcpy(server_name,argv[++i]);
        else if(strcmp(argv[i],"-p")==0 && i+1<argc) port=atoi(argv[++i]);
        else if(strcmp(argv[i],"-r")==0 && i+1<argc) {
            found=1;
            char *s=argv[++i];

            if(strlen(s)<3){usage(argv[0]);return 1;}
            req.type=s[0];
            if(s[1]!=' ') { printf("Errore: tipo non valido\n"); return 1; }

            int j=1; while(s[j]==' ') j++;
            if(!s[j]) { usage(argv[0]); return 1; }

            strncpy(req.city,&s[j],63);
            req.city[63]='\0';

            if(strlen(req.city)>=64) {
                printf("Errore: nome citt� troppo lungo\n");
                return 1;
            }
        }
    }

    if(!found){usage(argv[0]);return 1;}

    // DNS
    struct hostent *h;
    struct in_addr addr;
    char ip[16];
    char name[256];

    if(inet_addr(server_name)!=INADDR_NONE) {
        addr.s_addr=inet_addr(server_name);
        strcpy(ip,server_name);
        h=gethostbyaddr((char*)&addr,sizeof(addr),AF_INET);
        strcpy(name,(h)?h->h_name:server_name);
    } else {
        h=gethostbyname(server_name);
        if(!h){printf("Errore DNS\n");return 1;}
        strcpy(name,h->h_name);
        addr=*((struct in_addr*)h->h_addr_list[0]);
        strcpy(ip,inet_ntoa(addr));
    }

    int sock=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);

    struct sockaddr_in srv;
    srv.sin_family=AF_INET;
    srv.sin_port=htons(port);
    srv.sin_addr=addr;

    // Serialize request
    char buf[REQUEST_SIZE];
    memcpy(buf,&req.type,1);
    memcpy(buf+1,req.city,64);

    sendto(sock,buf,REQUEST_SIZE,0,(struct sockaddr*)&srv,sizeof(srv));

    // Receive
    char rbuf[RESPONSE_SIZE];
    struct sockaddr_in from;
    int flen=sizeof(from);
    int n=recvfrom(sock,rbuf,RESPONSE_SIZE,0,(struct sockaddr*)&from,&flen);
    if(n<=0) { printf("Errore ricezione\n"); return 1; }

    weather_response_t resp;
    int off=0;
    uint32_t st;
    memcpy(&st,rbuf+off,sizeof(uint32_t));
    resp.status=ntohl(st);
    off+=sizeof(uint32_t);

    memcpy(&resp.type,rbuf+off,1);
    off++;

    uint32_t tmp;
    memcpy(&tmp,rbuf+off,sizeof(tmp));
    tmp=ntohl(tmp);
    memcpy(&resp.value,&tmp,sizeof(float));

    capitalizza(req.city);

    printf("Ricevuto risultato dal server %s (ip %s). ",name,ip);

    if(resp.status==STATUS_OK) {
        if(resp.type=='t')
            printf("%s: Temperatura = %.1f�C\n",req.city,resp.value);
        else if(resp.type=='h')
            printf("%s: Umidit� = %.1f%%\n",req.city,resp.value);
        else if(resp.type=='w')
            printf("%s: Vento = %.1f km/h\n",req.city,resp.value);
        else if(resp.type=='p')
            printf("%s: Pressione = %.1f hPa\n",req.city,resp.value);
    } else if(resp.status==STATUS_CITY_NOT_FOUND) {
        printf("Citt� non disponibile\n");
    } else {
        printf("Richiesta non valida\n");
    }

    closesocket(sock);
#if defined WIN32
    WSACleanup();
#endif
}

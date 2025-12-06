#include "protocol.h"

#if defined WIN32
#include <winsock.h>
#else
#include <signal.h>
#endif



int server_socket = -1;

void clearwinsock() {
#if defined WIN32
    WSACleanup();
#endif
}

// Meteo
float get_temperature() { return -10.0f + (rand()/(float)RAND_MAX)*50.0f; }
float get_humidity()    { return 20.0f + (rand()/(float)RAND_MAX)*80.0f; }
float get_wind()        { return (rand()/(float)RAND_MAX)*100.0f; }
float get_pressure()    { return 950.0f + (rand()/(float)RAND_MAX)*100.0f; }

// Verifica città
int is_valid_city(const char *city) {
    const char *cities[] = {"Bari","Roma","Milano","Napoli","Torino",
                            "Palermo","Genova","Bologna","Firenze","Venezia"};
    for(int i=0;i<10;i++) {
        if(strcasecmp(city,cities[i])==0) return 1;
    }
    return 0;
}

// Caratteri vietati
int has_invalid_chars(const char *s) {
    for(int i=0;s[i];i++) {
        if(s[i]=='\t'||s[i]=='@'||s[i]=='#'||s[i]=='$'||
           s[i]=='%'||s[i]=='&'||s[i]=='*'||s[i]=='!'||s[i]=='?')
            return 1;
    }
    return 0;
}

// Processa richiesta
void process_request(weather_request_t *req, weather_response_t *resp) {
    resp->type = req->type;

    if(req->type!='t'&&req->type!='h'&&req->type!='w'&&req->type!='p') {
        resp->status = STATUS_INVALID_REQUEST;
        resp->value = 0;
        return;
    }

    if(has_invalid_chars(req->city)) {
        resp->status = STATUS_INVALID_REQUEST;
        return;
    }

    if(!is_valid_city(req->city)) {
        resp->status = STATUS_CITY_NOT_FOUND;
        return;
    }

    resp->status = STATUS_OK;

    switch(req->type) {
    case 't': resp->value = get_temperature(); break;
    case 'h': resp->value = get_humidity(); break;
    case 'w': resp->value = get_wind(); break;
    case 'p': resp->value = get_pressure(); break;
    }
}

int main(int argc, char *argv[]) {
    int port = SERVER_PORT;

    for(int i=1;i<argc;i++) {
        if(strcmp(argv[i],"-p")==0 && i+1<argc) {
            port = atoi(argv[++i]);
        }
    }

#if defined WIN32
    WSADATA wsa;
    if(WSAStartup(MAKEWORD(2,2), &wsa)!=0) return 1;
#endif

    srand(time(NULL));

    server_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(server_socket<0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in server_addr, client_addr;
    int client_len;

    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(server_socket,(struct sockaddr*)&server_addr,sizeof(server_addr))<0) {
        perror("bind");
        return 1;
    }

    printf("Server in ascolto sulla porta %d...\n",port);

    while(1) {
        char req_buf[REQUEST_SIZE];
        char resp_buf[RESPONSE_SIZE];
        weather_request_t req;
        weather_response_t resp;

        client_len = sizeof(client_addr);

        int n = recvfrom(server_socket,req_buf,REQUEST_SIZE,0,
                         (struct sockaddr*)&client_addr,&client_len);
        if(n<=0) continue;

        // Deserialize
        memcpy(&req.type, req_buf, 1);
        memcpy(req.city, req_buf+1, 64);
        req.city[63]='\0';

        struct hostent *h = gethostbyaddr((char*)&client_addr.sin_addr,
                                          sizeof(client_addr.sin_addr),
                                          AF_INET);
        char *client_name = (h)?h->h_name:"unknown";

        printf("Richiesta ricevuta da %s (ip %s): type='%c', city='%s'\n",
               client_name, inet_ntoa(client_addr.sin_addr),
               req.type, req.city);

        process_request(&req,&resp);

        // Serialize
        int off=0;
        uint32_t net_status = htonl(resp.status);
        memcpy(resp_buf+off,&net_status,sizeof(uint32_t));
        off+=sizeof(uint32_t);

        memcpy(resp_buf+off,&resp.type,1);
        off+=1;

        uint32_t tmp;
        memcpy(&tmp,&resp.value,sizeof(float));
        tmp=htonl(tmp);
        memcpy(resp_buf+off,&tmp,sizeof(float));

        sendto(server_socket,resp_buf,RESPONSE_SIZE,0,
               (struct sockaddr*)&client_addr,client_len);
    }
}

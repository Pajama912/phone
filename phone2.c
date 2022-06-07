#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define N 4096

void die(char *s){ perror(s); exit(1);};
void server(int port, char *option);
void clnt(const char *address, int port, char *option);
void print_option();
int main(int argc, char **argv){
    

    if (argc < 2) die("Input port number or adress");
    if (argc == 2){
        if ( strcmp( argv[1], "option" ) == 0) print_option();
        else{
            short read_data[1];
            while(read(0, read_data, sizeof(short))) write(1, read_data, sizeof(short));

            int port = atoi( argv[1] );
            char *option = "n";
            printf("option: none\n\n");
            server(port, option);
        }
    }
    
    else if (argc == 3){

        if ( strcmp(argv[2], "r") * strcmp(argv[2], "v") == 0){
            int port = atoi( argv[1] );
            char *option = argv[2];
            if ( strcmp(argv[2], "r") == 0) printf("option: record\n\n");
            if ( strcmp(argv[2], "v") == 0) printf("option: voice change\n\n");
            server(port, option);
        }
        else{
            char *address = argv[1];
            int port = atoi( argv[2] );
            char *option = "n"; 
            printf("option: none\n\n");
            clnt(address, port, option);
        }
        
    }
    
    else if(argc == 4){

        char *option = argv[3];
        char *address = argv[1];
        int port = atoi( argv[2] );
        clnt(address, port, option);

    } 

}

void server(int port, char *option){
        
        int srv_s = socket(PF_INET, SOCK_STREAM, 0);
        if (srv_s == -1) die("server socket error");
        struct sockaddr_in srv_addr;
        srv_addr.sin_family = AF_INET;
        srv_addr.sin_addr.s_addr = INADDR_ANY;
        srv_addr.sin_port = htons(port);
        
        if( bind(srv_s, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) == -1 ) die("bind error");
        
        printf("Waiting for clients\n");

        int lstn = listen(srv_s, 10);
        if(  lstn == 0 ) {
            printf("listening\n");
        }
        else die("listen error");

        struct sockaddr_in clnt_addr;
        socklen_t len = sizeof(struct sockaddr_in);

        // short read_music[1];

        // int cs = -1;
        // while( cs == -1) {
        //     cs = accept(srv_s, (struct sockaddr *)&clnt_addr, &len);
        //     // read(0, read_music, sizeof(short));
        //     // write(1, read_music, sizeof(short));
        //     printf("hurry!\n");
        // }
        int cs = accept(srv_s, (struct sockaddr *)&clnt_addr, &len);
        // if (cs > 0 ) printf("%d %d\n", cs, len);
        if (cs == -1) die("accept error");
        close(srv_s);

        FILE *rec_pipe = popen("rec -t raw -b 16 -c 1 -e s -r 44100 -", "r");
        if ( rec_pipe == NULL ) die ("wrong pipe command");  

        FILE *play_pipe = popen("play -t raw -b 16 -c 1 -e s -r 44100 -", "w");
        if ( strcmp(option, "v") == 0) play_pipe = popen("play -t raw -b 16 -c 1 -e s -r 66150 -", "w");
        if ( play_pipe == NULL ) die ("wrong pipe command");  
        
        short read_data[N], send_data[N];

        if ( strcmp(option , "r") == 0){ // if option "r", recorded.raw would be created
            int recorded = open("recorded.raw", O_WRONLY | O_CREAT | O_TRUNC, 0644);

            while(1){ 
                int send_r = fread(send_data, sizeof(short), N, rec_pipe); //read my voice
                if (send_r == -1) die("fread_server");
                int send_w = write(cs, send_data, sizeof(short)*N); //send recorded voice
                if (send_w == -1) die("write_server");

                if (send_r == 0) break;

                int recv_r = read(cs, read_data, sizeof(short)*N); //recieve oponent's voice
                if (recv_r == -1) die("recieve_server");
                int recv_w = fwrite(read_data, sizeof(short), N, play_pipe); //play oponent's voice
                if (recv_w == -1) die("fwrite_server");

                if (recv_r == 0) break;
            
                // write(recorded, send_data, sizeof(short)*N);
                write(recorded, read_data, sizeof(short)*N);
            }          
            close(recorded);
        }

        else{
            while(1){ 
                int send_r = fread(send_data, sizeof(short), N, rec_pipe); //read my voice
                if (send_r == -1) die("fread_server");
                int send_w = write(cs, send_data, 2*N); //send recorded voice
                if (send_w == -1) die("write_server");

                if (send_r == 0) break;

                int recv_r = read(cs, read_data, sizeof(short)*N); //recieve oponent's voice
                if (recv_r == -1) die("recieve_server");
                int recv_w = fwrite(read_data, sizeof(short), N, play_pipe); //play oponent's voice
                if (recv_w == -1) die("fwrite_server");

                if (recv_r == 0) break;
            } 
        }
           
        pclose(rec_pipe);
        pclose(play_pipe);
        close(cs);
}

void clnt(const char*address, int port, char *option){
    
        int clnt_s = socket(PF_INET, SOCK_STREAM, 0);
        if (clnt_s == -1) die("client socket error");
        struct sockaddr_in clnt_addr;
        clnt_addr.sin_family = AF_INET;
        int t = inet_aton(address, &clnt_addr.sin_addr);
        if (t == 0) die("address error");
        clnt_addr.sin_port = htons( port );
        if (connect(clnt_s, (struct sockaddr *)&clnt_addr,sizeof(clnt_addr)) == -1) die("connect error");

        FILE *rec_pipe = popen("rec -t raw -b 16 -c 1 -e s -r 44100 -", "r");
        if ( rec_pipe == NULL ) die ("wrong pipe command");  
        FILE *play_pipe = popen("play -t raw -b 16 -c 1 -e s -r 44100 -", "w");
        if ( strcmp(option, "v") == 0) play_pipe = popen("play -t raw -b 16 -c 1 -e s -r 66150 -", "w");
        if ( play_pipe == NULL ) die ("wrong pipe command");  

        short read_data[N], send_data[N];

        if ( strcmp(option , "r") == 0){ // if option "r", recorded.raw would be created

            int recorded = open("recorded.raw", O_WRONLY | O_CREAT | O_TRUNC, 0644);

            while(1){ 
                int send_r = fread(send_data, sizeof(short), N, rec_pipe); //read my voice
                if (send_r == -1) die("fread_server");
                int send_w = write(clnt_s, send_data, sizeof(short)*N); //send recorded voice
                if (send_w == -1) die("write_server");

                if (send_r == 0) break;

                int recv_r = read(clnt_s, read_data, sizeof(short)*N); //recieve oponent's voice
                if (recv_r == -1) die("recieve_server");
                int recv_w = fwrite(read_data, sizeof(short), N, play_pipe); //play oponent's voice
                if (recv_w == -1) die("fwrite_server");

                if (recv_r == 0) break;
            
                // write(recorded, send_data, sizeof(short)*N);
                write(recorded, read_data, sizeof(short)*N);
            }          
            close(recorded);
        }
        
        else{
            while(1){ 
                int send_r = fread(send_data, sizeof(short), N, rec_pipe); //read my voice
                if (send_r == -1) die("fread_client");
                int send_w = write(clnt_s, send_data, 2*N); //send recorded voice
                if (send_w == -1) die("write");
                if (send_r == 0) break; 

                int recv_r = read(clnt_s, read_data, sizeof(short)*N); //recieve oponent's voice
                if (recv_r == -1) die("recieve_client");
                int recv_w = fwrite(read_data, sizeof(short), N, play_pipe); //play oponent's voice
                if (recv_w == -1) die("fwrite_clinet");
            
                if (recv_r == 0) break;
            }
        }
        pclose(rec_pipe);
        pclose(play_pipe);
        close(clnt_s);
}

void print_option(){
    printf("n: no option");
    printf("r: record\n");
    printf("v: voice change\n");
    printf("\nex) if server: ./phone <port> <option>\n");
    printf("ex) if client: ./phone <address> <port> <option>\n");
}
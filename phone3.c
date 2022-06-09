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
#include <math.h>
#define N 1024
#define s1 44100
#define end_time 1
#define noise_max 10000
void die(char *s){ perror(s); exit(1);};
void server(int port, char *option);
void clnt(const char *address, int port, char *option);
void print_option();
void play_coloring();
int main(int argc, char **argv){
    if (argc < 2) print_option();
    else if (argc == 2){
        if ( strcmp( argv[1], "h" ) == 0) print_option();
        if(atoi(argv[1]) >= 10000 && atoi(argv[1]) <= 65535){
            int port = atoi( argv[1] );
            char *option = "n";
            printf("option: none\n\n");
            server(port, option);
        } else {
            print_option();
            printf("\n---port number should be a number from 10000 to 65535---\n");
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
        if ( strcmp(argv[2], "r") * strcmp(argv[2], "v") == 0) {
            int port = atoi( argv[1] );
            char *option = "all";
            printf("option: record and voice change\n");
            server(port, option);
        }
        else {
            char *address = argv[1];
            int port = atoi( argv[2] );
            char *option = argv[3];
            clnt(address, port, option);
        }
    } 
    else {
        char *address = argv[1];
        int port = atoi( argv[2] );
        char *option = "all";
        clnt(address, port, option);
    }
}
void server(int port, char *option){
        
        play_coloring();
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
        // FILE *read_music = popen("play -b 16 -c 1 -e s -r 44100 merry_go_round", "r");
        int cs = accept(srv_s, (struct sockaddr *)&clnt_addr, &len);
        if (cs != -1) printf("accpted");
        else die("accept error");
        // fclose(read_music);
        close(srv_s);
        FILE *rec_pipe = popen("rec -t raw -b 16 -c 1 -e s -r 44100 -", "r");
        if ( rec_pipe == NULL ) die ("wrong pipe command");  
        FILE *play_pipe = popen("play -t raw -b 16 -c 1 -e s -r 44100 -", "w");
        if ( strcmp(option, "v") * strcmp(option , "all") == 0) {
            play_pipe = popen("play -t raw -b 16 -c 1 -e s -r 66150 -", "w");
        }
        if ( play_pipe == NULL ) die ("wrong pipe command");  
        
        short read_data[N], send_data[N];
        if ( strcmp(option , "r") * strcmp(option , "all") == 0 ){ // if option "r", recorded.raw would be created
            int recorded = open("recorded.raw", O_WRONLY | O_CREAT | O_TRUNC, 0644);
            short recorded_data[N];
            long sum_send, sum_read, sum_all;
            while(1){ 
                sum_send = 0;
                sum_read = 0;
                sum_all = 0;
                int send_r = fread(send_data, sizeof(short), N, rec_pipe); //read my voice
                if (send_r == -1) die("frea_server");
                int send_w = write(cs, send_data, sizeof(short)*N); //send recorded voice
                if (send_w == -1) die("write_server");
                if (send_r == 0) break;
                int recv_r = read(cs, read_data, sizeof(short)*N); //recieve oponent's voice
                if (recv_r == -1) die("recieve_server");
                int recv_w = fwrite(read_data, sizeof(short), N, play_pipe); //play oponent's voice
                if (recv_w == -1) die("fwrite_server");
                if (recv_r == 0) break;
                
                for (short i = 0 ; i < N ; i++){
                    sum_send += abs(send_data[i]);
                    sum_read += abs(read_data[i]);
                    //printf("%f %f\n", sum_send , sum_read);
                    // recorded_data[i] = (send_data[i] + 10*read_data[i]) / 11 ;
                } 
                
                int include[2] = {1, 1};
                if(sum_send < noise_max) include[0] = 0;
                if(sum_read < noise_max) include[1] = 0;
                sum_all = sum_read + sum_send;
    
                if(include[0] * include[1] == 1){
                    for(short i = 0; i < N; ++i){
                        recorded_data[i] = (sum_read * send_data[i] + sum_send * read_data[i]) / sum_all;
                    }
                } 
                else {
                    for(short i = 0; i < N; ++i) {
                        recorded_data[i] = include[0] * send_data[i] + include[1] * read_data[i];                    
                    }                    
                }
                write(recorded, recorded_data, sizeof(short)*N);
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
        printf("call terminated");
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
        if ( strcmp(option, "v") * strcmp(option , "all") == 0) {
            play_pipe = popen("play -t raw -b 16 -c 1 -e s -r 66150 -", "w");
        }
        if ( play_pipe == NULL ) die ("wrong pipe command");  
        short read_data[N], send_data[N];
        if ( strcmp(option , "r") * strcmp(option , "all") == 0){ // if option "r", recorded.raw would be created
            int recorded = open("recorded.raw", O_WRONLY | O_CREAT | O_TRUNC, 0644);
            short recorded_data[N];
            long sum_send, sum_read, sum_all;
            while(1){ 
                sum_send = 0;
                sum_read = 0;
                sum_all = 0;
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
            
                // for (short i = 0 ; i < N ; i++) recorded_data[i] = (send_data[i] + read_data[i]) / 2 ;
                for (short i = 0 ; i < N ; i++){
                    sum_send += abs(send_data[i]);
                    sum_read += abs(read_data[i]);
                    // printf("%f %f\n", sum_send , sum_read);
                    //recorded_data[i] = (send_data[i] + 10*read_data[i]) / 11 ;
                } 
                
                int include[2] = {1, 1};
                if(sum_send < noise_max) include[0] = 0;
                if(sum_read < noise_max) include[1] = 0;
                sum_all = sum_read + sum_send;
    
                if(include[0] * include[1] == 1){
                    for(short i = 0; i < N; ++i){
                        recorded_data[i] = (sum_read * send_data[i] + sum_send * read_data[i]) / sum_all;
                        //recorded_data[i] = (10*send_data[i] + read_data[i]) / 11 ;
                    }
                } 
                else {
                    for(short i = 0; i < N; ++i) {
                        recorded_data[i] = include[0] * send_data[i] + include[1] * read_data[i];                    
                    }                    
                }
                write(recorded, recorded_data, sizeof(short)*N);
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
        printf("call terminated");
        close(clnt_s);
}
void print_option(){
    printf("n: no option\n");
    printf("r: record\n");
    printf("v: voice change\n");
    printf("\nex) if server: ./phone <port> <option1> <option2>...\n");
    printf("ex) if client: ./phone <address> <port> <option1> <option2>...\n");
}
void play_coloring() {
    FILE *read_music = popen("cat merry_go_round80.raw", "r");
    FILE *write_music = popen("play -t raw -b 16 -c 1 -e s -r 44100 -", "w");
    short read_data[1];
    int i = 0;
    while(i < s1*end_time){
        if( fread(read_data, 1, sizeof(short), read_music) == 0) break;
        fwrite(read_data, 1, sizeof(short), write_music);
        ++i;
    } 
}
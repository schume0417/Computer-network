#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <netinet/ip.h>
#include <sys/time.h>
//#include <netinet/ip_icmp.h>

#define BACKLOG 20

int main(int argc, char const *argv[]){

  int status;
  struct addrinfo hints;
  struct addrinfo *servinfo; // 將指向結果
  struct sockaddr_storage their_addr;  // 連線者的位址資訊 
  socklen_t addr_size;
  memset(&hints, 0, sizeof(hints)); // 確保 struct 為空
  hints.ai_family = AF_UNSPEC; // 不用管是 IPv4 或 IPv6
  hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
  hints.ai_flags = AI_PASSIVE; // 幫我填好我的 IP 
  int sockfd;
  int forClientSockfd;

  if ((status = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
  fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
  exit(1);
  }

  sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

  //setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

  bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);

  //connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen); client做的

  //freeaddrinfo(servinfo);

  listen(sockfd, BACKLOG);

  int fdmax;
  fdmax = sockfd;

  fd_set readset;
  FD_ZERO(&readset);

  FD_SET(sockfd, &readset);

  while(1){

    fd_set workset;
    FD_ZERO(&workset);
    memcpy(&workset, &readset, sizeof(readset));
    select(fdmax+1, &workset, NULL, NULL, NULL);
    //printf("running\n");

    for (int i = 0; i <= fdmax; i++){
      if (FD_ISSET(i, &workset)) { // 我們找到一個！！
        if (i == sockfd) {
          // handle new connections
          addr_size = sizeof(their_addr);
          forClientSockfd = accept(sockfd,(struct sockaddr*) &their_addr, &addr_size);
          FD_SET(forClientSockfd, &readset); // 新增到 readset
          if (forClientSockfd > fdmax){ // 持續追蹤最大的 fd
            fdmax = forClientSockfd;
          }
          //printf("getting new client\n");
        } 
        else {
          // 處理來自 client 的資料
          //printf("getting data\n");
          int nbytes;
          char inputBuffer[256] = {};
          char message[] = {"Hi,this is server.\n"};
          if ((nbytes = recv(i,inputBuffer,sizeof(inputBuffer),0)) <= 0) {
            // got error or connection closed by client
            //printf("bye\n");
            close(i); // bye!
            FD_CLR(i, &readset); // 從 master set 中移除
          } 
          else {
            send(i,message,sizeof(message),0);
            //printf("Get:%s\n",inputBuffer);
            int port;
            char ipstr[INET6_ADDRSTRLEN];
            getpeername(i, (struct sockaddr*) &their_addr, &addr_size);
            struct sockaddr_in *s = (struct sockaddr_in *)&their_addr;
            port = ntohs(s->sin_port);
            inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof(ipstr));
            printf("recv from %s:%d\n",ipstr,port);
          }
        } // END handle data from client
      } // END got new incoming connection
    }
  }
	return 0;
}





/*

// ICMP报文数据结构
struct icmp{
    unsigned char           type;      // 类型
    unsigned char           code;      // 代码
    unsigned short          checksum;  // 校验和
    unsigned short          id;        // 标识符
    unsigned short          sequence;  // 序号 
    struct timeval  timestamp; // 时间戳
};

unsigned short checkSum(unsigned short *addr, int len){
    unsigned int sum = 0;  
    while(len > 1){
        sum += *addr++;
        len -= 2;
    }

    // 处理剩下的一个字节
    if(len == 1){
        sum += *(unsigned char *)addr;
    }

    // 将32位的高16位与低16位相加
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);

    return (unsigned short) ~sum;
}

void pack(struct icmp * icmp, int sequence){
    icmp->type = 8;
    icmp->code = 0;
    icmp->checksum = 0;
    icmp->id = getpid();
    icmp->sequence = sequence;
    gettimeofday(&icmp->timestamp, 0);
    icmp->checksum = checkSum((unsigned short *)icmp, sizeof(struct icmp));
}








char buf[1024] = {0};
recvfrom(forClientSockfd, buf, 1024, 0, (struct sockaddr *)&their_addr, 0);

struct icmp sendicmp;
memset(&sendicmp, 0, sizeof(struct icmp));
pack(&sendicmp, 0);
sendto(forClientSockfd, &sendicmp, sizeof(struct icmp), 0, (struct sockaddr *)&servinfo, sizeof(servinfo));
*/

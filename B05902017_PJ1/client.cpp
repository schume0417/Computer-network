#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
//#include <netinet/ip_icmp.h>

int main(int argc, char const *argv[]){
  int n = 0;
  double timeout = 1000;
  int flag = 0;

  struct timeval time_out;      
  time_out.tv_sec = 1;
  time_out.tv_usec = 0;


  if (argc >= 4){
    for (int i = 1; i < argc; i++){
      if (strcmp(argv[i],"-n") == 0){
        n = atof(argv[i+1]);
      } 
      if (strcmp(argv[i],"-t") == 0){
        timeout = atof(argv[i+1]);
        time_out.tv_sec = 0;
        time_out.tv_usec = timeout * 1000;
      }
    }
  }   // 了解spec

  if (n > 0){
  //  printf("flag set\n");
    flag = 1;
  }

  //printf("%d\n%f\n",n,timeout );
  struct addrinfo hints;
  struct addrinfo *servinfo;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  for (int i = 1; i < argc; i++){
    char *host;
    char *port;
    char str[100];
    strcpy(str,argv[i]);
    
    char delim[2] = ":";
    host = strtok(str, delim);
    //printf("%s\n",host);
    port = strtok(NULL, delim);
    //printf("%s\n",port );
    int pid;
    if (port != NULL){
    if ( (pid = fork()) == 0){
      getaddrinfo(host, port, &hints, &servinfo); //127.0.0.1 //切割string
	
	    int sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

      setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&time_out,sizeof(time_out));

      int success;

      success = connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);

      //printf("%d\n", success);

      if (success < 0){
        char ipstr[INET6_ADDRSTRLEN];
        void *addr;
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)servinfo->ai_addr; 
        addr = &(ipv4->sin_addr);
        inet_ntop(servinfo->ai_family, addr, ipstr, sizeof(ipstr));
        printf("timeout when connect to %s\n", ipstr);
      }

      while(1){
        if (flag == 1){
          if (n == 0){
            //printf("end_of_child\n");
            _exit(0);
          }
          n -= 1;
        }
        clock_t t1, t2;
        char message[] = {"Hi there"};
        char receiveMessage[1024] = {};
        t1 = clock();
        //printf("before send\n");
        send(sockfd,message,sizeof(message),0);
        //printf("after send\n");
        char ipstr[INET6_ADDRSTRLEN];
        void *addr;
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)servinfo->ai_addr; 
        addr = &(ipv4->sin_addr);
        inet_ntop(servinfo->ai_family, addr, ipstr, sizeof(ipstr));

        if (recv(sockfd,receiveMessage,sizeof(receiveMessage),0) <= 0){
          printf("timeout when connect to %s\n", ipstr);
        }  //recieve不到也要印超時)
        else{
        t2 = clock();
        //printf("after recieve\n");
        //printf("%s",receiveMessage);  // hi this is server 
        //if ((t2-t1) * 1000/(double)(CLOCKS_PER_SEC) > timeout){
        //  printf("timeout when connect to %s\n", ipstr);
        //}
          printf("recv from %s, RTT = %lfmsec\n", ipstr , (t2-t1) * 1000/(double)(CLOCKS_PER_SEC));
        }
      }
      //printf("end_of_child\n");
      _exit(0);
    }
    else{
      continue;
    }
    }
  }
	
  sleep(1);
  //printf("end_of_parent\n");

	return 0;
}




//fd_set readset;
//FD_ZERO(&readset);













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

struct ip{
    // 主机字节序判断
    #if __BYTE_ORDER == __LITTLE_ENDIAN
    unsigned char   hlen:4;        // 首部长度
    unsigned char   version:4;     // 版本      
    #endif
    unsigned char   tos;             // 服务类型
    unsigned short  len;             // 总长度
    unsigned short  id;                // 标识符
    unsigned short  offset;            // 标志和片偏移
    unsigned char   ttl;            // 生存时间
    unsigned char   protocol;       // 协议
    unsigned short  checksum;       // 校验和
    struct in_addr ipsrc;    // 32位源ip地址
    struct in_addr ipdst;   // 32位目的ip地址
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

float timediff(struct timeval *begin, struct timeval *end){
    int n;
    // 先计算两个时间点相差多少微秒
    n = ( end->tv_sec - begin->tv_sec ) * 1000000 + ( end->tv_usec - begin->tv_usec );
    // 转化为毫秒返回
    return (float) (n / 1000);
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

int unpack(char * buf, int len, char * addr){
   int i, ipheadlen;
   struct ip * ip;
   struct icmp * icmp;
   float rtt;          // 记录往返时间
   struct timeval end; // 记录接收报文的时间戳

   ip = (struct ip *)buf;

   // 计算ip首部长度，即ip首部的长度标识乘4
   ipheadlen = ip->hlen << 2;

   // 越过ip首部，指向ICMP报文
   icmp = (struct icmp *)(buf + ipheadlen);

   // ICMP报文的总长度
   len -= ipheadlen;

   // 计算往返时间
   gettimeofday(&end, 0);
   rtt = timediff(&icmp->timestamp, &end);

   // 打印ttl，rtt，seq
   printf("%d bytes from %s : icmp_seq=%u ttl=%d rtt=%fms \n",
           len, addr, icmp->sequence, ip->ttl, rtt);
   return 0;
}


  struct icmp sendicmp;
  char buf[1024] = {0};
  struct sockaddr_in from;
  memset(&from, 0, sizeof(struct sockaddr_in));
  memset(&sendicmp, 0, sizeof(struct icmp));
  pack(&sendicmp,0);
    // 发送报文
  sendto(sockfd, &sendicmp, sizeof(struct icmp), 0, (struct sockaddr *)&addr, sizeof(addr));

  recvfrom(sockfd, buf, 1024, 0, (struct sockaddr *)&addr, 0);
    
  unpack(buf, n, inet_ntoa(from.sin_addr));
*/


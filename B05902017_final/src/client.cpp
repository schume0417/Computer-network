#include "protocol.hpp"
#include "client_cmd_handler.hpp"

User user;
mutex std_mtx; //處理stdout, stderr同時使用的mutex
mutex sock_mtx; //處理send, recv同時使用的mutex
mutex in_mtx;
char Sock_Buf[SOCK_BUF_SIZE];
int Buf_Pointer = 0;

int main(int argc, char **argv){
	memset(Sock_Buf, 0, sizeof(Sock_Buf));
	if(argc != 3){
		cerr<<"usage: ./client [host] [port]\n";
		exit(-1);
	}
	int sockfd = 0;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1){
		cerr<<"Fail to create a socket.\n";
		exit(0);
	}

	struct sockaddr_in info;
	bzero(&info, sizeof(info));
	info.sin_family = PF_INET;

	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if(getaddrinfo(argv[1], argv[2], &hints, &res) != 0){
		cerr<<"get info error.\n";
		exit(1);
	}

	string s_ip = inet_ntoa(((struct sockaddr_in *)res->ai_addr)->sin_addr);
	info.sin_addr.s_addr = inet_addr(s_ip.c_str());
	info.sin_port = htons(atoi(argv[2]));

	if(connect(sockfd, (struct sockaddr *)&info, sizeof(info)) == -1){
		cerr<<"Connection error.\n";
		exit(2);
	}
	
	thread stdin_handle(cmd_handle, sockfd);

	while(1){
		char recv_buf[sizeof(Request)];
		memset(recv_buf, 0, sizeof(Request));

		sock_mtx.lock();
		ssize_t recv_val = recv(sockfd, recv_buf, sizeof(Request), MSG_DONTWAIT);
		sock_mtx.unlock();
		if(recv_val <= 0){
			continue;
		}
		memcpy(&Sock_Buf[Buf_Pointer], recv_buf, recv_val);
		Buf_Pointer += recv_val;
/*
		cerr<<"recv_val = "<<recv_val<<"\n";
		cerr<<"Buf_Pointer = "<<Buf_Pointer<<"\n";
*/
		bool Start_Correct = false, End_Correct = false;
		char start_cmp[4], end_cmp[4];
		memset(start_cmp, START_MARK, MARK_SIZE);
		memset(end_cmp, END_MARK, MARK_SIZE);

		if(memcmp(Sock_Buf, start_cmp, MARK_SIZE) == 0)
			Start_Correct = true;
		if(memcmp(&Sock_Buf[sizeof(Request)-MARK_SIZE], end_cmp, MARK_SIZE) == 0)
			End_Correct = true;
		if(Start_Correct && End_Correct){
			Request *recv_msg = new Request(0, 0, 0);

			memcpy(recv_msg, Sock_Buf, sizeof(Request));
			req_handle(recv_msg, sockfd);

			delete recv_msg;

			char tmp_buf[SOCK_BUF_SIZE - sizeof(Request)];
			memset(tmp_buf, 0, sizeof(tmp_buf));
			memcpy(tmp_buf, &Sock_Buf[sizeof(Request)], sizeof(tmp_buf));
			memset(Sock_Buf, 0, sizeof(Sock_Buf));
			memcpy(Sock_Buf, tmp_buf, sizeof(tmp_buf));

			Buf_Pointer -= sizeof(Request);
		}
	}
	stdin_handle.join();
	close(sockfd);
	
	return 0;
}

#include "protocol.hpp"
#include "command_handler.hpp"

unordered_map<int, ClientInfo> Client;   // fd 對client資訊 大寫的是我們自己定義的
unordered_map<string, int> Name_Table;   // client名字對fd
unordered_map<string, string> Name_Pw_map;  //client名字對password
unordered_map<string, vector<string>> Friend;
unordered_map<string, vector<string>> History;
unordered_map<string, vector<pair<string, string>>> Offline_Msg;//儲存傳給不在線上的好友的訊息,第一個欄位存的是要給的人, pair第一個欄位是來自誰
unordered_map<string, string> Personal_Signature;
unordered_map<string, vector<string>> Weird_people;

mutex c_mtx, nt_mtx, np_mtx, fr_mtx, ht_mtx, off_mtx, std_mtx;

void thread_stdin(){
	while(1){
		string cmd;
		cin>>cmd;
		if(cmd == "Name_Table"){
			std_mtx.lock();
			for(auto &it : Name_Table)
				cout<<"name: "<<it.first<<", fd: "<<it.second<<"\n";
			std_mtx.unlock();
		}
		else if(cmd == "Client"){
			std_mtx.lock();
			for(auto &it : Client){
				cout<<"fd: "<<it.first<<"\n";
				cout<<"IP: "<<it.second.IP<<", st: "<<it.second.status<<", name: "<<it.second.name<<"\n";
			}
			std_mtx.unlock();
		}
	}
	
}

void client_handle(int fd){
	char Sock_Buf[SOCK_BUF_SIZE];
	int Buf_Pointer = 0;
	memset(Sock_Buf, 0, sizeof(Sock_Buf));
	while(1){
		char recv_buf[sizeof(Request)];
		memset(recv_buf, 0, sizeof(Request));
		ssize_t recv_val = recv(fd, recv_buf, sizeof(Request), 0);
		
		if(recv_val <= 0){
			if(recv_val == 0){
				cerr<<Client[fd].IP<<" is closed.\n";
				if(Client[fd].status == ONLINE){
					nt_mtx.lock();
					Name_Table.erase(Client[fd].name);
					nt_mtx.unlock();
				}
				c_mtx.lock();
				Client.erase(fd);
				c_mtx.unlock();
				close(fd);
				return;
			}
			return;
		}
		memcpy(&Sock_Buf[Buf_Pointer], recv_buf, recv_val);
		Buf_Pointer += recv_val;

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
			Request ans = cmd_handle(fd, recv_msg);
			ssize_t ret_val = send(fd, &ans, sizeof(Request), 0);
			assert(ret_val == sizeof(Request));
			delete recv_msg;

			char tmp_buf[SOCK_BUF_SIZE - sizeof(Request)];
			memset(tmp_buf, 0, sizeof(tmp_buf));
			memcpy(tmp_buf, &Sock_Buf[sizeof(Request)], sizeof(tmp_buf));
			memset(Sock_Buf, 0, sizeof(Sock_Buf));
			memcpy(Sock_Buf, tmp_buf, sizeof(tmp_buf));

			Buf_Pointer -= sizeof(Request);

		}

	}

}

int main(int argc, char **argv){
	

	vector<thread> for_client;
	int sockfd = 0;
	int max_fd = 0;
	//unordered_map<int, string> client_ip;
	//unordered_map<string, string> name_pw_map;

	if(argc < 2){
		cerr<<"usage: ./server <listen_port>\n";
		exit(0);
	}

	const char *PORT = argv[1];

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1){
		cerr<<"Fail to create a socket.\n";
		exit(1);
	}
	
	struct sockaddr_in serverInfo;
	bzero(&serverInfo, sizeof(serverInfo));

	serverInfo.sin_family = PF_INET;
	serverInfo.sin_addr.s_addr = INADDR_ANY;
	serverInfo.sin_port = htons(atoi(argv[1]));
	int a = ::bind(sockfd, (struct sockaddr *)&serverInfo, sizeof(serverInfo));
	if (a != 0){
		cerr<<"bind failed!\n";
		exit(2);
	}

	assert(listen(sockfd, 500) == 0);

	ifstream db("database.txt");

	if(db.is_open()){
		string line;
		while(getline(db, line)){
			string name = line.substr(0, line.find(":"));
			string pw = line.substr(line.find(":") + 1);
			cout<<name<<", "<<pw<<"\n";
			Name_Pw_map[name] = pw;
		}
	}
	db.close();

	ifstream fr("friend.txt");

	if(fr.is_open()){
		string line;
		string user = "";
		while(getline(fr, line)){
			string user = line.substr(0, line.find(":"));
			string user_friend = line.substr(line.find(":") + 1);
			Friend[user].push_back(user_friend);
		}
	}
	fr.close();

	ifstream ht("history.txt");

	if(ht.is_open()){
		string line;
		string user = "";
		while(getline(ht, line))
			if(line.find("User: ") != string::npos)
				user = line.substr(6);
			else
				History[user].push_back(line);
	}
	ht.close();

	ifstream off_msg("offline_msg.txt");

	if(off_msg.is_open()){
		string line;
		string to_user = "";
		while(getline(off_msg, line))
			if(line.find("User: ") != string::npos)
				to_user = line.substr(6);
			else{
				string from = line.substr(0, line.find(":"));
				string m = line.substr(line.find(":")+1);
				Offline_Msg[to_user].push_back(make_pair(from, m));
			}
	}

	off_msg.close();

	ifstream wp("weird_people.txt");

	if(wp.is_open()){
		string line;
		string user = "";
		while(getline(fr, line)){
			string user = line.substr(0, line.find(":"));
			string user_friend = line.substr(line.find(":") + 1);
			Weird_people[user].push_back(user_friend);
		}
	}

	wp.close();

	thread server_stdin(thread_stdin);
	server_stdin.detach();

	while(1){
		struct sockaddr_in clientInfo;
		socklen_t addrlen = sizeof(clientInfo);
		int client_fd = accept(sockfd, (struct sockaddr *)&clientInfo, &addrlen);
		if(client_fd < 0)
			cerr<<"client "<<client_fd<<" accept failed.\n";
		else{
			string c_ip = inet_ntoa(clientInfo.sin_addr);
			string c_port = to_string(ntohs(clientInfo.sin_port));
			c_mtx.lock();
			Client[client_fd].IP = c_ip + ":" + c_port;
			Client[client_fd].status = OFFLINE;
			Client[client_fd].name = "";
			cerr<<"client: " + Client[client_fd].IP + " connection succeeded!\n";
			c_mtx.unlock();

			thread for_client(client_handle, client_fd);
			for_client.detach();
		}
	}
	
	close(sockfd);
	return 0;
}

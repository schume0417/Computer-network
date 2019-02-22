#include "client_cmd_handler.hpp"
#include "protocol.hpp"

void my_cout(string str){
	std_mtx.lock();
	cout<<str;
	cout.flush();
	std_mtx.unlock();
}

void my_cerr(string str){
	std_mtx.lock();
	cerr<<str;
	std_mtx.unlock();
}

string rot13(string str){
	string rot;	
	for (int i = 0; i < str.length() ; i++){
		char a;
		a = str[i];
		if (islower(a)){
			a -= 'a';
			a += 13;
			if (a >= 26){
				a = a %26;
			}
			a += 'a';
		}
		else if (isupper(a)){
			a -= 'A';
			a += 13;
			if (a >= 26){
				a = a %26;
			}
			a += 'A';
		}
		rot += a;
	}

	return rot;
}

void send_file(int sockfd, string file_name, string target){
    ifstream fi(file_name, fstream::in | fstream::binary);
    if (fi.is_open()){
    	fi.seekg (0, fi.end);
    	int total_size = fi.tellg();
    	fi.seekg (0, fi.beg);
    	int count = 0;
    	int SIZE = DATA_MAX - 3*NAME_LEN_MAX;
    	while (count != total_size){
        	char buffer[SIZE];
        	memset(buffer,0,sizeof(buffer));
        	Request req(CLIENT, OP_SENDFILE, ST_REQ, user.getName());
        	fi.read(buffer,SIZE);
        	int get_size = fi.gcount();
        	count += get_size;

        	if(get_size == SIZE) //還沒傳完
            	req.header.status = ST_SENDING;
        	else if(get_size < (SIZE) || count == total_size)//傳完了
            	req.header.status = ST_SEND_FIN;
            
        	req.setSendFile(file_name, target, buffer, get_size);
	    	sock_mtx.lock();
	    	int retval = send(sockfd, &req, sizeof(req), 0);
	    	sock_mtx.unlock();
			usleep(1000);
	    	assert(retval == sizeof(req));
	    
    	}
    }
    else{
    	my_cout("no corresponding file\n");
    }
	fi.close();
    
	return;
}

void cmd_handle(int sockfd){
	while(1){
		if (user.getStatus() == OFFLINE)
			my_cout("select your option, 1 for register, 2 for login\n");
		if (user.getStatus() == ONLINE)
			my_cout("1 for send message, 2 for add friend, 3 for get history,  4 for send file, 5 to logout, 6 to edit personal information, 7 to put someone in blacklist, 8 to list friend\n");
		string cmd;
		cin>>cmd;
		if( (cmd == "register"|| cmd == "1") && user.getStatus() == OFFLINE){
			string n, pw, repw;
			my_cout("Name: \n");
			cin>>n;
			my_cout("Password: \n");
			cin>>pw;
			my_cout("Please type your password again: \n");
			cin>>repw;

			Request req(CLIENT, OP_REGISTER, ST_REQ);
			std_mtx.lock();
			int success = req.setRegister(n, pw, repw);
			std_mtx.unlock();

			if(success){
				sock_mtx.lock();
				int retval = send(sockfd, &req, sizeof(req), 0);
				sock_mtx.unlock();
				assert(retval == sizeof(req));
			}
		}
		else if( (cmd == "login" || cmd == "2") && user.getStatus() == OFFLINE ){
			string n, pw;
			my_cout("Name: \n");
			cin>>n;
			my_cout("Password: \n");
			cin>>pw;
			Request req(CLIENT, OP_LOGIN, ST_REQ, user.getName());
			
			std_mtx.lock();
			int success = req.setLogin(n, pw);
			std_mtx.unlock();
			
			if(success){
				user.setName(n);
				sock_mtx.lock();
				int retval = send(sockfd, &req, sizeof(req), 0);
				sock_mtx.unlock();
				assert(retval == sizeof(req));
			}
		}
		else if( user.getStatus() == OFFLINE ){
			my_cout("Please register or login\n");
		}
		//照理講下面的函式應該要是登入後才能使用的
		//先假設一次send完
		else if( (cmd == "send" || cmd == "1") && user.getStatus() == ONLINE){
			string n, msg;
			my_cout("Name: \n");
			cin>>n;
			my_cout("Message: \n");
			cin.ignore(100, '\n');
			getline(cin, msg);

			msg = rot13(msg);

			Request req(CLIENT, OP_SEND, ST_REQ, user.getName());
			std_mtx.lock();
			int success = req.setSend(n, msg);
			std_mtx.unlock();

			if(success){
				sock_mtx.lock();
				int retval = send(sockfd, &req, sizeof(req), 0);
				sock_mtx.unlock();
				assert(retval == sizeof(req));
			}
		}
		else if ( (cmd == "add" || cmd == "2") && user.getStatus() == ONLINE ){
			string n,fr;
			n = user.getName();
			my_cout("Friend: \n");
			cin>>fr;
			Request req(CLIENT, OP_ADDFRIEND, ST_REQ, user.getName());
			std_mtx.lock();
			int success = req.setAddFriend(n, fr);
			std_mtx.unlock();

			if(success){
				sock_mtx.lock();
				int retval = send(sockfd, &req, sizeof(req), 0);
				sock_mtx.unlock();
				assert(retval == sizeof(req));
			}
		}
		else if ( (cmd == "history" || cmd == "3") && user.getStatus() == ONLINE ){
			Request req(CLIENT, OP_HISTORY, ST_REQ, user.getName());
			sock_mtx.lock();
			int retval = send(sockfd, &req, sizeof(req), 0);
			sock_mtx.unlock();
			assert(retval == sizeof(req));
		}
		else if ( (cmd == "file" || cmd == "4") && user.getStatus() == ONLINE ){
 			string file_name, target;
 			my_cout("Name: ");
			cin >> target;
			if(target.length() > NAME_LEN_MAX - 1){
                my_cerr(string("the length of name can be up to ") + to_string(NAME_LEN_MAX - 1) + string("\n"));
                return;
            }
             
			my_cout("File Name: ");
			cin >> file_name;
			if(file_name.length() > NAME_LEN_MAX - 1){
                my_cerr(string("the length of file name can be up to ") + to_string(NAME_LEN_MAX - 1) + string("\n"));
                return;
            }
            
 			thread thread_send_file(send_file, sockfd, file_name, target);
 			thread_send_file.detach();
 		}
 		else if ( (cmd == "logout" || cmd == "5") && user.getStatus() == ONLINE ){
 			Request req(CLIENT, OP_LOGOUT, ST_REQ, user.getName());
			
			std_mtx.lock();
			int success = req.setLogout(user.getName());
			std_mtx.unlock();
			
			if(success){
				sock_mtx.lock();
				int retval = send(sockfd, &req, sizeof(req), 0);
				sock_mtx.unlock();
				assert(retval == sizeof(req));
			}
			user.setName("\0");
 			user.setLogout();
 		}
 		else if ( (cmd == "personal" || cmd == "6") && user.getStatus() == ONLINE ){
 			my_cout("watch or set\n");
 			string opt;
 			cin >> opt;
 			if (opt == "watch"){
 				my_cout("who do you want to watch\n");
 				string watch_user;
 				cin >> watch_user;
 				Request req(CLIENT, OP_WATCHPERSONAL, ST_REQ, user.getName());
				std_mtx.lock();
				int success = req.watchPersonal(user.getName(),watch_user);
				std_mtx.unlock();
				if(success){
					sock_mtx.lock();
					int retval = send(sockfd, &req, sizeof(req), 0);
					sock_mtx.unlock();
					assert(retval == sizeof(req));
				}
 			}
 			else if (opt == "set"){
 				my_cout("enter your signature\n");
 				string signature;
 				cin.ignore(100, '\n');
				getline(cin, signature);
 				Request req(CLIENT, OP_SETPERSONAL, ST_REQ, user.getName());
				std_mtx.lock();
				int success = req.setPersonal(user.getName(),signature);
				std_mtx.unlock();
				if(success){
					sock_mtx.lock();
					int retval = send(sockfd, &req, sizeof(req), 0);
					sock_mtx.unlock();
					assert(retval == sizeof(req));
				}
 			}
 		}
 		else if ( (cmd == "delete" || cmd == "7") && user.getStatus() == ONLINE ){
 			string n,fr;
			n = user.getName();
			my_cout("Delete Friend: \n");
			cin>>fr;
			Request req(CLIENT, OP_DELETEFRIEND, ST_REQ, user.getName());
			std_mtx.lock();
			int success = req.setDeleteFriend(n, fr);
			std_mtx.unlock();
			if(success){
				sock_mtx.lock();
				int retval = send(sockfd, &req, sizeof(req), 0);
				sock_mtx.unlock();
				assert(retval == sizeof(req));
			}
 		}
 		else if ( (cmd == "list" || cmd == "8") && user.getStatus() == ONLINE ){
 			string n;
			n = user.getName();
			Request req(CLIENT, OP_LISTFRIEND, ST_REQ, user.getName());
			std_mtx.lock();
			int success = req.setListFriend(n);
			std_mtx.unlock();
			if(success){
				sock_mtx.lock();
				int retval = send(sockfd, &req, sizeof(req), 0);
				sock_mtx.unlock();
				assert(retval == sizeof(req));
			}
 		}
		else{
			if (cmd != "yes" || cmd != "no" || cmd != "y" | cmd != "n")
				my_cout(cmd);
			my_cout("Please follow the instruction\n");
		}

		sleep(1);  //讓他收完req再接收cmd 
	}
}

int req_handle(Request *req,int sockfd){
	char data[DATA_MAX];
	memcpy(data, req->getData(), DATA_MAX);

	//cerr<<"header.op: "<<int(req->header.op)<<"\n";
	//cerr<<"header.status: "<<int(req->header.status)<<"\n";
	//cerr<<"header.name: "<<req->header.name<<"\n";
	 
	//不太可能遇到的問題
	if(req->header.from != SERVER){
		my_cerr("You are not a server.\n");
		return 0;
	}


	if(req->header.op == OP_REGISTER_ANS){
		my_cerr(data);
		if(req->header.status == ST_OK){
			//成功就成功，甚麼事都不用做（也不用輸出錯誤訊息，server會回傳)
			return 1;
		}
		return 0;
	}
	else if(req->header.op == OP_LOGIN_ANS){
		my_cerr(data);
		if(req->header.status == ST_OK){  //多餘的if判斷式
			user.setLogin();
		//	user.printUserInfo();
			//這邊UI要實作的是轉換成登入後的介面
			//UI中的畫面應該要由server取得(EX: 好友...等)
			//TODO....
			return 1;
		}
		return 0;
	}
	else if (req->header.op == OP_SEND_ANS){
		my_cerr(data);
		if (req->header.status == ST_OK){
			return 1;
		}
	}	
	else if (req->header.op == OP_SERVER_SEND){
		if (req->header.status == ST_OK){
			char name[NAME_LEN_MAX];
			char msg[DATA_MAX - NAME_LEN_MAX];
			memcpy(name,data,NAME_LEN_MAX);
			memcpy(msg,&data[NAME_LEN_MAX],sizeof(msg));

			string final_msg;
			final_msg = rot13(msg);

			my_cout(string("from: ") + name + string(", msg: ") + final_msg + string("\n"));
			//在cmd_handle那個thread還在等cin，所以先印出來這一行騙騙使用者
			my_cout("1 for send message, 2 for add friend, 3 for get history,  4 for send file, 5 to logout, 6 to edit personal information, 7 to delete friend, 8 to list friend\n");
			return 1;					
		}
	}
	else if (req->header.op == OP_ADDFRIEND_ANS){
		//加朋友成功
		my_cerr(data);
		if (req->header.status == ST_OK){
			return 1;
		}
	}
	else if(req->header.op == OP_HISTORY_ANS){
	    my_cout(data + string("\n"));
		return 1;
	}
	else if(req->header.op == OP_SERVER_SENDING){
		//my_cerr("getting file...\n");
	    char file_name[NAME_LEN_MAX];
        memcpy(file_name, data, NAME_LEN_MAX);
        ofstream fr((user.getName() + "_" + file_name), fstream::out | fstream::app | fstream::binary);
		int size = atoi(&data[NAME_LEN_MAX*2]);

		fr.write(&data[NAME_LEN_MAX*3], size);
	    fr.close();
	    return 1;
	}
	else if(req->header.op == OP_SERVER_SEND_FIN){
		//my_cerr("getting file...\n");
	    char file_name[NAME_LEN_MAX];
        memcpy(file_name, data, NAME_LEN_MAX);
        ofstream fr((user.getName() + "_" + file_name), fstream::out | fstream::app | fstream::binary);
		
		int size = atoi(&data[NAME_LEN_MAX*2]);

		fr.write(&data[NAME_LEN_MAX*3], size);
	    fr.close();
	    my_cerr("you get file!!!!!!!\n");
	    return 1;
	}
	else if(req->header.op == OP_SENDING_ANS){
		//my_cerr(data);
		if (req->header.status == ST_OK){
			return 1;
		}
	}
	else if(req->header.op == OP_SEND_FIN_ANS){
		my_cerr(data);
		if (req->header.status == ST_OK){
			return 1;
		}
	}
	else if(req->header.op == OP_ERR_MSG){//單純server想跟client聊聊天
		my_cerr(data);
		return 1;
	}

	else if(req->header.op == OP_FRIEND_REQUEST){
		/*
		my_cerr(string("do you want to add ")+ data+(" to be your friend? Type yes or no\n"));
		string answer;
		cin >> answer;
		my_cout(answer);
		if (answer == "yes" | answer == "y"){
			Request reqire(CLIENT, OP_ADDFRIEND, ST_REQ, user.getName());
			std_mtx.lock();
			int success = reqire.setAddFriend(user.getName(), data);
			std_mtx.unlock();
			if(success){
				sock_mtx.lock();
				int retval = send(sockfd, &req, sizeof(req), 0);
				sock_mtx.unlock();
				assert(retval == sizeof(req));
			}
		}
		*/
		return 1;
	}

	else if(req->header.op == OP_SETPERSONAL_ANS){
		std_mtx.lock();
		cerr<<data<<"\n";
		std_mtx.unlock();
		if (req->header.status == ST_OK){
			return 1;
		}
	}
	else if(req->header.op == OP_WATCHPERSONAL_ANS){
		std_mtx.lock();
		cerr<<data<<'\n';
		std_mtx.unlock();
		if (req->header.status == ST_OK){
			return 1;
		}
	}
	else if (req->header.op == OP_DELETEFRIEND_ANS){
		//刪朋友成功
		my_cerr(data);
		if (req->header.status == ST_OK){
			return 1;
		}
	}
	else if (req->header.op == OP_LISTFRIEND_ANS){
		//刪朋友成功
		//my_cout("These are your friends\n");
		string str;
		string line;
		line = string(data);
		if (line.size() == 0)
			my_cout("I'm sorry but you don't have any friend\n");
		else{
			my_cout("These are your friends\n");
			while(line.size()>0){
				str = line.substr(0, line.find(":"));
				line = line.substr(line.find(":") + 1);
				std_mtx.lock();
				cerr<<str<<"\n";
				std_mtx.unlock();
			}
		}

		
		if (req->header.status == ST_OK){
			return 1;
		}
	}
	else if (req->header.op == OP_LOGOUT_ANS){
		//刪朋友成功
		my_cerr(data);
		if (req->header.status == ST_OK){
			return 1;
		}
	}
	else if (req->header.op == OP_OFFLINE_SEND){
		if (req->header.status == ST_OK){
			char name[NAME_LEN_MAX];
			char msg[DATA_MAX - NAME_LEN_MAX];
			memcpy(name,data,NAME_LEN_MAX);
			memcpy(msg,&data[NAME_LEN_MAX],sizeof(msg));
			my_cout(string("from: ") + name + string(", msg: ") + msg + string("\n"));
			return 1;					
		}
	}


}












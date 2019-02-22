#include "protocol.hpp"
#include "command_handler.hpp"

void dump_name_passwd_map(){
	lock_guard<mutex> mLock(np_mtx);
	ofstream db("database.txt", fstream::out | fstream::trunc);
	for(auto &it:Name_Pw_map)
		db<<it.first<<":"<<it.second<<"\n";
	db.close();
}

void dump_friend_map(){
	lock_guard<mutex> mLock(fr_mtx);
	ofstream fr("friend.txt", fstream::out | fstream::trunc);
	for(auto &it:Friend){
		for(auto &p:it.second){
			fr<<it.first<<":"<<p<<"\n";
		}
	}
	fr.close();
}

void dump_history_map(){
	lock_guard<mutex> mLock(ht_mtx);
	ofstream ht("history.txt", fstream::out | fstream::trunc);
	for(auto &it:History){
		ht<<"User: "<<it.first<<"\n";
		for(auto &msg:it.second)
			ht<<msg<<"\n";
	}
	ht.close();
}

void dump_off_msg_map(){
	lock_guard<mutex> mLock(off_mtx);
	ofstream off("offline_msg.txt", fstream::out | fstream::trunc);
	for(auto &it:Offline_Msg){
		off<<"User: "<<it.first<<"\n";
		for(auto &p:it.second){
			off<<p.first<<":"<<p.second<<"\n";
		}
	}
	off.close();
}

void dump_signature_map(){
	ofstream db("signature.txt", fstream::out | fstream::trunc);
	for(auto &it:Personal_Signature)
		db<<it.first<<":"<<it.second<<"\n";
	db.close();
}

void dump_weird_people_map(){
	ofstream fr("weird_people.txt", fstream::out | fstream::trunc);
	for(auto &it:Weird_people){
		for(auto &p:it.second){
			fr<<it.first<<":"<<p<<"\n";
		}
	}
	fr.close();
}


void my_cerr(string str){
	lock_guard<mutex> mLock(std_mtx);
	cerr<<str;
}


Request cmd_handle(int fd, Request *recv_msg){
	Header h;
	string err;
	char data[DATA_MAX];
	h = recv_msg->header;
	memcpy(data, recv_msg->getData(), DATA_MAX);
	
	Request ans(SERVER, 0, 0);  //enum Group.SERVER
	//不太可能遇到的情況 bonus!!!只會一招的菜雞駭客會被我們擋掉
	if(h.from != CLIENT){
		err = "You are not my client.\n";
		ans.header.status = RECV_FAIL;
		ans.setErrMsg(err);
		my_cerr(err);
		return ans;
	}

		
	if(h.op == OP_REGISTER && h.status == ST_REQ){
		ans.header.op = OP_REGISTER_ANS;
		
		char name[NAME_LEN_MAX], passwd[PASSWD_LEN_MAX];
		//讀出請求中的帳號跟密碼
		memcpy(name, data, NAME_LEN_MAX);
		memcpy(passwd, &data[NAME_LEN_MAX], PASSWD_LEN_MAX);
		
		//確定有沒有人註冊過
		if(Name_Pw_map.find(name) != Name_Pw_map.end()){
			err = "the name has been registered before.\n";
			ans.header.status = ST_FAIL;
			ans.setErrMsg(err);
			my_cerr(err);
			return ans;
		}
		//把帳號密碼寫到資料庫中,
		//若要加分，可以把資料庫存取的密碼做md5
		np_mtx.lock();
		Name_Pw_map[name] = passwd;
		np_mtx.unlock();
		dump_name_passwd_map();
	
		ans.header.status = ST_OK;
		memcpy(ans.header.name, name, sizeof(name));

		my_cerr(string("register successed, name: ")+name+string(", passwd: ")+passwd+string("\n"));
		ans.setErrMsg("register successed!\n");
		return ans;
	}

	if(h.op == OP_LOGIN && h.status == ST_REQ){
		ans.header.op = OP_LOGIN_ANS;

		char name[NAME_LEN_MAX], passwd[PASSWD_LEN_MAX];
		//讀出請求中的帳號跟密碼
		memcpy(name, data, NAME_LEN_MAX);
		memcpy(passwd, &data[NAME_LEN_MAX], PASSWD_LEN_MAX);

		//找不到使用者
		if(Name_Pw_map.find(name) == Name_Pw_map.end()){
			err = "Couldn't find your account, please sign up\n";
			ans.header.status = ST_FAIL;
			ans.setErrMsg(err);
			my_cerr(err);
			return ans;
		}
		string true_pw = Name_Pw_map[name];

		//若要加分，這部份要先做一次md5
		if(passwd != true_pw){
			err = "Wrong password, please try again.\n";
			ans.header.status = ST_FAIL;
			ans.setErrMsg(err);
			my_cerr(err);
			return ans;
		}

		if (Name_Table.find(name) != Name_Table.end()){
			err = "user already login\n";
			ans.header.status = ST_FAIL;
			ans.setErrMsg(err);
			my_cerr(err);
			return ans;
		}
		
		c_mtx.lock();
		ans.header.status = ST_OK;
		Client[fd].status = ONLINE;
		Client[fd].name = name;
		c_mtx.unlock();
		nt_mtx.lock();
		Name_Table[name] = fd;
		nt_mtx.unlock();
		my_cerr(string("login successed, name: ") + name + string(", passwd: ") + passwd + string("\n"));
		ans.setErrMsg("login successed!\n");
		
		//傳送離線時的訊息
		if(Offline_Msg.find(name) != Offline_Msg.end()){
			Request start(SERVER, OP_ERR_MSG, ST_OK, name);
			start.setErrMsg("==========Message sent to you when you are offline==========\n");

			int retval = send(Name_Table[name], &start, sizeof(start), 0);
			assert(retval == sizeof(start));
			
			for(auto& p:Offline_Msg[name]){
				char d[DATA_MAX] = {0};
				strncpy(d, p.first.c_str(), p.first.length());
				strncpy(&d[NAME_LEN_MAX], p.second.c_str(), p.second.length());
				Request send_msg(SERVER, OP_OFFLINE_SEND, ST_OK, name);
				send_msg.server_char_Msg(d);

				retval = send(Name_Table[name], &send_msg, sizeof(Request), 0);
				assert(retval == sizeof(Request));
				
				char from[NAME_LEN_MAX], msg[DATA_MAX - NAME_LEN_MAX];
				memcpy(from, d, NAME_LEN_MAX);
				memcpy(msg, &d[NAME_LEN_MAX], sizeof(msg));
				ht_mtx.lock();
				if (msg[strlen(msg - 1)] == '\n')
					History[name].push_back("from: " + string(from) + ", to: " + string(name) + ", msg: " + string(msg));
				else
					History[name].push_back("from: " + string(from) + ", to: " + string(name) + ", msg: " + string(msg) + "\n");

				ht_mtx.unlock();
			}
			
			off_mtx.lock();
			Offline_Msg.erase(name);
			off_mtx.unlock();
			dump_off_msg_map();
			dump_history_map();
		}

		return ans;
	}

	if(h.op == OP_SEND && h.status == ST_REQ){
		ans.header.op = OP_SEND_ANS;
		char target_name[NAME_LEN_MAX], msg[DATA_MAX - NAME_LEN_MAX];
		memcpy(target_name, data, NAME_LEN_MAX);
		memcpy(msg, &data[NAME_LEN_MAX], sizeof(msg));
		//要傳的人在線上
		//不論要傳的人在不在線上，都要先判斷他是不是朋友
		for(auto &it:Friend){
			for(auto &p:it.second){
				if(it.first == target_name && p == h.name){
					//判斷人在不在線上
					char Send_to_b_data[DATA_MAX];  // 暫存的資料丟給server_char_msg
					memcpy(Send_to_b_data,h.name,NAME_LEN_MAX);
					memcpy(&Send_to_b_data[NAME_LEN_MAX], msg, sizeof(msg));
					//儲存進歷史訊息
					string his_msg = "from: " + string(h.name) + ", to: " + string(target_name) + ", msg: " + string(msg);
					ht_mtx.lock();
					History[h.name].push_back(his_msg);
					ht_mtx.unlock();

					if(Name_Table.find(target_name) != Name_Table.end()){
						Request Send_to_b(SERVER, OP_SERVER_SEND, ST_OK, target_name);
						
						Send_to_b.server_char_Msg(Send_to_b_data);
						int success = send(Name_Table[target_name], &Send_to_b, sizeof(Send_to_b),0);
						if (success == sizeof(Send_to_b)){
							ans.header.status = ST_OK;
							ans.setErrMsg("Send message to another client successed!\n");
						}
						else{
							ans.header.status = ST_FAIL;
							ans.setErrMsg("Send to another failed!\n");
						}

						return ans;

					}
					else{

						off_mtx.lock();
						Offline_Msg[target_name].push_back(make_pair(h.name, msg));
						off_mtx.unlock();
						dump_off_msg_map();
						ans.header.status = ST_OK;
						ans.setErrMsg("Send successed, but user is not online\n");

						return ans;
					}

					dump_history_map();
				}
			}
		}
		ans.header.status = ST_FAIL;
		ans.setErrMsg("You are not friend!!!\n");
		return ans;
        //對shit而言fuck是他好友 對fuck而言shit不是他好友 所以shit傳給fuck會被擋下來因為好友名單中
		//沒有fuck:shit的資料 而fuck可以傳給shit因為有shit:fuck 
		//也就是說你只有把你當朋友的人才會收到你的訊息，而你只會收到來自朋友的訊息	     	
	}

	if(h.op == OP_ADDFRIEND && h.status == ST_REQ){
		ans.header.op = OP_ADDFRIEND_ANS;
		char user_name[NAME_LEN_MAX], friend_name[NAME_LEN_MAX];
		memcpy(user_name, data, NAME_LEN_MAX);
		memcpy(friend_name, &data[NAME_LEN_MAX], NAME_LEN_MAX);
		int is_friend = 0;
		for(auto &it:Friend){
			for(auto &p:it.second){
				if (it.first == user_name && p == friend_name){
					err = "he is already your friend\n";
					ans.header.status = ST_FAIL;
					ans.setErrMsg(err);
					my_cerr(err);
					return ans;
				}
			}
		}

		for(auto &it:Friend){
			for(auto &p:it.second){
				if (it.first == friend_name && p == user_name){
					is_friend = 1;
				}
			}
		}
		
		np_mtx.lock();
		if(Name_Pw_map.find(friend_name) == Name_Pw_map.end()){
			err = "no user name\n";
			ans.header.status = ST_FAIL;
			ans.setErrMsg(err);
			return ans;
		}
		np_mtx.unlock();

		fr_mtx.lock();
		Friend[user_name].push_back(friend_name);
		fr_mtx.unlock();

		int is_weird = 0;
		for(auto &it:Weird_people){
			int i = 0;
			int jump = 0;
			for(auto &p:it.second){
				if (it.first == friend_name && p == user_name){
					is_weird = 1;
					jump = 1;
					break;	
				}
			}
			if (jump == 1)
				break;
		}
		if (is_weird == 0)
			Friend[friend_name].push_back(user_name);
		dump_friend_map();
		ans.header.status = ST_OK;
		memcpy(ans.header.name, user_name, sizeof(user_name));

		my_cerr(string("add friend successed, user: ")+user_name+string(", friend: ")+friend_name+string("\n"));
		ans.setErrMsg("add friend successed!\n");

		if (is_friend == 0){
			Request Friend_Request(SERVER, OP_FRIEND_REQUEST, ST_OK,friend_name);  //enum Group.SERVER
			Friend_Request.setFriendRequest(user_name);
			int success = send(Name_Table[friend_name], &Friend_Request, sizeof(Friend_Request),0);
		}
		return ans;
	}

	if(h.op == OP_HISTORY && h.status == ST_REQ){
		ans.header.op = OP_HISTORY_ANS;
		if(History.find(h.name) != History.end()){
			Request start(SERVER, OP_ERR_MSG, ST_OK, h.name);
			start.setErrMsg("==========History Message==========\n");

			int retval = send(Name_Table[h.name], &start, sizeof(Request), 0);
			assert(retval == sizeof(Request));

			for(auto& it:History[h.name]){
				Request msg(SERVER, OP_HISTORY_ANS, ST_OK, h.name);
				char d[DATA_MAX] = {0};
				strncpy(d, it.c_str(), it.length());
				msg.server_char_Msg(d);

				retval = send(Name_Table[h.name], &msg, sizeof(msg), 0);
				assert(retval == sizeof(Request));
			}
			ans.header.status = ST_OK;
	   		ans.setErrMsg("Your history");
	   		return ans;
		}
		else{
			Request msg(SERVER, OP_HISTORY_ANS, ST_OK, h.name);
			msg.setErrMsg("No History");
			return msg;
		}
	}

    if(h.op == OP_SENDFILE && h.status == ST_SENDING){
    	ans.header.op = OP_SENDING_ANS;
        char file[NAME_LEN_MAX];
        char target[NAME_LEN_MAX];
		
        memcpy(file, data, NAME_LEN_MAX);
        memcpy(target, &data[NAME_LEN_MAX], NAME_LEN_MAX);

		int size = atoi(&data[NAME_LEN_MAX*2]);

    	if(Name_Pw_map.find(target) == Name_Pw_map.end()){
			err = "Couldn't find target account\n";
			ans.header.status = ST_FAIL;
			ans.setErrMsg(err);
			my_cerr(err);
			return ans;
		}
/*
        ofstream fr((string("server_") + file), fstream::out | fstream::app | fstream::binary);
		fr.write(&data[3*NAME_LEN_MAX], size);
	    fr.close();
*/
		Request Send_to_b(SERVER, OP_SERVER_SENDING, ST_OK, target);			
		Send_to_b.server_char_Msg(data);
		nt_mtx.lock();
		int success = send(Name_Table[target], &Send_to_b, sizeof(Send_to_b),0);
		nt_mtx.unlock();
	
	    ans.header.status = ST_OK;
	    ans.setErrMsg("Sending\n");
		return ans;
    }
    
    if(h.op == OP_SENDFILE && h.status == ST_SEND_FIN){
        ans.header.op = OP_SEND_FIN_ANS;
        char file[NAME_LEN_MAX];
        char target[NAME_LEN_MAX];
        memcpy(file, data, NAME_LEN_MAX);
        memcpy(target, &data[NAME_LEN_MAX], NAME_LEN_MAX);

		int size = atoi(&data[NAME_LEN_MAX*2]);

        if(Name_Pw_map.find(target) == Name_Pw_map.end()){
			err = "Couldn't find target account\n";
			ans.header.status = ST_FAIL;
			ans.setErrMsg(err);
			my_cerr(err);
			return ans;
		}
		/*
        ofstream fr((string("server_")) + file, fstream::out | fstream::app | fstream::binary);
		fr.write(&data[3*NAME_LEN_MAX], size);
	    fr.close();
	    */
	    Request Send_to_b(SERVER, OP_SERVER_SEND_FIN, ST_OK, target);			
		Send_to_b.server_char_Msg(data);
		nt_mtx.lock();
		int success = send(Name_Table[target], &Send_to_b, sizeof(Send_to_b),0);
		nt_mtx.unlock();

	    ans.header.status = ST_OK;
	    ans.setErrMsg("Send ok\n");
		return ans;
		//要送東西了
    }

    if(h.op == OP_WATCHPERSONAL && h.status == ST_REQ){
    	ans.header.op = OP_WATCHPERSONAL_ANS;
    	char user_name[NAME_LEN_MAX], watch_name[NAME_LEN_MAX];
		memcpy(user_name, data, NAME_LEN_MAX);
		memcpy(watch_name, &data[NAME_LEN_MAX], NAME_LEN_MAX);

		if(Name_Pw_map.find(watch_name) == Name_Pw_map.end()){
			err = "Couldn't find user\n";
			ans.header.status = ST_FAIL;
			ans.setErrMsg(err);
			return ans;
		}
		if (Personal_Signature.find(watch_name) == Personal_Signature.end()){
			err = "No signature\n";
			ans.header.status = ST_FAIL;
			ans.setErrMsg(err);
			return ans;
		}
		else{
			err = Personal_Signature[watch_name];
			ans.header.status = ST_OK;
			ans.setErrMsg(err);
			return ans;
		}
    	return ans;
    }

    if(h.op == OP_SETPERSONAL && h.status == ST_REQ){
    	ans.header.op = OP_SETPERSONAL_ANS;
    	char user_name[NAME_LEN_MAX], signature[DATA_MAX- NAME_LEN_MAX];
		memcpy(user_name, data, NAME_LEN_MAX);
		memcpy(signature, &data[NAME_LEN_MAX], DATA_MAX- NAME_LEN_MAX);
		Personal_Signature[user_name] = signature; 
		dump_signature_map();
		err = string("your signature is ") + signature + "\n";
		ans.header.status = ST_OK;
		ans.setErrMsg(err);
    	return ans;
    }

    if(h.op == OP_DELETEFRIEND && h.status == ST_REQ){
    	ans.header.op = OP_DELETEFRIEND_ANS;
    	char user_name[NAME_LEN_MAX], friend_name[NAME_LEN_MAX];
		memcpy(user_name, data, NAME_LEN_MAX);
		memcpy(friend_name, &data[NAME_LEN_MAX], NAME_LEN_MAX);
		for(auto &it:Friend){
			int i = 0;
			for(auto &p:it.second){
				if (it.first == user_name && p == friend_name){
					it.second.erase(it.second.begin()+i);
					err = "Delete Friend ok\n";
					ans.header.status = ST_OK;
					ans.setErrMsg(err);
					my_cerr(err);
					dump_friend_map();
					return ans;
				}
				i++;
			}
		}
		err = "blacklist ok\n";
		ans.header.status = ST_OK;
		ans.setErrMsg(err);
		Weird_people[user_name].push_back(friend_name);
		dump_weird_people_map();
		return ans;
    }

    if(h.op == OP_LISTFRIEND && h.status == ST_REQ){
    	ans.header.op = OP_LISTFRIEND_ANS;
    	char user_name[NAME_LEN_MAX];
		memcpy(user_name, data, NAME_LEN_MAX);
		for(auto &it:Friend){
			if (it.first == user_name){
				for(auto &p:it.second){
					err = err + p + string(":");
				}
			}
		}
		ans.header.status = ST_OK;
		ans.setErrMsg(err);
		return ans;
    }

    if (h.op == OP_LOGOUT && h.status == ST_REQ){
    	ans.header.op = OP_LOGOUT_ANS;
    	char name[NAME_LEN_MAX];
		memcpy(name, data, NAME_LEN_MAX);
    	Name_Table.erase(name);
    	std_mtx.lock();
    	cout << name;
		std_mtx.unlock();
    	err = "Successful Logout\n";
		ans.header.status = ST_OK;
		ans.setErrMsg(err);
		return ans;

    }

}


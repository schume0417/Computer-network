#include "protocol.hpp"


Request::Request(uint8_t f, uint8_t o, uint8_t s){
	header.from = f;
	header.op = o;
	header.status = s;
	memset(header.name, 0, sizeof(header.name));

	memset(start_mark, START_MARK, MARK_SIZE);
	memset(end_mark, END_MARK, MARK_SIZE);

}

Request::Request(uint8_t f, uint8_t o, uint8_t s, string n){
	header.from = f;
	header.op = o;
	header.status = s;
	strncpy(header.name, n.c_str(), NAME_LEN_MAX-1);
	memset(start_mark, START_MARK, MARK_SIZE);
	memset(end_mark, END_MARK, MARK_SIZE);
}

Request::Request(Header h){
	header = h;
	memset(start_mark, START_MARK, MARK_SIZE);
	memset(end_mark, END_MARK, MARK_SIZE);
}

int Request::setErrMsg(string msg){
	memset(data, 0, sizeof(data));
	
	if(header.from == SERVER){
		strncpy(data, msg.c_str(), msg.length());
		return 1;
	}
	
	return 0;
}

int Request::server_char_Msg(char Msg[DATA_MAX]){
	memset(data, 0, sizeof(data));
	if(header.from == SERVER){
		memcpy(data, Msg, DATA_MAX);
		return 1;
	}
	return 0;
}


int Request::setRegister(string name, string pw, string repw){
	memset(data, 0, sizeof(data));
	if(name.length() > NAME_LEN_MAX - 1){
		cerr<<"the length limit of name is "<<NAME_LEN_MAX-1<<"\n";
		return 0;
	}
	strncpy(data, name.c_str(), NAME_LEN_MAX-1);

	if(pw.length() > PASSWD_LEN_MAX-1){
		cerr<<"the password can be up to "<<PASSWD_LEN_MAX-1<<"\n";
		return 0;
	}

	if(pw != repw){
		cerr<<"The passwords you typed do not match, please retype them.\n";
		return 0;
	}

	strncpy(&data[NAME_LEN_MAX], pw.c_str(), PASSWD_LEN_MAX-1);

	strncpy(&data[NAME_LEN_MAX+PASSWD_LEN_MAX], repw.c_str(), PASSWD_LEN_MAX-1);

	return 1;
}

int Request::setLogin(string name, string pw){
	memset(data, 0, sizeof(data));

	if(name.length() > NAME_LEN_MAX - 1){
		cerr<<"the length limit of name is "<<NAME_LEN_MAX-1<<"\n";
		return 0;
	}
	strncpy(data, name.c_str(), NAME_LEN_MAX-1);

	if(pw.length() > PASSWD_LEN_MAX-1){
		cerr<<"the password can be up to "<<PASSWD_LEN_MAX-1<<"\n";
		return 0;
	}
	strncpy(&data[NAME_LEN_MAX], pw.c_str(), PASSWD_LEN_MAX-1);

	return 1;
}

int Request::setSend(string name, string msg){
	memset(data, 0, sizeof(data));
	if(msg.length() > (DATA_MAX-NAME_LEN_MAX)){
		cerr<<"The length of message can be up to "<<(DATA_MAX-NAME_LEN_MAX)<<"\n";
		return 0;
	}
	strncpy(data, name.c_str(), NAME_LEN_MAX);
	strncpy(&data[NAME_LEN_MAX], msg.c_str(), msg.length());
	return 1;
}

int Request::setAddFriend(string name, string fr){
	memset(data, 0, sizeof(data));
	if(name.length() > NAME_LEN_MAX - 1){
		cerr<<"the length limit of name is "<<NAME_LEN_MAX-1<<"\n";
		return 0;
	}
	strncpy(data, name.c_str(), NAME_LEN_MAX-1);

	if(fr.length() > NAME_LEN_MAX-1){
		cerr<<"the password can be up to "<<NAME_LEN_MAX-1<<"\n";
		return 0;
	}

	strncpy(&data[NAME_LEN_MAX], fr.c_str(), NAME_LEN_MAX-1);

	return 1;

}

int Request::setSendFile(string file_name, string name, char *file, int size){
    memset(data, 0, sizeof(data));
    
    strncpy(data, file_name.c_str(), NAME_LEN_MAX-1);
    strncpy(&data[NAME_LEN_MAX], name.c_str(), NAME_LEN_MAX-1);
	strncpy(&data[NAME_LEN_MAX*2], to_string(size).c_str(), to_string(size).length());
    memcpy(&data[NAME_LEN_MAX * 3], file, size);
    return 1;
}

int Request::setFriendRequest(string user_name){
	memset(data, 0, sizeof(data));
    strncpy(data, user_name.c_str(), NAME_LEN_MAX-1);
    return 1;
}

int Request::setPersonal(string user_name,string signature){
	memset(data, 0, sizeof(data));
    strncpy(data, user_name.c_str(), NAME_LEN_MAX-1);
    strncpy(&data[NAME_LEN_MAX], signature.c_str(), DATA_MAX - NAME_LEN_MAX);
    return 1;
}

int Request::watchPersonal(string user_name,string watch){
	memset(data, 0, sizeof(data));
    strncpy(data, user_name.c_str(), NAME_LEN_MAX-1);
    strncpy(&data[NAME_LEN_MAX], watch.c_str(), NAME_LEN_MAX-1);
    return 1;
}

int Request::setDeleteFriend(string name, string fr){
	memset(data, 0, sizeof(data));
	if(name.length() > NAME_LEN_MAX - 1){
		cerr<<"the length limit of name is "<<NAME_LEN_MAX-1<<"\n";
		return 0;
	}
	strncpy(data, name.c_str(), NAME_LEN_MAX-1);

	if(fr.length() > NAME_LEN_MAX-1){
		cerr<<"the length limit of name is "<<NAME_LEN_MAX-1<<"\n";
		return 0;
	}

	strncpy(&data[NAME_LEN_MAX], fr.c_str(), NAME_LEN_MAX-1);

	return 1;

}

int Request::setListFriend(string name){
	memset(data, 0, sizeof(data));
	if(name.length() > NAME_LEN_MAX - 1){
		cerr<<"the length limit of name is "<<NAME_LEN_MAX-1<<"\n";
		return 0;
	}
	strncpy(data, name.c_str(), NAME_LEN_MAX-1);

	return 1;

}

int Request::setLogout(string name){
	memset(data, 0, sizeof(data));

	if(name.length() > NAME_LEN_MAX - 1){
		cerr<<"the length limit of name is "<<NAME_LEN_MAX-1<<"\n";
		return 0;
	}
	strncpy(data, name.c_str(), NAME_LEN_MAX-1);

	return 1;
}

User::User(){
	status = OFFLINE;
	name = "\0";
}

void User::setName(string n){
	name = n;
}

void User::setLogin(){
	status = ONLINE;
}


void User::setLogout(){
	status = OFFLINE;
}

void User::printUserInfo(){
	string s;
	if(status == ONLINE)
		s = "online";
	else if(status == OFFLINE)
		s = "offline";
	else
		s = "unknown";
	cout<<"Status: "<<s<<"\n";
	cout<<"Name: "<<name<<"\n";
}


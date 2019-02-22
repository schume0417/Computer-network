#ifndef _PROTOCOL_HPP_
#define _PROTOCOL_HPP_
#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <thread>
#include <mutex>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

using namespace std;
#define NAME_LEN_MAX 31
#define PASSWD_LEN_MAX 31
#define DATA_MAX 4096
#define SOCK_BUF_SIZE 10000
#define MARK_SIZE 4
//client傳送訊息時，header裡的operation都用這組

typedef enum {
	OP_REGISTER = 0x00,
	OP_LOGIN = 0x01,
	OP_SEND = 0x02,
	OP_ADDFRIEND = 0x03,
	OP_HISTORY = 0x04,
	OP_SENDFILE = 0x05,
	OP_SETPERSONAL = 0x06,
	OP_WATCHPERSONAL = 0x07,
	OP_DELETEFRIEND = 0x08,
	OP_LISTFRIEND = 0x09,
	OP_LOGOUT = 0x0a,
} Operation;

//server傳送訊息時，header裡的operation都用這組
typedef enum {
	OP_REGISTER_ANS = 0x10,
	OP_LOGIN_ANS = 0x11,
	OP_SEND_ANS = 0x12,
	OP_SERVER_SEND = 0x13,
	OP_ADDFRIEND_ANS = 0x14,
	OP_HISTORY_ANS = 0x15,
	OP_SENDING_ANS = 0x16,
	OP_SEND_FIN_ANS = 0x17,
	OP_SERVER_SENDING = 0x18,
	OP_SERVER_SEND_FIN = 0x19,
	OP_ERR_MSG = 0x1a,
	OP_FRIEND_REQUEST = 0x1b,
	OP_RE_SEND = 0x1c,
	OP_SETPERSONAL_ANS = 0x1d,
	OP_WATCHPERSONAL_ANS = 0x1e,
	OP_DELETEFRIEND_ANS = 0x60,
	OP_LISTFRIEND_ANS = 0x61,
	OP_LOGOUT_ANS = 0x62,
	OP_OFFLINE_SEND = 0x63,
} Server_Operation;

//client傳送訊息時，header裡的status都用這組
typedef enum {
	ST_REQ = 0x20,
	ST_SENDING = 0x21, //還在傳
	ST_SEND_FIN = 0x22,//傳完了(最後一組)
} Client_Status;

//server傳送訊息時，header裡的status都用這組
typedef enum {
	ST_OK = 0x30,
	ST_FAIL = 0x31,
	RECV_END = 0x32,
	RECV_FAIL = 0x33,
} Server_Status;

//紀錄Client端User在使用時的狀態
typedef enum {
	OFFLINE = 0x40,
	ONLINE = 0x41,
} User_Status;


//表示server or client
typedef enum{
	SERVER = 0x50,
	CLIENT = 0x51,
} Group;

typedef enum{
	START_MARK = 0xbb,
	END_MARK = 0xcc,
} Mark;

//在server中紀錄所有Client的資訊
typedef struct{
	string IP;
	int status;
	string name;
} ClientInfo;   //非常危險

//所有的傳遞訊息都要加上這個header
typedef struct{
	uint8_t from;//確認req來自server // client
	uint8_t op;
	uint8_t status;
	char name[NAME_LEN_MAX];
} Header;

//Header裡包含name，這樣client傳給server的時候可以直接驗證user是誰(其實根據IP可以直接判斷，但是現實可能有同IP不同User的情況，用user name判斷比較準）
//server回傳給client時，client可以檢查名字跟自己一不一樣，多一份確認


//傳遞所有的訊息都用這個class傳
class Request{
	public:
		char start_mark[MARK_SIZE];
		Header header;
		Request(Header);					//Constructor 初始化
		Request(uint8_t, uint8_t, uint8_t, string);	//Constructor 初始化，不同型式
		Request(uint8_t, uint8_t, uint8_t);
		//Client不是想傳甚麼就傳甚麼，想傳訊息給server，必須透過下面的函式
		int setErrMsg(string);//server 
		int server_char_Msg(char Msg[DATA_MAX]);
		int setRegister(string, string, string);//client
		int setLogin(string, string);//client
		int setSend(string, string);//client
		int setAddFriend(string,string);
		int setSendFile(string, string , char*, int);
		int setFriendRequest(string);
		int setPersonal(string,string);
		int watchPersonal(string,string);
		int setDeleteFriend(string,string);
		int setListFriend(string);
		int setLogout(string);
		//data屬於private，不能直接讀取，所以提供一個函式讀取data
		char *getData(){ return data;}
	private:
		char data[DATA_MAX];
	public:
		char end_mark[MARK_SIZE];
};

//紀錄Client中，使用者的資訊
class User{
	public:
		User();
		void setName(string);
		void setLogin();
		void setLogout();
		void printUserInfo();
		string getName(){ return name;}
		int getStatus(){ return status;}
	private:
		int status;
		string name;

};

#endif

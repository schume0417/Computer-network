#include "protocol.hpp"

#ifndef _COMMAND_HANDLER_HPP_
#define _COMMAND_HANDLER_HPP_
//根據fd儲存CLient資訊
extern unordered_map<int, ClientInfo> Client;
//根據user name儲存fd, 只有Login的人

extern mutex c_mtx;

extern unordered_map<string, int> Name_Table;

extern mutex nt_mtx;

extern unordered_map<string, string> Name_Pw_map;

extern mutex np_mtx;

extern unordered_map<string, vector<string>> Friend;

extern mutex fr_mtx;

extern unordered_map<string, vector<string>> History;

extern mutex ht_mtx;

extern unordered_map<string, vector<pair<string, string>>> Offline_Msg;

extern mutex off_mtx;

extern mutex std_mtx;

extern unordered_map<string, string> Personal_Signature;

extern unordered_map<string, vector<string>> Weird_people;

Request cmd_handle(int fd, Request *recv_msg);
#endif

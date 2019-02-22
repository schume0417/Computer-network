#include "protocol.hpp"

#ifndef _CLIENT_CMD_HANDLER_HPP_
#define _CLIENT_CMD_HANDLER_HPP_

extern User user;
extern mutex std_mtx;
extern mutex sock_mtx;
extern mutex in_mtx;

extern char Sock_Buf[SOCK_BUF_SIZE];
extern int Buf_Pointer;
void cmd_handle(int sockfd);

int req_handle(Request *req,int sockfd);

#endif

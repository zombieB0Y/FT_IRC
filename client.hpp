#pragma once

#include <iostream>
#include "server.hpp"
#include "help_func.hpp"

class client
{
private:
	int fd;
	std::string ip;
	std::string buffer;
	std::string _nickname;
	std::string _username;
	std::string _realname;
	bool is_auth;
	bool is_regtr;

public:
	client();
	client(const client &copy);
	client &operator=(const client &copy);
	~client();

	int getFd() const;
	std::string getIp() const;
	std::string getBuffer() const;
	void setFd(int _fd);
	void setIp(std::string _ip);
	bool is_authenticate() const;
	void authenticate();
	bool is_register() const;
	void register_client();
	void append_Buffer(std::string buff);
	bool handel_PASS(server &serv);
	bool handel_register(server &serv);
	void handel_CMDS();
};

void send_msg(std::string msg, int client_fd);
#pragma once

#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string>
#include <string.h>
#include <vector>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <csignal>
#include "client.hpp"

class server
{
private:
	int							serverSocket;
	int							port;
	static bool					signal;
	std::vector<client> 		clients;
	std::vector<struct pollfd>	fds;
public:
	server();
	server(const server &copy);
	server	&operator=(const server &copy);
	~server();
	server(int _port); // neeed to pass password too ig!!

	void	server_init();
	void	init_server_socket();
	void	accept_new_client();
	void	read_data(int fd);
	void	clear_fds();
	void	clear_client(int fd);

	static void	signalhandler(int sig);
};

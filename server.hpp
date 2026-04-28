#pragma once

#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
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
#include "help_func.hpp"

class client;

class server
{
private:
	int							serverSocket;
	int							port;
	std::string					password;
	static bool					signal;
	std::vector<client> 		clients;
	std::vector<struct pollfd>	fds;
public:
	server();
	server(const server &copy);
	server	&operator=(const server &copy);
	~server();
	server(int _port, std::string _password); // neeed to pass password too ig!!

	void	server_init();
	void	init_server_socket();
	void	accept_new_client();
	void	read_data(client &client);
	void	clear_fds();
	void	clear_client(int fd);
	bool	compaire_password(std::string &s);
	client	*get_client(int fd);

	// bool	isNicknametaken(const std::string nick);
	std::vector<client>	getClients() const;

	static void	signalhandler(int sig);
};

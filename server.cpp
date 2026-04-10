#include "server.hpp"
#include <arpa/inet.h>

bool server::signal = false;

server::server() : serverSocket(-1), port(-1), password("0000") {}

server::server(const server &copy) {
	*this = copy;
}

server	&server::operator=(const server &copy) {
	if (this != &copy) {
		this->port = copy.port;
		this->serverSocket = copy.serverSocket;
		this->clients = copy.clients;
		this->fds = copy.fds;
	}
	return *this;
}

server::~server() {
	clear_fds();
}

server::server(int _port, std::string _password) : port(_port), password(_password) {}

void	server::signalhandler(int sig) {
	(void)sig;
	signal = true;
}

void	server::accept_new_client() {
	client	client;
	struct sockaddr_in	client_address;
	socklen_t		add_len = sizeof(client_address);
	int	client_fd = accept(this->serverSocket, (struct sockaddr *)&client_address, &add_len);
	if (client_fd == -1) {
		std::cerr << "accept(): failed !\n";
		server::signalhandler(SIGQUIT);
		return ;
	}
	if (fcntl(client_fd, F_SETFL, O_NONBLOCK) == -1) {
		std::cerr << "NON BLOCKING failed !\n";
		server::signalhandler(SIGQUIT);
		return ;
	}
// ---------------------------------------------------------------------------------------
	std::string welcome_msg = "Welcome to the 1337 IRC Server!\r\nYou need to PASS the password to connect, u can use \"PASS <password>\" to connect!\r\n";
	ssize_t bytes_sent = send(client_fd, welcome_msg.c_str(), welcome_msg.length(), 0);
	if (bytes_sent == -1) {
		std::cerr << "send(): failed to send welcome message to client " << client_fd << "\n";
	} else {
		std::cout << "Client <" << client_fd << "> Connected. Welcome message sent.\n";
	}
// ---------------------------------------------------------------------------------------
	struct pollfd	clientpoll;
	clientpoll.fd = client_fd;
	clientpoll.events = POLLIN;
	clientpoll.revents = 0;
	this->fds.push_back(clientpoll);

	client.setFd(client_fd);
	client.setIp(inet_ntoa(client_address.sin_addr));
	this->clients.push_back(client);
}

void	server::read_data(client client) {
	
	std::string	buff;
	int max_bytes = 1024;
	buff.resize(max_bytes);
	int		fd = client.getFd();
	ssize_t bytes = recv(fd, &buff[0], max_bytes , 0);

	if(bytes <= 0) {
		std::cerr << "Client <" << client.getIp() << "> Disconnected" << std::endl;
		clear_client(fd);
		close(fd);
	}
	// write(1, buff, bytes);
	// client.setBuffer(buff);
	bool	done = false;
	while (done != true) {
		if (!client.is_authenticate()) {
			size_t newline_idx = buff.find("\n");
			if (newline_idx != std::string::npos)
				std::string	line = 
		}
	}
}

// ------------------------------------POLL--------------------------
void	server::server_init() {
	init_server_socket();

	while (server::signal == false) {
		if (poll(&fds[0], fds.size(), -1) == -1 && server::signal == false)
			throw std::runtime_error("poll(): failed !");
		for (size_t i = 0; i < fds.size(); i++) {
			if (fds[i].revents & POLLIN) {
				if (fds[i].fd == this->serverSocket)
					accept_new_client();
				else
					read_data(clients[i - 1]);
			}
		}
	}
	clear_fds();
}
//-----------------------------------------------------------------------

void	server::init_server_socket() {
	if (this->port == -1)
		this->port = 8080;
	this->serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (this->serverSocket == -1) throw std::runtime_error("opening server socket failed !");
	
	sockaddr_in	server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(this->port);
	server_address.sin_addr.s_addr = INADDR_ANY;
	int optval = 1;
	
	if (setsockopt(this->serverSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
		close(this->serverSocket);
		this->serverSocket = -1;
		throw std::runtime_error("faild to set option (SO_REUSEADDR) on socket");
	}
	if (fcntl(this->serverSocket, F_SETFL, O_NONBLOCK) == -1) {
		close(this->serverSocket);
		this->serverSocket = -1;
		throw std::runtime_error("failed to set non blocking flag !");
	}
	if (bind(this->serverSocket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
		close(this->serverSocket);
		this->serverSocket = -1;
		throw std::runtime_error("binding server socket failed !");
	}
	if (listen(this->serverSocket, SOMAXCONN) == -1) {
		close(this->serverSocket);
		this->serverSocket = -1;
		throw std::runtime_error("listening server socket failed !");
	}

	struct pollfd	newpoll;
	newpoll.fd = this->serverSocket;
	newpoll.events = POLLIN;
	newpoll.revents = 0;
	this->fds.push_back(newpoll);
}

void	server::clear_fds() {
	for (size_t i = 0; i < clients.size(); i++) {
		if (clients[i].getFd() != -1) {
			std::cout << "Client <" << clients[i].getFd() << "> Disconnected" << std::endl;
			close(clients[i].getFd());
			clients[i].setFd(-1);
		}
		if (this->serverSocket != -1) {
			std::cout << "SERVER <" << this->serverSocket << "> Disconnected" << std::endl;
			close(this->serverSocket);
			this->serverSocket = -1;
		}
	}
}

void	server::clear_client(int fd) {
	for (size_t i = 0; i < fds.size(); i++) {
		if (fds[i].fd == fd) {
			fds.erase(fds.begin() + i);
			clients.erase(clients.begin() + (i - 1));
			break ;
		}
	}
}
#include "client.hpp"

client::client() : fd(-1), is_auth(false), is_regtr(false) {}

client::client(const client &copy) {
	*this = copy;
}

client  &client::operator=(const client &copy) {
	if (this != &copy) {
		this->buffer = copy.getBuffer();
		setFd(copy.getFd());
		setIp(copy.getIp());
		this->is_auth = copy.is_authenticate();
		this->is_regtr = copy.is_register();
	}
	return *this;
}

client::~client() {}

int client::getFd() const {
	return this->fd;
}

std::string client::getIp() const {
	return this->ip;
}

std::string	client::getBuffer() const {
	return this->buffer;
}

void    client::setFd(int _fd) {
	this->fd = _fd;
}

void    client::setIp(std::string _ip) {
	this->ip = _ip;
}

bool    client::is_authenticate() const {
	return this->is_auth;
}

void    client::authenticate() {
	this->is_auth = true;
}

void    client::append_Buffer(std::string buff) {
	this->buffer + buff;
}

bool    client::handel_PASS(server &serv) {
	rtrim(this->buffer);
	ltrim(this->buffer);
	this->append_Buffer("\n");

	size_t newline_idx;
	if ((newline_idx = this->buffer.find("\n")) != std::string::npos) {
		std::string line = this->buffer.substr(0, newline_idx);
		try {
			if (line.compare(0, 4, "PASS") == 0) {
				line.erase(0, 4);
				ltrim(line);
				if (!serv.compaire_password(line)) {
					std::cerr << "wrong password !\n";
					return false;
				}
				this->authenticate();
				this->buffer.erase(0, newline_idx + 1);
				send_msg("you are authenticated !\n", this->fd);
				return true;
			}
		}
		catch (std::exception &e) {
			return false;
		}
	}
}

bool	client::handel_register(server &serv) {
	rtrim(this->buffer);
	ltrim(this->buffer);
	this->append_Buffer("\n");
	
	size_t	newline_idx;
	if ((newline_idx = this->buffer.find("\n"))) {
		std::string line = this->buffer.substr(0, newline_idx);
	}
}

bool	client::is_register() const {
	return this->is_regtr;
}

void	client::register_client() {
	this->is_regtr = true;
}

void	client::handel_CMDS() {

}

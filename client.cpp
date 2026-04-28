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
	this->buffer += buff;
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
					std::cerr << "wrong password !" << std::endl;
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
	return false;
}

bool	client::handel_register(server &serv) {
	// (void)serv;
	size_t	newline_idx;
	
	while ((newline_idx = this->buffer.find('\n')) != std::string::npos) {
		std::string line = this->buffer.substr(0, newline_idx);
		std::string clean_line = line;
		rtrim(clean_line);
		ltrim(clean_line);
		
		if (clean_line.compare(0, 4, "NICK") == 0) {
			std::string nick = clean_line.substr(4);
			ltrim(nick);
			if (this->valid_nick(nick, serv))
				this->_nickname = nick;
		}
		else if (clean_line.compare(0, 4, "USER") == 0) {
			std::string user_info = clean_line.substr(4);
			ltrim(user_info);
			size_t space_idx = user_info.find(' ');
			if (space_idx != std::string::npos) {
				this->_username = user_info.substr(0, space_idx);
				size_t colon_idx = user_info.find(':');
				if (colon_idx != std::string::npos) {
					this->_realname = user_info.substr(colon_idx + 1);
				}
			} else {
				this->_username = user_info;
			}
		}
		// else if () // -------- if the cmd parsed not NIKC or USER need to return false 
		this->buffer.erase(0, newline_idx + 1);
		
		if (!this->_nickname.empty() && !this->_username.empty()) {
			this->register_client();
			send_msg("001 " + this->_nickname + " :Welcome to the 1337 IRC Network " + this->_nickname + "\r\n", this->fd);
			return true;
		}
	}
	return false;
}

bool	client::valid_nick(std::string nick, server serv) {
	if (nick.empty() || nick.length() > 9) {
        return false;
    }
    if (!std::isalpha(nick[0]) && !isspecial(nick[0])) {
        return false;
    }
    for (size_t i = 1; i < nick.length(); ++i) {
        char c = nick[i];
        if (!std::isalnum(c) && c != '-' && !isspecial(c)) {
            return false;
        }
    }
	for (std::vector<client>::iterator it = serv.getClients().begin(); it != serv.getClients().end(); ++it) {
    	if (it->getNickname() == nick) {
      	  return true; 
  	  	}
	}
    return false;
}

bool	client::is_register() const {
	return this->is_regtr;
}

void	client::register_client() {
	this->is_regtr = true;
}

std::string	client::getNickname() const {
	return this->_nickname;
}

// ------------- not completed !! -------------------

void	client::handel_CMDS() {

}

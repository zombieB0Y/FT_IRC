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

void    client::Replace_Buffer(std::string buff) {
	this->buffer = buff;
}

void    client::Erase_Buffer() {
	this->buffer.erase();
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
					std::cerr << "User <" << this->getIp() << "> :" << "wrong password !" << std::endl;
					return false;
				}
				this->authenticate();
				this->buffer.erase(0, newline_idx + 1);
				send_msg("you are authenticated !\n", this->fd);
				this->Erase_Buffer();	
				return true;
			}
		}
		catch (std::exception &e) {
			this->Erase_Buffer();
			return false;
		}
	}
	this->Erase_Buffer();
	return false;
}

bool	client::handel_register(server &serv) {
	// (void)serv;
	size_t	newline_idx;
	int		count = 2;
	bool	flags[2] = {0};
	
	while ((newline_idx = this->buffer.find('\n')) != std::string::npos && count != 0) {
		std::string line = this->buffer.substr(0, newline_idx);
		std::string clean_line = line;
		rtrim(clean_line);
		ltrim(clean_line);
		
		if (clean_line.compare(0, 4, "NICK") == 0 && !flags[0]) {
			std::string nick = clean_line.substr(4);
			ltrim(nick);
			if (this->valid_nick(nick, serv)) {
				this->_nickname = nick;
				flags[0] = true;
			}
		}
		else if (clean_line.compare(0, 4, "USER") == 0 && !flags[1]) {
			std::string user_info = clean_line.substr(4);
			ltrim(user_info);
			std::vector<std::string> tokens;
			std::string token;
			size_t i = 0;
			while (i < user_info.length() && tokens.size() < 3) {
				if (user_info[i] != ' ' && user_info[i] != ':') {
					token += user_info[i];
				}
				else if (user_info[i] == ' ') {
					if (!token.empty()) {
						tokens.push_back(token);
						token.clear();
					}
				}
				else if (user_info[i] == ':') {
					break;
				}
				i++;
			} // ---------------- i can add the hostname and the servername mn be3d if needed !
			if (!token.empty() && tokens.size() < 3) {
				tokens.push_back(token);
			}
			if (tokens.size() < 3) {
				send_msg("461 USER :Not enough parameters\r\n", this->fd);
			}
			else {
				std::string parsed_user = tokens[0];
				if (this->valid_user(parsed_user)) {
					this->_username = parsed_user;
					size_t colon_idx = user_info.find(':');
					if (colon_idx != std::string::npos) {
						this->_realname = user_info.substr(colon_idx + 1);
						flags[1] = true;
					}
					else {
						while (i < user_info.length() && std::isspace(user_info[i])) {
							i++;
						}
						if (i < user_info.length()) {
							this->_realname = user_info.substr(i);
							flags[1] = true;
						}
						else {
							send_msg("461 USER :Not enough parameters\r\n", this->fd);
							this->_username.clear();
						}
					}
				}
				else {
					send_msg("461 USER :Not a valid Username\r\n", this->fd);
					this->_username.clear();
				}
			}
		}
		this->buffer.erase(0, newline_idx + 1);
		--count;
		if (!this->_nickname.empty() && !this->_username.empty()) {
			this->register_client();
			send_msg("001 " + this->_nickname + " :Welcome to the 1337 IRC Network " + this->_nickname + "\r\n", this->fd);
			return true;
		}
	}
	this->_username.clear();
	this->_realname.clear();
	send_msg("Error : no valid NICK or USER !\r\n", this->fd);
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

bool	client::valid_user(std::string username) {
	if (username.empty()) {
		return false;
	}
	for (size_t i = 0; i < username.length(); ++i) {
		char c = username[i];
		if (c == ' ' || c == '@') {
			return false;
		}
	}
	return true;
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

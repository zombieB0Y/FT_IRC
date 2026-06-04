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
	size_t newline_idx;
	if ((newline_idx = this->buffer.find("\n")) != std::string::npos) {
		std::string line = this->buffer.substr(0, newline_idx);
		rtrim(line);
		ltrim(line);
		
		if (line.empty()) {
			this->buffer.erase(0, newline_idx + 1);
			return false;
		}
		
		try {
			std::string cmd = to_upper(line.substr(0, (line.find(' ') != std::string::npos) ? line.find(' ') : line.length()));
			if (cmd == "PASS") {
				std::string pass_arg = line.substr(4);
				ltrim(pass_arg);
				rtrim(pass_arg);
				
				if (!serv.compaire_password(pass_arg)) {
					send_msg(":irc.server 464 * :Password incorrect\r\n", this->fd);
					this->buffer.erase(0, newline_idx + 1);
					return false;
				}
				this->authenticate();
				this->buffer.erase(0, newline_idx + 1);
				send_msg(":irc.server :You are authenticated!\r\n", this->fd);
				return true;
			}
			else {
				// Non-PASS command received before authentication
				send_msg(":irc.server 451 * :You have not registered\r\n", this->fd);
				this->buffer.erase(0, newline_idx + 1);
				return false;
			}
		}
		catch (std::exception &e) {
			this->buffer.erase(0, newline_idx + 1);
			return false;
		}
	}
	return false;
}

bool	client::handel_register(server &serv) {
	size_t	newline_idx;
	
	while ((newline_idx = this->buffer.find('\n')) != std::string::npos) {
		std::string line = this->buffer.substr(0, newline_idx);
		rtrim(line);
		ltrim(line);
		
		if (line.empty()) {
			this->buffer.erase(0, newline_idx + 1);
			continue;
		}
		
		// Extract command keyword
		size_t space_pos = line.find(' ');
		std::string cmd_str = (space_pos != std::string::npos) ? line.substr(0, space_pos) : line;
		std::string cmd_upper = to_upper(cmd_str);
		std::string args = (space_pos != std::string::npos) ? line.substr(space_pos + 1) : "";
		ltrim(args);
		
		if (cmd_upper == "NICK") {
			rtrim(args);
			if (args.empty()) {
				send_msg(":irc.server 431 * :No nickname given\r\n", this->fd);
			} else if (this->valid_nick(args, serv)) {
				this->_nickname = args;
			} else {
				send_msg(":irc.server 433 * " + args + " :Nickname is already in use\r\n", this->fd);
			}
		}
		else if (cmd_upper == "USER") {
			std::vector<std::string> tokens;
			std::string token;
			size_t i = 0;
			
			// Parse tokens before colon
			while (i < args.length() && tokens.size() < 3) {
				if (args[i] != ' ' && args[i] != ':') {
					token += args[i];
				}
				else if (args[i] == ' ') {
					if (!token.empty()) {
						tokens.push_back(token);
						token.clear();
					}
				}
				else if (args[i] == ':') {
					break;
				}
				i++;
			}
			
			if (!token.empty() && tokens.size() < 3) {
				tokens.push_back(token);
			}
			
			if (tokens.size() < 3) {
				send_msg(":irc.server 461 * USER :Not enough parameters\r\n", this->fd);
			}
			else {
				std::string parsed_user = tokens[0];
				if (this->valid_user(parsed_user)) {
					this->_username = parsed_user;
					
					// Find realname after colon
					size_t colon_idx = args.find(':');
					if (colon_idx != std::string::npos) {
						this->_realname = args.substr(colon_idx + 1);
					} else {
						// No colon, use remaining tokens as realname
						this->_realname = tokens[2];
					}
				} else {
					send_msg(":irc.server 468 * :Invalid username\r\n", this->fd);
				}
			}
		}
		else {
			// Unknown command during registration - just skip it
			send_msg(":irc.server 421 * " + cmd_upper + " :Unknown command\r\n", this->fd);
		}
		
		this->buffer.erase(0, newline_idx + 1);
		
		// Check if registration is complete
		if (!this->_nickname.empty() && !this->_username.empty()) {
			this->register_client();
			return true;
		}
	}
	
	return false;
}

bool	client::valid_nick(std::string nick, const server &serv) {
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
	// Check if nickname is already taken - should return false if taken (not valid)
	for (std::vector<client>::const_iterator it = serv.getClients().begin(); it != serv.getClients().end(); ++it) {
    	if (it->getNickname() == nick && it->getFd() != this->getFd()) {
      	  return false; // Nickname is taken, so it's NOT valid
  	  	}
	}
    return true; // Nickname is available and valid
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

std::string	client::getUsername() const {
	return this->_username;
}

std::string	client::getRealname() const {
	return this->_realname;
}

void	client::setNickname(std::string nick) {
	this->_nickname = nick;
}

// ------------- Command Handler -------------------

void	client::handel_CMDS(server &serv) {
	size_t newline_idx;
	
	while ((newline_idx = this->buffer.find('\n')) != std::string::npos) {
		std::string line = this->buffer.substr(0, newline_idx);
		rtrim(line);
		ltrim(line);
		
		if (line.empty()) {
			this->buffer.erase(0, newline_idx + 1);
			continue;
		}
		
		// Extract command (everything before the first space)
		size_t space_idx = line.find(' ');
		std::string cmd = line.substr(0, space_idx);
		std::string args = (space_idx != std::string::npos) ? line.substr(space_idx + 1) : "";
		
		// Convert command to uppercase for case-insensitive comparison
		cmd = to_upper(cmd);
		
		if (cmd == "PRIVMSG") {
			// PRIVMSG <target> :<message>
			size_t colon_idx = args.find(':');
			if (colon_idx != std::string::npos) {
				std::string target = args.substr(0, colon_idx);
				rtrim(target);
				std::string message = args.substr(colon_idx + 1);
				
				// Send PRIVMSG to target
				client *target_client = serv.get_client_by_nick(target);
				if (target_client && target_client->getFd() != -1) {
					std::string privmsg = ":" + this->_nickname + " PRIVMSG " + target + " :" + message + "\r\n";
					send_msg(privmsg, target_client->getFd());
				} else {
					send_msg(":irc.server 401 " + this->_nickname + " " + target + " :No such nick/channel\r\n", this->fd);
				}
			}
		}
		else if (cmd == "QUIT") {
			std::string quit_msg = (args.length() > 0 && args[0] == ':') ? args.substr(1) : args;
			send_msg(":irc.server QUIT :" + quit_msg + "\r\n", this->fd);
			serv.clear_client(this->fd);
			close(this->fd);
			this->setFd(-1);
			break;
		}
		else if (cmd == "NICK") {
			std::string new_nick = args;
			ltrim(new_nick);
			rtrim(new_nick);
			if (this->valid_nick(new_nick, serv)) {
				std::string old_nick = this->_nickname;
				this->_nickname = new_nick;
				send_msg(":" + old_nick + " NICK " + new_nick + "\r\n", this->fd);
			} else {
				send_msg(":irc.server 433 * " + new_nick + " :Nickname is already in use\r\n", this->fd);
			}
		}
		else if (cmd == "JOIN") {
			std::string channel = args;
			ltrim(channel);
			rtrim(channel);
			if (channel.length() > 0 && channel[0] == '#') {
				send_msg(":" + this->_nickname + " JOIN " + channel + "\r\n", this->fd);
				send_msg(":irc.server 331 " + this->_nickname + " " + channel + " :End of /NAMES list.\r\n", this->fd);
			} else {
				send_msg(":irc.server 476 * " + channel + " :Bad channel mask\r\n", this->fd);
			}
		}
		else if (cmd == "PART") {
			std::string channel = args;
			ltrim(channel);
			rtrim(channel);
			if (channel.length() > 0) {
				send_msg(":" + this->_nickname + " PART " + channel + "\r\n", this->fd);
			} else {
				send_msg(":irc.server 461 * PART :Not enough parameters\r\n", this->fd);
			}
		}
		else if (cmd == "WHO") {
			std::string target = args;
			ltrim(target);
			rtrim(target);
			const std::vector<client>	&clients = serv.getClients();
			for (size_t i = 0; i < clients.size(); i++) {
				if (clients[i].is_register()) {
					// RFC 2812 format: :server 352 <requesting_nick> <channel> <user> <host> <server> <nick> <flags> :<hopcount> <realname>
					std::string response = ":irc.server 352 " + this->_nickname + " * " + clients[i].getUsername() + " " 
						+ clients[i].getIp() + " irc.server " + clients[i].getNickname() + " H :0 " + clients[i].getRealname();
					send_msg(response + "\r\n", this->fd);
				}
			}
			send_msg(":irc.server 315 " + this->_nickname + " * :End of /WHO list.\r\n", this->fd);
		}
		else if (cmd == "PING") {
			std::string param = args;
			ltrim(param);
			send_msg(":irc.server PONG :" + param + "\r\n", this->fd);
		}
		else {
			// Unknown command
			send_msg(":irc.server 421 " + this->_nickname + " " + cmd + " :Unknown command\r\n", this->fd);
		}
		
		this->buffer.erase(0, newline_idx + 1);
	}
}

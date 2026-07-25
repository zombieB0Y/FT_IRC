#include "Server.hpp"

volatile bool Server::running = false;

// ═════════════════════════════════════════════════════════════════════════════
// Construction / destruction
// ═════════════════════════════════════════════════════════════════════════════

Server::Server(int port, const std::string& pass)
	: port(port)
	, password(pass)
	, serverName("ircserv")
	, listenFd(-1)
{}

Server::~Server()
{
	for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
		close(it->first);
	clients.clear();

	if (listenFd >= 0)
		close(listenFd);
}

// ═════════════════════════════════════════════════════════════════════════════
// Initialisation
// ═════════════════════════════════════════════════════════════════════════════

bool Server::init()
{
	_initcommadlist();
	return setupListenSocket();
}

bool Server::setupListenSocket()
{
	int opt = 1;

	listenFd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenFd < 0) {
		std::cerr << "socket() failed" << std::endl;
		return false;
	}
	if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
		std::cerr << "setsockopt() failed" << std::endl;
		return false;
	}
	if (fcntl(listenFd, F_SETFL, O_NONBLOCK) == -1) {
		std::cerr << "fcntl() failed: could not make socket non-blocking" << std::endl;
		return false;
	}

	sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port        = htons(port);

	if (bind(listenFd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		std::cerr << "bind() failed" << std::endl;
		return false;
	}
	if (listen(listenFd, 5) == -1) {
		std::cerr << "listen() failed" << std::endl;
		return false;
	}
	return true;
	if (bind(listenFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
		std::cerr << "bind() failed" << std::endl;
		return false;
	}
	if (listen(listenFd, 5) == -1) {
		std::cerr << "listen() failed" << std::endl;
		return false;
	}
	return true;
}


void Server::_initcommadlist() {
	this->_commandslist.push_back("CAP");
	this->_commandslist.push_back("QUIT");
	this->_commandslist.push_back("PASS");
	this->_commandslist.push_back("NICK");
	this->_commandslist.push_back("USER");
	this->_commandslist.push_back("PING");
	this->_commandslist.push_back("JOIN");
	this->_commandslist.push_back("PART");
	this->_commandslist.push_back("PRIVMSG");
	this->_commandslist.push_back("KICK");
	this->_commandslist.push_back("INVITE");
	this->_commandslist.push_back("TOPIC");
	this->_commandslist.push_back("MODE");
	this->_commandslist.push_back("WHO");
}

bool	Server::_isacmd(std::string m) const {
	if (std::find(_commandslist.begin(), _commandslist.end(), m) == _commandslist.end())
		return false;
	return true;
}

// ═════════════════════════════════════════════════════════════════════════════
// Main event loop
// ═════════════════════════════════════════════════════════════════════════════

void Server::run()
{
	running = true;
	while (running) {
		rebuildPollFds();

		int ready = poll(pfds.data(), pfds.size(), -1);
		if (ready < 0) {
			if (errno == EINTR)
				continue;
			std::cerr << "poll() failed" << std::endl;
			break;
		}

		for (size_t i = 0; i < pfds.size(); ++i) {
			if (pfds[i].revents == 0)
				continue;

			if (pfds[i].fd == listenFd) {
				if (pfds[i].revents & POLLIN)
					acceptClient();
				continue;
			}

			// FIX: handle error conditions first, then skip POLLIN/POLLOUT so
			// we never touch a fd that has already been removed from `clients`.
			if (pfds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
				disconnectClient(pfds[i].fd, "socket error");
				continue;
			}
			if (pfds[i].revents & POLLIN)
				handleClientRead(pfds[i].fd);
			// Guard: handleClientRead may have disconnected the client.
			if (clients.find(pfds[i].fd) == clients.end())
				continue;
			if (pfds[i].revents & POLLOUT)
				handleClientWrite(pfds[i].fd);
		}
	}
	_clear();
}

void Server::rebuildPollFds()
{
	pfds.clear();

	pollfd listenPfd;
	listenPfd.fd      = listenFd;
	listenPfd.events  = POLLIN;
	listenPfd.revents = 0;
	pfds.push_back(listenPfd);

	for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it) {
		pollfd pfd;
		pfd.fd      = it->first;
		pfd.events  = POLLIN;
		pfd.revents = 0;
		if (!it->second.sendBuffer.empty())
			pfd.events |= POLLOUT;
		pfds.push_back(pfd);
	}
}

// ═════════════════════════════════════════════════════════════════════════════
// Accept / read / write / disconnect
// ═════════════════════════════════════════════════════════════════════════════

void Server::acceptClient()
{
		sockaddr_in addr;
		socklen_t   addrLen = sizeof(addr);
		int clientFd = accept(listenFd, (struct sockaddr*)&addr, &addrLen);

		if (clientFd < 0) {
			std::cerr << "accept() failed" << std::endl;
			return;
		}
		if (fcntl(clientFd, F_SETFL, O_NONBLOCK) == -1) {
			close(clientFd);
			std::cerr << "fcntl() failed on accepted socket" << std::endl;
			return;
		}

		Client c;
		c.setFd(clientFd);
		c.setPort(ntohs(addr.sin_port));
		c.setIp(inet_ntoa(addr.sin_addr));
		clients[clientFd] = c;
}

void Server::handleClientRead(int fd)
{
	Client& c = clients[fd];

		char    buf[4096];
		ssize_t n = recv(fd, buf, sizeof(buf), 0);

		if (n > 0) {
			c.recvBuffer.append(buf, n);

			// Process every complete IRC line (terminated by \n).
			while (true) {
				size_t pos = c.recvBuffer.find('\n');
				if (pos == std::string::npos)
					break;

				std::string line = c.recvBuffer.substr(0, pos);
				c.recvBuffer.erase(0, pos + 1);

				// Strip trailing \r and \n.
				if (!line.empty() && line[line.size() - 1] == '\r')
					line.erase(line.size() - 1);

				if (!line.empty()) {
					processLine(fd, line);
					// processLine may have disconnected this client.
					if (clients.find(fd) == clients.end())
						return;
				}
			}
		}
		else if (n == 0) {
			disconnectClient(fd, "Client closed connection");
			return;
		}
		else {
			disconnectClient(fd, "recv error");
			return;
		}
}

void Server::handleClientWrite(int fd)
{
	std::map<int, Client>::iterator it = clients.find(fd);
	if (it == clients.end())
		return;

	Client& c = it->second;
	if (c.sendBuffer.empty())
		return;

	ssize_t n = send(fd, c.sendBuffer.c_str(), c.sendBuffer.size(), 0);
	if (n < 0) {
		disconnectClient(fd, "send() error");
		return;
	}
	if (n > 0)
		c.sendBuffer.erase(0, static_cast<size_t>(n));
}

void Server::disconnectClient(int fd, const std::string& reason)
{
	std::map<int, Client>::iterator it = clients.find(fd);
	if (it == clients.end())
		return;

	// Notify the client before closing (best-effort, direct send).
	std::string errorMsg = "ERROR :Closing link: " + reason + "\r\n";
	send(fd, errorMsg.c_str(), errorMsg.size(), 0);

	removeClientFromAllChannels(fd, "Quit: " + reason);

	if (it->second.getHasNick())
		nickToFd.erase(it->second.getNick());

	close(fd);
	clients.erase(it);
}

// ═════════════════════════════════════════════════════════════════════════════
// Low-level send helpers
// ═════════════════════════════════════════════════════════════════════════════

void Server::sendRaw(int fd, const std::string& msg)
{
	std::map<int, Client>::iterator it = clients.find(fd);
	if (it == clients.end())
		return;
	it->second.sendBuffer += msg + "\r\n";
}

void Server::sendNumeric(int fd, int code, const std::string& msg)
{
	std::string nick = "*";
	std::map<int, Client>::iterator it = clients.find(fd);
	if (it != clients.end() && it->second.getHasNick())
		nick = it->second.getNick();

	std::ostringstream oss;
	// oss << ":" << serverName << " " << code << " " << nick << " " << msg;
	oss << ":" << serverName << " "
        << std::setw(3) << std::setfill('0') << code
        << " " << nick << " " << msg;
	sendRaw(fd, oss.str());
}

// ═════════════════════════════════════════════════════════════════════════════
// Channel helpers
// ═════════════════════════════════════════════════════════════════════════════

std::string Server::clientPrefix(int fd) const
{
	std::map<int, Client>::const_iterator it = clients.find(fd);
	if (it == clients.end())
		return ":unknown!unknown@unknown";

	const Client& c   = it->second;
	std::string   nick = c.getHasNick() ? c.getNick()      : "*";
	std::string   user = c.getHasUser() ? c.getUsername()  : "unknown";
	return ":" + nick + "!" + user + "@" + c.getIp();
}

std::string Server::channelMemberPrefix(const Channel& ch, int memberFd) const
{
	if (ch.operators.find(memberFd) != ch.operators.end())
		return "@";
	return "";
}

void Server::broadcastToChannel(const Channel& ch, const std::string& msg)
{
	for (std::set<int>::const_iterator it = ch.members.begin(); it != ch.members.end(); ++it)
		sendRaw(*it, msg);
}

void Server::sendNamesReply(int fd, const Channel& ch)
{
	std::string namesList;
	for (std::set<int>::const_iterator it = ch.members.begin(); it != ch.members.end(); ++it) {
		std::map<int, Client>::const_iterator cit = clients.find(*it);
		if (cit == clients.end())
			continue;
		if (!namesList.empty())
			namesList += " ";
		namesList += channelMemberPrefix(ch, *it);
		namesList += cit->second.getNick();
	}
	sendNumeric(fd, RPL_NAMREPLY, "= " + ch.name + " :" + namesList);
	sendNumeric(fd, RPL_ENDOFNAMES, ch.name + " :End of /NAMES list");
}

void Server::ensureChannelOperator(Channel& ch)
{
	if (ch.members.empty() || !ch.operators.empty())
		return;

	int firstMember = *ch.members.begin();
	ch.operators.insert(firstMember);

	std::string newOpNick = clients[firstMember].getNick();
	broadcastToChannel(ch, ":" + serverName + " MODE " + ch.name + " +o " + newOpNick);
}

void Server::removeClientFromAllChannels(int fd, const std::string& partReason)
{
	std::vector<std::string> emptyChannels;

	for (std::map<std::string, Channel>::iterator it = channels.begin();
		 it != channels.end(); ++it)
	{
		Channel& ch = it->second;
		if (ch.members.find(fd) == ch.members.end())
			continue;

		broadcastToChannel(ch, clientPrefix(fd) + " PART " + ch.name + " :" + partReason);

		ch.members.erase(fd);
		ch.operators.erase(fd);
		ch.invited.erase(fd);

		ensureChannelOperator(ch);

		if (ch.members.empty())
			emptyChannels.push_back(ch.name);
	}

	for (size_t i = 0; i < emptyChannels.size(); ++i)
		channels.erase(emptyChannels[i]);
}

// ═════════════════════════════════════════════════════════════════════════════
// Registration helpers
// ═════════════════════════════════════════════════════════════════════════════

bool Server::Passaccepted(Client c) const {return c.getPassAccepted();}

bool Server::isRegistered(const Client& c) const
{
	return c.getPassAccepted() && c.getHasNick() && c.getHasUser();
}

void Server::maybeFinishRegistration(int fd)
{
	std::map<int, Client>::iterator it = clients.find(fd);
	if (it == clients.end())
		return;

	Client& c = it->second;
	if (!c.getPassAccepted() || !c.getHasNick() || !c.getHasUser() || c.getWelcome())
		return;

	c.setWelcome(true);
	sendNumeric(fd, RPL_WELCOME,  ":Welcome to the Internet Relay Chat Network "
						+ c.getNick() + "!" + c.getUsername() + "@" + c.getIp());
	sendNumeric(fd, RPL_YOURHOST,  ":Your host is " + serverName + ", running version 1.0");
	sendNumeric(fd, RPL_CREATED,  ":This server was created on January 1 2025");
	sendNumeric(fd, RPL_MYINFO,  serverName + " 1.0 i itkol");
	sendNumeric(fd, RPL_MOTDSTART, ":- " + serverName + " Message of the Day -");
	sendNumeric(fd, RPL_MOTD,      ":-  Welcome to ft_irc!");
	sendNumeric(fd, RPL_MOTD,  	   ":-  42 Network IRC Server");
	sendNumeric(fd, RPL_ENDOFMOTD, ":End of MOTD command");
}

// ═════════════════════════════════════════════════════════════════════════════
// Command parser
// ═════════════════════════════════════════════════════════════════════════════


bool iswhitespace(char c) {
    std::string a = " \t\n\r\v\f";
    return a.find(c) != std::string::npos;
}

std::vector<std::string> Server::parseCommand(const std::string& line)
{
	std::vector<std::string> result;
	size_t i = 0;

	// Skip leading spaces.
	while (i < line.size() && iswhitespace(line[i]))
		++i;
	if (i == line.size())
		return result;

	// Extract command token and upper-case it.
	size_t start = i;
	while (i < line.size() && !iswhitespace(line[i]))
		++i;
	std::string cmd = line.substr(start, i - start);
	for (size_t j = 0; j < cmd.size(); ++j)
		cmd[j] = std::toupper(cmd[j]);
	result.push_back(cmd);

	// Extract remaining parameters.
	while (i < line.size()) {
		while (i < line.size() && iswhitespace(line[i]))
			++i;
		if (i == line.size())
			break;
		if (line[i] == ':') {
			// Trailing parameter: everything after the colon.
			result.push_back(line.substr(i + 1));
			break;
		}
		start = i;
		while (i < line.size() && !iswhitespace(line[i]))
			++i;
		result.push_back(line.substr(start, i - start));
	}
	return result;
}

std::vector<std::string> Server::splitString(const std::string& str, char delimiter)
{
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream stream(str);
	while (std::getline(stream, token, delimiter))
		tokens.push_back(token);
	return tokens;
}

// ═════════════════════════════════════════════════════════════════════════════
// Command dispatcher
// ═════════════════════════════════════════════════════════════════════════════

void Server::processLine(int fd, const std::string& line)
{
	std::vector<std::string> args = parseCommand(line);
	if (args.empty())
		return;
	std::string cmd = args[0];
	if (!_isacmd(cmd)) {
		sendNumeric(fd, ERR_UNKNOWNCOMMAND, cmd + " :Unknown command");
		return;
	}
	
	std::map<int, Client>::iterator it = clients.find(fd);
	if (it == clients.end())
		return;

	// Commands handled before registration is complete.
	if (cmd == "QUIT") {
		std::string reason = (args.size() > 1) ? args[1] : "Quit";
		disconnectClient(fd, reason);
		return;
	}
	if (cmd == "CAP" || cmd == "WHO")
		return;
	if (cmd == "PASS")  { cmdPass(fd, args); return; }

	// cant do nick or any other thing without getting thought PASS

	if(!Passaccepted(it->second)) {
		sendNumeric(fd, ERR_NOTREGISTERED, ":You have not authenticate");
		return;
	} // 3
	if (cmd == "NICK")  { cmdNick(fd, args); return; }
	if (cmd == "USER")  { cmdUser(fd, args); return; }
	if (cmd == "PING") {
		if (args.size() > 1)
			sendRaw(fd, ":" + serverName + " PONG " + serverName + " :" + args[1]);
		else
			sendRaw(fd, ":" + serverName + " PONG " + serverName);
		return;
	}

	// Commands that require a fully registered client.

	if (!isRegistered(it->second)) {
		sendNumeric(fd, ERR_NOTREGISTERED, ":You have not registered");
		return;
	}
	// 7
	if (cmd == "JOIN")    { cmdJoin(fd, args);    return; }
	if (cmd == "PART")    { cmdPart(fd, args);    return; }
	if (cmd == "PRIVMSG") { cmdPrivmsg(fd, args); return; }
	if (cmd == "KICK")    { cmdKick(fd, args);    return; }
	if (cmd == "INVITE")  { cmdInvite(fd, args);  return; }
	if (cmd == "TOPIC")   { cmdTopic(fd, args);   return; }
	if (cmd == "MODE")    { cmdMode(fd, args);    return; }
}

// ═════════════════════════════════════════════════════════════════════════════
// IRC command implementations
// ═════════════════════════════════════════════════════════════════════════════

// ── PASS ─────────────────────────────────────────────────────────────────────

void Server::cmdPass(int fd, const std::vector<std::string>& args)
{
	std::map<int, Client>::iterator it = clients.find(fd);
	if (it == clients.end())
		return;

	Client& c = it->second;

	// FIX: block PASS only once fully welcomed, not as soon as passAccepted is
	// set — this allows the client to retry the password before completing
	// registration (e.g. after a wrong first attempt would have disconnected
	// them, but also keeps the spec requirement of no re-registration).
	if (args.size() < 2) {
		sendNumeric(fd, ERR_NEEDMOREPARAMS, "PASS :Not enough parameters");
		return;
	}
	if (c.getPassAccepted()) {
		sendNumeric(fd, ERR_ALREADYREGISTRED, ":Already Authorized");
		return;
	}
	if (args[1] == password) 
		c.setPassAccepted(true);
	else
		sendNumeric(fd, ERR_PASSWDMISMATCH, ":Password incorrect");
}

// ── NICK ─────────────────────────────────────────────────────────────────────

static bool _isValidNick(const std::string nick)
{
	if (nick.empty() || nick.size() > 30) // 30 max limit in nickname
		return false;
	const std::string special = "-_[]{}\\`|";
	for (size_t i = 0; i < nick.size(); ++i)
	{
		char c = nick[i];
		if (i == 0 && isdigit(c))
			return false;
		if (!isalnum(c) && special.find(c) == std::string::npos)
			return false;
	}
	return true;
}

void Server::cmdNick(int fd, const std::vector<std::string>& args)
{
	// FIX: removed stray semicolon that was present after the opening brace.
	std::map<int, Client>::iterator it = clients.find(fd); // why always need to find the client why nt pass it as anrgumant !?
	if (it == clients.end())
		return;

	if (args.size() < 2) {
		sendNumeric(fd, ERR_NEEDMOREPARAMS, "NICK :Not enough parameters");
		return;
	}

	const std::string& nick = args[1];

	if (!_isValidNick(nick)) {
		sendNumeric(fd, ERR_ERRONEUSNICKNAME, nick + " :Erroneous nickname");
		return;
	}

	// Check for collision with another client.
	std::map<std::string, int>::iterator ni = nickToFd.find(nick);
	if (ni != nickToFd.end() && ni->second != fd) {
		sendNumeric(fd, ERR_NICKNAMEINUSE, nick + " :Nickname is already in use");
		return;
	}

	Client& c = it->second;

	if (c.getHasNick()) {
		const std::string oldNick = c.getNick();
		if (oldNick == nick)
			return; // No-op: nick unchanged.

		// FIX: broadcast NICK change to all channels the client is in so other
		// users are informed of the new nick.
		std::string nickMsg = clientPrefix(fd) + " NICK :" + nick;
		std::set<int> notified;
		notified.insert(fd);

		for (std::map<std::string, Channel>::iterator chIt = channels.begin();
			 chIt != channels.end(); ++chIt)
		{
			const Channel& ch = chIt->second;
			if (ch.members.find(fd) == ch.members.end())
				continue;
			for (std::set<int>::const_iterator m = ch.members.begin();
				 m != ch.members.end(); ++m)
			{
				if (notified.insert(*m).second)
					sendRaw(*m, nickMsg);
			}
		}
		// Notify the client itself if it wasn't in any channel.
		if (notified.find(fd) == notified.end() || notified.size() == 1)
			sendRaw(fd, nickMsg);

		nickToFd.erase(oldNick);
	}

	c.setNick(nick);
	c.setHasNick(true);
	nickToFd[nick] = fd;
	maybeFinishRegistration(fd);
}

// ── USER ─────────────────────────────────────────────────────────────────────

bool parseUserMode(const std::string token, int *value)
{
    if (token.empty())
        return false;

    for (std::string::size_type i = 0; i < token.size(); ++i)
    {
        if (!std::isdigit(static_cast<unsigned char>(token[i])))
            return false;
    }

    std::istringstream iss(token);
    iss >> (*value);
    return !iss.fail();
}


void Server::cmdUser(int fd, const std::vector<std::string>& args)
{
	std::map<int, Client>::iterator it = clients.find(fd);
	if (it == clients.end())
		return;

	Client& c = it->second;

	if (c.getHasUser()) {
		sendNumeric(fd, ERR_ALREADYREGISTRED, ":Already Registred");
		return;
	}
	if (args.size() != 5) {
		sendNumeric(fd, ERR_NEEDMOREPARAMS, "USER :Not enough parameters");
		return;
	}
	int	mode;
	if (!parseUserMode(args[2], &mode))
		mode = 0;
	// USER <username> <mode> <unused> :<realname>
	c.setUsername(args[1]);
	c.setRealname(args[4]);
	(void)mode; // ignore for now
	c.setHasUser(true);
	maybeFinishRegistration(fd);
}

// ── JOIN ─────────────────────────────────────────────────────────────────────

void Server::cmdJoin(int fd, const std::vector<std::string>& args)
{
	if (args.size() < 2) {
		sendNumeric(fd, ERR_NEEDMOREPARAMS, "JOIN :Not enough parameters");
		return;
	}

	// if (args[1] == "0") { // part from all
	// 	std::vector<std::string>	chans = _GetClient(fd).channels;
	// 	for (size_t i = 0; i < chans.size(); ++i) {
	// 		Channel& c = channels[chans.at(i)];
			
	// 		broadcastToChannel(c, clientPrefix(fd) + " PART " + chans.at(i));
	// 		// _GetClient(fd).channels.erase() // need to remove the channel name from this vector and do it too to PART command
	// 		c.members.erase(fd);
	// 		c.operators.erase(fd);
	// 		c.invited.erase(fd);

	// 		ensureChannelOperator(c);

	// 		if (c.members.empty())
	// 			channels.erase(chans.at(i));
	// 	}
	// 	return;
	// }

	// JOIN #1,#2,#3 key1, key2
	std::vector<std::string> channelNames = splitString(args[1], ',');
	std::vector<std::string> keys;
	if (args.size() > 2)
		keys = splitString(args[2], ',');

	for (size_t i = 0; i < channelNames.size(); ++i) {
		const std::string& channelName = channelNames[i];

		if (channelName.empty() || channelName[0] != '#') {
			sendNumeric(fd, ERR_NOSUCHCHANNEL, channelName + " :No such channel");
			continue;
		}

		// Create channel on first join.
		if (channels.find(channelName) == channels.end()) {
			Channel newCh;
			newCh.name = channelName;
			channels[channelName] = newCh;
		}

		Channel& ch = channels[channelName];

		if (ch.members.find(fd) != ch.members.end())
			continue; // Already in channel.

		if (ch.inviteOnly && ch.invited.find(fd) == ch.invited.end()) {
			sendNumeric(fd, ERR_INVITEONLYCHAN, channelName + " :Cannot join channel (+i)");
			continue;
		}

		std::string key = (i < keys.size()) ? keys[i] : "";
		if (ch.hasKey && key != ch.key) {
			sendNumeric(fd, ERR_BADCHANNELKEY, channelName + " :Cannot join channel (+k)");
			continue;
		}

		if (ch.userLimit > 0 && static_cast<int>(ch.members.size()) >= ch.userLimit) {
			sendNumeric(fd, ERR_CHANNELISFULL, channelName + " :Cannot join channel (+l)");
			continue;
		}

		// Admit the client.
		ch.members.insert(fd);
		// _GetClient(fd).channels.push_back(channelName); // -------------
		if (ch.members.size() == 1)
			ch.operators.insert(fd); // First joiner becomes operator.
		ch.invited.erase(fd);

		broadcastToChannel(ch, clientPrefix(fd) + " JOIN " + channelName);

		if (!ch.topic.empty())
			sendNumeric(fd, RPL_TOPIC, channelName + " :" + ch.topic);
		else
			sendNumeric(fd, RPL_NOTOPIC, channelName + " :No topic is set");

		sendNamesReply(fd, ch);
	}
}

// ── PART ─────────────────────────────────────────────────────────────────────

void Server::cmdPart(int fd, const std::vector<std::string>& args)
{
	if (args.size() < 2) {
		sendNumeric(fd, ERR_NEEDMOREPARAMS, "PART :Not enough parameters");
		return;
	}

	std::vector<std::string> channelName = splitString(args[1], ',');
	// const std::string& channelName = args[1];
	std::string reason = (args.size() > 2) ? args[2] : clients[fd].getNick();
	for (size_t i = 0; i < channelName.size(); ++i) {
		if (channelName.at(i).empty() || channelName.at(i)[0] != '#') {
			sendNumeric(fd, ERR_NOSUCHCHANNEL, channelName.at(i) + " :No such channel");
			return;
		}
	
		std::map<std::string, Channel>::iterator chIt = channels.find(channelName.at(i));
		if (chIt == channels.end()) {
			sendNumeric(fd, ERR_NOSUCHCHANNEL, channelName.at(i) + " :No such channel");
			return;
		}
	
		Channel& ch = chIt->second;
	
		if (ch.members.find(fd) == ch.members.end()) {
			sendNumeric(fd, ERR_NOTONCHANNEL, channelName.at(i) + " :You're not on that channel");
			return;
		}
	
		broadcastToChannel(ch, clientPrefix(fd) + " PART " + channelName.at(i) + " :" + reason);
	
		ch.members.erase(fd);
		ch.operators.erase(fd);
		ch.invited.erase(fd);
	
		ensureChannelOperator(ch);
	
		if (ch.members.empty())
			channels.erase(channelName.at(i));
	}
}

// ── PRIVMSG ──────────────────────────────────────────────────────────────────

void Server::cmdPrivmsg(int fd, const std::vector<std::string>& args)
{
	if (args.size() < 3) {
		sendNumeric(fd, ERR_NEEDMOREPARAMS, "PRIVMSG :Not enough parameters");
		return;
	}


	std::vector<std::string> target = splitString(args[1], ',');
	// const std::string& target  = args[1];
	const std::string& message = args[2];
	for (size_t	i = 0; i < target.size(); ++i) {
		if (!target.at(i).empty() && target.at(i)[0] == '#') {
			// Channel message.
			std::map<std::string, Channel>::iterator chIt = channels.find(target.at(i));
			if (chIt == channels.end()) {
				sendNumeric(fd, ERR_NOSUCHCHANNEL, target.at(i) + " :No such channel");
				return;
			}
			Channel& ch = chIt->second;
			if (ch.members.find(fd) == ch.members.end()) {
				sendNumeric(fd, ERR_NOTONCHANNEL, target.at(i) + " :You're not on that channel");
				return;
			}
			std::string msg = clientPrefix(fd) + " PRIVMSG " + target.at(i) + " :" + message;
			for (std::set<int>::const_iterator it = ch.members.begin();it != ch.members.end(); ++it)
			{
				if (*it != fd)
					sendRaw(*it, msg);
			}
		} else {
			// Private message to a nick.
			std::map<std::string, int>::iterator it = nickToFd.find(target.at(i));
			if (it == nickToFd.end()) {
				sendNumeric(fd, ERR_NOSUCHNICK, target.at(i) + " :No such nick");
				return;
			}
			sendRaw(it->second, clientPrefix(fd) + " PRIVMSG " + target.at(i) + " :" + message);
		}
	}
}

// ── KICK ─────────────────────────────────────────────────────────────────────

void Server::cmdKick(int fd, const std::vector<std::string>& args)
{
    if (args.size() < 3) {
        sendNumeric(fd, ERR_NEEDMOREPARAMS, "KICK :Not enough parameters");
        return;
    }

    std::vector<std::string> chanVals = splitString(args[1], ',');
    std::vector<std::string> userVals = splitString(args[2], ',');

    std::string reason = (args.size() > 3) ? args[3] : clients[fd].getNick();

    if (chanVals.size() != 1 && chanVals.size() != userVals.size()) {
        sendNumeric(fd, ERR_NEEDMOREPARAMS, "KICK :Mismatched channel and user lists");
        return;
    }

    for (size_t i = 0; i < userVals.size(); ++i) {
        
        std::string channelName = (chanVals.size() == 1) ? chanVals[0] : chanVals[i];
        std::string targetNick  = userVals[i];

        std::map<std::string, Channel>::iterator chIt = channels.find(channelName);
        if (chIt == channels.end()) {
            sendNumeric(fd, ERR_NOSUCHCHANNEL, channelName + " :No such channel");
            continue;
        }

        Channel& ch = chIt->second;

        if (ch.members.find(fd) == ch.members.end()) {
            sendNumeric(fd, ERR_NOTONCHANNEL, channelName + " :You're not on that channel");
            continue; 
        }
        if (ch.operators.find(fd) == ch.operators.end()) {
            sendNumeric(fd, ERR_CHANOPRIVSNEEDED, channelName + " :You're not channel operator");
            continue;
        }

        std::map<std::string, int>::iterator ni = nickToFd.find(targetNick);
        if (ni == nickToFd.end()) {
            sendNumeric(fd, ERR_NOSUCHNICK, targetNick + " :No such nick");
            continue;
        }

        int targetFd = ni->second;
        if (ch.members.find(targetFd) == ch.members.end()) {
            sendNumeric(fd, ERR_USERNOTINCHANNEL, targetNick + " " + channelName + " :They aren't on that channel");
            continue;
        }

        broadcastToChannel(ch, clientPrefix(fd) + " KICK " + channelName
                               + " " + targetNick + " :" + reason);

        ch.members.erase(targetFd);
        ch.operators.erase(targetFd);
        ch.invited.erase(targetFd);

        ensureChannelOperator(ch);

        if (ch.members.empty()) {
            channels.erase(channelName);
        }
    }
}

// ── INVITE ───────────────────────────────────────────────────────────────────

void Server::cmdInvite(int fd, const std::vector<std::string>& args)
{
	if (args.size() < 3) {
		sendNumeric(fd, ERR_NEEDMOREPARAMS, "INVITE :Not enough parameters");
		return;
	}

	const std::string& targetNick  = args[1];
	const std::string& channelName = args[2];

	std::map<std::string, Channel>::iterator chIt = channels.find(channelName);
	if (chIt == channels.end()) {
		sendNumeric(fd, ERR_NOSUCHCHANNEL, channelName + " :No such channel");
		return;
	}

	Channel& ch = chIt->second;

	if (ch.members.find(fd) == ch.members.end()) {
		sendNumeric(fd, ERR_NOTONCHANNEL, channelName + " :You're not on that channel");
		return;
	}
	if (ch.operators.find(fd) == ch.operators.end()) {
		sendNumeric(fd, ERR_CHANOPRIVSNEEDED, channelName + " :You're not channel operator");
		return;
	}

	std::map<std::string, int>::iterator ni = nickToFd.find(targetNick);
	if (ni == nickToFd.end()) {
		sendNumeric(fd, ERR_NOSUCHNICK, targetNick + " :No such nick");
		return;
	}

	int targetFd = ni->second;
	if (ch.members.find(targetFd) != ch.members.end()) {
		sendNumeric(fd, ERR_USERONCHANNEL, targetNick + " " + channelName + " :is already on channel");
		return;
	}

	ch.invited.insert(targetFd);
	sendRaw(targetFd, clientPrefix(fd) + " INVITE " + targetNick + " :" + channelName);
	sendNumeric(fd, RPL_INVITING , targetNick + " " + channelName);
}

// ── TOPIC ────────────────────────────────────────────────────────────────────

void Server::cmdTopic(int fd, const std::vector<std::string>& args)
{
	if (args.size() < 2) {
		sendNumeric(fd, ERR_NEEDMOREPARAMS, "TOPIC :Not enough parameters");
		return;
	}

	const std::string& channelName = args[1];

	std::map<std::string, Channel>::iterator chIt = channels.find(channelName);
	if (chIt == channels.end()) {
		sendNumeric(fd, ERR_NOSUCHCHANNEL, channelName + " :No such channel");
		return;
	}

	Channel& ch = chIt->second;

	if (ch.members.find(fd) == ch.members.end()) {
		sendNumeric(fd, ERR_NOTONCHANNEL, channelName + " :You're not on that channel");
		return;
	}

	if (args.size() == 2) {
		// Query current topic.
		if (ch.topic.empty())
			sendNumeric(fd, RPL_NOTOPIC, channelName + " :No topic is set");
		else
			sendNumeric(fd, RPL_TOPIC, channelName + " :" + ch.topic);
		return;
	}

	// Set new topic.
	if (ch.topicOpOnly && ch.operators.find(fd) == ch.operators.end()) {
		sendNumeric(fd, ERR_CHANOPRIVSNEEDED, channelName + " :You're not channel operator");
		return;
	}

	ch.topic = args[2];
	broadcastToChannel(ch, clientPrefix(fd) + " TOPIC " + channelName + " :" + ch.topic);
}

// ── MODE ─────────────────────────────────────────────────────────────────────

void Server::cmdMode(int fd, const std::vector<std::string>& args)
{
	if (args.size() < 2) {
		sendNumeric(fd, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters");
		return;
	}

	const std::string& target = args[1];
	if (target.empty() || target[0] != '#') {
		sendNumeric(fd, ERR_USERSDONTMATCH, ":Cannot change user mode");
		return;
	}

	std::map<std::string, Channel>::iterator chIt = channels.find(target);
	if (chIt == channels.end()) {
		sendNumeric(fd, ERR_NOSUCHCHANNEL, target + " :No such channel");
		return;
	}

	Channel& ch = chIt->second;

	if (ch.members.find(fd) == ch.members.end()) {
		sendNumeric(fd, ERR_NOTONCHANNEL, target + " :You're not on that channel");
		return;
	}

	// Query current modes (no operator required).
	if (args.size() == 2) {
		std::string modeStr = "+";
		if (ch.inviteOnly)    modeStr += "i";
		if (ch.topicOpOnly)   modeStr += "t";
		if (ch.hasKey)        modeStr += "k";
		if (ch.userLimit > 0) modeStr += "l";
		sendNumeric(fd, RPL_CHANNELMODEIS, target + " " + modeStr);
		return;
	}

	if (ch.operators.find(fd) == ch.operators.end()) {
		sendNumeric(fd, ERR_CHANOPRIVSNEEDED, target + " :You're not channel operator");
		return;
	}

	const std::string& modeStr = args[2];
	bool   adding   = true;
	size_t argIndex = 3;

	std::string appliedModes = "";
	std::vector<std::string> appliedArgs;
	char lastSign = '\0';

	for (size_t i = 0; i < modeStr.size(); ++i) {
		char m = modeStr[i];

		if      (m == '+') { adding = true;  continue; }
		else if (m == '-') { adding = false; continue; }

		bool modeChanged = false;
		std::string appliedArg = "";

		if (m == 'i') {
			if (ch.inviteOnly != adding) { 
				ch.inviteOnly = adding; 
				modeChanged = true; 
			}
		} else if (m == 't') {
			if (ch.topicOpOnly != adding) { 
				ch.topicOpOnly = adding; 
				modeChanged = true; 
			}
		} else if (m == 'k') {
			if (adding) {
				if (argIndex >= args.size()) {
					sendNumeric(fd, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters for +k");
					continue;
				}
				std::string newKey = args[argIndex++];
				if (!ch.hasKey || ch.key != newKey) {
					ch.hasKey = true;
					ch.key = newKey;
					modeChanged = true;
					appliedArg = newKey;
				}
			} else {
				if (ch.hasKey) {
					ch.hasKey = false;
					ch.key.clear();
					modeChanged = true;
				}
			}
		} else if (m == 'o') {
			if (argIndex >= args.size()) {
				sendNumeric(fd, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters for +/-o");
				continue;
			}
			const std::string& targetNick = args[argIndex++];
			std::map<std::string, int>::iterator ni = nickToFd.find(targetNick);
			
			if (ni == nickToFd.end() || ch.members.find(ni->second) == ch.members.end()) {
				sendNumeric(fd, ERR_USERNOTINCHANNEL, targetNick + " " + target + " :They aren't on that channel");
				continue;
			}
			
			int targetFd = ni->second;
			if (adding) {
				if (ch.operators.find(targetFd) == ch.operators.end()) {
					ch.operators.insert(targetFd);
					modeChanged = true;
					appliedArg = targetNick;
				}
			} else {
				if (ch.operators.find(targetFd) != ch.operators.end()) {
					ch.operators.erase(targetFd);
					ensureChannelOperator(ch);
					modeChanged = true;
					appliedArg = targetNick;
				}
			}
		} else if (m == 'l') {
			if (adding) {
				if (argIndex >= args.size()) {
					sendNumeric(fd, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters for +l");
					continue;
				}
				std::string limitStr = args[argIndex++];
				int limit = std::atoi(limitStr.c_str());
				if (limit > 0 && ch.userLimit != limit) {
					ch.userLimit = limit;
					modeChanged = true;
					appliedArg = limitStr;
				}
			} else {
				if (ch.userLimit != 0) {
					ch.userLimit = 0;
					modeChanged = true;
				}
			}
		} else {
			// Catch all unknown modes and send error 472
			sendNumeric(fd, ERR_UNKNOWNMODE, std::string(1, m) + " :is unknown mode char to me");
			continue;
		}

		// Only append to the output string if the mode successfully updated the channel state
		if (modeChanged) {
			char currentSign = adding ? '+' : '-';
			if (currentSign != lastSign) {
				appliedModes += currentSign;
				lastSign = currentSign;
			}
			appliedModes += m;
			if (!appliedArg.empty()) {
				appliedArgs.push_back(appliedArg);
			}
		}
	}

	// If nothing changed, do not broadcast a message
	if (appliedModes.empty())
		return;

	// Construct and echo the cleanly filtered mode change back to the channel
	std::string modeChangeMsg = clientPrefix(fd) + " MODE " + target + " " + appliedModes;
	for (size_t i = 0; i < appliedArgs.size(); ++i) {
		modeChangeMsg += " " + appliedArgs[i];
	}
	broadcastToChannel(ch, modeChangeMsg);
}

void	Server::_clear() {
	for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
		close(it->first);
	clients.clear();
	channels.clear();
	nickToFd.clear();
}
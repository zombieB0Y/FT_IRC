#include "../include/IrcServer.hpp"
#include <ctime>
#include <cstring>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <cerrno>

volatile bool IrcServer::running = true;

// ─────────────────────────────────────────────────────────────────────────────
// Constructor / Destructor
// ─────────────────────────────────────────────────────────────────────────────
IrcServer::IrcServer(int port, const std::string &password)
	: _serverFd(-1), _epollFd(-1), _port(port), _password(password)
{
	time_t now = time(NULL);
	_createdAt = std::string(ctime(&now));
	if (!_createdAt.empty() && _createdAt[_createdAt.size()-1] == '\n')
		_createdAt.erase(_createdAt.size()-1);

	_initcommadlist();
	_initSocket();
	_initEpoll();
}

IrcServer::~IrcServer()
{
	// Disconnect all clients
	for (std::map<int,Client*>::iterator it = _clients.begin();
		 it != _clients.end(); ++it)
	{
		close(it->first);
		delete it->second;
	}
	// Delete channels
	for (std::map<std::string,Channel*>::iterator it = _channels.begin();
		 it != _channels.end(); ++it)
		delete it->second;
	if (_epollFd >= 0) close(_epollFd);
	if (_serverFd >= 0) close(_serverFd);
}

void IrcServer::_initcommadlist() {

	this->_commandslist.push_back("CAP");
	this->_commandslist.push_back("QUIT");
	this->_commandslist.push_back("PASS");
	this->_commandslist.push_back("NICK");
	this->_commandslist.push_back("USER");
	this->_commandslist.push_back("PING");
	this->_commandslist.push_back("PONG");
	this->_commandslist.push_back("JOIN");
	this->_commandslist.push_back("PART");
	this->_commandslist.push_back("PRIVMSG");
	this->_commandslist.push_back("NOTICE");
	this->_commandslist.push_back("KICK");
	this->_commandslist.push_back("INVITE");
	this->_commandslist.push_back("TOPIC");
	this->_commandslist.push_back("MODE");
	this->_commandslist.push_back("WHO");
	this->_commandslist.push_back("WHOIS");
	this->_commandslist.push_back("LIST");
	this->_commandslist.push_back("NAMES");
	this->_commandslist.push_back("OPER");
	this->_commandslist.push_back("KILL");
}

bool	IrcServer::_isacmd(std::string &m) const {
	if (std::find(_commandslist.begin(), _commandslist.end(), m) == _commandslist.end())
		return false;
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Socket & Epoll init
// ─────────────────────────────────────────────────────────────────────────────
void IrcServer::_initSocket()
{
	_serverFd = socket(AF_INET, SOCK_STREAM, 0);
	if (_serverFd < 0) throw std::runtime_error("socket() failed");

	int opt = 1;
	setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	// Non-blocking
	int flags = fcntl(_serverFd, F_GETFL, 0);
	fcntl(_serverFd, F_SETFL, flags | O_NONBLOCK);

	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port        = htons(_port);

	if (bind(_serverFd, (sockaddr*)&addr, sizeof(addr)) < 0)
		throw std::runtime_error("bind() failed");
	if (listen(_serverFd, SOMAXCONN) < 0)
		throw std::runtime_error("listen() failed");

	std::cout << "[ft_irc] Listening on port " << _port << std::endl;
}

void IrcServer::_initEpoll()
{
	_epollFd = epoll_create1(0);
	if (_epollFd < 0) throw std::runtime_error("epoll_create1() failed");

	epoll_event ev;
	ev.events  = EPOLLIN;
	ev.data.fd = _serverFd;
	epoll_ctl(_epollFd, EPOLL_CTL_ADD, _serverFd, &ev);
}

// ─────────────────────────────────────────────────────────────────────────────
// Main loop
// ─────────────────────────────────────────────────────────────────────────────
void IrcServer::run()
{
	epoll_event events[MAX_EVENTS];

	while (running)
	{
		int n = epoll_wait(_epollFd, events, MAX_EVENTS, 100);
		if (n < 0)
		{
			if (errno == EINTR) continue;
			break;
		}
		for (int i = 0; i < n; ++i)
		{
			int fd = events[i].data.fd;
			if (fd == _serverFd)
			{
				_acceptClient();
			}
			else if (events[i].events & (EPOLLERR | EPOLLHUP))
			{
				_disconnectClient(fd, "Connection reset");
			}
			else if (events[i].events & EPOLLIN)
			{
				_handleClient(fd);
			}
			else if (events[i].events & EPOLLOUT)
			{
				_flushClient(fd);
			}
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Accept new connection
// ─────────────────────────────────────────────────────────────────────────────
void IrcServer::_acceptClient()
{
	sockaddr_in addr;
	socklen_t   len = sizeof(addr);
	int fd = accept(_serverFd, (sockaddr*)&addr, &len);
	if (fd < 0) 
		return;
	int flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);

	std::string host = inet_ntoa(addr.sin_addr);
	Client *client   = new Client(fd, host);
	_clients[fd]     = client;

	epoll_event ev;
	ev.events  = EPOLLIN | EPOLLET;
	ev.data.fd = fd;
	epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &ev);

	std::cout << "[+] Client connected: fd=" << fd << " host=" << host << std::endl;
	_send(client, " ---------- irc server ! --------- ");
	client->flushOutput();
}

// ─────────────────────────────────────────────────────────────────────────────
// Read data from client
// ─────────────────────────────────────────────────────────────────────────────
void IrcServer::_handleClient(int fd)
{
	std::map<int,Client*>::iterator it = _clients.find(fd);
	if (it == _clients.end())
		return;
	Client *client = it->second;

	char buf[BUFFER_SIZE];
	while (true)
	{
		ssize_t n = recv(fd, buf, sizeof(buf) - 1, 0);
		if (n > 0)
		{
			buf[n] = '\0';
			client->appendBuffer(std::string(buf, n));
		}
		else if (n == 0)
		{
			_disconnectClient(fd, "Connection closed");
			return;
		}
		else
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			_disconnectClient(fd, "Read error");
			return;
		}
	}
	std::string data = client->getBuffer();
	client->clearBuffer();

	std::string::size_type pos;
	while ((pos = data.find('\n')) != std::string::npos)
	{
		std::string line = data.substr(0, pos);
		data.erase(0, pos + 1);
		if (!line.empty() && line[line.size()-1] == '\r')
			line.erase(line.size()-1);
		if (!line.empty())
			_processMessage(client, line);
		if (_clients.find(fd) == _clients.end())
			return;
	}
	if (!data.empty())
		client->appendBuffer(data);
	client->flushOutput();
}

void IrcServer::_flushClient(int fd)
{
	std::map<int,Client*>::iterator it = _clients.find(fd);
	if (it != _clients.end())
		it->second->flushOutput();
}

// ─────────────────────────────────────────────────────────────────────────────
// Disconnect
// ─────────────────────────────────────────────────────────────────────────────
void IrcServer::_disconnectClient(int fd, const std::string &reason)
{
	std::map<int,Client*>::iterator it = _clients.find(fd);
	if (it == _clients.end())
		return;
	Client *client = it->second;
	std::vector<Channel*> chans = client->getChannels();
	for (size_t i = 0; i < chans.size(); ++i)
	{
		Channel *ch = chans[i];
		std::string qmsg = ":" + client->getPrefix() + " QUIT :" + reason + "\r\n";
		ch->broadcast(qmsg, client);
		ch->removeMember(client);
		if (ch->getMemberCount() == 0)
		{
			std::string chanName = ch->getName();
			_channels.erase(chanName);
			delete ch;
		}
	}

	std::cout << "[-] Client disconnected: fd=" << fd
			  << " nick=" << client->getNick()
			  << " (" << reason << ")" << std::endl;

	epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL);
	close(fd);
	_clients.erase(fd);

	delete client;
}

// ─────────────────────────────────────────────────────────────────────────────
// Message parsing
// ─────────────────────────────────────────────────────────────────────────────
IrcMessage IrcServer::_parseMessage(const std::string &raw)
{
	IrcMessage msg;
	std::string str = raw;

	if (!str.empty() && str[0] == ':')
	{
		std::string::size_type sp = str.find(' ');
		msg.prefix = str.substr(1, sp - 1);
		str.erase(0, sp + 1);
	}
	size_t sp = str.find(' ');
	if (sp == std::string::npos)
	{
		std::string holder;
		msg.command = str;
		holder = str;
		// std::cout << str << std::endl;
		
		for (size_t i = 0; i < msg.command.size(); ++i)
			msg.command[i] = (char)toupper((unsigned char)msg.command[i]);
		if (!_isacmd(msg.command))
			msg.command = holder;
		return msg;
	}
	msg.command = str.substr(0, sp);
	str.erase(0, sp + 1);
	for (size_t i = 0; i < msg.command.size(); ++i)
		msg.command[i] = (char)toupper((unsigned char)msg.command[i]);
	// std::cout << msg.command << std::endl;
	while (!str.empty())
	{
		if (str[0] == ':')
		{
			msg.params.push_back(str.substr(1));
			break;
		}
		sp = str.find(' ');
		if (sp == std::string::npos)
		{
			msg.params.push_back(str);
			break;
		}
		msg.params.push_back(str.substr(0, sp));
		str.erase(0, sp + 1);
	}
	return msg;
}

void IrcServer::_processMessage(Client *client, const std::string &raw)
{
	IrcMessage msg = _parseMessage(raw);
	if (msg.command.empty())
		return;
	int fd = client->getFd();
	_dispatch(client, msg);
	if (_clients.find(fd) != _clients.end())
		client->flushOutput();
}

// ─────────────────────────────────────────────────────────────────────────────
// Dispatch table
// ─────────────────────────────────────────────────────────────────────────────
void IrcServer::_dispatch(Client *client, const IrcMessage &msg)
{
	const std::string &cmd = msg.command;
	
	if (cmd == "CAP")
		return;
	if (cmd == "QUIT") {
		_cmdQuit(client, msg);
		return;
	}
	if (cmd == "PASS") {
		_cmdPass(client, msg);
		return;
	}
	if (!client->isPassAccepted()) {
		_sendErr(client, ERR_NOTREGISTERED, ":You have not authenticate");
		return;
	}
	if (cmd == "NICK") {
		_cmdNick(client, msg);
		return;
	}
	if (cmd == "USER") {
		_cmdUser(client, msg);
		return;
	}
	if (cmd == "PING") {
		_cmdPing(client, msg);
		return;
	}
	if (cmd == "PONG") {
		_cmdPong(client, msg);
		return;
	}
	if (!client->isRegistered())
	{
		_sendErr(client, ERR_NOTREGISTERED, ":You have not registered");
		return;
	}

	if (cmd == "JOIN")    {
		_cmdJoin(client, msg);   
		return;
	}
	if (cmd == "PART")    {
		_cmdPart(client, msg);
		return;
	}
	if (cmd == "PRIVMSG") {
		_cmdPrivmsg(client, msg);
		return;
	}
	if (cmd == "NOTICE")  {
		_cmdNotice(client, msg);
		return;
	}
	if (cmd == "KICK")    {
		_cmdKick(client, msg);
		return;
	}
	if (cmd == "INVITE")  {
		_cmdInvite(client, msg);
		return;
	}
	if (cmd == "TOPIC")   {
		_cmdTopic(client, msg);
		return;
	}
	if (cmd == "MODE")    {
		_cmdMode(client, msg);   
		return;
	}
	if (cmd == "WHO")     {
		_cmdWho(client, msg);    
		return;
	}
	if (cmd == "WHOIS")   {
		_cmdWhois(client, msg);  
		return;
	}
	if (cmd == "LIST")    {
		_cmdList(client, msg);   
		return;
	}
	if (cmd == "NAMES")   {
		_cmdNames(client, msg);  
		return;
	}
	if (cmd == "OPER")    {
		_cmdOper(client, msg);   
		return;
	}
	if (cmd == "KILL")    {
		_cmdKill(client, msg);   
		return;
	}

	_sendErr(client, ERR_UNKNOWNCOMMAND, cmd + " :Unknown command");
}

// ─────────────────────────────────────────────────────────────────────────────
// Utility helpers
// ─────────────────────────────────────────────────────────────────────────────
std::string IrcServer::_num(const std::string &code, Client *c, const std::string &text) const
{
	return ":" + std::string(SERVER_NAME) + " " + code + " " + c->getNick() + " " + text + "\r\n";
}

void IrcServer::_send(Client *c, const std::string &msg) const
{
	c->sendMsg(msg);
}

void IrcServer::_sendErr(Client *c, const std::string &code, const std::string &text) const
{
	_send(c, _num(code, c, text));
}

bool IrcServer::_isNickInUse(const std::string &nick) const
{
	for (std::map<int,Client*>::const_iterator it = _clients.begin();
		 it != _clients.end(); ++it)
		if (it->second->getNick() == nick)
			return true;
	return false;
}

bool IrcServer::_isValidNick(const std::string &nick) const
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

bool IrcServer::_isValidChannel(const std::string &name) const
{
	if (name.size() < 2 || name.size() > 50)
		return false;
	return (name[0] == '#' || name[0] == '&');
}

Client* IrcServer::_getClientByNick(const std::string &nick) const
{
	for (std::map<int,Client*>::const_iterator it = _clients.begin();
		 it != _clients.end(); ++it)
		if (it->second->getNick() == nick) return it->second;
	return NULL;
}

Channel* IrcServer::_getChannel(const std::string &name) const
{
	std::map<std::string,Channel*>::const_iterator it = _channels.find(name);
	if (it != _channels.end())
		return it->second;
	return NULL;
}

Channel* IrcServer::_getOrCreateChannel(const std::string &name, Client *creator)
{
	Channel *ch = _getChannel(name);
	if (!ch)
	{
		ch = new Channel(name, creator);
		_channels[name] = ch;
	}
	return ch;
}

void IrcServer::_removeChannelIfEmpty(const std::string &name)
{
	Channel *ch = _getChannel(name);
	if (ch && ch->getMemberCount() == 0)
	{
		delete ch;
		_channels.erase(name);
	}
}

void IrcServer::_sendWelcome(Client *c)
{
	std::string nick = c->getNick();
	_send(c, _num(RPL_WELCOME, c,
		":Welcome to the " + std::string(SERVER_NAME) + " IRC Network " + c->getPrefix()));
	_send(c, _num(RPL_YOURHOST, c,
		":Your host is " + std::string(SERVER_NAME) + ", running version " + std::string(SERVER_VER)));
	_send(c, _num(RPL_CREATED, c,
		":This server was created " + _createdAt));
	_send(c, _num(RPL_MYINFO, c,
		SERVER_NAME " " SERVER_VER " o itkol"));
	// MOTD
	_send(c, _num(RPL_MOTDSTART, c, ":- " SERVER_NAME " Message of the Day -"));
	_send(c, _num(RPL_MOTD, c,      ":-  Welcome to ft_irc!"));
	_send(c, _num(RPL_MOTD, c,      ":-  42 Network IRC Server"));
	_send(c, _num(RPL_ENDOFMOTD, c, ":End of MOTD command"));
}

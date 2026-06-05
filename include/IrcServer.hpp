#pragma once
#include "Server.hpp"

struct IrcMessage
{
	std::string prefix;
	std::string command;
	std::vector<std::string> params;
};

class IrcServer
{
private:
	// Core networking
	void _initSocket();
	void _initEpoll();
	void _acceptClient();
	void _handleClient(int fd);
	void _disconnectClient(int fd, const std::string &reason = "");
	void _flushClient(int fd);

	// Message parsing & dispatch
	IrcMessage _parseMessage(const std::string &raw);
	void _processMessage(Client *client, const std::string &raw);
	void _dispatch(Client *client, const IrcMessage &msg);

	// ── Command Handlers ──────────────────────────────────────────
	void _cmdPass(Client *c, const IrcMessage &m);
	void _cmdNick(Client *c, const IrcMessage &m);
	void _cmdUser(Client *c, const IrcMessage &m);
	void _cmdQuit(Client *c, const IrcMessage &m);
	void _cmdJoin(Client *c, const IrcMessage &m);
	void _cmdPart(Client *c, const IrcMessage &m);
	void _cmdPrivmsg(Client *c, const IrcMessage &m);
	void _cmdNotice(Client *c, const IrcMessage &m);
	void _cmdKick(Client *c, const IrcMessage &m);
	void _cmdInvite(Client *c, const IrcMessage &m);
	void _cmdTopic(Client *c, const IrcMessage &m);
	void _cmdMode(Client *c, const IrcMessage &m);
	void _cmdWho(Client *c, const IrcMessage &m);
	void _cmdWhois(Client *c, const IrcMessage &m);
	void _cmdList(Client *c, const IrcMessage &m);
	void _cmdNames(Client *c, const IrcMessage &m);
	void _cmdPing(Client *c, const IrcMessage &m);
	void _cmdPong(Client *c, const IrcMessage &m);
	void _cmdOper(Client *c, const IrcMessage &m);
	void _cmdKill(Client *c, const IrcMessage &m);

	// Mode helpers
	void _handleChannelMode(Client *c, Channel *ch, const IrcMessage &m);
	void _handleUserMode(Client *c, Client *target, const IrcMessage &m);

	// Registration & utilities
	void _sendWelcome(Client *c);
	bool _isNickInUse(const std::string &nick) const;
	bool _isValidNick(const std::string &nick) const;
	bool _isValidChannel(const std::string &name) const;
	Client *_getClientByNick(const std::string &nick) const;
	Channel *_getChannel(const std::string &name) const;
	Channel *_getOrCreateChannel(const std::string &name, Client *creator);
	void _removeChannelIfEmpty(const std::string &name);

	// Numeric reply helpers
	std::string _num(const std::string &code, Client *c, const std::string &text) const;
	void _send(Client *c, const std::string &msg) const;
	void _sendErr(Client *c, const std::string &code, const std::string &text) const;

	int _serverFd;
	int _epollFd;
	int _port;
	std::string _password;
	std::string _createdAt;

	std::vector<std::string> _commandslist;
	std::map<int, Client *> _clients;			// fd → Client
	std::map<std::string, Channel *> _channels; // name → Channel

public:
	IrcServer(int port, const std::string &password);
	~IrcServer();

	void run();
	void _initcommadlist();
	bool _isacmd(std::string &m) const;
	static volatile bool running;
};

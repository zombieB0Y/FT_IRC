#pragma once

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cctype>
#include <algorithm>
#include <vector>
#include <map>
#include <sstream>
#include <iomanip>

#include "Client.hpp"
#include "Channel.hpp"
#include "Bot/Bot.hpp"

// ─── Server ──────────────────────────────────────────────────────────────────
// Single-threaded IRC server using poll() for non-blocking I/O.

class Server {
public:
	Server(int port, const std::string& password);
	~Server();

	bool init();
	void run();

	static volatile bool running;
	bool    Passaccepted(Client c) const;
	void    _initcommadlist();
	bool	_isacmd(std::string m) const;
	Client&	_GetClient(int fd) {return (this->clients.find(fd)->second);}
	void	_clear();
private:
	// ── Configuration ────────────────────────────────────────────────────────
	int         port;
	std::string password;
	std::string serverName;

	// ── Network state ────────────────────────────────────────────────────────
	int  listenFd;
	std::vector<struct pollfd> pfds;

	// ── Client / channel registries ──────────────────────────────────────────
	std::vector<std::string> _commandslist;
	std::map<int, Client>         clients;    // fd  → Client
	std::map<std::string, Channel> channels;  // name → Channel
	std::map<std::string, int>    nickToFd;   // nick → fd

	// ── Network setup ────────────────────────────────────────────────────────
	bool setupListenSocket();
	void rebuildPollFds();

	// ── Event handlers ───────────────────────────────────────────────────────
	void acceptClient();
	void handleClientRead(int fd);
	void handleClientWrite(int fd);
	void disconnectClient(int fd, const std::string& reason);

	// ── Protocol helpers ─────────────────────────────────────────────────────
	void                     processLine(int fd, const std::string& line);
	std::vector<std::string> parseCommand(const std::string& line);
	bool                     isRegistered(const Client& c) const;
	void                     maybeFinishRegistration(int fd);

	void        sendRaw(int fd, const std::string& msg);
	void        sendNumeric(int fd, int code, const std::string& msg);
	std::string clientPrefix(int fd) const;
	std::string channelMemberPrefix(const Channel& ch, int memberFd) const;

	void broadcastToChannel(const Channel& ch, const std::string& msg);
	void sendNamesReply(int fd, const Channel& ch);
	void ensureChannelOperator(Channel& ch);
	void removeClientFromAllChannels(int fd, const std::string& partReason);

	std::vector<std::string> splitString(const std::string& str, char delimiter);

	// ── IRC command handlers ─────────────────────────────────────────────────
	void cmdPass(int fd, const std::vector<std::string>& args);
	void cmdNick(int fd, const std::vector<std::string>& args);
	void cmdUser(int fd, const std::vector<std::string>& args);
	void cmdJoin(int fd, const std::vector<std::string>& args);
	void cmdPart(int fd, const std::vector<std::string>& args);
	void cmdPrivmsg(int fd, const std::vector<std::string>& args);
	void cmdKick(int fd, const std::vector<std::string>& args);
	void cmdInvite(int fd, const std::vector<std::string>& args);
	void cmdTopic(int fd, const std::vector<std::string>& args);
	void cmdMode(int fd, const std::vector<std::string>& args);

	// ── Non-copyable ─────────────────────────────────────────────────────────
	// Server(const Server&);
	// Server& operator=(const Server&);
};
bool validPort(const std::string& portStr);
bool emptyPassword(const std::string& pass);
#define RPL_WELCOME          1
#define RPL_YOURHOST         2
#define RPL_CREATED          3
#define RPL_MYINFO           4
#define RPL_ISUPPORT         5
#define RPL_UMODEIS          221
#define RPL_WHOISUSER        311
#define RPL_WHOISSERVER      312
#define RPL_WHOISCHANNELS    319
#define RPL_ENDOFWHOIS       318
#define RPL_LIST             322
#define RPL_LISTEND          323
#define RPL_CHANNELMODEIS    324
#define RPL_NOTOPIC          331
#define RPL_TOPIC            332
#define RPL_INVITING         341
#define RPL_NAMREPLY         353
#define RPL_ENDOFNAMES       366
#define RPL_MOTDSTART        375
#define RPL_MOTD             372
#define RPL_ENDOFMOTD        376
#define RPL_YOUREOPER        381
// Errors
#define ERR_NOSUCHNICK       401
#define ERR_NOSUCHSERVER     402
#define ERR_NOSUCHCHANNEL    403
#define ERR_CANNOTSENDTOCHAN 404
#define ERR_TOOMANYCHANNELS  405
#define ERR_UNKNOWNCOMMAND   421
#define ERR_NONICKNAMEGIVEN  431
#define ERR_ERRONEUSNICKNAME 432
#define ERR_NICKNAMEINUSE    433
#define ERR_USERNOTINCHANNEL 441
#define ERR_NOTONCHANNEL     442
#define ERR_USERONCHANNEL    443
#define ERR_NOTREGISTERED    451
#define ERR_NEEDMOREPARAMS   461
#define ERR_ALREADYREGISTRED 462
#define ERR_PASSWDMISMATCH   464
#define ERR_KEYSET           467
#define ERR_CHANNELISFULL    471
#define ERR_UNKNOWNMODE      472
#define ERR_INVITEONLYCHAN   473
#define ERR_BADCHANNELKEY    475
#define ERR_NOPRIVILEGES     481
#define ERR_CHANOPRIVSNEEDED 482
#define ERR_UMODEUNKNOWNFLAG 501
#define ERR_USERSDONTMATCH   502

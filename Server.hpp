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
#include "Client.hpp"
#include "Channel.hpp"
#include <iomanip>

class Server {
public:
    Server(int port, const std::string& password);
    ~Server();

    bool init();
    void run();

private:
    int port;
    std::string password;
    int listenFd;
    bool running;
    std::string serverName;

    std::vector<struct pollfd> pfds;
    std::map<int, Client> clients;
    std::map<std::string, Channel> channels;
    std::map<std::string, int> nickToFd;

    bool setupListenSocket();
    void rebuildPollFds();

    void acceptClient();
    void handleClientRead(int fd);
    void handleClientWrite(int fd);
    void disconnectClient(int fd, const std::string& reason);

    void processLine(int fd, const std::string& line);
    std::vector<std::string> parseCommand(const std::string& line);
    bool isRegistered(const Client& c) const;
    void maybeFinishRegistration(int fd);

    void sendRaw(int fd, const std::string& msg);
    void sendNumeric(int fd, int code, const std::string& msg);
    std::string clientPrefix(int fd) const;
    std::string channelMemberPrefix(const Channel& ch, int memberFd) const;

    void broadcastToChannel(const Channel& ch, const std::string& msg);
    void sendNamesReply(int fd, const Channel& ch);
    void ensureChannelOperator(Channel& ch);
    void removeClientFromAllChannels(int fd, const std::string& partReason);
    std::vector<std::string> splitString(const std::string& str, char delimiter);

    void cmdPass(int fd, const std::vector<std::string>& args);
    void cmdNick(int fd, const std::vector<std::string>& args);
    void cmdUser(int fd, const std::vector<std::string>& args);
    void cmdJoin(int fd, const std::vector<std::string>& args);
    void cmdPrivmsg(int fd, const std::vector<std::string>& args);
    void cmdKick(int fd, const std::vector<std::string>& args);
    void cmdInvite(int fd, const std::vector<std::string>& args);
    void cmdTopic(int fd, const std::vector<std::string>& args);
    void cmdMode(int fd, const std::vector<std::string>& args);
    void cmdPart(int fd, const std::vector<std::string>& args);

    Server(const Server&);
    Server& operator=(const Server&);
};
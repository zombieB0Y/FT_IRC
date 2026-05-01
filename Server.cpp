#include "Server.hpp"

Server::Server(int port, const std::string& pass) : port(port), password(pass), listenFd(-1), running(false), serverName("ircserv") {}

Server::~Server(){
    std::map<int, Client>::iterator it = clients.begin();
    while (it != clients.end()){
        close(it->first);
        ++it;
    }
    clients.clear();
    if (listenFd >= 0)
        close(listenFd);
}

bool Server::setupListenSocket(){
    int opt = 1;
    listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd < 0){
        std::cerr << "socket() failed" << std::endl;
        return false;
    }
    if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1){
        std::cerr << "setsocket() failed" << std::endl;
        return false;
    }
    if (fcntl(listenFd, F_SETFL, O_NONBLOCK) == -1){
        std::cerr << "Failed to make the socket non blocking" << std::endl;
        return false;
    }
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(listenFd, (sockaddr *)&addr, sizeof(addr)) == -1){
        std::cerr << "bind() failed" << std::endl;
        return false;
    }

    if (listen(listenFd, 5) == -1){
        std::cerr << "listen() failed" << std::endl;
        return false;
    }
    return true;
}

void Server::rebuildPollFds(){
    pfds.clear();
    pollfd listenSocket;
    listenSocket.fd = listenFd;
    listenSocket.events = POLLIN;
    listenSocket.revents = 0;
    pfds.push_back(listenSocket);

    std::map<int, Client>::iterator it = clients.begin();
    while (it != clients.end())
    {
        pollfd client;
        client.fd = it->first;
        client.events = POLLIN;
        client.revents = 0;
        if (!it->second.sendBuffer.empty())
            client.events |= POLLOUT;
        pfds.push_back(client);
        ++it;
    }
}

void Server::acceptClient(){
    while (true){
        sockaddr_in clients_in;
        socklen_t clients_in_len = sizeof(clients_in);
        int client_fd = accept(listenFd, (sockaddr *)&clients_in, &clients_in_len);
        if (client_fd < 0){
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return ;
            std::cerr << "accept() failed" << std::endl;
            return ;
        }
        if (fcntl(client_fd, F_SETFL, O_NONBLOCK) == -1)
        {
            close(client_fd);
            std::cerr << "Failed to make the socket non blocking" << std::endl;
            continue ;
        }
        Client c;
        c.setFd(client_fd);
        c.setPort(ntohs(clients_in.sin_port));
        c.setIp(inet_ntoa(clients_in.sin_addr));
        clients[client_fd] = c;
    }
}

void Server::handleClientRead(int fd){
    Client &c = clients[fd];
    while (true){
        char buffer[4096];
        ssize_t n = recv(fd, buffer, sizeof(buffer), 0);
        if (n > 0){
            c.recvBuffer.append(buffer, n);
            while (true){
                size_t pos;
                pos = c.recvBuffer.find('\n');
                if (pos == std::string::npos)
                    break;
                std::string line = c.recvBuffer.substr(0, pos + 1);
                c.recvBuffer.erase(0, pos + 1);

                if (!line.empty() && line[line.size()-1] == '\n')
                    line.erase(line.size()-1);
                if (!line.empty() && line[line.size()-1] == '\r')
                    line.erase(line.size()-1);

                if (!line.empty())
                    processLine(fd, line);
            }
        }
        else if (n == 0){
            disconnectClient(fd, "Client closed connection");
            return ;
        }
        else{
            if (errno == EAGAIN || errno == EWOULDBLOCK)
               break;
            disconnectClient(fd, "recv error");
            return ;
        }
    }
}

std::vector<std::string> Server::parseCommand(const std::string& line){
    size_t i = 0;
    size_t y;
    std::vector<std::string> result;
    while (i < line.size() && line[i] == ' ')
        ++i;
    if (i == line.size())
        return result;
    y = i;
    while (i < line.size() && line[i] != ' ')
        ++i;
    std::string cmd = line.substr(y, i - y);
    for (size_t j = 0; j < cmd.size(); ++j)
        cmd[j] = std::toupper(cmd[j]);
    result.push_back(cmd);
    while (i < line.size()){
        while (i < line.size() && line[i] == ' ')
            ++i;
        if (i == line.size())
            break;
        if (line[i] == ':'){
            result.push_back(line.substr(i + 1));
            break;
        }
        else{
            y = i;
            while (i < line.size() && line[i] != ' '){
                ++i;
            }
            result.push_back(line.substr(y, i - y));
        }
    }
    return result;
}

void Server::sendNumeric(int fd, int code, const std::string& msg) {
    std::string nick = "*";
    std::map<int, Client>::iterator it = clients.find(fd);
    if (it != clients.end() && it->second.getHasNick()) {
        nick = it->second.getNick();
    }
    std::ostringstream oss;
    oss << ":" << serverName << " " 
        << std::setw(3) << std::setfill('0') << code // This ensures 001 instead of 1
        << " " << nick << " " << msg;
    sendRaw(fd, oss.str());
}

void Server::sendRaw(int fd, const std::string& msg){
    std::map<int, Client>::iterator it = clients.find(fd);
    if (it == clients.end())
        return;
    it->second.sendBuffer += msg + "\r\n";
}

bool Server::isRegistered(const Client &c) const{
    return c.getPassAccepted() && c.getHasNick() && c.getHasUser();
}

void Server::cmdPass(int fd, const std::vector<std::string>& args){
    if (args.size() < 2){
        sendNumeric(fd, 461, "PASS :Not enough parameters");
        return;
    }
    std::map<int, Client>::iterator c = clients.find(fd);
    if (c == clients.end())
        return;
    if (c->second.getWelcome() || c->second.getPassAccepted()){
        sendNumeric(fd, 462, ":You may not reregister");
        return ;
    }
    if (args[1] == password){
        c->second.setPassAccepted(true);
        maybeFinishRegistration(fd);
    }
    else{
        std::string err = ":" + serverName + " 464 * :Password incorrect\r\n";
        send(fd, err.c_str(), err.size(), 0);
        disconnectClient(fd, "Wrong pass");
    }
}

static bool isValidNickCharacter(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') ||
           c == '-' || c == '_' ||
           c == '[' || c == ']' || c == '\\' ||
           c == '`' || c == '^' || c == '{' || c == '}';
}

void Server::cmdNick(int fd, const std::vector<std::string>& args) {;
    if (args.size() < 2) {
        sendNumeric(fd, 461, "NICK :Not enough parameters");
        return;
    }

    std::map<int, Client>::iterator c = clients.find(fd);
    if (c == clients.end())
        return;

    const std::string& nick = args[1];

    if (nick.empty() || nick.size() > 9) {
        sendNumeric(fd, 432, nick + " :Erroneous nickname");
        return;
    }

    if (!((nick[0] >= 'a' && nick[0] <= 'z') || (nick[0] >= 'A' && nick[0] <= 'Z'))) {
        sendNumeric(fd, 432, nick + " :Erroneous nickname");
        return;
    }

    for (size_t i = 1; i < nick.size(); ++i) {
        if (!isValidNickCharacter(nick[i])) {
            sendNumeric(fd, 432, nick + " :Erroneous nickname");
            return;
        }
    }

    std::map<std::string, int>::iterator ni = nickToFd.find(nick);
    if (ni != nickToFd.end() && ni->second != fd) {
        sendNumeric(fd, 433, nick + " :Nickname is already in use");
        return;
    }

    if (c->second.getHasNick()) {
        std::string oldNick = c->second.getNick();
        if (oldNick != nick) {
            nickToFd.erase(oldNick);
        }
    }

    c->second.setNick(nick);
    c->second.setHasNick(true);
    nickToFd[nick] = fd;
    maybeFinishRegistration(fd);
}

void Server::maybeFinishRegistration(int fd) {
    std::map<int, Client>::iterator it = clients.find(fd);
    if (it == clients.end())
        return;

    Client& c = it->second;
    if (c.getPassAccepted() && c.getHasNick() && c.getHasUser() && !c.getWelcome()) {

        c.setWelcome(true);
        sendNumeric(fd, 001, ":Welcome to the Internet Relay Chat Network, " +
                    c.getNick() + "!" + c.getUsername() + "@" + c.getIp());
        sendNumeric(fd, 002, ":Your host is " + serverName + ", running version 1.0");
        sendNumeric(fd, 003, ":This server was created on January 1 2025");
        sendNumeric(fd, 004, serverName + " 1.0 i itkol");
    }
}

void Server::cmdJoin(int fd, const std::vector<std::string>& args) {
    std::map<int, Client>::iterator it = clients.find(fd);
    if (it == clients.end())
        return;
    
    Client& c = it->second;
    if (!isRegistered(c)) {
        sendNumeric(fd, 451, ":You have not registered");
        return;
    }

    if (args.size() < 2) {
        sendNumeric(fd, 461, "JOIN :Not enough parameters");
        return;
    }

    std::string channelName = args[1];
    
    if (channelName.empty() || channelName[0] != '#') {
        sendNumeric(fd, 403, channelName + " :No such channel");
        return;
    }

    std::map<std::string, Channel>::iterator chIt = channels.find(channelName);
    if (chIt == channels.end()) {
        Channel newChannel;
        newChannel.name = channelName;
        channels[channelName] = newChannel;
        chIt = channels.find(channelName);
    }

    Channel& ch = chIt->second;

    if (ch.members.find(fd) != ch.members.end()) {
        return;
    }

    if (ch.inviteOnly) {
        if (ch.invited.find(fd) == ch.invited.end()) {
            sendNumeric(fd, 473, channelName + " :Cannot join channel (+i)");
            return;
        }
    }

    std::string key;
    if (args.size() > 2)
        key = args[2];
    
    if (ch.hasKey && key != ch.key) {
        sendNumeric(fd, 475, channelName + " :Cannot join channel (+k)");
        return;
    }

    if (ch.userLimit > 0 && static_cast<int>(ch.members.size()) >= ch.userLimit) {
        sendNumeric(fd, 471, channelName + " :Cannot join channel (+l)");
        return;
    }

    ch.members.insert(fd);
    
    if (ch.members.size() == 1) {
        ch.operators.insert(fd);
    }
    
    ch.invited.erase(fd);

    std::string joinMsg = clientPrefix(fd) + " JOIN " + channelName;
    broadcastToChannel(ch, joinMsg);

    if (!ch.topic.empty()) {
        sendNumeric(fd, 332, channelName + " :" + ch.topic);
    } else {
        sendNumeric(fd, 331, channelName + " :No topic is set");
    }

    sendNamesReply(fd, ch);
}

void Server::sendNamesReply(int fd, const Channel& ch) {
    std::string namesList;
    
    for (std::set<int>::const_iterator it = ch.members.begin(); it != ch.members.end(); ++it) {
        std::map<int, Client>::const_iterator clientIt = clients.find(*it);
        if (clientIt == clients.end())
            continue;
        
        if (!namesList.empty())
            namesList += " ";
        
        namesList += channelMemberPrefix(ch, *it);
        namesList += clientIt->second.getNick();
    }
    
    sendNumeric(fd, 353, "= " + ch.name + " :" + namesList);
    sendNumeric(fd, 366, ch.name + " :End of /NAMES list");
}

std::string Server::channelMemberPrefix(const Channel& ch, int memberFd) const {
    if (ch.operators.find(memberFd) != ch.operators.end()) {
        return "@";
    }
    return "";
}

void Server::broadcastToChannel(const Channel& ch, const std::string& msg) {
    for (std::set<int>::const_iterator it = ch.members.begin(); it != ch.members.end(); ++it) {
        sendRaw(*it, msg);
    }
}

std::string Server::clientPrefix(int fd) const {
    std::map<int, Client>::const_iterator it = clients.find(fd);
    if (it == clients.end())
        return ":unknown!unknown@unknown";
    
    const Client& c = it->second;
    std::string nick = c.getHasNick() ? c.getNick() : "*";
    std::string user = c.getHasUser() ? c.getUsername() : "unknown";
    
    return ":" + nick + "!" + user + "@" + c.getIp();
}

void Server::cmdPrivmsg(int fd, const std::vector<std::string>& args) {
    std::map<int, Client>::iterator it = clients.find(fd);
    if (it == clients.end())
        return;
    
    Client& c = it->second;
    if (!isRegistered(c)) {
        sendNumeric(fd, 451, ":You have not registered");
        return;
    }
    if (args.size() < 3) {
        sendNumeric(fd, 461, "PRIVMSG :Not enough parameters");
        return;
    }

    std::string target = args[1];
    std::string message = args[2];
    if (!target.empty() && target[0] == '#') {
        std::map<std::string, Channel>::iterator chIt = channels.find(target);
        if (chIt == channels.end()) {
            sendNumeric(fd, 403, target + " :No such channel");
            return;
        }
        
        Channel& ch = chIt->second;
        if (ch.members.find(fd) == ch.members.end()) {
            sendNumeric(fd, 442, target + " :You're not on that channel");
            return;
        }
        std::string msg = clientPrefix(fd) + " PRIVMSG " + target + " :" + message;
        for (std::set<int>::const_iterator it = ch.members.begin(); it != ch.members.end(); ++it) {
            if (*it != fd) {
                sendRaw(*it, msg);
            }
        }
    } else {
        std::map<std::string, int>::iterator nickIt = nickToFd.find(target);
        if (nickIt == nickToFd.end()) {
            sendNumeric(fd, 401, target + " :No such nick");
            return;
        }
        
        int targetFd = nickIt->second;
        std::string msg = clientPrefix(fd) + " PRIVMSG " + target + " :" + message;
        sendRaw(targetFd, msg);
    }
}

void Server::cmdKick(int fd, const std::vector<std::string>& args) {

    std::map<int, Client>::iterator it = clients.find(fd);
    if (it == clients.end())
        return;
    
    Client& c = it->second;
    if (!isRegistered(c)) {
        sendNumeric(fd, 451, ":You have not registered");
        return;
    }

    if (args.size() < 3) {
        sendNumeric(fd, 461, "KICK :Not enough parameters");
        return;
    }

    std::string channelName = args[1];
    std::string targetNick = args[2];
    std::string reason = (args.size() > 3) ? args[3] : c.getNick();

    std::map<std::string, Channel>::iterator chIt = channels.find(channelName);
    if (chIt == channels.end()) {
        sendNumeric(fd, 403, channelName + " :No such channel");
        return;
    }

    Channel& ch = chIt->second;

    if (ch.members.find(fd) == ch.members.end()) {
        sendNumeric(fd, 442, channelName + " :You're not on that channel");
        return;
    }

    if (ch.operators.find(fd) == ch.operators.end()) {
        sendNumeric(fd, 482, channelName + " :You're not channel operator");
        return;
    }

    std::map<std::string, int>::iterator nickIt = nickToFd.find(targetNick);
    if (nickIt == nickToFd.end()) {
        sendNumeric(fd, 401, targetNick + " :No such nick");
        return;
    }

    int targetFd = nickIt->second;

    if (ch.members.find(targetFd) == ch.members.end()) {
        sendNumeric(fd, 441, targetNick + " " + channelName + " :They aren't on that channel");
        return;
    }

    std::string kickMsg = clientPrefix(fd) + " KICK " + channelName + " " + targetNick + " :" + reason;
    broadcastToChannel(ch, kickMsg);

    ch.members.erase(targetFd);
    ch.operators.erase(targetFd);
    ch.invited.erase(targetFd);

    ensureChannelOperator(ch);

    if (ch.members.empty()) {
        channels.erase(channelName);
    }
}

void Server::cmdInvite(int fd, const std::vector<std::string>& args) {
    std::map<int, Client>::iterator it = clients.find(fd);
    if (it == clients.end())
        return;
    
    Client& c = it->second;
    if (!isRegistered(c)) {
        sendNumeric(fd, 451, ":You have not registered");
        return;
    }

    if (args.size() < 3) {
        sendNumeric(fd, 461, "INVITE :Not enough parameters");
        return;
    }

    std::string targetNick = args[1];
    std::string channelName = args[2];

    std::map<std::string, Channel>::iterator chIt = channels.find(channelName);
    if (chIt == channels.end()) {
        sendNumeric(fd, 403, channelName + " :No such channel");
        return;
    }

    Channel& ch = chIt->second;

    if (ch.members.find(fd) == ch.members.end()) {
        sendNumeric(fd, 442, channelName + " :You're not on that channel");
        return;
    }

    if (ch.operators.find(fd) == ch.operators.end()) {
        sendNumeric(fd, 482, channelName + " :You're not channel operator");
        return;
    }

    std::map<std::string, int>::iterator nickIt = nickToFd.find(targetNick);
    if (nickIt == nickToFd.end()) {
        sendNumeric(fd, 401, targetNick + " :No such nick");
        return;
    }

    int targetFd = nickIt->second;

    if (ch.members.find(targetFd) != ch.members.end()) {
        sendNumeric(fd, 443, targetNick + " " + channelName + " :is already on channel");
        return;
    }

    ch.invited.insert(targetFd);

    sendRaw(targetFd, clientPrefix(fd) + " INVITE " + targetNick + " :" + channelName);

    sendNumeric(fd, 341, targetNick + " " + channelName);
}

void Server::cmdTopic(int fd, const std::vector<std::string>& args) {
    std::map<int, Client>::iterator it = clients.find(fd);
    if (it == clients.end())
        return;
    
    Client& c = it->second;
    if (!isRegistered(c)) {
        sendNumeric(fd, 451, ":You have not registered");
        return;
    }

    if (args.size() < 2) {
        sendNumeric(fd, 461, "TOPIC :Not enough parameters");
        return;
    }

    std::string channelName = args[1];

    std::map<std::string, Channel>::iterator chIt = channels.find(channelName);
    if (chIt == channels.end()) {
        sendNumeric(fd, 403, channelName + " :No such channel");
        return;
    }

    Channel& ch = chIt->second;

    if (ch.members.find(fd) == ch.members.end()) {
        sendNumeric(fd, 442, channelName + " :You're not on that channel");
        return;
    }

    if (args.size() == 2) {
        if (ch.topic.empty()) {
            sendNumeric(fd, 331, channelName + " :No topic is set");
        } else {
            sendNumeric(fd, 332, channelName + " :" + ch.topic);
        }
        return;
    }

    std::string newTopic = args[2];

    if (ch.topicOpOnly && ch.operators.find(fd) == ch.operators.end()) {
        sendNumeric(fd, 482, channelName + " :You're not channel operator");
        return;
    }

    ch.topic = newTopic;

    std::string topicMsg = clientPrefix(fd) + " TOPIC " + channelName + " :" + newTopic;
    broadcastToChannel(ch, topicMsg);
}

void Server::cmdMode(int fd, const std::vector<std::string>& args) {
    std::map<int, Client>::iterator it = clients.find(fd);
    if (it == clients.end())
        return;
    
    Client& c = it->second;
    if (!isRegistered(c)) {
        sendNumeric(fd, 451, ":You have not registered");
        return;
    }
    if (args.size() < 2) {
        sendNumeric(fd, 461, "MODE :Not enough parameters");
        return;
    }

    std::string target = args[1];
    if (target.empty() || target[0] != '#') {
        sendNumeric(fd, 502, ":Cannot change user mode");
        return;
    }

    std::map<std::string, Channel>::iterator chIt = channels.find(target);
    if (chIt == channels.end()) {
        sendNumeric(fd, 403, target + " :No such channel");
        return;
    }

    Channel& ch = chIt->second;

    if (ch.members.find(fd) == ch.members.end()) {
        sendNumeric(fd, 442, target + " :You're not on that channel");
        return;
    }

    if (args.size() == 2) {
        std::string modeStr = "+";
        if (ch.inviteOnly) modeStr += "i";
        if (ch.topicOpOnly) modeStr += "t";
        if (ch.hasKey) modeStr += "k";
        if (ch.userLimit > 0) modeStr += "l";
        sendNumeric(fd, 324, target + " " + modeStr);
        return;
    }

    if (ch.operators.find(fd) == ch.operators.end()) {
        sendNumeric(fd, 482, target + " :You're not channel operator");
        return;
    }

    std::string modeStr = args[2];
    bool adding = true;
    size_t argIndex = 3;

    for (size_t i = 0; i < modeStr.size(); ++i) {
        char m = modeStr[i];

        if (m == '+') {
            adding = true;
        } else if (m == '-') {
            adding = false;
        } else if (m == 'i') {
            ch.inviteOnly = adding;
        } else if (m == 't') {
            ch.topicOpOnly = adding;
        } else if (m == 'k') {
            if (adding) {
                if (argIndex >= args.size()) {
                    sendNumeric(fd, 461, "MODE :Not enough parameters for +k");
                    continue;
                }
                ch.hasKey = true;
                ch.key = args[argIndex++];
            } else {
                ch.hasKey = false;
                ch.key.clear();
            }
        } else if (m == 'o') {
            if (argIndex >= args.size()) {
                sendNumeric(fd, 461, "MODE :Not enough parameters for +/-o");
                continue;
            }
            std::string targetNick = args[argIndex++];
            std::map<std::string, int>::iterator nickIt = nickToFd.find(targetNick);
            if (nickIt == nickToFd.end() || ch.members.find(nickIt->second) == ch.members.end()) {
                sendNumeric(fd, 441, targetNick + " " + target + " :They aren't on that channel");
                continue;
            }
            int targetFd = nickIt->second;
            if (adding) {
                ch.operators.insert(targetFd);
            } else {
                ch.operators.erase(targetFd);
                ensureChannelOperator(ch);
            }
        } else if (m == 'l') {
            if (adding) {
                if (argIndex >= args.size()) {
                    sendNumeric(fd, 461, "MODE :Not enough parameters for +l");
                    continue;
                }
                int limit = std::atoi(args[argIndex++].c_str());
                if (limit > 0) {
                    ch.userLimit = limit;
                }
            } else {
                ch.userLimit = 0;
            }
        }
    }
    std::string modeChangeMsg = clientPrefix(fd) + " MODE " + target + " " + modeStr;
    broadcastToChannel(ch, modeChangeMsg);
}

void Server::ensureChannelOperator(Channel& ch) {
    if (ch.members.empty())
        return;
    
    if (ch.operators.empty()) {
        int firstMember = *ch.members.begin();
        ch.operators.insert(firstMember);
    }
}

void Server::removeClientFromAllChannels(int fd, const std::string& partReason) {
    std::vector<std::string> emptyChannels;

    for (std::map<std::string, Channel>::iterator it = channels.begin(); it != channels.end(); ++it) {
        Channel& ch = it->second;

        if (ch.members.find(fd) != ch.members.end()) {
            std::string partMsg = clientPrefix(fd) + " PART " + ch.name + " :" + partReason;
            broadcastToChannel(ch, partMsg);

            ch.members.erase(fd);
            ch.operators.erase(fd);
            ch.invited.erase(fd);

            ensureChannelOperator(ch);

            if (ch.members.empty()) {
                emptyChannels.push_back(ch.name);
            }
        }
    }

    for (size_t i = 0; i < emptyChannels.size(); ++i) {
        channels.erase(emptyChannels[i]);
    }
}

void Server::disconnectClient(int fd, const std::string& reason) {
    std::map<int, Client>::iterator it = clients.find(fd);
    if (it == clients.end())
        return;

    removeClientFromAllChannels(fd, "Quit: " + reason);

    if (it->second.getHasNick()) {
        nickToFd.erase(it->second.getNick());
    }

    close(fd);
    clients.erase(it);
}

void Server::handleClientWrite(int fd) {
    std::map<int, Client>::iterator it = clients.find(fd);
    if (it == clients.end())
        return;

    Client& c = it->second;
    if (c.sendBuffer.empty())
        return;
    ssize_t n = send(fd, c.sendBuffer.c_str(), c.sendBuffer.size(), 0);

    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        disconnectClient(fd, "send error");
        return;
    }
    if (n > 0) {
        c.sendBuffer.erase(0, static_cast<size_t>(n));
    }
}

void Server::cmdUser(int fd, const std::vector<std::string>& args) {
    std::map<int, Client>::iterator it = clients.find(fd);
    if (it == clients.end())
        return;

    Client& c = it->second;

    if (c.getWelcome() || c.getHasUser()) {
        sendNumeric(fd, 462, ":You may not reregister");
        return;
    }

    if (args.size() < 5) {
        sendNumeric(fd, 461, "USER :Not enough parameters");
        return;
    }

    c.setUsername(args[1]);
    c.setRealname(args[4]);
    c.setHasUser(true);

    maybeFinishRegistration(fd);
}

void Server::cmdPart(int fd, const std::vector<std::string>& args) {
    std::map<int, Client>::iterator it = clients.find(fd);
    if (it == clients.end())
        return;

    Client& c = it->second;
    if (!isRegistered(c)) {
        sendNumeric(fd, 451, ":You have not registered");
        return;
    }

    if (args.size() < 2) {
        sendNumeric(fd, 461, "PART :Not enough parameters");
        return;
    }

    std::string channelName = args[1];
    std::string reason = (args.size() > 2) ? args[2] : c.getNick();

    if (channelName.empty() || channelName[0] != '#') {
        sendNumeric(fd, 403, channelName + " :No such channel");
        return;
    }

    std::map<std::string, Channel>::iterator chIt = channels.find(channelName);
    if (chIt == channels.end()) {
        sendNumeric(fd, 403, channelName + " :No such channel");
        return;
    }

    Channel& ch = chIt->second;

    if (ch.members.find(fd) == ch.members.end()) {
        sendNumeric(fd, 442, channelName + " :You're not on that channel");
        return;
    }

    std::string partMsg = clientPrefix(fd) + " PART " + channelName + " :" + reason;
    broadcastToChannel(ch, partMsg);

    ch.members.erase(fd);
    ch.operators.erase(fd);
    ch.invited.erase(fd);

    ensureChannelOperator(ch);

    if (ch.members.empty()) {
        channels.erase(channelName);
    }
}

void Server::processLine(int fd, const std::string& line){
    std::vector<std::string> parsedLine = parseCommand(line);
    if (parsedLine.empty())
        return ;
    if (parsedLine[0] == "CAP")
        return;
    if (parsedLine[0] == "PASS")
        cmdPass(fd, parsedLine);
    else if (parsedLine[0] == "NICK")
        cmdNick(fd, parsedLine);
    else if (parsedLine[0] == "USER")
        cmdUser(fd, parsedLine);
    else if (parsedLine[0] == "JOIN" )
        cmdJoin(fd, parsedLine);
    else if (parsedLine[0] == "PRIVMSG" && isRegistered(clients[fd]))
        cmdPrivmsg(fd, parsedLine);
    else if (parsedLine[0] == "KICK" && isRegistered(clients[fd]))
        cmdKick(fd, parsedLine);
    else if (parsedLine[0] == "INVITE" && isRegistered(clients[fd]))
        cmdInvite(fd, parsedLine);
    else if (parsedLine[0] == "TOPIC" && isRegistered(clients[fd]))
        cmdTopic(fd, parsedLine);
    else if (parsedLine[0] == "MODE" && isRegistered(clients[fd]))
        cmdMode(fd, parsedLine);
    else if (parsedLine[0] == "PART" && isRegistered(clients[fd]))
        cmdPart(fd, parsedLine);
    else if (parsedLine[0] == "PING") {
        if (parsedLine.size() > 1)
            sendRaw(fd, ":" + serverName + " PONG " + serverName + " :" + parsedLine[1]);
        else
            sendRaw(fd, ":" + serverName + " PONG " + serverName);
    }
    else{
        sendNumeric(fd, 421, parsedLine[0] + " :Unknown command");
    }
}

bool Server::init(){
    if (!setupListenSocket())
        return false;
    return true;
}

void Server::run(){
    running = true;
    while (running){
        rebuildPollFds();
        int ready = poll(pfds.data(), pfds.size(), -1);
        if (ready < 0){
            if (errno == EINTR)
                continue;
            std::cerr << "poll() failed" << std::endl;
            break;
        }
        for (size_t i = 0; i < pfds.size(); i++){
            if (pfds[i].revents == 0)
                continue;
            if (pfds[i].fd == listenFd && (pfds[i].revents & POLLIN)){
                acceptClient();
            }
            else{
                if (pfds[i].revents & (POLLERR | POLLHUP | POLLNVAL)){
                    disconnectClient(pfds[i].fd, "socket error");
                }
                else{
                    if (pfds[i].revents & POLLIN)
                        handleClientRead(pfds[i].fd);
                    if (pfds[i].revents & POLLOUT)
                        handleClientWrite(pfds[i].fd);
                }
            }
        }
    }
}
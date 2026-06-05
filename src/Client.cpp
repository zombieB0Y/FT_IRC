#include "../include/Client.hpp"
#include "../include/Channel.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <algorithm>

Client::Client(int fd, const std::string &host)
    : _fd(fd), _nick("*"), _user(""), _realname(""),
      _host(host), _passAccepted(false), _isOper(false)
{
}

Client::~Client() {
    this->_sendBuf.clear();
}

int Client::getFd() const { return _fd; }
std::string Client::getNick() const { return _nick; }
std::string Client::getUser() const { return _user; }
std::string Client::getRealname() const { return _realname; }
std::string Client::getHost() const { return _host; }
bool Client::isPassAccepted() const { return _passAccepted; }
bool Client::isOperator() const { return _isOper; }

bool Client::isRegistered() const { // ---------------------------------------------
    return (_passAccepted && _nick != "*" && !_nick.empty() && !_user.empty());
}

std::string Client::getPrefix() const {
    return _nick + "!" + _user + "@" + _host;
}

void Client::setNick(const std::string &nick) { _nick = nick; }
void Client::setUser(const std::string &user) { _user = user; }
void Client::setRealname(const std::string &r) { _realname = r; }
void Client::setPassAccepted() { _passAccepted = true; }
void Client::setOperator(bool val) { _isOper = val; }

void Client::appendBuffer(const std::string &data) { _recvBuf += data; }
std::string Client::getBuffer() const { return _recvBuf; }
void Client::clearBuffer() { _recvBuf.clear(); }

bool Client::hasNewline() const
{
    return _recvBuf.find('\n') != std::string::npos;
}

void Client::sendMsg(const std::string &msg)
{
    _sendBuf += msg;
    if (msg.size() < 2 || msg.substr(msg.size() - 2) != "\r\n")
        _sendBuf += "\r\n";
    flushOutput();
}

void Client::addToPendingOutput(const std::string &msg) { sendMsg(msg); }
bool Client::hasPendingOutput() const { return !_sendBuf.empty(); }

void Client::flushOutput()
{
    if (_sendBuf.empty())
        return;
    // std::cout << _sendBuf << std::endl;
    ssize_t sent = send(_fd, _sendBuf.c_str(), _sendBuf.size(), MSG_NOSIGNAL);
    if (sent > 0)
        _sendBuf.erase(0, sent);
}

void Client::joinChannel(Channel *ch)
{
    if (!isInChannel(ch->getName()))
        _channels.push_back(ch);
}

void Client::leaveChannel(const std::string &name)
{
    for (std::vector<Channel *>::iterator it = _channels.begin();
         it != _channels.end(); ++it)
    {
        if ((*it)->getName() == name)
        {
            _channels.erase(it);
            return;
        }
    }
}

std::vector<Channel *> Client::getChannels() const {
    return _channels;
}

bool Client::isInChannel(const std::string &name) const
{
    for (size_t i = 0; i < _channels.size(); ++i)
        if (_channels[i]->getName() == name)
            return true;
    return false;
}

void Client::addInvite(const std::string &channel)
{
    if (!isInvited(channel))
        _invites.push_back(channel);
}

bool Client::isInvited(const std::string &channel) const
{
    return std::find(_invites.begin(), _invites.end(), channel) != _invites.end();
}

void Client::removeInvite(const std::string &channel)
{
    std::vector<std::string>::iterator it =
        std::find(_invites.begin(), _invites.end(), channel);
    if (it != _invites.end())
        _invites.erase(it);
}

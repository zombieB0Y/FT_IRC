#include "../include/Channel.hpp"
#include "../include/Client.hpp"
#include <algorithm>
#include <sstream>

Channel::Channel(const std::string &name, Client *creator)
    : _name(name), _topic(""), _key(""), _limit(0),
      _inviteOnly(false), _topicProtected(true)
{
    _members.push_back(creator);
    _operators.push_back(creator);
}

Channel::~Channel() {}

const std::string &Channel::getName()  const { return _name; }
const std::string &Channel::getTopic() const { return _topic; }
const std::string &Channel::getKey()   const { return _key; }
int                Channel::getLimit() const { return _limit; }

bool Channel::isInviteOnly()     const { return _inviteOnly; }
bool Channel::isTopicProtected() const { return _topicProtected; }
bool Channel::hasKey()           const { return !_key.empty(); }
bool Channel::hasLimit()         const { return _limit > 0; }

void Channel::setInviteOnly(bool val)            { _inviteOnly = val; }
void Channel::setTopicProtected(bool val)        { _topicProtected = val; }
void Channel::setKey(const std::string &key)     { _key = key; }
void Channel::removeKey()                        { _key = ""; }
void Channel::setLimit(int limit)                { _limit = limit; }
void Channel::removeLimit()                      { _limit = 0; }
void Channel::setTopic(const std::string &topic) { _topic = topic; }

void Channel::addMember(Client *client)
{
    if (!hasMember(client))
        _members.push_back(client);
}

void Channel::removeMember(Client *client)
{
    _members.erase(std::remove(_members.begin(), _members.end(), client), _members.end());
    _operators.erase(std::remove(_operators.begin(), _operators.end(), client), _operators.end());
}

bool Channel::hasMember(Client *client) const
{
    return std::find(_members.begin(), _members.end(), client) != _members.end();
}

bool Channel::hasMember(const std::string &nick) const
{
    for (size_t i = 0; i < _members.size(); ++i)
        if (_members[i]->getNick() == nick) return true;
    return false;
}

Client* Channel::getMember(const std::string &nick) const
{
    for (size_t i = 0; i < _members.size(); ++i)
        if (_members[i]->getNick() == nick) return _members[i];
    return NULL;
}

std::vector<Client*> Channel::getMembers() const { return _members; }
int Channel::getMemberCount() const { return (int)_members.size(); }

void Channel::addOperator(Client *client)
{
    if (!isOperator(client))
        _operators.push_back(client);
}

void Channel::removeOperator(Client *client)
{
    _operators.erase(std::remove(_operators.begin(), _operators.end(), client), _operators.end());
}

bool Channel::isOperator(Client *client) const
{
    return std::find(_operators.begin(), _operators.end(), client) != _operators.end();
}

void Channel::broadcast(const std::string &msg, Client *exclude) const
{
    for (size_t i = 0; i < _members.size(); ++i)
        if (_members[i] != exclude)
            _members[i]->sendMsg(msg);
}

void Channel::broadcastAll(const std::string &msg) const
{
    broadcast(msg, NULL);
}

std::string Channel::getModeString() const
{
    std::string modes = "+";
    std::string params = "";
    if (_inviteOnly)     modes += "i";
    if (_topicProtected) modes += "t";
    if (!_key.empty())   { modes += "k"; params += " " + _key; }
    if (_limit > 0)      {
        modes += "l";
        std::ostringstream oss;
        oss << _limit;
        params += " " + oss.str();
    }
    if (modes == "+") return "+";
    return modes + params;
}

std::string Channel::getNamesReply() const
{
    std::string names;
    for (size_t i = 0; i < _members.size(); ++i)
    {
        if (i > 0)
            names += " ";
        if (isOperator(_members[i]))
            names += "@";
        names += _members[i]->getNick();
    }
    return names;
}

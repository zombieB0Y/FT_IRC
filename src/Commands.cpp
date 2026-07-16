#include "../include/IrcServer.hpp"
#include <sstream>
#include <iostream>
#include <cstdlib>

// ─────────────────────────────────────────────────────────────────────────────
// PASS
// ─────────────────────────────────────────────────────────────────────────────
void IrcServer::_cmdPass(Client *c, const IrcMessage &m)
{
	if (c->isPassAccepted() || c->isRegistered())
	{
		_sendErr(c, ERR_ALREADYREGISTRED, ":Unauthorized command (already registered)");
		return;
	}
	if (m.params.empty())
	{
		_sendErr(c, ERR_NEEDMOREPARAMS, "PASS :Not enough parameters");
		return;
	}
	if (m.params[0] != _password)
	{
		_sendErr(c, ERR_PASSWDMISMATCH, ":Password incorrect");
		return;
	}
	c->setPassAccepted();
}

// ─────────────────────────────────────────────────────────────────────────────
// NICK
// ─────────────────────────────────────────────────────────────────────────────
void IrcServer::_cmdNick(Client *c, const IrcMessage &m)
{
	if (m.params.empty())
	{
		_sendErr(c, ERR_NONICKNAMEGIVEN, ":No nickname given");
		return;
	}
	const std::string &newNick = m.params[0];
	if (!_isValidNick(newNick))
	{
		_sendErr(c, ERR_ERRONEUSNICKNAME, newNick + " :Erroneous nickname");
		return;
	}
	if (_isNickInUse(newNick) && newNick != c->getNick())
	{
		_sendErr(c, ERR_NICKNAMEINUSE, newNick + " :Nickname is already in use");
		return;
	}
	if (c->isRegistered())
	{
		std::string msg = ":" + c->getPrefix() + " NICK :" + newNick + "\r\n";
		std::vector<Channel*> chans = c->getChannels();
		for (size_t i = 0; i < chans.size(); ++i)
			chans[i]->broadcastAll(msg);
		if (chans.empty())
			_send(c, msg);
	}
	if (!(c->getNick() == "*") && !c->getNick().empty()) {
		_sendErr(c, ERR_NICKNAMEINUSE, newNick + " :Nickname is already set");
		return;
	}
	c->setNick(newNick);
	if (c->isRegistered())
		_sendWelcome(c);
}

// ─────────────────────────────────────────────────────────────────────────────
// USER
// ─────────────────────────────────────────────────────────────────────────────
void IrcServer::_cmdUser(Client *c, const IrcMessage &m)
{
	if (c->isRegistered()) {
		_sendErr(c, ERR_ALREADYREGISTRED, ":Unauthorized command (already registered)");
		return;
	}
	if (m.params.size() < 4) {
		_sendErr(c, ERR_NEEDMOREPARAMS, "USER :Not enough parameters");
		return;
	}
	if (!c->getUser().empty()) {
		_sendErr(c, ERR_ALREADYREGISTRED, "USER :Already registered");
		return ;
	}
	c->setUser(m.params[0]);
	c->setRealname(m.params[3]);

	if (c->isRegistered())
		_sendWelcome(c);
}

// ─────────────────────────────────────────────────────────────────────────────
// QUIT
// ─────────────────────────────────────────────────────────────────────────────
void IrcServer::_cmdQuit(Client *c, const IrcMessage &m)
{
	std::string reason = m.params.empty() ? "Quit" : m.params[0];
	std::string msg = ":" + c->getPrefix() + " QUIT :" + reason + "\r\n";

	std::vector<Channel*> chans = c->getChannels();
	for (size_t i = 0; i < chans.size(); ++i)
	{
		chans[i]->broadcast(msg, c);
		chans[i]->removeMember(c);
		_removeChannelIfEmpty(chans[i]->getName());
	}
	_send(c, "ERROR :Closing Link: " + c->getHost() + " (Quit: " + reason + ")\r\n");
	// c->flushOutput();
	_disconnectClient(c->getFd(), reason);
}

// ─────────────────────────────────────────────────────────────────────────────
// JOIN
// ─────────────────────────────────────────────────────────────────────────────
void IrcServer::_cmdJoin(Client *c, const IrcMessage &m)
{
	if (m.params.empty())
	{
		_sendErr(c, ERR_NEEDMOREPARAMS, "JOIN :Not enough parameters");
		return;
	}
	if (m.params[0] == "0") // khrej
	{
		std::vector<Channel*> chans = c->getChannels();
		for (size_t i = 0; i < chans.size(); ++i)
		{
			std::string partMsg = ":" + c->getPrefix() + " PART " + chans[i]->getName() + "\r\n";
			chans[i]->broadcastAll(partMsg);
			chans[i]->removeMember(c);
			c->leaveChannel(chans[i]->getName());
			_removeChannelIfEmpty(chans[i]->getName());
		}
		return;
	}
	std::vector<std::string> chanNames, keys; // JOIN #a,#b key1,key2
	{
		std::istringstream ss(m.params[0]);
		std::string tok;
		while (std::getline(ss, tok, ','))
			chanNames.push_back(tok);
	}
	if (m.params.size() > 1)
	{
		std::istringstream ss(m.params[1]);
		std::string tok;
		while (std::getline(ss, tok, ','))
			keys.push_back(tok);
	}
	for (size_t i = 0; i < chanNames.size(); ++i)
	{
		const std::string &name = chanNames[i];
		std::string key = (i < keys.size()) ? keys[i] : "";

		if (!_isValidChannel(name))
		{
			_sendErr(c, ERR_NOSUCHCHANNEL, name + " :No such channel");
			continue;
		}
		if (c->isInChannel(name))
			continue;

		Channel *ch = _getChannel(name);
		bool isNew  = (ch == NULL);

		if (!isNew)
		{
			if (ch->isInviteOnly() && !c->isInvited(name))
			{
				_sendErr(c, ERR_INVITEONLYCHAN, name + " :Cannot join channel (+i)");
				continue;
			}
			if (ch->hasKey() && ch->getKey() != key)
			{
				_sendErr(c, ERR_BADCHANNELKEY, name + " :Cannot join channel (+k)");
				continue;
			}
			if (ch->hasLimit() && ch->getMemberCount() >= ch->getLimit())
			{
				_sendErr(c, ERR_CHANNELISFULL, name + " :Cannot join channel (+l)");
				continue;
			}
			ch->addMember(c);
		}
		else {
			// std::cout << "here ----- >\n";
			ch = _getOrCreateChannel(name, c);
		}
		c->joinChannel(ch);
		c->removeInvite(name);
		std::string joinMsg = ":" + c->getPrefix() + " JOIN " + name + "\r\n";
		ch->broadcastAll(joinMsg);
		// Send topic
		if (!ch->getTopic().empty())
			_send(c, _num(RPL_TOPIC, c, name + " :" + ch->getTopic()));
		else
			_send(c, _num(RPL_NOTOPIC, c, name + " :No topic is set"));
		_send(c, _num(RPL_NAMREPLY, c, "= " + name + " :" + ch->getNamesReply()));
		_send(c, _num(RPL_ENDOFNAMES, c, name + " :End of NAMES list"));
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// PART
// ─────────────────────────────────────────────────────────────────────────────
void IrcServer::_cmdPart(Client *c, const IrcMessage &m)
{
	if (m.params.empty())
	{
		_sendErr(c, ERR_NEEDMOREPARAMS, "PART :Not enough parameters");
		return;
	}
	std::vector<std::string> chanNames;
	{
		std::istringstream ss(m.params[0]);
		std::string tok;
		while (std::getline(ss, tok, ',')) chanNames.push_back(tok);
	}
	std::string reason = m.params.size() > 1 ? m.params[1] : c->getNick();

	for (size_t i = 0; i < chanNames.size(); ++i)
	{
		const std::string &name = chanNames[i];
		Channel *ch = _getChannel(name);
		if (!ch)
		{
			_sendErr(c, ERR_NOSUCHCHANNEL, name + " :No such channel");
			continue;
		}
		if (!ch->hasMember(c))
		{
			_sendErr(c, ERR_NOTONCHANNEL, name + " :You're not on that channel");
			continue;
		}
		std::string partMsg = ":" + c->getPrefix() + " PART " + name + " :" + reason + "\r\n";
		ch->broadcastAll(partMsg);
		ch->removeMember(c);
		c->leaveChannel(name);
		_removeChannelIfEmpty(name);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// PRIVMSG / NOTICE
// ─────────────────────────────────────────────────────────────────────────────
void IrcServer::_cmdPrivmsg(Client *c, const IrcMessage &m)
{
	if (m.params.size() < 2)
	{
		_sendErr(c, ERR_NEEDMOREPARAMS, "PRIVMSG :Not enough parameters");
		return;
	}
	const std::string &target = m.params[0];
	const std::string &text   = m.params[1];

	if (target[0] == '#' || target[0] == '&')
	{
		Channel *ch = _getChannel(target);
		if (!ch)
		{
			_sendErr(c, ERR_NOSUCHCHANNEL, target + " :No such channel");
			return;
		}
		if (!ch->hasMember(c))
		{
			_sendErr(c, ERR_CANNOTSENDTOCHAN, target + " :Cannot send to channel");
			return;
		}
		std::string msg = ":" + c->getPrefix() + " PRIVMSG " + target + " :" + text + "\r\n";
		ch->broadcast(msg, c);
	}
	else
	{
		Client *dest = _getClientByNick(target);
		if (!dest)
		{
			_sendErr(c, ERR_NOSUCHNICK, target + " :No such nick/channel");
			return;
		}
		std::string msg = ":" + c->getPrefix() + " PRIVMSG " + target + " :" + text + "\r\n";
		_send(dest, msg);
		// dest->flushOutput();
	}
}

void IrcServer::_cmdNotice(Client *c, const IrcMessage &m)
{
	if (m.params.size() < 2) return; // NOTICE silently ignores errors
	const std::string &target = m.params[0];
	const std::string &text   = m.params[1];

	if (target[0] == '#' || target[0] == '&')
	{
		Channel *ch = _getChannel(target);
		if (!ch || !ch->hasMember(c)) return;
		std::string msg = ":" + c->getPrefix() + " NOTICE " + target + " :" + text + "\r\n";
		ch->broadcast(msg, c);
	}
	else
	{
		Client *dest = _getClientByNick(target);
		if (!dest) return;
		std::string msg = ":" + c->getPrefix() + " NOTICE " + target + " :" + text + "\r\n";
		_send(dest, msg);
		dest->flushOutput();
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// KICK
// ─────────────────────────────────────────────────────────────────────────────
void IrcServer::_cmdKick(Client *c, const IrcMessage &m)
{
	if (m.params.size() < 2)
	{
		_sendErr(c, ERR_NEEDMOREPARAMS, "KICK :Not enough parameters");
		return;
	}
	const std::string &chanName = m.params[0];
	const std::string &nickName = m.params[1];
	std::string reason = (m.params.size() > 2) ? m.params[2] : c->getNick();

	Channel *ch = _getChannel(chanName);
	if (!ch)
	{
		_sendErr(c, ERR_NOSUCHCHANNEL, chanName + " :No such channel");
		return;
	}
	if (!ch->hasMember(c))
	{
		_sendErr(c, ERR_NOTONCHANNEL, chanName + " :You're not on that channel");
		return;
	}
	if (!ch->isOperator(c))
	{
		_sendErr(c, ERR_CHANOPRIVSNEEDED, chanName + " :You're not channel operator");
		return;
	}
	Client *target = ch->getMember(nickName);
	if (!target)
	{
		_sendErr(c, ERR_USERNOTINCHANNEL, nickName + " " + chanName + " :They aren't on that channel");
		return;
	}

	std::string kickMsg = ":" + c->getPrefix() + " KICK " + chanName + " " + nickName + " :" + reason + "\r\n";
	ch->broadcastAll(kickMsg);
	ch->removeMember(target);
	target->leaveChannel(chanName);
	_removeChannelIfEmpty(chanName);
}

// ─────────────────────────────────────────────────────────────────────────────
// INVITE
// ─────────────────────────────────────────────────────────────────────────────
void IrcServer::_cmdInvite(Client *c, const IrcMessage &m)
{
	if (m.params.size() < 2)
	{
		_sendErr(c, ERR_NEEDMOREPARAMS, "INVITE :Not enough parameters");
		return;
	}
	const std::string &nickName = m.params[0];
	const std::string &chanName = m.params[1];

	Channel *ch = _getChannel(chanName);
	if (ch && !ch->hasMember(c))
	{
		_sendErr(c, ERR_NOTONCHANNEL, chanName + " :You're not on that channel");
		return;
	}
	if (ch && ch->isInviteOnly() && !ch->isOperator(c))
	{
		_sendErr(c, ERR_CHANOPRIVSNEEDED, chanName + " :You're not channel operator");
		return;
	}
	Client *target = _getClientByNick(nickName);
	if (!target)
	{
		_sendErr(c, ERR_NOSUCHNICK, nickName + " :No such nick/channel");
		return;
	}
	if (ch && ch->hasMember(target))
	{
		_sendErr(c, ERR_USERONCHANNEL, nickName + " " + chanName + " :is already on channel");
		return;
	}

	target->addInvite(chanName);
	_send(c, _num(RPL_INVITING, c, nickName + " " + chanName));
	std::string invMsg = ":" + c->getPrefix() + " INVITE " + nickName + " :" + chanName + "\r\n";
	_send(target, invMsg);
	target->flushOutput();
}

// ─────────────────────────────────────────────────────────────────────────────
// TOPIC
// ─────────────────────────────────────────────────────────────────────────────
void IrcServer::_cmdTopic(Client *c, const IrcMessage &m)
{
	if (m.params.empty())
	{
		_sendErr(c, ERR_NEEDMOREPARAMS, "TOPIC :Not enough parameters");
		return;
	}
	const std::string &chanName = m.params[0];
	Channel *ch = _getChannel(chanName);
	if (!ch)
	{
		_sendErr(c, ERR_NOSUCHCHANNEL, chanName + " :No such channel");
		return;
	}
	if (!ch->hasMember(c))
	{
		_sendErr(c, ERR_NOTONCHANNEL, chanName + " :You're not on that channel");
		return;
	}
	if (m.params.size() == 1)
	{
		if (ch->getTopic().empty())
			_send(c, _num(RPL_NOTOPIC, c, chanName + " :No topic is set"));
		else
			_send(c, _num(RPL_TOPIC, c, chanName + " :" + ch->getTopic()));
		return;
	}
	if (ch->isTopicProtected() && !ch->isOperator(c))
	{
		_sendErr(c, ERR_CHANOPRIVSNEEDED, chanName + " :You're not channel operator");
		return;
	}
	ch->setTopic(m.params[1]);
	std::string topicMsg = ":" + c->getPrefix() + " TOPIC " + chanName + " :" + m.params[1] + "\r\n";
	ch->broadcastAll(topicMsg);
}

// ─────────────────────────────────────────────────────────────────────────────
// MODE
// ─────────────────────────────────────────────────────────────────────────────
void IrcServer::_cmdMode(Client *c, const IrcMessage &m)
{
	if (m.params.empty())
	{
		_sendErr(c, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters");
		return;
	}
	const std::string &target = m.params[0];

	if (target[0] == '#' || target[0] == '&')
	{
		Channel *ch = _getChannel(target);
		if (!ch)
		{
			_sendErr(c, ERR_NOSUCHCHANNEL, target + " :No such channel");
			return;
		}
		_handleChannelMode(c, ch, m);
	}
	else
	{
		Client *targetClient = _getClientByNick(target);
		if (!targetClient)
		{
			_sendErr(c, ERR_NOSUCHNICK, target + " :No such nick");
			return;
		}
		_handleUserMode(c, targetClient, m);
	}
}

void IrcServer::_handleChannelMode(Client *c, Channel *ch, const IrcMessage &m)
{
	if (m.params.size() == 1)
	{
		_send(c, _num(RPL_CHANNELMODEIS, c, ch->getName() + " " + ch->getModeString()));
		return;
	}
	if (!ch->isOperator(c))
	{
		_sendErr(c, ERR_CHANOPRIVSNEEDED, ch->getName() + " :You're not channel operator");
		return;
	}

	const std::string &modeStr = m.params[1];
	bool adding = true;
	int paramIdx = 2; // next param
	std::string appliedModes = "";
	std::string appliedParams = "";

	for (size_t i = 0; i < modeStr.size(); ++i)
	{
		char mode = modeStr[i];
		if (mode == '+') { adding = true; continue; }
		if (mode == '-') { adding = false; continue; }

		switch (mode)
		{
			case 'i':
				ch->setInviteOnly(adding);
				appliedModes += (adding ? "+" : "-");
				appliedModes += "i";
				break;

			case 't':
				ch->setTopicProtected(adding);
				appliedModes += (adding ? "+" : "-");
				appliedModes += "t";
				break;

			case 'k':
				if (adding)
				{
					if (paramIdx >= (int)m.params.size())
					{
						_sendErr(c, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters");
						break;
					}
					ch->setKey(m.params[paramIdx]);
					appliedModes  += "+k";
					appliedParams += " " + m.params[paramIdx];
					++paramIdx;
				}
				else
				{
					ch->removeKey();
					appliedModes += "-k";
				}
				break;

			case 'o':
			{
				if (paramIdx >= (int)m.params.size())
				{
					_sendErr(c, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters");
					break;
				}
				Client *target = ch->getMember(m.params[paramIdx]);
				if (!target)
				{
					_sendErr(c, ERR_USERNOTINCHANNEL, m.params[paramIdx] + " " + ch->getName() + " :They aren't on that channel");
					++paramIdx;
					break;
				}
				if (adding) ch->addOperator(target);
				else        ch->removeOperator(target);
				appliedModes  += (adding ? "+" : "-");
				appliedModes  += "o";
				appliedParams += " " + m.params[paramIdx];
				++paramIdx;
				break;
			}

			case 'l':
				if (adding)
				{
					if (paramIdx >= (int)m.params.size())
					{
						_sendErr(c, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters");
						break;
					}
					int lim = atoi(m.params[paramIdx].c_str());
					if (lim > 0)
					{
						ch->setLimit(lim);
						appliedModes  += "+l";
						std::ostringstream oss; oss << lim;
						appliedParams += " " + oss.str();
					}
					++paramIdx;
				}
				else
				{
					ch->removeLimit();
					appliedModes += "-l";
				}
				break;

			default:
				_sendErr(c, ERR_UNKNOWNMODE, std::string(1, mode) + " :is unknown mode char to me");
				break;
		}
	}

	if (!appliedModes.empty())
	{
		std::string modeMsg = ":" + c->getPrefix() + " MODE " + ch->getName()
							+ " " + appliedModes + appliedParams + "\r\n";
		ch->broadcastAll(modeMsg);
	}
}

void IrcServer::_handleUserMode(Client *c, Client *target, const IrcMessage &m)
{
	if (c->getFd() != target->getFd())
	{
		_sendErr(c, ERR_USERSDONTMATCH, ":Cannot change mode for other users");
		return;
	}
	if (m.params.size() == 1)
	{
		_send(c, _num(RPL_UMODEIS, c, "+"));
		return;
	}
	std::string modeMsg = ":" + c->getPrefix() + " MODE " + c->getNick() + " " + m.params[1] + "\r\n";
	_send(c, modeMsg);
}

// ─────────────────────────────────────────────────────────────────────────────
// WHO
// ─────────────────────────────────────────────────────────────────────────────
void IrcServer::_cmdWho(Client *c, const IrcMessage &m)
{
	if (m.params.empty())
	{
		_send(c, _num("315", c, "* :End of WHO list"));
		return;
	}
	const std::string &mask = m.params[0];

	if (mask[0] == '#' || mask[0] == '&')
	{
		Channel *ch = _getChannel(mask);
		if (!ch) { _send(c, _num("315", c, mask + " :End of WHO list")); return; }
		std::vector<Client*> members = ch->getMembers();
		for (size_t i = 0; i < members.size(); ++i)
		{
			Client *m2 = members[i];
			std::string flags = "H";
			if (ch->isOperator(m2)) flags += "@";
			_send(c, _num("352", c, mask + " " + m2->getUser() + " " + m2->getHost()
				+ " " + SERVER_NAME + " " + m2->getNick() + " " + flags
				+ " :0 " + m2->getRealname()));
		}
	}
	else
	{
		Client *target = _getClientByNick(mask);
		if (target)
		{
			_send(c, _num("352", c, "* " + target->getUser() + " " + target->getHost()
				+ " " + SERVER_NAME + " " + target->getNick()
				+ " H :0 " + target->getRealname()));
		}
	}
	_send(c, _num("315", c, mask + " :End of WHO list"));
}

// ─────────────────────────────────────────────────────────────────────────────
// WHOIS
// ─────────────────────────────────────────────────────────────────────────────
void IrcServer::_cmdWhois(Client *c, const IrcMessage &m)
{
	if (m.params.empty())
	{
		_sendErr(c, ERR_NONICKNAMEGIVEN, ":No nickname given");
		return;
	}
	const std::string &nick = m.params[m.params.size() > 1 ? 1 : 0];
	Client *target = _getClientByNick(nick);
	if (!target)
	{
		_sendErr(c, ERR_NOSUCHNICK, nick + " :No such nick/channel");
		_send(c, _num(RPL_ENDOFWHOIS, c, nick + " :End of WHOIS list"));
		return;
	}
	_send(c, _num(RPL_WHOISUSER, c, nick + " " + target->getUser()
		+ " " + target->getHost() + " * :" + target->getRealname()));
	_send(c, _num(RPL_WHOISSERVER, c, nick + " " + SERVER_NAME + " :ft_irc server"));

	std::vector<Channel*> chans = target->getChannels();
	if (!chans.empty())
	{
		std::string chanList;
		for (size_t i = 0; i < chans.size(); ++i)
		{
			if (i) chanList += " ";
			if (chans[i]->isOperator(target)) chanList += "@";
			chanList += chans[i]->getName();
		}
		_send(c, _num(RPL_WHOISCHANNELS, c, nick + " :" + chanList));
	}
	_send(c, _num(RPL_ENDOFWHOIS, c, nick + " :End of WHOIS list"));
}

// ─────────────────────────────────────────────────────────────────────────────
// LIST
// ─────────────────────────────────────────────────────────────────────────────
void IrcServer::_cmdList(Client *c, const IrcMessage &m)
{
	(void)m;
	for (std::map<std::string,Channel*>::const_iterator it = _channels.begin();
		 it != _channels.end(); ++it)
	{
		Channel *ch = it->second;
		std::ostringstream oss;
		oss << ch->getMemberCount();
		_send(c, _num(RPL_LIST, c, ch->getName() + " " + oss.str() + " :" + ch->getTopic()));
	}
	_send(c, _num(RPL_LISTEND, c, ":End of LIST"));
}

// ─────────────────────────────────────────────────────────────────────────────
// NAMES
// ─────────────────────────────────────────────────────────────────────────────
void IrcServer::_cmdNames(Client *c, const IrcMessage &m)
{
	if (m.params.empty())
	{
		for (std::map<std::string,Channel*>::const_iterator it = _channels.begin();
			 it != _channels.end(); ++it)
		{
			Channel *ch = it->second;
			_send(c, _num(RPL_NAMREPLY, c, "= " + ch->getName() + " :" + ch->getNamesReply()));
			_send(c, _num(RPL_ENDOFNAMES, c, ch->getName() + " :End of NAMES list"));
		}
		return;
	}
	const std::string &chanName = m.params[0];
	Channel *ch = _getChannel(chanName);
	if (!ch)
	{
		_send(c, _num(RPL_ENDOFNAMES, c, chanName + " :End of NAMES list"));
		return;
	}
	_send(c, _num(RPL_NAMREPLY, c, "= " + chanName + " :" + ch->getNamesReply()));
	_send(c, _num(RPL_ENDOFNAMES, c, chanName + " :End of NAMES list"));
}

// ─────────────────────────────────────────────────────────────────────────────
// PING / PONG
// ─────────────────────────────────────────────────────────────────────────────
void IrcServer::_cmdPing(Client *c, const IrcMessage &m)
{
	std::string token = m.params.empty() ? SERVER_NAME : m.params[0];
	_send(c, ":" + std::string(SERVER_NAME) + " PONG " + SERVER_NAME + " :" + token + "\r\n");
}

void IrcServer::_cmdPong(Client *c, const IrcMessage &m) { // hhhh khalih just leave it
	(void)c;
	(void)m;
}

// ─────────────────────────────────────────────────────────────────────────────
// OPER
// ─────────────────────────────────────────────────────────────────────────────
void IrcServer::_cmdOper(Client *c, const IrcMessage &m)
{
	if (m.params.size() < 2)
	{
		_sendErr(c, ERR_NEEDMOREPARAMS, "OPER :Not enough parameters");
		return;
	}
	if (m.params[0] != "oper" || m.params[1] != _password)
	{
		_sendErr(c, ERR_PASSWDMISMATCH, ":Password incorrect");
		return;
	}
	c->setOperator(true);
	_send(c, _num(RPL_YOUREOPER, c, ":You are now an IRC operator"));
	std::string modeMsg = ":" + c->getPrefix() + " MODE " + c->getNick() + " +o\r\n";
	_send(c, modeMsg);
}

// ─────────────────────────────────────────────────────────────────────────────
// KILL (IRC operator command)
// ─────────────────────────────────────────────────────────────────────────────
void IrcServer::_cmdKill(Client *c, const IrcMessage &m)
{
	if (!c->isOperator())
	{
		_sendErr(c, ERR_NOPRIVILEGES, ":Permission Denied- You're not an IRC operator");
		return;
	}
	if (m.params.size() < 2)
	{
		_sendErr(c, ERR_NEEDMOREPARAMS, "KILL :Not enough parameters");
		return;
	}
	Client *target = _getClientByNick(m.params[0]);
	if (!target)
	{
		_sendErr(c, ERR_NOSUCHNICK, m.params[0] + " :No such nick");
		return;
	}
	std::string reason = m.params[1];
	std::string killMsg = ":" + c->getPrefix() + " KILL " + target->getNick() + " :" + reason + "\r\n";
	_send(target, killMsg);
	target->flushOutput();
	_disconnectClient(target->getFd(), "Killed by " + c->getNick() + " (" + reason + ")");
}

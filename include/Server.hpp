#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <cerrno>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include "Client.hpp"
#include "Channel.hpp"

class Client;
class Channel;
class Server;

#define MAX_EVENTS   64
#define BUFFER_SIZE  4096
#define SERVER_NAME  "ircserv"
#define SERVER_VER   "1.0"

// IRC Numeric Replies
#define RPL_WELCOME          "001"
#define RPL_YOURHOST         "002"
#define RPL_CREATED          "003"
#define RPL_MYINFO           "004"
#define RPL_ISUPPORT         "005"
#define RPL_UMODEIS          "221"
#define RPL_WHOISUSER        "311"
#define RPL_WHOISSERVER      "312"
#define RPL_WHOISCHANNELS    "319"
#define RPL_ENDOFWHOIS       "318"
#define RPL_LIST             "322"
#define RPL_LISTEND          "323"
#define RPL_CHANNELMODEIS    "324"
#define RPL_NOTOPIC          "331"
#define RPL_TOPIC            "332"
#define RPL_INVITING         "341"
#define RPL_NAMREPLY         "353"
#define RPL_ENDOFNAMES       "366"
#define RPL_MOTDSTART        "375"
#define RPL_MOTD             "372"
#define RPL_ENDOFMOTD        "376"
#define RPL_YOUREOPER        "381"
// Errors
#define ERR_NOSUCHNICK       "401"
#define ERR_NOSUCHSERVER     "402"
#define ERR_NOSUCHCHANNEL    "403"
#define ERR_CANNOTSENDTOCHAN "404"
#define ERR_TOOMANYCHANNELS  "405"
#define ERR_UNKNOWNCOMMAND   "421"
#define ERR_NONICKNAMEGIVEN  "431"
#define ERR_ERRONEUSNICKNAME "432"
#define ERR_NICKNAMEINUSE    "433"
#define ERR_USERNOTINCHANNEL "441"
#define ERR_NOTONCHANNEL     "442"
#define ERR_USERONCHANNEL    "443"
#define ERR_NOTREGISTERED    "451"
#define ERR_NEEDMOREPARAMS   "461"
#define ERR_ALREADYREGISTRED "462"
#define ERR_PASSWDMISMATCH   "464"
#define ERR_KEYSET           "467"
#define ERR_CHANNELISFULL    "471"
#define ERR_UNKNOWNMODE      "472"
#define ERR_INVITEONLYCHAN   "473"
#define ERR_BADCHANNELKEY    "475"
#define ERR_NOPRIVILEGES     "481"
#define ERR_CHANOPRIVSNEEDED "482"
#define ERR_UMODEUNKNOWNFLAG "501"
#define ERR_USERSDONTMATCH   "502"


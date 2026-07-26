#pragma once
#include <string>
#include <iostream>
#include <sstream>
#include "../Server.hpp"
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdlib>

struct Bot_command
{
    std::string prefix;
    std::string command;
    std::vector<std::string> parameters;
};


class Bot {
    private:
        int _botFd;
        std::string _host;
        int _port;
        std::string _password;
        std::string _nickname;
        std::string _buffer;
        Bot_command command;
        std::vector<std::pair <std::string, std::string> > jokesList;
        std::vector<std::string> messages;
        std::vector<std::string> manualLines;
    public:
        Bot(std::string host,int port,std::string password) : _host(host),_port(port), _password(password){
            setter();
        };
        void _regesterWithServer();
        void _handleCommand(const std::string &prefix,const std::string &cmd,const std::vector<std::string> &args);
        int connectToServer();
        void run();
        void setter();
        void read_message(std::string message);
        void joke(const std::string &prefix,const std::string &cmd,const std::vector<std::string> &args);
        void accept_invite(const std::vector<std::string> &args);
        void help(const std::string &prefix, const std::string &cmd, const std::vector<std::string> &args);
        void manual(const std::string &prefix, const std::string &cmd, const std::vector<std::string> &args);
        void draw(const std::string &prefix, const std::string &cmd, const std::vector<std::string> &args);
        std::string draw_animals(const std::string &target, const std::string &animal);
};
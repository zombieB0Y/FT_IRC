#pragma once

#include <string>

class Client {
    public:
        bool getHasNick() const;
        bool getPassAccepted() const;
        bool getHasUser() const;
        bool getWelcome() const;
        std::string getNick() const;
        void setFd(int fds);
        void setIp(std::string i);
        void setPort(int p);
        void setPassAccepted(bool value);
        void setHasNick(bool n);
        void setHasUser(bool n);
        void setWelcome(bool value);
        void setNick(std::string n);
        void setUsername(const std::string& u);
        void setRealname(const std::string& r);
        const std::string& getRealname() const;
        const std::string& getUsername() const;
        const std::string& getIp() const;
        Client();
        std::string recvBuffer;
        std::string sendBuffer;
    private:
        int fd;
        std::string ip;
        int port;

        bool passAccepted;
        bool hasNick;
        bool hasUser;
        bool welcomed;
        std::string nick;
        std::string username;
        std::string realname;

};
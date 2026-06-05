#pragma once
#include <string>
#include <vector>
#include <map>

class Channel;

class Client
{
    private:
        int         _fd;
        std::string _nick;
        std::string _user;
        std::string _realname;
        std::string _host;
        bool        _passAccepted;
        bool        _isOper;
        std::string _recvBuf;
        std::string _sendBuf;
    
        std::vector<Channel*>    _channels;
        std::vector<std::string> _invites;

public:
    Client(int fd, const std::string &host);
    ~Client();

    int         getFd()       const;
    std::string getNick()     const;
    std::string getUser()     const;
    std::string getRealname() const;
    std::string getHost()     const;
    std::string getPrefix()   const;
    bool        isRegistered()   const;
    bool        isPassAccepted() const;
    bool        isOperator()     const;

    void setNick(const std::string &nick);
    void setUser(const std::string &user);
    void setRealname(const std::string &realname);
    void setPassAccepted();
    void setOperator(bool val);

    void        appendBuffer(const std::string &data);
    std::string getBuffer() const;
    void        clearBuffer();
    bool        hasNewline() const;

    void    sendMsg(const std::string &msg);
    void    addToPendingOutput(const std::string &msg);
    bool    hasPendingOutput() const;
    void    flushOutput();

    // Channel membership
    void            joinChannel(Channel *ch);
    void            leaveChannel(const std::string &name);
    std::vector<Channel*> getChannels() const;
    bool            isInChannel(const std::string &name) const;

    // Invite list
    void addInvite(const std::string &channel);
    bool isInvited(const std::string &channel) const;
    void removeInvite(const std::string &channel);

};

#pragma once
#include <string>
#include <vector>
#include <map>

class Client;

class Channel
{
private:
    std::string _name;
    std::string _topic;
    std::string _key;
    int _limit; // 0 = no limit
    bool _inviteOnly;
    bool _topicProtected;

    std::vector<Client *> _members;
    std::vector<Client *> _operators;

public:
    Channel(const std::string &name, Client *creator);
    ~Channel();

    const std::string &getName() const;
    const std::string &getTopic() const;
    const std::string &getKey() const;
    int getLimit() const;
    std::string getModeString() const;

    // Member management
    void addMember(Client *client);
    void removeMember(Client *client);
    bool hasMember(Client *client) const;
    bool hasMember(const std::string &nick) const;
    Client *getMember(const std::string &nick) const;
    std::vector<Client *> getMembers() const;
    int getMemberCount() const;

    // Operator management
    void addOperator(Client *client);
    void removeOperator(Client *client);
    bool isOperator(Client *client) const;

    // Modes: i=invite-only, t=topic-protected, k=key, o=op, l=limit
    bool isInviteOnly() const;
    bool isTopicProtected() const;
    bool hasKey() const;
    bool hasLimit() const;

    void setInviteOnly(bool val);
    void setTopicProtected(bool val);
    void setKey(const std::string &key);
    void removeKey();
    void setLimit(int limit);
    void removeLimit();
    void setTopic(const std::string &topic);

    // Broadcast
    void broadcast(const std::string &msg, Client *exclude = NULL) const;
    void broadcastAll(const std::string &msg) const;

    // Utilities
    std::string getNamesReply() const;
};

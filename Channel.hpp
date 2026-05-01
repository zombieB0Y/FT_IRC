#pragma once

#include <set>
#include <string>

class Channel {
    public:
        std::string name;
        std::string topic;

        bool inviteOnly;
        bool topicOpOnly;
        bool hasKey;
        std::string key;
        int userLimit;

        std::set<int> members;
        std::set<int> operators;
        std::set<int> invited;

        Channel();
};

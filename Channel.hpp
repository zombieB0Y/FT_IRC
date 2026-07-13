#pragma once

#include <set>
#include <string>

// ─── Channel ────────────────────────────────────────────────────────────────
// Represents a single IRC channel and its state.

class Channel {
public:
    // ── Identity ─────────────────────────────────────────────────────────────
    std::string name;
    std::string topic;

    // ── Mode flags ───────────────────────────────────────────────────────────
    bool        inviteOnly;   // +i : only invited users may join
    bool        topicOpOnly;  // +t : only operators may change the topic
    bool        hasKey;       // +k : channel is key-protected
    std::string key;          // the key value when hasKey == true
    int         userLimit;    // +l : 0 means no limit

    // ── Member sets (keyed by client fd) ─────────────────────────────────────
    std::set<int> members;
    std::set<int> operators;
    std::set<int> invited;

    Channel();
};

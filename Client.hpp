#pragma once

#include <string>

// ─── Client ─────────────────────────────────────────────────────────────────
// Holds the per-connection state for one IRC client.

class Client {
private:
	// ── Connection ───────────────────────────────────────────────────────────
	int         fd;
	std::string ip;
	int         port;

	// ── Registration flags ───────────────────────────────────────────────────
	bool        passAccepted;
	bool        hasNick;
	bool        hasUser;
	bool        welcomed;

	// ── Identity ─────────────────────────────────────────────────────────────
	std::string nick;
	std::string username;
	std::string realname;

public:
    // ── I/O buffers (accessed directly by Server for efficiency) ─────────────
    std::string recvBuffer;
    std::string sendBuffer;

    // ── Constructor ──────────────────────────────────────────────────────────
    Client();

    // ── Connection metadata setters ──────────────────────────────────────────
    void setFd(int fd);
    void setIp(const std::string& ip);
    void setPort(int port);

    // ── Registration state setters ───────────────────────────────────────────
    void setPassAccepted(bool value);
    void setHasNick(bool value);
    void setHasUser(bool value);
    void setWelcome(bool value);

    // ── Identity setters ─────────────────────────────────────────────────────
    void setNick(const std::string& nick);
    void setUsername(const std::string& username);
    void setRealname(const std::string& realname);

    // ── Getters ──────────────────────────────────────────────────────────────
    int                getFd()           const;
    const std::string& getIp()           const;
    int                getPort()         const;

    bool               getPassAccepted() const;
    bool               getHasNick()      const;
    bool               getHasUser()      const;
    bool               getWelcome()      const;

    std::string        getNick()         const;
    const std::string& getUsername()     const;
    const std::string& getRealname()     const;

};

#include "Client.hpp"

// ─── Constructor ─────────────────────────────────────────────────────────────

Client::Client()
    : fd(-1)
    , port(0)
    , passAccepted(false)
    , hasNick(false)
    , hasUser(false)
    , welcomed(false)
{}

// ─── Connection metadata ─────────────────────────────────────────────────────

void Client::setFd(int f)               { fd   = f; }
void Client::setIp(const std::string& i){ ip   = i; }
void Client::setPort(int p)             { port = p; }

int                Client::getFd()   const { return fd;   }
const std::string& Client::getIp()   const { return ip;   }
int                Client::getPort() const { return port; }

// ─── Registration state ──────────────────────────────────────────────────────

void Client::setPassAccepted(bool v) { passAccepted = v; }
void Client::setHasNick(bool v)      { hasNick      = v; }
void Client::setHasUser(bool v)      { hasUser      = v; }
void Client::setWelcome(bool v)      { welcomed     = v; }

bool Client::getPassAccepted() const { return passAccepted; }
bool Client::getHasNick()      const { return hasNick;      }
bool Client::getHasUser()      const { return hasUser;      }
bool Client::getWelcome()      const { return welcomed;     }

// ─── Identity ────────────────────────────────────────────────────────────────

void Client::setNick(const std::string& n)     { nick     = n; }
void Client::setUsername(const std::string& u) { username = u; }
void Client::setRealname(const std::string& r) { realname = r; }

std::string        Client::getNick()     const { return nick;     }
const std::string& Client::getUsername() const { return username; }
const std::string& Client::getRealname() const { return realname; }

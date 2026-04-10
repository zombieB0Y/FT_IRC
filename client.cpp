#include "client.hpp"

client::client() : fd(-1), is_auth(false) {}

client::client(const client &copy) {
    *this = copy;
}

client  &client::operator=(const client &copy) {
    if (this != &copy) {
        setFd(copy.getFd());
        setIp(copy.getIp());
        this->is_auth = copy.is_authenticate();
    }
    return *this;
}

client::~client() {}

int client::getFd() const {
    return this->fd;
}

std::string client::getIp() const {
    return this->ip;
}

void    client::setFd(int _fd) {
    this->fd = _fd;
}

void    client::setIp(std::string _ip) {
    this->ip = _ip;
}

bool    client::is_authenticate() const {
    return this->is_auth;
}

void    client::authenticate() {
    this->is_auth = true;
}

void    client::setBuffer(std::string buff) {
    this->buffer = buff;
}
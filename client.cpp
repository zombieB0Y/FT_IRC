#include "client.hpp"

client::client() : fd(-1) {}

client::client(const client &copy) {
    *this = copy;
}

client  &client::operator=(const client &copy) {
    if (this != &copy) {
        setFd(copy.getFd());
        setIp(copy.getIp());
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
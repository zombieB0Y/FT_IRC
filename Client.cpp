#include "Client.hpp"

Client::Client() : fd(-1), port(0), passAccepted(false), hasNick(false), hasUser(false), welcomed(false) {}

bool Client::getHasNick() const{
    return hasNick;
}

void Client::setRealname(const std::string& r) {
    realname = r;
}

void Client::setHasUser(bool n){
    hasUser = n;
}

const std::string& Client::getRealname() const {
    return realname;
}

const std::string& Client::getIp() const {
    return ip;
}

const std::string& Client::getUsername() const {
    return username;
}

void Client::setHasNick(bool n){
    this->hasNick = n;
}

bool Client::getWelcome() const{
    return welcomed;
}

bool Client::getPassAccepted() const{
    return passAccepted;
}

void Client::setPassAccepted(bool value){
    this->passAccepted = value;
}

void Client::setUsername(const std::string& u) {
    username = u;
}

void Client::setNick(std::string n){
    this->nick = n;
}

std::string Client::getNick() const{
    return nick;
}

bool Client::getHasUser() const{
    return hasUser;
}

void Client::setFd(int fds){
    this->fd = fds;
}

void Client::setPort(int p) {
    this->port = p;
}

void Client::setIp(std::string i) {
    this->ip = i;
}

void Client::setWelcome(bool value) {
    this->welcomed = value;
}
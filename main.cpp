#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include <cstdlib>

bool valid_port(std::string port){
    std::stringstream ss;
    int p = 0;
    ss << port;
    ss >> p;
    if (ss.fail() || !ss.eof())
        return false;
    if (p < 1 || p > 65535)
        return false;
    return true;
}
bool empty_pass(std::string pass){
    if (pass.length() == 0)
        return true;
    std::string whitespace = " \t\n\r";
    size_t start = pass.find_first_not_of(whitespace);
    if (start == std::string::npos)
        return true;
    return false;
}


int main(int argc, char** argv){
    if (argc != 3){
        std::cerr << "Usage: ./ircserv <port> <pass>" << std::endl;
        return 1;
    }
    if (!valid_port(argv[1])){
        std::cerr << "Not a valid port" << std::endl;
        return 1;
    }
    if (empty_pass(argv[2])){
        std::cerr << "password must not be empty" << std::endl;
        return 1;
    }
    int port = std::atoi(argv[1]);
    Server ser(port, argv[2]);
    if (!ser.init()){
        std::cerr << "Could not start the server" << std::endl;
        return 1;
    }
    ser.run();
}
#include "../include/IrcServer.hpp"
#include <iostream>
#include <cstdlib>
#include <csignal>

static IrcServer *g_server = NULL;

static void signalHandler(int sig)
{
    (void)sig;
    std::cout << "\n[ft_irc] Signal received, shutting down..." << std::endl;
    IrcServer::running = false;
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
        return 1;
    }

    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535)
    {
        std::cerr << "Error: invalid port number" << std::endl;
        return 1;
    }

    std::string password = argv[2];
    if (password.empty())
    {
        std::cerr << "Error: password cannot be empty" << std::endl;
        return 1;
    }

    signal(SIGINT,  signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGPIPE, SIG_IGN); // Ignore broken pipe

    try
    {
        IrcServer server(port, password);
        g_server = &server;
        server.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "[ft_irc] Server stopped." << std::endl;
    return 0;
}

#include "Server.hpp"
#include <cstdlib>
#include <sstream>
#include <csignal>


// --- signal handler -----------------------------------------------------------
static void signalHandler(int sig)
{
	(void)sig;
	std::cout << "\n[ft_irc] Signal received, shutting down..." << std::endl;
	Server::running = false;
}


// ─── Argument validation ──────────────────────────────────────────────────────

static bool validPort(const std::string& portStr)
{
	std::istringstream ss(portStr);
	int port = 0;
	ss >> port;
	if (ss.fail() || !ss.eof())
		return false;
	return port >= 1 && port <= 65535;
}

static bool emptyPassword(const std::string& pass)
{
	if (pass.empty())
		return true;
	const std::string ws = " \t\n\r";
	return pass.find_first_not_of(ws) == std::string::npos;
}

// ─── Entry point ─────────────────────────────────────────────────────────────

int main(int argc, char** argv)
{
	signal(SIGINT,  signalHandler);
	signal(SIGTERM, signalHandler);
	signal(SIGPIPE, SIG_IGN); // Ignore broken pipe

	if (argc != 3) {
		std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
		return 1;
	}
	if (!validPort(argv[1])) {
		std::cerr << "Error: invalid port number" << std::endl;
		return 1;
	}
	if (emptyPassword(argv[2])) {
		std::cerr << "Error: password must not be empty or whitespace-only" << std::endl;
		return 1;
	}

	int port = std::atoi(argv[1]);
	Server server(port, argv[2]);

	if (!server.init()) {
		std::cerr << "Error: could not start the server" << std::endl;
		return 1;
	}

	server.run();
	return 0;
}

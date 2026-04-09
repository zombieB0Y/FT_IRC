#include "server.hpp"
#include "client.hpp"

int main(int ac, char **av) {
	if (ac > 1) {
		server	server(atoi(av[1]));
		try{
			signal(SIGINT, server::signalhandler);
			signal(SIGQUIT, server::signalhandler);
			server.server_init();
		}
		catch(const std::exception& e){
			server.clear_fds();
			std::cerr << e.what() << std::endl;
		}
	}
}
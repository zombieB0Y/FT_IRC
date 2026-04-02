#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string>
#include <unistd.h>

int main() {
	int	servsocket = socket(AF_INET, SOCK_STREAM, 0);
	if (servsocket == -1) {
		std::cerr << "opening server socket failed !\n";
		return 1;
	}

	sockaddr_in	server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(8080);
	server_address.sin_addr.s_addr = INADDR_ANY;
	if (bind(servsocket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
		std::cerr << "binding server socket failed !\n";
		return 1;
	}
	if (listen(servsocket, 5) == -1) {
		std::cerr << "listening server socket failed !\n";
		return 1;
	}
	
	int clientsocket = accept(servsocket, 0, 0);
	char	buffer[1024];
	// char	line[1025];
	std::cout << "CLIENT Message >>> ";
	int bit_read = recv(clientsocket, buffer, sizeof(buffer), 0);
	while (bit_read) {
		write(1, buffer, bit_read);
		bit_read = recv(clientsocket, buffer, sizeof(buffer), 0);
	}
	close(servsocket);
	close(clientsocket);
}
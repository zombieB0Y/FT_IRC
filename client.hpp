#pragma once

#include <iostream>


class client
{
private:
	int	fd;
	std::string	ip;
public:
	client();
	client(const client &copy);
	client	&operator=(const client &copy);
	~client();

	int	getFd() const;
	std::string	getIp() const;
	void	setFd(int _fd);
	void	setIp(std::string _ip);
};

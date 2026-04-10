#pragma once

#include <iostream>


class client
{
private:
	int	fd;
	std::string	ip;
	std::string	buffer;
	bool		is_auth;
public:
	client();
	client(const client &copy);
	client	&operator=(const client &copy);
	~client();

	int	getFd() const;
	std::string	getIp() const;
	void	setFd(int _fd);
	void	setIp(std::string _ip);
	bool	is_authenticate() const;
	void	authenticate();
	void	setBuffer(std::string buff);
};

#include "Bot.hpp"

volatile bool running;

static void signalHandler(int sig)
{
	(void)sig;
	std::cout << "\nshutting down..." << std::endl;
	running = false;

}

int Bot::connectToServer(){
	_botFd = socket(AF_INET, SOCK_STREAM, 0);
	if (_botFd < 0){
		std::cout << "failed to create a socket"  << std::endl;
			return 0;
	}
	struct sockaddr_in serverAdress;
	std::memset(&serverAdress, 0, sizeof(serverAdress));
	serverAdress.sin_family = AF_INET;
	serverAdress.sin_addr.s_addr = inet_addr(_host.c_str());
	serverAdress.sin_port = htons(_port);
	if (connect(_botFd,(struct sockaddr*)&serverAdress,sizeof(serverAdress)) == -1){
		std::cerr << "failed to connect" << std::endl;
		return 0;
	}
	std::cout << "connected to IRC" << std::endl;
	return 1;
}

void Bot::_regesterWithServer(){
	std::string a[4];
	a[0] = "PASS " + _password + "\r\n";
	a[1] = "NICK " + _nickname + "\r\n";
	a[2] = "USER Bot 0 * :" + _nickname + "\r\n";
	a[3] = "JOIN #GENERAL \r\n";
	for(int i = 0; i < 4 ;i++)
		send(_botFd,a[i].c_str(),a[i].size(),0);
}

void Bot::setter(){
	_nickname = "Bot1";
	jokesList.push_back(std::make_pair("What did the shark say when he ate the clownfish?","This tastes a little funny."));
	jokesList.push_back(std::make_pair("Why do programmers prefer dark mode?","Because light attracts bugs!"));
	jokesList.push_back(std::make_pair("How many programmers does it take to change a light bulb?","None. It's a hardware problem."));
	jokesList.push_back(std::make_pair("Why did the C++ programmer get bad grades?","Because they couldn't get the class pointers right."));
	jokesList.push_back(std::make_pair("Why do Java developers wear glasses?","Because they don't C#!"));
	messages = "PRIVMSG :🤖 Available commands for Bot1:\r\nr\nPRIVMSG :!joke - I'll tell you a funny joke.\r\nPRIVMSG :!help - Shows this list of commands.\r\nPRIVMSG :!manual - Shows the IRC server manual.\r\n";
	manualLines = "📖 IRC Server Manual:\r\nJOIN <#channel> : Join one or more channels.\r\nPRIVMSG <target> :<message> : Send a message to a user or channel.\r\nKICK <#channel> <nickname> [reason] : Kick a user from a channel (Operator only).\r\nINVITE <nickname> <#channel> : Invite a user to a channel (Operator only).\r\nTOPIC <#channel> [topic] : View or change a channel's topic.\r\nMODE <#channel> <+/-itkol> [args] : Change channel modes.\r\nPART <#channel> [reason] : Leave a channel.\r\nQUIT [reason] : Disconnect from the server.\r\n";
}

void Bot::read_message(std::string message){
	int i = 0;
	std::stringstream ss(message);
	std::string word;
	command.command.clear();
	command.prefix.clear();
	command.parameters.clear();
	while(ss >> word){
		if (i == 0 && word[0] == ':'){
			command.prefix = word;
		}
		else if (command.command.empty())
			command.command = word;
		else{
			if (word[0] == ':'){
				std::string rest;
				std::getline(ss,rest);
				word += rest;
				word.erase(0,1);
				command.parameters.push_back(word);
				break;
			}
			else
				command.parameters.push_back(word);
		}
		i++;
	}
	_handleCommand(command.prefix,command.command,command.parameters);
}
void Bot::accept_invite(const std::vector<std::string> &args){
	if (args.size() >= 2) {
		std::string reply = "JOIN " + args[1] + "\r\n";
		send(_botFd, reply.c_str(), reply.size(), 0);
	}
}
void Bot::help(const std::string &prefix, const std::string &cmd, const std::vector<std::string> &args){
	(void)cmd;
	std::string target = args[0];    
	if (target[0] != '#') {
		size_t excl_pos = prefix.find('!');
		if (excl_pos != std::string::npos) {
			target = prefix.substr(1, excl_pos - 1);
		}
	}
		send(_botFd, messages.c_str(), messages.size(), 0);
}

std::string Bot::draw_animals(const std::string &target, const std::string &animal) {
	std::string p = "PRIVMSG " + target + " :";
	
	if (animal == "cat") {
		return p + " /\\_/\\ \r\n" + 
			   p + "( o.o )\r\n" + 
			   p + " > ^ < \r\n";
	}
	else if (animal == "dog") {
		return p + " / \\__\r\n" + 
			   p + "(    @\\___\r\n" + 
			   p + " /         O\r\n";
	}
	else if (animal == "fish") {
		return p + "><(((('>\r\n";
	}
	else if (animal == "rabbit") {
		return p + " (\\_._/ )\r\n" + 
			   p + " ( o o )\r\n" + 
			   p + "  > ^ <       \r\n";
	}
	else if (animal == "turtle") {
		return p + "     _____     \r\n" + 
			   p + "   /       \\   \r\n" + 
			   p + "  |  O   O  |  \r\n" + 
			   p + "  |    ^    |  \r\n" + 
			   p + "   \\_______/   \r\n";
	}
	return p + "Unknown animal command.\r\n";
}

void Bot::draw(const std::string &prefix, const std::string &cmd, const std::vector<std::string> &args){
	(void)cmd;
	std::string target = args[0];    
	if (target[0] != '#') {
		size_t pos = prefix.find('!');
		if (pos != std::string::npos) {
			target = prefix.substr(1, pos - 1);
		}
	}
	std::string reply;
	
	if (args[1].find("cat") != std::string::npos)
		reply = draw_animals(target, "cat");
	else if (args[1].find("dog") != std::string::npos)
		reply = draw_animals(target, "dog");
	else if (args[1].find("fish") != std::string::npos)
		reply = draw_animals(target, "fish");
	else if (args[1].find("rabbit") != std::string::npos)
		reply = draw_animals(target, "rabbit");
	else if (args[1].find("turtle") != std::string::npos)
		reply = draw_animals(target, "turtle");

	if (!reply.empty()) {
		send(_botFd, reply.c_str(), reply.size(), 0);
	}
}

void Bot::joke(const std::string &prefix,const std::string &cmd,const std::vector<std::string> &args){
	 (void)cmd;
	std::string target = args[0];
	
	if (target[0] != '#') {
		size_t pos = prefix.find('!');
		if (pos != std::string::npos) {
			target = prefix.substr(1, pos - 1);
		}
	}
	int randomIndex = rand() % 5;
	std::string reply = "PRIVMSG " + target + " :" + jokesList[randomIndex].first + " \r\n";
	send(_botFd,reply.c_str(),reply.size(),0);
	reply.clear();
	reply = "PRIVMSG " + target + " :" + jokesList[randomIndex].second + "\r\n";
	send(_botFd,reply.c_str(),reply.size(),0);
}
void Bot::manual(const std::string &prefix, const std::string &cmd, const std::vector<std::string> &args){
	(void)cmd;
	std::string target = args[0];
	if (target[0] != '#') {
		size_t pos = prefix.find('!');
		if (pos != std::string::npos) {
			target = prefix.substr(1, pos - 1);
		}
	}

		std::string reply = "PRIVMSG " + target + " :" + manualLines + "\r\n";
		send(_botFd, reply.c_str(), reply.size(), 0);
}

void Bot::_handleCommand(const std::string &prefix,const std::string &cmd,const std::vector<std::string> &args){
	if (cmd == "PRIVMSG" && args.size() >= 2 && args[1].find("!joke") != std::string::npos)
		joke(prefix,cmd,args);
	if (cmd == "INVITE")
		accept_invite(args);
	if (cmd == "PRIVMSG" && args.size() >= 2 && args[1].find("!help") != std::string::npos)
		help(prefix,cmd,args);
	if (cmd == "PRIVMSG" && args.size() >= 2 && args[1].find("!manual") != std::string::npos)
		manual(prefix,cmd,args);
	if (cmd == "PRIVMSG" && args.size() >= 2 && args[1].find("!draw") != std::string::npos)
		draw(prefix,cmd,args);
}

void Bot::run() {
	running = true;
	while(running) {
		
		char tempBuffer[1024];
		int bytesReceived = recv(_botFd, tempBuffer, sizeof(tempBuffer) - 1, 0);
		if (bytesReceived <= 0)
			return;
		else{
			tempBuffer[bytesReceived ] = '\0';
			_buffer += tempBuffer;
			while (true) {
				size_t pos = _buffer.find("\r\n",0);
				
				if (pos != std::string::npos){
					read_message(_buffer.substr(0,pos));
					_buffer.erase(0,pos + 2);
				}
				else {
					break;
				}
			}
		}
	}
}

bool validPort(const std::string& portStr)
{
	std::istringstream ss(portStr);
	int port = 0;
	ss >> port;
	if (ss.fail() || !ss.eof())
		return false;
	return port >= 1 && port <= 65535;
}

bool emptyPassword(const std::string& pass)
{
	if (pass.empty())
		return true;
	const std::string ws = " \t\n\r";
	return pass.find_first_not_of(ws) == std::string::npos;
}

int main(int ac, char **argv){
	if (ac != 4){
		std::cout << "wrong args" << std::endl;
		return 1;
	}
	if (!validPort(argv[2])){
		std::cerr << "Not a valid port" << std::endl;
		return 1;
	}
	if (emptyPassword(argv[3])){
		std::cerr << "password must not be empty" << std::endl;
		return 1;
	}
	struct sigaction sa;
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; // The magic happens here: NO SA_RESTART
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    // sigaction(SIGPIPE, &sa, NULL);
	std::string host = argv[1];
	int port = atoi(argv[2]);
	std::string password = argv[3];
	
	Bot myBot(host,port,password);

	signal(SIGPIPE, SIG_IGN); // Ignore broken pipe
	if(!myBot.connectToServer())
		return 1;
	myBot._regesterWithServer();
	myBot.run();
}
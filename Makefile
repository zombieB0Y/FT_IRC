CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

NAME = ircserv

SRCS = server.cpp client.cpp main.cpp help_func.cpp

OBJS = $(SRCS:.cpp=.o)

all: $(NAME) 

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)

clean:
	rm -f $(OBJS) 

fclean: clean
	rm -f $(NAME) $(NAME_client)

re: fclean all

.PHONY: all clean fclean re client
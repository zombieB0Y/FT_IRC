CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

NAME = ircserv

NAME_client = client

SRCS = server.cpp

OBJS = $(SRCS:.cpp=.o)

SRCS_client = client.cpp

OBJS_client = $(SRCS_client:.cpp=.o)

all: $(NAME) 

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)

client: $(OBJS_client)
	$(CXX) $(CXXFLAGS) -o $(NAME_client) $(OBJS_client)

clean:
	rm -f $(OBJS) $(OBJS_client)

fclean: clean
	rm -f $(NAME) $(NAME_client)

re: fclean all

.PHONY: all clean fclean re client
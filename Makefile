# **************************************************************************** #
#                                 Makefile                                     #
# **************************************************************************** #

NAME		= ircserv
CXX			= c++
CXXFLAGS	= -Wall -Wextra -Werror -std=c++98
RM			= rm -f
BN			= ircBOT

# Source files (add all your .cpp files here)
SRCS		= main.cpp \
			  Server.cpp \
			  Client.cpp \
			  Channel.cpp \

B_SRCS		= Bot/Bot.cpp

# Object files (replace .cpp with .o)
OBJS		= $(SRCS:.cpp=.o)

B_OBJS		= $(B_SRCS:.cpp=.o)

# Colours for pretty output (optional)
GREEN		= \033[0;32m
RED			= \033[0;31m
RESET		= \033[0m

all: $(NAME)

$(NAME): $(OBJS)
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@echo "$(GREEN)✓ Compiled $(NAME) successfully$(RESET)"

%.o: %.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@
	@echo "  Compiling $<"

Bonus: $(B_OBJS)
	@$(CXX) $(CXXFLAGS) $(B_OBJS) -o $(BN)
	@echo "$(GREEN)✓ Compiled $(BN) successfully$(RESET)"

clean:
	@$(RM) $(OBJS)
	@$(RM) $(B_OBJS)

	@echo "$(RED)✗ Removed object files$(RESET)"

fclean: clean
	@$(RM) $(NAME)
	@$(RM) $(BN)

	@echo "$(RED)✗ Removed $(RESET)"

re: fclean all

.PHONY: all clean fclean re
# **************************************************************************** #
#                                 Makefile                                     #
# **************************************************************************** #

NAME		= ircserv
CXX			= c++
CXXFLAGS	= -Wall -Wextra -Werror -std=c++98
RM			= rm -f

# Source files (add all your .cpp files here)
SRCS		= main.cpp \
			  Server.cpp \
			  Client.cpp \
			  Channel.cpp \

# Object files (replace .cpp with .o)
OBJS		= $(SRCS:.cpp=.o)

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

clean:
	@$(RM) $(OBJS)
	@echo "$(RED)✗ Removed object files$(RESET)"

fclean: clean
	@$(RM) $(NAME)
	@echo "$(RED)✗ Removed $(NAME)$(RESET)"

re: fclean all

.PHONY: all clean fclean re
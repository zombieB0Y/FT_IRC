NAME     = ircserv

CXX      = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

SRCDIR   = src
INCDIR   = include
OBJDIR   = obj

SRCS     = $(SRCDIR)/main.cpp       \
           $(SRCDIR)/IrcServer.cpp  \
           $(SRCDIR)/Commands.cpp   \
           $(SRCDIR)/Client.cpp     \
           $(SRCDIR)/Channel.cpp

OBJS     = $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRCS))

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^
	@echo "✓ $(NAME) built successfully"

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -c $< -o $@

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re

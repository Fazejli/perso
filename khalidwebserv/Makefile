CC = c++
CFLAGS = -std=c++98 -Wall -Werror -Wextra -MMD -MP
NAME = webserv

SRCS = main.cpp \
		
OBJ = $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME) : $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(NAME)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean :
	rm -f $(OBJ) $(OBJ:.o=.d)

fclean : clean
	rm -f $(NAME)

re: fclean all

-include $(OBJ:.o=.d)

.PHONY: all clean fclean re
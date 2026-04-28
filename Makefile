NAME        := webserv

CXX         := c++
CXXFLAGS    := -Wall -Wextra -Werror -std=c++98 -pedantic
DEPFLAGS    := -MMD -MP

SRC_DIR     := srcs
OBJ_DIR     := objs
INC_DIR     := includes

INCLUDES    := -I$(INC_DIR)

SRCS        := $(shell find $(SRC_DIR) -name '*.cpp')
OBJS        := $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
DEPS        := $(OBJS:.o=.d)

# Test binary — links against the project sources it needs (no main.o)
TEST_DIR        := tests
TEST_OBJ_DIR    := $(OBJ_DIR)/tests
TEST_BIN        := run_tests
TEST_SRCS       := $(TEST_DIR)/tests.cpp
LIB_SRCS        := $(SRC_DIR)/http/HttpRequest.cpp \
                   $(SRC_DIR)/config/ConfigParser.cpp \
                   $(SRC_DIR)/config/Config.cpp \
                   $(SRC_DIR)/config/ServerConfig.cpp \
                   $(SRC_DIR)/config/LocationConfig.cpp \
                   $(SRC_DIR)/utils/StringUtils.cpp \
                   $(SRC_DIR)/utils/Logger.cpp
TEST_ALL_SRCS   := $(TEST_SRCS) $(LIB_SRCS)
TEST_OBJS       := $(TEST_ALL_SRCS:%.cpp=$(TEST_OBJ_DIR)/%.o)
TEST_DEPS       := $(TEST_OBJS:.o=.d)

GREEN       := \033[0;32m
BLUE        := \033[0;34m
YELLOW      := \033[0;33m
RESET       := \033[0m

all: $(NAME)

$(NAME): $(OBJS)
	@printf "$(BLUE)Linking $(NAME)$(RESET)\n"
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $@
	@printf "$(GREEN)Built $(NAME)$(RESET)\n"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@printf "$(YELLOW)Compiling $<$(RESET)\n"
	@$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

$(TEST_OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@printf "$(YELLOW)Compiling $<$(RESET)\n"
	@$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

$(TEST_BIN): $(TEST_OBJS)
	@printf "$(BLUE)Linking $(TEST_BIN)$(RESET)\n"
	@$(CXX) $(CXXFLAGS) $(TEST_OBJS) -o $@
	@printf "$(GREEN)Built $(TEST_BIN)$(RESET)\n"

test: $(TEST_BIN)
	@./$(TEST_BIN)

clean:
	@printf "$(YELLOW)Cleaning object files$(RESET)\n"
	@rm -rf $(OBJ_DIR)

fclean: clean
	@printf "$(YELLOW)Removing binaries$(RESET)\n"
	@rm -f $(NAME) $(TEST_BIN)

re: fclean all

-include $(DEPS)
-include $(TEST_DEPS)

.PHONY: all clean fclean re test

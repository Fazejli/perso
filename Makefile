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

TEST_DIR        := tests
TEST_OBJ_DIR    := $(OBJ_DIR)/tests

TEST_PARSER_BIN  := test_parser
TEST_PARSER_SRCS := $(TEST_DIR)/test_parser.cpp \
                    $(SRC_DIR)/http/HttpRequest.cpp \
                    $(SRC_DIR)/utils/StringUtils.cpp
TEST_PARSER_OBJS := $(TEST_PARSER_SRCS:%.cpp=$(TEST_OBJ_DIR)/%.o)

TEST_INTERACTIVE_BIN  := test_interactive
TEST_INTERACTIVE_SRCS := $(TEST_DIR)/test_interactive.cpp \
                        $(SRC_DIR)/http/HttpRequest.cpp \
                        $(SRC_DIR)/utils/StringUtils.cpp
TEST_INTERACTIVE_OBJS := $(TEST_INTERACTIVE_SRCS:%.cpp=$(TEST_OBJ_DIR)/%.o)

TEST_DEPS   := $(TEST_PARSER_OBJS:.o=.d) $(TEST_INTERACTIVE_OBJS:.o=.d)

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

$(TEST_PARSER_BIN): $(TEST_PARSER_OBJS)
	@printf "$(BLUE)Linking $(TEST_PARSER_BIN)$(RESET)\n"
	@$(CXX) $(CXXFLAGS) $(TEST_PARSER_OBJS) -o $@
	@printf "$(GREEN)Built $(TEST_PARSER_BIN)$(RESET)\n"

$(TEST_INTERACTIVE_BIN): $(TEST_INTERACTIVE_OBJS)
	@printf "$(BLUE)Linking $(TEST_INTERACTIVE_BIN)$(RESET)\n"
	@$(CXX) $(CXXFLAGS) $(TEST_INTERACTIVE_OBJS) -o $@
	@printf "$(GREEN)Built $(TEST_INTERACTIVE_BIN)$(RESET)\n"

test: $(TEST_PARSER_BIN)
	@printf "$(BLUE)Running parser tests$(RESET)\n"
	@./$(TEST_PARSER_BIN)

interactive: $(TEST_INTERACTIVE_BIN)

test_configs: $(NAME)
	@for f in configs/valid/*.conf; do \
		./$(NAME) $$f || echo "FAIL: $$f"; \
	done
	@for f in configs/invalid/*.conf; do \
		./$(NAME) $$f 2>/dev/null && echo "UNEXPECTED PASS: $$f" || true; \
	done

clean:
	@printf "$(YELLOW)Cleaning object files$(RESET)\n"
	@rm -rf $(OBJ_DIR)

fclean: clean
	@printf "$(YELLOW)Removing $(NAME)$(RESET)\n"
	@rm -f $(NAME) $(TEST_PARSER_BIN) $(TEST_INTERACTIVE_BIN)

re: fclean all

-include $(DEPS)
-include $(TEST_DEPS)

.PHONY: all clean fclean re test interactive test_configs

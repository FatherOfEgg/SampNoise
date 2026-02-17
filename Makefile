TARGET_NAME := sampnoise

SRC_DIR := src
OBJS_DIR := objs
BIN_DIR := bin

LIB_ROOT := ./PhyloParse/
LIB := $(LIB_ROOT)/lib/libphyloparse.a
LIB_INC = $(LIB_ROOT)/include

CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Werror \
			-Wno-error=unused-function \
			-Wno-error=unused-parameter \
			-Wno-error=unused-but-set-variable \
			-Wno-error=unused-variable \
			-O2 \
			-g -MMD -MP
LDFLAGS := $(LIB)

INCLUDES := -I$(LIB_INC)

rwildcard = $(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

SRCS := $(call rwildcard,$(SRC_DIR),*.cpp)
OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(OBJS_DIR)/%.o)

DEPS := $(OBJS:.o=.d)

TARGET := $(BIN_DIR)/$(TARGET_NAME)

.PHONY: all clean test directories check-and-reinit-submodules

all: directories $(TARGET)

$(TARGET): $(OBJS) $(LIB)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(OBJS_DIR)/%.o: $(SRC_DIR)/%.cpp $(LIB)
	@mkdir -p $(dir $@)
	@echo "Compiling: $<"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

directories:
	@mkdir -p $(OBJS_DIR)
	@mkdir -p $(BIN_DIR)

$(LIB): check-and-reinit-submodules
	@if [ ! -f "$(LIB)" ]; then \
		echo "PhyloParse library not found or outdated. Building library..."; \
		$(MAKE) -C $(LIB_ROOT); \
	fi

# https://stackoverflow.com/a/52407662
check-and-reinit-submodules:
	@if git submodule status | egrep -q '^[-+]' ; then \
		echo "Need to reinitialize git submodules..."; \
		git submodule update --init --remote; \
		git add PhyloParse; \
		git commit -m "Update PhyloParse submodule"; \
	fi

clean:
	@echo "Cleaning up..."
	@rm -rf $(OBJS_DIR) $(BIN_DIR)

-include $(DEPS)

#----------src directory----------
SRC_DIR = $(abspath $(PWD)/../..)
UNITTEST_DIR = $(abspath $(PWD)/..)
MOCK_DIR = $(abspath $(PWD)/mock)

EXCLUDE_SRC = 	


EXCLUDE_UNITTEST = 	
					

INCLUDE_DIR = 	$(MOCK_DIR)/include		\
				$(SRC_DIR)/include				\
				$(UNITTEST_DIR)/include			\


#---executable file trigger source file-------------
TRIGGER_OBJS := $(patsubst %.c, %.o, $(wildcard *.c))
#----module source file----------------
EXCLUDE_SRC += $(notdir $(wildcard $(MOCK_DIR)/src/*.c))
SRC_OBJS := $(patsubst %.c, %.o, $(filter-out $(addprefix $(SRC_DIR)/src/, $(EXCLUDE_SRC)), $(wildcard $(SRC_DIR)/src/*.c)))
#----unit test source file--------------
UNITTEST_OBJS := $(patsubst %.c, %.o, $(filter-out $(addprefix $(UNITTEST_DIR)/, $(EXCLUDE_UNITTEST)), $(wildcard $(UNITTEST_DIR)/*.c)))
#----mock source file--------
MOCK_OBJS := $(patsubst %.c, %.o, $(wildcard $(MOCK_DIR)/src/*.c))
MOCK_OBJS += $(patsubst %.c, %.o, $(wildcard $(MOCK_DIR)/src/linux/*.c))
OBJS += $(MOCK_OBJS)
OBJS += $(TRIGGER_OBJS)
OBJS += $(SRC_OBJS)
OBJS += $(UNITTEST_OBJS)

#----------compile flags------------
CXX_INCLUDE_DIR = $(addprefix -I, $(INCLUDE_DIR))

EXECUTABLE = trigger
CXX = gcc
CXX_FLAGS = -Wall -Werror $(CXX_INCLUDE_DIR)
LD_FLAGS = -lpthread

EXTRA_FLAGS = -g -DDEBUG
EXTRA_LDFLAGS = 

#-----------compile implement---------
COMPILE_FLAGS = $(CXX_FLAGS) $(EXTRA_FLAGS)
LINK_FLAGS = $(LD_FLAGS) $(EXTRA_LDFLAGS)

all: $(OBJS) $(EXECUTABLE)

$(EXECUTABLE): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LINK_FLAGS)
# link flags一定要放到最后去

%.o:%.c
	$(CXX) -c $< -o $@ $(COMPILE_FLAGS)

.PHONY: clean
clean:
	rm -f $(OBJS)

.PHONY: debug
debug:
	gdb $(EXECUTABLE)

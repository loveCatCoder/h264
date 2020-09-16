#定义编译工具
CC := gcc
CXX := g++

Target  := test
#引入所有头文件
INC  := -I.

#定义编译模式
MODE = debug

#引入所有lib文件
#LIBS := -lprotobuf -lpthread

#得到所有obj文件
SRCS := $(wildcard *.cpp)

OBJS := $(SRCS:%.cpp=%.o)

#自定义CXXFLAGS
CXXFLAGS := $(INC)
ifeq ($(MODE),debug)
	CXXFLAGS += -g -DDEBUG -std=c++11 
	CFLAGS += -g -DDEBUG 
else
	CXXFLAGS += -Os -Wall -fmessage-length=0 -std=c++11 
	CFLAGS += -Os -Wall -fmessage-length=0 
endif

#定义目标
all:
	make depend
	make exe

#生成依赖
depend:
	$(CC) $(CXXFLAGS) $(SRCS) -MM >MAKEFILE.DEPEND
-include MAKEFILE.DEPEND

#生成主程序
exe:$(Target)
$(Target):$(OBJS)
	$(CXX) $(CXXFLAGS)   -o $@ $^  $(LIBS)
	
.PHONY :clean
clean:
	rm -f *.o test MAKEFILE.DEPEND
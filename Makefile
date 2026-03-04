CXX = clang++
CXXFLAGS = -std=c++17 -Wall

PG_CONFIG := $(shell command -v pg_config 2>/dev/null)
INCLUDES =
LIBS = -lpq

ifdef PG_CONFIG
INCLUDES += -I$(shell $(PG_CONFIG) --includedir)
LIBS += -L$(shell $(PG_CONFIG) --libdir)
else
INCLUDES += -I/opt/homebrew/opt/libpq/include -I/usr/local/opt/libpq/include
LIBS += -L/opt/homebrew/opt/libpq/lib -L/usr/local/opt/libpq/lib
endif

TARGET = server
SRCS = main.cpp TcpServer.cpp  PostgresManager.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

.cpp.o:
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	rm -f *.o $(TARGET)

run: $(TARGET)
	./$(TARGET)

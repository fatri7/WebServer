CXX := g++
CXXFLAGS := -std=c++20 -O2 -Wall -g -Iinclude
LDFLAGS := -pthread -L/usr/local/lib
LIBS := -lboost_coroutine -lboost_context

TARGET := tiny_web_server_2025
SRCDIR := src
BINDIR := bin
OBJDIR := obj

SOURCES := $(wildcard $(SRCDIR)/*.cpp)
OBJECTS := $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SOURCES))

$(shell mkdir -p $(BINDIR) $(OBJDIR))

$(BINDIR)/$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS) $(LDFLAGS) $(LIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(OBJDIR) $(BINDIR)/$(TARGET)

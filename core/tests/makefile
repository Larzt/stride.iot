CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++2b -I../plugin/lexer/include -Iinclude -g

TARGET = run_tests

SRCS_LANG = $(wildcard ../plugin/lexer/*.cc)
SRCS_TESTS = $(wildcard *.cc)
SRCS = $(SRCS_LANG) $(SRCS_TESTS)

OBJS = $(SRCS:.cc=.o)

all: $(TARGET)
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "==================================================="
	@echo "¡Compilación exitosa! Ejecuta los tests con: ./$(TARGET)"
	@echo "==================================================="

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
	@echo "Archivos limpios."

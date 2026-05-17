CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread
GTKFLAGS = `pkg-config --cflags --libs gtk+-3.0`

all: security_checker test


security_checker: main.o checker.o
	$(CXX) -o security_checker main.o checker.o $(GTKFLAGS) $(CXXFLAGS)


test: test.o checker.o
	$(CXX) -o test test.o checker.o $(CXXFLAGS)

main.o: main.cpp checker.h
	$(CXX) -c main.cpp $(GTKFLAGS) $(CXXFLAGS)

checker.o: checker.cpp checker.h
	$(CXX) -c checker.cpp $(CXXFLAGS)

test.o: test.cpp checker.h
	$(CXX) -c test.cpp $(CXXFLAGS)

clean:
	rm -f *.o security_checker test

run: security_checker
	./security_checker

run-tests: test
	./test

.PHONY: all clean run run-tests
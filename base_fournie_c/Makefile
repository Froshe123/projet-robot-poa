CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g

BASE = json.o reconfort_io.o

ROBOT = types.o carte_robot.o graphe.o dictionnaire.o roue.o choix_casier.o environnement.o memoire.o agent.o validation.o

all: robot tests

robot: $(BASE) $(ROBOT) main.o
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp $(wildcard *.hpp)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f robot tests *.o

tests: types.o carte_robot.o graphe.o dictionnaire.o roue.o choix_casier.o memoire.o reconfort_io.o json.o tests.o environnement.o validation.o agent.o
	$(CXX) $(CXXFLAGS) -o $@ $^

test: tests
	./tests

.PHONY: all clean test

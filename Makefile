# Person 6 - Observer + Strategy.  Language: C++11
CXX      = g++
CXXFLAGS = -std=c++11 -pedantic -Wall -Wextra -g -Iobserver -Istrategy -Istubs -Itests
LIBSRC   = $(wildcard observer/*.cpp) $(wildcard strategy/*.cpp)
TESTSRC  = $(wildcard tests/*.cpp)
LIBOBJ   = $(LIBSRC:.cpp=.o)
TESTOBJ  = $(TESTSRC:.cpp=.o)

test: test_person6
	./test_person6

test_person6: $(LIBOBJ) $(TESTOBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

docs:
	doxygen Doxyfile

clean:
	rm -f $(LIBOBJ) $(TESTOBJ) test_person6

.PHONY: test docs clean
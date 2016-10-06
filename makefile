CPPFLAGS += -std=c++11

app : rn2483.o comm.o main.o
	$(CXX) -o app $(CPPFLAGS) $(CXXFLAGS) rn2483.o comm.o main.o
rn2483.o : rn2483.cpp rn2483.h
comm.o : comm.cpp comm.h
main.o : main.cpp

.PHONY : clean
clean :
	rm app *.o .*.swp

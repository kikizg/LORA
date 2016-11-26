CPPFLAGS += -std=c++11 -lboost_program_options -lcrypto

app : rn2483.o comm.o main.o clock.o uart.o
	$(CXX) -o app $(CPPFLAGS) $(CXXFLAGS) rn2483.o comm.o main.o clock.o uart.o
rn2483.o : rn2483.cpp rn2483.h
uart.o : uart.cpp uart.h
comm.o : comm.cpp comm.h
main.o : main.cpp
clock.o : clock.cpp clock.h

.PHONY : clean
clean :
	rm app *.o

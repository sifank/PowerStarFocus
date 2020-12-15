CFLAGS = -O2 -Wall
CC = g++ 

stdtest: marks.o student.o main.o
	$(CC) $(CFLAGS) -o stdtest marks.o student.o main.o

main.o: main.cpp         
	$(CC) $(CFLAGS) -c main.cpp 

marks.o: marks.cpp         
	$(CC) $(CFLAGS) -c marks.cpp 

student.o: student.cpp         
	$(CC) $(CFLAGS) -c student.cpp
	
clean:         
	@rm -f stdtest *.o

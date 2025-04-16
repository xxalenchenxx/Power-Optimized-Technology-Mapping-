TARGET  = ace
CC      = g++
ABC     = ./abc90

SRCS    = main.cpp
OBJS    = ${SRCS:.cpp=.o}
LIB     = ${ABC}/libabc.a -lm -ldl -lrt -rdynamic -lreadline -ltermcap -lpthread
INCLUDE = -I. -I${ABC}/src
CFLAGW  = -Wall -O3
CFLAGS  = -O3

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIB) $(LIBS) -o $(TARGET)

.SUFFIXES: .cpp .o

.cpp.o:
	$(CC) $(CFLAGW) $(INCLUDE) -o $@ -c $<

clean:
	rm -f core *~ $(TARGET); \
	rm *.o

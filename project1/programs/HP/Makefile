# apo domes dedomenwn-xatzikokolakhs tropopoihmeno katallhla

# paths
LIB = ../../lib
INCLUDE = ../../include
MODULES = ../../modules

# compiler
CC = gcc

# Compile options. Το -I<dir> λέει στον compiler να αναζητήσει εκεί include files
CFLAGS = -Wall -g -I$(INCLUDE)
LDFLAGS = -lm -no-pie

# Αρχεία .o
OBJS = main_HP.o $(MODULES)/hp.o

# Αρχεία .a
LIBS = $(LIB)/BF_64.a

# Το εκτελέσιμο πρόγραμμα
EXEC = main_HP


$(EXEC): $(OBJS) $(LIBS)
	$(CC) $(OBJS) $(LIBS) -o $(EXEC) $(LDFLAGS)

# Για να φτιάξουμε το BF_64.a, τρέχουμε το make στο lib directory.
$(LIB)/BF_64.a:
	$(MAKE) -C $(LIB) BF_64.a

clean:
	rm -f $(OBJS) $(EXEC) hp_file

run: $(EXEC)
	./$(EXEC) $(ARGS)

valgrind: $(EXEC)
	valgrind ./$(EXEC) $(ARGS)
all: nsh-sf-reflector

CC = gcc
INCLUDE_DIR = inc
CFLAGS = -I$(INCLUDE_DIR)


LIBS = -lm -lpthread
OBJ_DIR = obj

REFLECTOR_OBJ = nsh-sf-reflector.o cli.o transport.o
OBJS = $(patsubst %,$(OBJ_DIR)/%,$(REFLECTOR_OBJ))

$(OBJ_DIR)/%.o: src/%.c $(DEPS)
	@mkdir -p $(@D)
	$(CC) -Wall -g -c -o $@ $< $(CFLAGS)

nsh-sf-reflector: $(OBJS)
	$(CC) -Wall -g -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(OBJ_DIR)/*.o *~ core nsh-sf-reflector
	rm -r $(OBJ_DIR)/

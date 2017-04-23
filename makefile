IDIR=include
ODIR=obj
SDIR=src
CC=gcc
CFLAGS=-I $(IDIR) -g

_DEPS=disco.h menu.h
DEPS=$(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ=my_drive.o menu.o disco.o
OBJ=$(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

install: $(OBJ)
	$(CC) -o main.bin $^ $(CFLAGS) $(LIBS)

clean:
	rm -f $(ODIR)/*.o main.bin

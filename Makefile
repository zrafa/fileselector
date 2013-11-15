INCS = `sdl-config --cflags`
LIBS = `sdl-config --libs`

all: fileselector fileselector-bg

fileselector: fileselector.o
	$(CC) $(LDFLAGS) $(LIBS) -lSDL_ttf -o fileselector fileselector.o

fileselector.o: fileselector.c
	$(CC) $(CFLAGS) $(INCS) -c fileselector.c

fileselector-bg: fileselector-bg.o
	$(CC) $(LDFLAGS) -lX11 -o fileselector-bg fileselector-bg.o

fileselector-bg.o: fileselector-bg.c
	$(CC) $(CFLAGS) -c fileselector-bg.c

install:
	install -d $(DESTDIR)/usr/bin
	install -m 0755 fileselector $(DESTDIR)/usr/bin
	install -m 0755 fileselector-bg $(DESTDIR)/usr/bin

clean:
	rm -f fileselector fileselector.o fileselector-bg fileselector-bg.o

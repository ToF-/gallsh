gallsh:
	gcc `pkg-config --cflags --libs gtk4` src/gallsh.c `pkg-config --libs --cflags gtk4` -o bin/gallsh

clean:
	rm bin/gallsh

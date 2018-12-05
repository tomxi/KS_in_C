gcc -Wall -o synth synth.c support.c -I/usr/local/include \
	-L/usr/local/lib -lsndfile -lportaudio -lncurses

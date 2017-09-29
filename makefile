PROG = frogger

all: $(PROG)

$(PROG):
	emcc -s TOTAL_MEMORY=128000000 frogger.c -o frogger.html --preload-file img/ --preload-file sound/ --preload-file level/ --preload-file imgg/

clean:
	rm -r frogger.html frogger.js
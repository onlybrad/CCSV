.PHONY        := cleanup
CC            := gcc
CC++          := g++
STD           := -std=c99
STD++         := -std=c++11
INCLUDES      :=
FLAGS         := -Wall -Wextra -Wpedantic -Wconversion -Wstrict-overflow=5 -Wshadow -Wunused-macros -Wbad-function-cast -Wcast-qual -Wcast-align -Wwrite-strings -Wdangling-else -Wlogical-op -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes -Winline
FLAGS++       := -Wall -Wextra -Wpedantic -Wconversion -Wstrict-overflow=5 -Wshadow -Wunused-macros -Wcast-qual -Wcast-align -Wwrite-strings -Wdangling-else -Wlogical-op -Winline
SOURCE        := *.c

static: $(SOURCE)
	$(CC) $(INCLUDES) $(FLAGS) $(STD) -D NDEBUG -O3 -c $(SOURCE)
	ar rcs ccsv.a *.o
	make cleanup

static++: $(SOURCE)
	$(CC++) $(INCLUDES) $(FLAGS++) $(STD++) -D NDEBUG -O3 -c $(SOURCE)
	ar rcs ccsv++.a *.o
	make cleanup

debug_static: $(SOURCE)
	$(CC) $(INCLUDES) $(FLAGS) $(STD) -g -O0 -c $(SOURCE)
	ar rcs ccsvd.a *.o
	make cleanup

all: static static++ test1.exe test1++.exe

test1.exe: ./tests/test1.c $(SOURCE)
	$(CC) $(INCLUDES) $(FLAGS) $(STD) -g -O0 -o test1 ./tests/test1.c $(SOURCE)

test1++.exe: ./tests/test1.c $(SOURCE)
	$(CC++) $(INCLUDES) $(FLAGS++) $(STD++) -g -O0 -o test1++ ./tests/test1.c $(SOURCE)

cleanup:
	rm *.o
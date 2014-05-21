pa4: main.c la4.h ipc.c list.c list.h clean
	gcc -std=c99 -Wall -pedantic *.c -o pa4 -L. -lruntime

tar: main.c la4.h ipc.c list.c list.h clean
	mkdir pa4
	cp main.c la4.h ipc.c list.c list.h pa4
	tar -czvf pa4.tar.gz pa4
	rm -rf pa4


clean:
	-rm -rf *.o *.log pa4 pa4.tar.gz

lib:
	export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:~/pa4/DistributedLab4" LD_PRELOAD=~/pa4/DistributedLab4/libruntime.so

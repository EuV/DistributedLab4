#include "pa2345.h"
#include "list.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>

typedef enum { false, true } bool;

enum {
	BUF_SIZE = 100,
	NUMBER_OF_PROCESS = 2
};

enum {
	READ = 0,
	WRITE = 1
};

enum {
	IPC_SUCCESS = 0,
	IPC_FAILURE = -1,
	IPC_PIPE_IS_EMPTY = 1,
	IPC_PIPE_IS_CLOSED = 2
};

typedef struct {
	int total;
	local_id localId;
	bool isMutex;
	local_id msgAuthor;
	int started;
	int replied;
	int done;
	TList* list;
} Process;

void childProcess( Process* );
void parentProcess( Process* const );

void getNumberOfProcessAndMutex( int argc, char* const argv[], int*, bool* );

void createFullyConnectedTopology( const int );
void closeUnusedPipes( const Process* const );
void closeTheOtherPipes( const Process* const );

void defaultCSExtendedCycle( Process* );

void makeChildren( const int, const bool );
void waitForChildren();

void fillMessage( Message*, const Process* const, const MessageType );

void receiveAll( Process* const, const MessageType, const int );

void makePipeLog( const int );
void makeIPCLog( const char* const );

int Pipes[ MAX_PROCESS_ID + 1 ][ MAX_PROCESS_ID + 1][ 2 ];
int EventsLog;
int PipesLog;
char LogBuf[ BUF_SIZE ];

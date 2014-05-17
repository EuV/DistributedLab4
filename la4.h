#include "banking.h"
#include "pa2345.h"

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

enum {
	BUF_SIZE = 100
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
    balance_t balance;
    BalanceHistory history;
} Process;

typedef enum { false, true } bool;

void accountService( Process* const );
void customerService( Process* const );

void fastForwardHistory( Process * const, const int );

bool getBranchesInitialBalance( const int, char** const, int*, balance_t* );

void createFullyConnectedTopology( const int );
void closeUnusedPipes( const Process* const );
void closeTheOtherPipes( const Process* const );

void createBranches( const int, const balance_t* const );
void waitForBranches();

void fillMessage( Message*, const Process* const, const MessageType );

void makePipeLog( const int );
void makeIPCLog( const char* const );

int Pipes[ MAX_PROCESS_ID + 1 ][ MAX_PROCESS_ID + 1][ 2 ];
int EventsLog;
int PipesLog;
char LogBuf[ BUF_SIZE ];
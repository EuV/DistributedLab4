#include "la4.h"

timestamp_t lamportTime = 0;


timestamp_t get_lamport_time() {
	return lamportTime;
}


void incLamportTime() {
	lamportTime++;
}


void setMaxLamportTime( timestamp_t msgTime ) {
	if( lamportTime < msgTime ) {
		lamportTime = msgTime;
	}
}


int main( int argc, char* argv[] ) {

	EventsLog = open( evengs_log, O_CREAT | O_TRUNC | O_WRONLY | O_APPEND, 0777 );
	PipesLog = open( pipes_log, O_CREAT | O_TRUNC | O_WRONLY | O_APPEND, 0777 );

	int procTotal;
	balance_t balance[ MAX_PROCESS_ID ];
	if( !getBranchesInitialBalance( argc, argv, &procTotal, balance ) ) {
		return 1;
	}

	createFullyConnectedTopology( procTotal );

	makePipeLog( procTotal );

	createBranches( procTotal, balance );

	return 0;
}


void accountService( Process * const proc ) {

	closeUnusedPipes( proc );

	incLamportTime();

	// STARTED
	Message startedMsg;
	fillMessage( &startedMsg, proc, STARTED );
	makeIPCLog( startedMsg.s_payload );
	send_multicast( proc, &startedMsg );

	// Receive all the messages
	int done = 0;
	while( done != proc -> total ) {

		Message msg;
		receive_any( proc, &msg );

		setMaxLamportTime( msg.s_header.s_local_time );
		incLamportTime();

		switch( msg.s_header.s_type ) {
			case STARTED: {
				static int started = 1;
				started++;
				if( started == proc -> total ) {
					sprintf( LogBuf, log_received_all_started_fmt, get_lamport_time(), proc -> localId );
					makeIPCLog( LogBuf );
				}
				break;
			}
			case TRANSFER: {

				fastForwardHistory( proc, get_lamport_time() );

				incLamportTime();

				TransferOrder order;
				memcpy( &order, msg.s_payload, msg.s_header.s_payload_len );

				// From (this) account
				if( order.s_src == proc -> localId ) {
					sprintf( LogBuf, log_transfer_out_fmt, get_lamport_time(), proc -> localId, order.s_amount, order.s_dst );
					proc -> balance -= order.s_amount;
					msg.s_header.s_local_time = get_lamport_time();
					send( proc, order.s_dst, &msg );
				}

				// To (this) account
				else {
					sprintf( LogBuf, log_transfer_in_fmt, get_lamport_time(), proc -> localId, order.s_amount, order.s_src );
					proc -> balance += order.s_amount;

					// Pending
					timestamp_t pendingFromTime = msg.s_header.s_local_time;
					while( pendingFromTime < get_lamport_time() ) {
						proc -> history.s_history[ pendingFromTime++ ].s_balance_pending_in = order.s_amount;
					}

					// Acknowledgement to the customer-process
					Message ackMsg;
					fillMessage( &ackMsg, proc, ACK );
					send( proc, PARENT_ID, &ackMsg );
				}

				makeIPCLog( LogBuf );

				BalanceState state = { proc -> balance, get_lamport_time(), 0 };
				proc -> history.s_history[ proc -> history.s_history_len++ ] = state;
				break;
			}
			case STOP: {
				done++; // This one

				incLamportTime();

				Message doneMsg;
				fillMessage( &doneMsg, proc, DONE );
				makeIPCLog( doneMsg.s_payload );
				send_multicast( proc, &doneMsg );

				fastForwardHistory( proc, get_lamport_time() );
				break;
			}
			case DONE: {
				done++; // The others
				break;
			}
			default: {
				fprintf( stderr, "Unexpected message of type: %d\n", msg.s_header.s_type );
				break;
			}
		}
	}

	sprintf( LogBuf, log_received_all_done_fmt, get_lamport_time(), proc -> localId );
	makeIPCLog( LogBuf );

	incLamportTime();

	Message historyMsg;
	historyMsg.s_header.s_payload_len = 2 + sizeof( BalanceState ) * proc -> history.s_history_len;
	memcpy( historyMsg.s_payload, &( proc -> history ), historyMsg.s_header.s_payload_len );
	historyMsg.s_header.s_type = BALANCE_HISTORY;
	historyMsg.s_header.s_magic = MESSAGE_MAGIC;
	historyMsg.s_header.s_local_time = get_lamport_time();
	send( proc, PARENT_ID, &historyMsg );

	closeTheOtherPipes( proc );
}


void fastForwardHistory( Process * const proc, const int newPresent ) {
	if( proc -> history.s_history_len > newPresent ) return;
	while( proc -> history.s_history_len <= newPresent ) {
		BalanceState state = { proc -> balance, proc -> history.s_history_len, 0 };
		proc -> history.s_history[ proc -> history.s_history_len++ ] = state;
	}
}


void customerService( Process * const proc ) {

	closeUnusedPipes( proc );

	AllHistory allHistory = { 0 };

	while( allHistory.s_history_len != proc -> total ) {

		Message msg;
		receive_any( proc, &msg );

		setMaxLamportTime( msg.s_header.s_local_time );
		incLamportTime();

		switch( msg.s_header.s_type ) {
			case STARTED: {
				static int started = 0;
				started++;
				if( started == proc -> total ) {
					sprintf( LogBuf, log_received_all_started_fmt, get_lamport_time(), proc -> localId );
					makeIPCLog( LogBuf );

					bank_robbery( proc, proc -> total );

					incLamportTime();

					Message stopMsg;
					fillMessage( &stopMsg, proc, STOP );
					send_multicast( proc, &stopMsg );
				}
				break;
			}
			case DONE: {
				static int done = 0;
				done++;
				if( done == proc -> total ) {
					sprintf( LogBuf, log_received_all_done_fmt, get_lamport_time(), proc -> localId );
					makeIPCLog( LogBuf );
				}
				break;
			}
			case BALANCE_HISTORY: {
				BalanceHistory history;
				memcpy( &history, msg.s_payload, msg.s_header.s_payload_len );
				allHistory.s_history[ allHistory.s_history_len++ ] = history;
				break;
			}
			default: {
				fprintf( stderr, "Unexpected message of type: %d\n", msg.s_header.s_type );
				break;
			}
		}
	}

	//extendHistoryTillEnd( &allHistory );
	print_history( &allHistory );

	waitForBranches();

	closeTheOtherPipes( proc );
}


void transfer( void* parent_data, local_id src, local_id dst,  balance_t amount ) {

	incLamportTime();

	TransferOrder order = { src, dst, amount };
	Message transferMsg;
	memcpy( transferMsg.s_payload, &order, sizeof( TransferOrder ) );
	fillMessage( &transferMsg, parent_data, TRANSFER );
	send( parent_data, src, &transferMsg );

	Message msg;
	while( receive( parent_data, dst, &msg ) == IPC_PIPE_IS_EMPTY ) {}

	setMaxLamportTime( msg.s_header.s_local_time );
	incLamportTime();

	if( msg.s_header.s_type != ACK ) {
		fprintf( stderr, "ACK is missing! Received %d instead of %d\n", msg.s_header.s_type, ACK );
		exit( 1 );
	}
}


bool getBranchesInitialBalance( const int argc, char** const argv, int* procTotal, balance_t* balance ) {

	if( argc < 5 ) return false;

	*procTotal = atoi( argv[ 2 ] );
	if( *procTotal < 2 || MAX_PROCESS_ID < *procTotal || argc - 3 < *procTotal ) return false;

	for( int i = 0; i < *procTotal; i++ ) {
		balance[ i ] = atoi( argv[ 3 + i ] );
	}

	return true;
}


void createFullyConnectedTopology( const int procTotal ) {
	for ( int row = 0; row <= procTotal; row++ ) {
		for ( int col = 0; col <= procTotal; col++ ) {
			if ( row == col ) continue;
			pipe( Pipes[ row ][ col ] );
			fcntl( Pipes[ row ][ col ][ READ ], F_SETFL, O_NONBLOCK );
			fcntl( Pipes[ row ][ col ][ WRITE ], F_SETFL, O_NONBLOCK );
		}
	}
}


void closeUnusedPipes( const Process * const proc ) {
	for ( int row = PARENT_ID; row <= proc -> total; row++ ) {
		for ( int col = PARENT_ID; col <= proc -> total; col++ ) {
			if ( row == col ) continue;
			if ( row == proc -> localId ) {
				close( Pipes[ row ][ col ][ READ ] );
			} else {
				close( Pipes[ row ][ col ][ WRITE ] );
				if ( col != proc -> localId ) {
					close( Pipes[ row ][ col ][ READ ] );
				}
			}
		}
	}
}


void closeTheOtherPipes( const Process * const proc ) {
	for ( int rowOrCol = PARENT_ID; rowOrCol <= proc -> total; rowOrCol++ ) {
		if ( rowOrCol == proc -> localId ) continue;
		close( Pipes[ proc -> localId ][ rowOrCol ][ WRITE ] );
		close( Pipes[ rowOrCol ][ proc -> localId ][ READ ] );
	}
}


void createBranches( const int procTotal, const balance_t* const balance ) {
	for ( int i = 1; i <= procTotal; i++ ) {
		Process process = { procTotal, PARENT_ID, balance[ i - 1 ], { i, 0 } };
		if ( fork() == 0 ) {
			process.localId = i;
			accountService( &process );
			break; // To avoid fork() in a child
		} else if ( i == procTotal ) { // The last child has been created
			customerService( &process );
		}
	}
}


void waitForBranches() {
	int status;
	pid_t pid;
	while ( ( pid = wait( &status ) ) != -1 ) {
		// printf( "Process %d has been done with exit code %d\n", pid, status );
		if( WIFSIGNALED( status ) ) fprintf( stderr, "!!! Interrupted by signal %d !!!\n", WTERMSIG( status ) );
	}
}


void fillMessage( Message * msg, const Process * const proc, const MessageType msgType ) {
	switch( msgType ) {
		case STARTED:
			sprintf( msg -> s_payload, log_started_fmt, get_lamport_time(), proc -> localId, getpid(), getppid(), proc -> balance );
			break;
		case ACK:
		case STOP:
		case TRANSFER:
			break;
		case DONE:
			sprintf( msg -> s_payload, log_done_fmt, get_lamport_time(), proc -> localId, proc -> balance );
			break;
		default:
			sprintf( msg -> s_payload, "Unsupported type of message\n" );
			break;
	}
	msg -> s_header.s_type = msgType;
	msg -> s_header.s_magic = MESSAGE_MAGIC;
	msg -> s_header.s_local_time = get_lamport_time();
	msg -> s_header.s_payload_len = strlen( msg -> s_payload );
}


void makePipeLog( const int procTotal ) {
	char buf[ 10 ];
	for ( int row = 0; row <= procTotal; row++ ) {
		for ( int col = 0; col <= procTotal; col++ ) {
			sprintf( buf, "| %3d %3d ", Pipes[ row ][ col ][ READ ], Pipes[ row ][ col ][ WRITE ] );
			write( PipesLog, buf, strlen( buf ) );
		}
		write( PipesLog, "|\n", 2 );
	}
}


void makeIPCLog( const char * const buf ) {
	write( STDOUT_FILENO, buf, strlen( buf ) );
	write( EventsLog, buf, strlen( buf ) );
}

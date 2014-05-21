#include "la4.h"

int send( void* self, local_id dst, const Message* msg) {

	Process* proc = self;

	ssize_t wasWritten = write( Pipes[ proc -> localId ][ dst ][ WRITE ], msg, sizeof( MessageHeader ) +
		msg -> s_header.s_payload_len );

	// printf( "Send %d bytes from %d to %d\n", wasWritten, proc -> localId, dst );

	return ( wasWritten > 0 ) ? IPC_SUCCESS : IPC_FAILURE;
}


int send_multicast( void* self, const Message* msg ) {

	Process* proc = self;

	int status = IPC_SUCCESS;

	for ( local_id dst = PARENT_ID; dst <= proc -> total; dst++ ) {
		if ( dst == proc -> localId ) continue;
		status = send( self, dst, msg );
		if ( status == IPC_FAILURE ) break;
	}

	return status;
}


int receive( void* self, local_id from, Message* msg ) {

	Process* proc = self;

	// Read the header of the message (default size)
	ssize_t wasRead = read( Pipes[ from ][ proc -> localId ][ READ ], &( msg -> s_header ), sizeof( MessageHeader ) );

	if( wasRead == -1 ) {
		if( errno == EAGAIN ) {
			return IPC_PIPE_IS_EMPTY;
		} else {
			return IPC_FAILURE;
		}
	} else if( wasRead == 0 ) {
		return IPC_PIPE_IS_CLOSED;
	}

	// Read the rest part of the message which size has been known from the header
	wasRead += read( Pipes[ from ][ proc -> localId ][ READ ], &( msg -> s_payload ), msg -> s_header.s_payload_len );

	//printf( "Receive %d bytes by %d proc from %d proc\n", wasRead, proc -> localId, from );
	return IPC_SUCCESS;
}


int receive_any( void* self, Message* msg ) {

	Process* proc = self;
	static local_id sender = PARENT_ID;
	int status;

	do {
		if( sender == proc -> localId ) sender++;

		// printf( "receive_any by %d from %d\n", proc -> localId, sender );
		status = receive( self, sender, msg );
		proc -> msgAuthor = sender++;

		if( sender == proc -> localId ) sender++;
		if( sender > proc -> total ) sender = PARENT_ID;

	} while ( status == IPC_PIPE_IS_EMPTY || status == IPC_PIPE_IS_CLOSED );

	return status;
}

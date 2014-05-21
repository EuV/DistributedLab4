#include <stdio.h>
#include <stdlib.h>
#include "list.h"

TList* insert( TList* list, Request request ) {
	TNode* node = ( TNode* ) malloc( sizeof( TNode ) );
	node -> request = request;
	node -> next = NULL;

	if( list -> head == NULL ) {
		list -> head = list -> tail = node;
	} else {

		TNode* current = list -> head;

		// Insert before the head
		if( request.lamportTime < current -> request.lamportTime ||
			( request.lamportTime == current -> request.lamportTime &&
				request.localId < current -> request.localId ) ) {

			node -> next = list -> head;
			list -> head = node;
		}
		else {

			// Skip requests with less lamportTime value
			while( current -> next && current -> next -> request.lamportTime < request.lamportTime ) {
				current = current -> next;
			}

			// Skip requests with equal lamportTime value but less localId value
			while( current -> next &&
				current -> next -> request.lamportTime == request.lamportTime &&
				current -> next -> request.localId < request.localId ) {
				current = current -> next;
			}

			// Insert after the last skipped request
			node -> next = current -> next;
			current -> next = node;
			if( current == list -> tail ) {
				list -> tail = node;
			}
		}
	}

	return list;
}


Request pop( TList* list ) {
	TNode* node = list -> head;
	list -> head = node -> next;
	if( list -> head == NULL ) {
		list -> tail = NULL;
	}
	Request request = node -> request;
	free( node );
	return request;
}


int isFirst( const TList* list, local_id id ) {
	return ( list -> head -> request.localId == id );
}

#include "ipc.h"

typedef struct {
	timestamp_t lamportTime;
	local_id localId;
} Request;

typedef struct node_t {
	Request request;
	struct node_t* next;
} TNode;

typedef struct {
	TNode* head;
	TNode* tail;
} TList;


TList* insert( TList*, Request );

Request pop( TList* );

int isFirst( const TList*, local_id );

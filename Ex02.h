
#pragma once 
#include <pthread.h>

typedef struct {
    int         	height;        // Incremental ID of the block in the chain
    int         	timestamp;    // Time of the mine in seconds since epoch
    unsigned int    hash;        // Current block hash value
    unsigned int    prev_hash;    // Hash value of the previous block
    int        	difficulty;    // Amount of preceding zeros in the hash
    int         	nonce;        // Incremental integer to change the hash value
    int         	relayed_by;    // Miner ID
} BLOCK_T;

void* Mine(void* minerID); 
void* Server(void* arg); 
unsigned int calculateChecksum(BLOCK_T& TempBlock); 
bool meetsDifficulty(const unsigned int hash, const int difficulty);
int calculateInitialDifficulty(const int & hash);
void CopyBlock(BLOCK_T& destination,BLOCK_T& Source); 
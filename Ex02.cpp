#include <iostream>
#include <pthread.h>
#include <string>
#include "Ex02.h"
#include <zlib.h>
#include <cstring>
#include <list> 
#include <vector> 

using namespace std;

list<BLOCK_T> Block_Chain;
vector<BLOCK_T> communicationBlocks(4); // Communication between the threads and the server, one for each thread
pthread_mutex_t mutex;
pthread_cond_t BlockFound;
pthread_cond_t NewBlock;

int main() 
{
    pthread_t minorThread1, minorThread2, minorThread3, minorThread4, serverThread;
    int minerID1=1,minerID2=2,minerID3=3,minerID4=4;
    
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&BlockFound, NULL);
    pthread_cond_init(&NewBlock, NULL);

    pthread_create(&serverThread, NULL, Server, NULL);

    pthread_create(&minorThread1, NULL, Mine, &minerID1);
    pthread_create(&minorThread2, NULL, Mine, &minerID2);
    pthread_create(&minorThread3, NULL, Mine, &minerID3);
    pthread_create(&minorThread4, NULL, Mine, &minerID4);

    pthread_join(serverThread, NULL);
    pthread_join(minorThread1, NULL);
    pthread_join(minorThread2, NULL);
    pthread_join(minorThread3, NULL);
    pthread_join(minorThread4, NULL);

    return 0;  
}

void *Server(void* arg)
{
    // 1. Generate an initial genesis block
    BLOCK_T genesisBlock;
    genesisBlock.height = 0;
    genesisBlock.hash = calculateChecksum(genesisBlock); 
    genesisBlock.difficulty = calculateInitialDifficulty(genesisBlock.hash);
    Block_Chain.push_front(genesisBlock);

    // 2. Notify the miners they have a new block
    pthread_cond_signal(&NewBlock);

    while (true)
    {
        pthread_mutex_lock(&mutex);

        // 3. Wait for notification from one of the miners about a successfully mined block
        pthread_cond_wait(&BlockFound,&mutex); 

        bool blockAdded = false;
        for (int i = 0; i < 4; i++)
        {
            // 4. Verify its proof-of-work
            unsigned int hash=calculateChecksum(communicationBlocks[i]);
            if ((meetsDifficulty(hash,communicationBlocks[i].difficulty)))
            {
                // 5. If proved, append this block to the head of the chain
                communicationBlocks[i].height = Block_Chain.front().height + 1;
                Block_Chain.front().prev_hash = communicationBlocks[i].hash;

                Block_Chain.push_front(communicationBlocks[i]);

                cout << "Miner #" << communicationBlocks[i].relayed_by
                     << ": Mined a new block #" << communicationBlocks[i].height
                     << " with the hash 0x" << hex << communicationBlocks[i].hash << dec << endl;
                cout << "Server: New Block added by Miner #" << communicationBlocks[i].relayed_by
                     << ", attributes: height(" << communicationBlocks[i].height << "),"
                     << " timestamp(" << communicationBlocks[i].timestamp << "),"
                     << " hash(0x" << hex << communicationBlocks[i].hash << dec << "),"
                     << " prev_hash(0x" << hex << communicationBlocks[i].prev_hash << dec << "),"
                     << " difficulty(" << communicationBlocks[i].difficulty << "),"
                     << " nonce(" << communicationBlocks[i].nonce << ")" << endl;

                // Notify all miners there is a new block
                pthread_cond_signal(&NewBlock);
                blockAdded = true;
            }
        }
        if (!blockAdded)
        {
            // 6. If not proved, print error and leave the chain as is
            cout << "Server: Error - received block does not meet difficulty requirements." << endl;
        }

        pthread_mutex_unlock(&mutex);
         // 7. Go to 3
    }
}

void* Mine(void* minerID)
{
    int MinerID = *static_cast<int*>(minerID);
    BLOCK_T BlockToMine;

    while (true)
    {
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&NewBlock, &mutex);

        // Start from the current block the server points to
        CopyBlock(BlockToMine, *(Block_Chain.begin()));
        BlockToMine.relayed_by = MinerID; // Add miner's ID

        while (true)
        {
            // Increment nonce and update timestamp
            BlockToMine.nonce++;
            BlockToMine.timestamp = static_cast<int>(time(nullptr));

            // Calculate checksum of the block
            unsigned int hash = calculateChecksum(BlockToMine);

            // Check if the block meets the difficulty
            if (meetsDifficulty(hash, BlockToMine.difficulty))
            {
                // Notify the server with the new block data
                pthread_mutex_lock(&mutex);
                pthread_cond_signal(&BlockFound);

                // Update communicationBlocks with the mined block data
                communicationBlocks[MinerID].hash = hash;
                communicationBlocks[MinerID].relayed_by = MinerID;
                CopyBlock(communicationBlocks[MinerID], BlockToMine);

                pthread_mutex_unlock(&mutex);
            }
        }
    }
}
bool meetsDifficulty(const unsigned int hash, const int difficulty) 
{
    int leadingZeros = 0;
    unsigned int mask = 0x80000000; 

    while ((hash & mask) == 0 && leadingZeros < difficulty)
    {
        leadingZeros++;
        mask >>= 1; 
    }

    return leadingZeros >= difficulty;
}
unsigned int calculateChecksum(BLOCK_T& TempBlock) 
{
    string blockString = to_string(TempBlock.height)
                 + " " + to_string(TempBlock.timestamp) +  
                    " " + to_string(TempBlock.prev_hash) +
                        " " + to_string(TempBlock.nonce) +
                    " " + to_string(TempBlock.relayed_by);

    return crc32(0L,(const unsigned char*)blockString.c_str(),blockString.length());
}
void CopyBlock(BLOCK_T& destination,BLOCK_T& Source) 
{
    destination.difficulty = Source.difficulty;
    destination.height = Source.height;
    destination.nonce = Source.nonce;
    destination.prev_hash = Source.prev_hash;
    destination.relayed_by = Source.relayed_by;
    destination.timestamp = Source.timestamp;
}
int calculateInitialDifficulty(const int &hash)  
{ // Count number of leading zeros in the hash
    int difficulty = 0;
    unsigned int mask = 0x80000000;

    while ((hash & mask) == 0 && difficulty < sizeof(unsigned int) * 8) {
        difficulty++;
        mask >>= 1; 
    }

    return difficulty;
}
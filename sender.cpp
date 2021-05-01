#include <sys/shm.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "msg.h"    /* For the message struct */
#include <fstream>

/* The size of the shared memory chunk */
#define SHARED_MEMORY_CHUNK_SIZE 1000

/* The ids for the shared memory segment and the message queue */
int shmid, msqid;

/* The pointer to the shared memory */
void* sharedMemPtr;

/**
 * Sets up the shared memory segment and message queue
 * @param shmid - the id of the allocated shared memory
 * @param msqid - the id of the shared memory
 */

void init(int& shmid, int& msqid, void*& sharedMemPtr)
{
	/* TODO:
	    2. Use ftok("keyfile.txt", 'a') in order to generate the key.
		3. Use the key in the TODO's below. Use the same key for the queue
		    and the shared memory segment. This also serves to illustrate the difference
		    between the key and the id used in message queues and shared memory. The id
		    for any System V objest (i.e. message queues, shared memory, and sempahores)
		    is unique system-wide among all SYstem V objects. Two objects, on the other hand,
		    may have the same key.
	 */

	key_t key;
	key = ftok("keyfile.txt", 'a');

	/* TODO: Get the id of the shared memory segment. The size of the segment must be SHARED_MEMORY_CHUNK_SIZE */

	shmid = shmget(key, SHARED_MEMORY_CHUNK_SIZE, 0644 | IPC_CREAT);

	/* TODO: Attach to the shared memory */

	sharedMemPtr = shmat(shmid, (void *) NULL, 0);

	/* TODO: Attach to the message queue */

	msqid = msgget(key, 0644 | IPC_CREAT);

	/* Store the IDs and the pointer to the shared memory region in the corresponding parameters */

}

/**
 * Performs the cleanup functions
 * @param sharedMemPtr - the pointer to the shared memory
 * @param shmid - the id of the shared memory segment
 * @param msqid - the id of the message queue
 */

void cleanUp(const int& shmid, const int& msqid, void* sharedMemPtr)
{
	shmdt(sharedMemPtr);
	shmctl(shmid, IPC_RMID, NULL);
	msgctl(msqid, IPC_RMID, NULL);
}

/**
 * The main send function
 * @param fileName - the name of therea file
 */
void send(const char* fileName)
{
	// Open the file for reading
	FILE* fp = fopen(fileName, "r");

	// Was the file open?
	if (!fp)
	{
		perror("(fopen) Error opening file");
		exit(-1);
	}

	// A buffer to store message we will send to the receiver.
	message sndMsg;

	//Set message type to sender
	sndMsg.mtype = SENDER_DATA_TYPE;
	
	// A buffer to store message received from the receiver.
	message rcvMsg;

	printf("Ready to Send Message\n");
	
	/* Read the whole file */
	while (!feof(fp))
	{
		/* Read at most SHARED_MEMORY_CHUNK_SIZE from the file and store them in shared memory. 
		 * fread will return how many bytes it has actually read (since the last chunk may be less
		 * than SHARED_MEMORY_CHUNK_SIZE).
		 */

		if ( (sndMsg.size = fread(sharedMemPtr, sizeof(char), SHARED_MEMORY_CHUNK_SIZE, fp)) < 0) {
			perror("(fread) Error reading from shared memory");
			exit(-1);
		}
		
		
		/* Send message to queue alerting reciver message is ready */
		if (msgsnd(msqid, &sndMsg , sizeof(struct message) - sizeof(long), 0) == -1){
			perror("(msgsnd) Error sending message to alert receiver");
		}
			

		/* Wait until the receiver sends a message of type RECV_DONE_TYPE telling the sender 
		 * that it finished saving the memory chunk. 
		 */
		if ( msgrcv(msqid, &rcvMsg, sizeof(struct message) - sizeof(long), RECV_DONE_TYPE, 0) == -1 ) {
			perror("(msgrcv) Error receiving message from receiver");		
			exit(1);
		}
	}

	/* Send message to queue to tell reciver we have no more data to send, size = 0
	 * Set the message size in sndMsg to 0; siganls no more data
	 */
	sndMsg.size = 0;

	if (msgsnd(msqid, &sndMsg , sizeof(struct message) - sizeof(long) , 0) == -1) {
		perror("(msgsnd) Error sending a message");
	}

	// Close the file
	fclose(fp);
	
}


int main(int argc, char** argv)
{

	/* Check the command line arguments */
	if(argc < 2)
	{
		fprintf(stderr, "USAGE: %s <FILE NAME>\n", argv[0]);
		exit(-1);
	}

	/* Connect to shared memory and the message queue */
	init(shmid, msqid, sharedMemPtr);

	/* Send the file */
	send(argv[1]);

	/* Cleanup */
	cleanUp(shmid, msqid, sharedMemPtr);

	return 0;
}
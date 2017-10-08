/*  Producers and Consumers Assignment
    Samuel Bonner and Jack Neff
    CS444 - McGrath
*/    

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "twister.c"

#define MAX_ITEMS 32
#define TOTAL_ITEMS 100

//Function declarations
void* producer();
void* consumer();
void createItem();
void addItem();
struct Item removeItem();
void updateStatus();
int getValInRange(int min, int max);
/*Mersenne's Twister code was borrowed from
http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/MT2002/CODES/mt19937ar.c 
*/
double genrand_real2(void);
void init_genrand(unsigned long s);

struct Item {
	double number;
	int wait;
};

//Global vars
struct Item buffer[MAX_ITEMS];
int bufferState = 0;	//0 - Empty		1 - Contains items but not full		2 - Full
pthread_mutex_t mutex;
pthread_cond_t condConsumer, condProducer;

/*This code (main, producer, consumer) was adapted from 
http://cis.poly.edu/cs3224a/Code/ProducerConsumerUsingPthreads.c
Tanenbaum, Modern Operating Systems, 3rd Ed.
*/
int main(int argc, char **argv) {
	/*Consumers have wait time between 2-9 seconds,
	  Producers have wait time between 3-7 seconds
	  */
	//Setup mutex and conditions
	pthread_t threadP, threadC;
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&condConsumer, NULL);
	pthread_cond_init(&condProducer, NULL);

	//Create threads and join to main when finished
	pthread_create(&threadC, NULL, consumer, NULL);
	pthread_create(&threadP, NULL, producer, NULL);
	pthread_join(&threadC, NULL);
	pthread_join(&threadP, NULL);

	//End mutex and conditions
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&condConsumer);
	pthread_cond_destroy(&condProducer);
}

void* producer() {
	int i = 0;

	for (i = 0; i < TOTAL_ITEMS - 1; i++) {
		pthread_mutex_lock(&mutex);
		while (bufferState == 2)	//buffer full
			pthread_cond_wait(&condProducer, &mutex);
		
		//Create new item and add item to buffer
		//Buffer should check state after every add
		addItem();

		pthread_cond_signal(&condConsumer);
		pthread_mutex_unlock(&mutex);

		//Producer should get a random value
		//Then sleep for that time
		sleep( 5 );	//Change me
	}

	pthread_exit(0);	//Finished
}

void* consumer() {
	int i = 0;
	int waitTime = 0;

	for (i = 0; i < TOTAL_ITEMS - 1; i++) {
		pthread_mutex_lock(&mutex);
		while (bufferState == 0)
			pthread_cond_wait(&condConsumer, &mutex);

		//Remove an item from buffer and update waitTime
		//Buffer should update state after every consume
		//Report item's number
		struct Item consumed;
		consumed = removeItem();

		printf("Consumed number %f! Sleeping for %d seconds...\n", consumed.number, consumed.wait);

		pthread_cond_signal(&condProducer);
		pthread_mutex_unlock(&mutex);
		
		//Consumer should sleep for a period of time taken from last item consumed
		sleep( consumed.wait );
	}

	pthread_exit(0);	//Finished
}

int getValInRange(int min, int max) {
	//Uses Mersenne's Twister to generate a random value in range
	double rand = 0;
	int result = 0;
	unsigned long s = 4658346; //seed
	init_genrand(s);
	rand = genrand_real2();

	//Grab smaller value from rand


	return result;
}

void createItem() {
	//Generate a random value above 0 for item's number 
	
	//Generate a random value between 2 and 9 for item's wait
}

void addItem() {
	//Do nothing if buffer is full
	if (bufferState == 2)
		return;
	//Adds an item to the buffer
	int i = 0;
	for (i = 0; i < MAX_ITEMS - 1; i++) {
		if (buffer[i].number == 0) {
			//Space available
			buffer[i].number = 1.0;
			buffer[i].wait = 1;
			printf("Updated buffer spot %d with %f!\n", i, buffer[i].number);
			break;
		}
	}
	//Update buffer status
	updateStatus();
}

struct Item removeItem() {
	//Remove an item from the buffer
	if (bufferState == 0)
		return;

	//Get item and set buffer item number to 0
	int i = 0;
	struct Item ret;

	//Grab first available item
	for (i = 0; i < MAX_ITEMS - 1; i++) {
		if (buffer[i].number != 0) {
			//Get data then reset item
			ret.number = buffer[i].number;
			ret.wait = buffer[i].wait;
			buffer[i].number = 0;
			buffer[i].wait = 0;
			printf("Removed buffer spot %d with %f!\n", i, ret.number);
			break;
		}
	}
	//Update buffer status
	updateStatus();
	//Return item
	return ret;
}

void updateStatus() {
	//Used to update buffer status
	//0 - empty		1 - contains items but not full		2 - full
	int i = 0;
	int count = 0;
	for (i = 0; i < MAX_ITEMS; i++) {
		if (buffer[i].number != 0)
			count++;
	}
	//Determine state
	if (count == 0)
		bufferState = 0;
	else if (count == (MAX_ITEMS-1))
		bufferState = 2;
	else
		bufferState = 1;
}
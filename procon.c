/*  Producers and Consumers Assignment
    Samuel Bonner and Jack Neff
    CS444 - McGrath
*/    

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "twister.c"

#define MAX_ITEMS 32
#define TOTAL_ITEMS 100

//Function declarations
void* producer();
void* consumer();
void createItem();
void addItem();
void removeItem();
void checkStatus();
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

		pthread_cond_signal(&condConsumer);
		pthread_mutex_unlock(&mutex);

		//Producer should get a random value
		//Then sleep for that time
		printf("Producer zzz...\n");
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

		pthread_cond_signal(&condProducer);
		pthread_mutex_unlock(&mutex);
		
		//Consumer should sleep for a period of time taken from last item consumed
		//sleep
		printf("Consumer zzz...\n");
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
	//Generate a random value for item's number
	
	//Generate a random value between 2 and 9 for item's wait
}

void addItem() {
	//Adds an item to the buffer

	//Update buffer status
}

void removeItem() {
	//Remove an item from the buffer

	//Update buffer status
}

void checkStatus() {
	//Used to update buffer status
	//0 - empty		1 - contains items but not full		2 - full
}
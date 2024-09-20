#ifndef PIZZA_H 
#define PIZZA_H

/* CLOCK_REALTIME not defined error without this*/
#define _POSIX_C_SOURCE 199309L


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

/*The function executed by every thread*/
void* make_order(void*);

#define N_TEL 2
#define N_COOK 2
#define N_OVEN 10
#define N_DELIVERER 10
#define T_ORDERLOW 1    
#define T_ORDERHIGH 5
#define N_ORDERLOW 1   
#define N_ORDERHIGH 5   
#define P_MARGARITA 35
#define P_PEPPERONI 25
#define P_SPECIAL 40      
#define T_PAYMENTLOW 1  
#define T_PAYMENTHIGH 3
#define P_FAIL 5
#define C_MARGARITA 10
#define C_PEPPERONI 11
#define C_SPECIAL 12
#define T_PREP 1
#define T_PACK  1
#define T_BAKE 10
#define T_DELLOW 5
#define T_DEHIGH 15

/* Common shared resources along their mutexes and conditional variables*/

unsigned int telephone_operators = N_TEL;
pthread_mutex_t telephone_operator_mutex;
pthread_cond_t telephone_operator_cond;


unsigned int cooks = N_COOK;
pthread_mutex_t cooksMutex;
pthread_cond_t cooksCond;

unsigned int ovens = N_OVEN;
pthread_mutex_t ovensMutex;
pthread_cond_t ovensCond;

unsigned int deliverers = N_DELIVERER;
pthread_mutex_t deliverersMutex;
pthread_cond_t delivererCond;

struct Sales{
    unsigned int margaritaPizzasSold;
    unsigned int pepperoniPizzasSold;
    unsigned int specialPizzasSold;
    unsigned int earnings;
    pthread_mutex_t salesMutex;
};

struct OrderStatistics{
    unsigned int successfulOrders;
    unsigned int cancelledOrders;
    pthread_mutex_t mutex; 
};

struct ServiceTime{
    time_t sum;
    time_t max;
    pthread_mutex_t mutex;
};

struct CoolingTime{
    time_t sum;
    time_t max;
    pthread_mutex_t mutex;
};

pthread_mutex_t printMutex;

/* Original seed given as a second parametre to our program*/
unsigned int ORIGINALSEED;

unsigned int getRandomInRange(unsigned int* seedp, unsigned int lowest, unsigned int highest);


#endif
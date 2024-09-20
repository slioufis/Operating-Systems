/*All includes and declaration sare contained inside this header*/
#include "p3210209-p3180224-p3180265-pizza.h"

/* Initialise statistics*/
struct Sales sales = {0, 0, 0};
struct OrderStatistics orderStats = {0, 0};
struct ServiceTime serviceTime = {0, 0};
struct CoolingTime coolingTime = {0, 0};

int main(int argc, char* argv[]){

    if(argc != 3){
        fprintf(stderr, "[Error] Usage: ./prog 'N_CUST' 'seed'\n");
        exit(EXIT_FAILURE);
    }

    if(atoi(argv[1]) <= 0){
        fprintf(stderr, "[Error] %s is not valid input\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    if(atoi(argv[2]) <= 0){
        fprintf(stderr, "[Error] %s is not a valid seed\n", argv[2]);
        exit(EXIT_FAILURE);
    }

    // Number of customers/orders, given as a first parametre to our program
    const unsigned int N_CUST = atoi(argv[1]);

    //Copy of the orginal seed, which is the second parametre of our program
    unsigned int copySeed = (ORIGINALSEED = atoi(argv[2]));

    // An array of threads, one for each order/customer, might malloc to avoid vla
    pthread_t *th;
    // An array of integers representing the id of every order
    int *order_ids;

    int order_time; // Time until the next customer shows up;

    if(NULL == (th = malloc(sizeof(pthread_t) * N_CUST))){  // Allocate an array of space as large as the number of customers
        fprintf(stderr, "[Error]: malloc Failed to allocate enough space for every thread\n"); 
        exit(EXIT_FAILURE);
    }

    if(NULL == (order_ids = malloc(sizeof(int) * N_CUST))){
        fprintf(stderr, "[Error]: malloc Failed to allocate enough space for every order id\n");
        free(th);   // Free the array of threads allocated earlier
        exit(EXIT_FAILURE);

    }

    /*Initialize out mutexes and condition variables*/
    pthread_mutex_init(&telephone_operator_mutex, NULL);
    pthread_cond_init(&telephone_operator_cond, NULL);
    pthread_mutex_init(&cooksMutex, NULL);
    pthread_cond_init(&cooksCond, NULL);
    pthread_mutex_init(&ovensMutex, NULL);
    pthread_cond_init(&ovensCond, NULL);
    pthread_mutex_init(&deliverersMutex, NULL);
    pthread_cond_init(&delivererCond, NULL);
    pthread_mutex_init(&(sales.salesMutex), NULL);
    pthread_mutex_init(&(orderStats.mutex), NULL);
    pthread_mutex_init(&(serviceTime.mutex), NULL);
    pthread_mutex_init(&(coolingTime.mutex), NULL);
    pthread_mutex_init(&printMutex, NULL);

    for(int i = 0; i < N_CUST; i++){ 

        order_time = getRandomInRange(&copySeed, T_ORDERLOW, T_ORDERHIGH);

        order_ids[i] = i + 1; 

        /* Create a new thread for each order and pass the order id as parametre*/
        if(pthread_create(th + i, NULL, &make_order, order_ids + i) != 0){
            fprintf(stderr, "[Error] Thread %d failed to create\n", i + 1);
            free(th); 
            free(order_ids); // Free the allocated memory
            exit(EXIT_FAILURE); // Terminate process
        }

        /* Pause execution until the next customer arrives*/
        sleep(order_time);
    }

    for(int i = 0; i < N_CUST; i++){

        if(pthread_join(th[i], NULL) != 0){
            fprintf(stderr, "[Error] Thread %d failed to join\n", i + 1);
            free(th);
            free(order_ids);
            exit(EXIT_FAILURE);
        }
    }

    // Free the  allocated memory
    free(th); 
    free(order_ids);

    /*Destroy/free our mutexed and condition variables*/
    pthread_mutex_destroy(&cooksMutex);
    pthread_cond_destroy(&cooksCond);
    pthread_mutex_destroy(&ovensMutex);
    pthread_cond_destroy(&ovensCond);
    pthread_mutex_destroy(&deliverersMutex);
    pthread_cond_destroy(&delivererCond);
    pthread_mutex_destroy(&(sales.salesMutex));
    pthread_mutex_destroy(&(orderStats.mutex));
    pthread_mutex_destroy(&(serviceTime.mutex));
    pthread_mutex_destroy(&coolingTime.mutex);
    pthread_mutex_destroy(&printMutex);

    /*Print our final results*/
    printf("\n\nTotal Earnings = %d Euros\n", sales.earnings);
    printf("Margatita Pizzas Sold = %d\n", sales.margaritaPizzasSold);
    printf("Pepperoni Pizzas Sold = %d\n", sales.pepperoniPizzasSold);
    printf("Special Pizzas Sold = %d\n", sales.specialPizzasSold);
    printf("Successful Orders = %d/%d\n", orderStats.successfulOrders, orderStats.successfulOrders + orderStats.cancelledOrders);
    printf("Average Service Time = %.3f Minutes\n", (unsigned int)serviceTime.sum / (float)orderStats.successfulOrders);
    printf("Max Service Time = %d Minutes\n", (unsigned int)serviceTime.max);
    printf("Average Cooling Time = %.3f Minutes\n", (unsigned int)coolingTime.sum / (float)orderStats.successfulOrders);
    printf("Max Cooling Time = %d Minutes\n\n", (unsigned int)coolingTime.max);
    
    exit(EXIT_SUCCESS);
}


void* make_order(void* arg){

    struct timespec start_timer;            // Time instance when the customer shows up
    struct timespec bake_timer_finish;      // Time instance when the pizzas are baked
    struct timespec packing_timer_finish;   // Time instance when the pizzas are packed
    struct timespec delivery_timer_finish;  // Time instance when the order is delivered
    
    unsigned int numOfMargaritaPizzas = 0;
    unsigned int numOfPepperoniPizzas = 0;
    unsigned int numOfSpecialPizzas = 0;
    unsigned int numOfPizzas = 0;

    // Thread spesific seed to be used locally for random number generation
    unsigned int localSeed = *(unsigned int*)arg + ORIGINALSEED * (*(unsigned int*)arg);

    time_t timeUntilPackaging;  // Time from the moment the customer arrived until the order is packed
    time_t timeUntilDelivery;   // Time from the moment the customer arrived until the order is delivered
    time_t timeToCool;          // Time from the moment the pizzas are baked until the order is delivered

    // Start Order Timer
    if(clock_gettime(CLOCK_REALTIME, &start_timer) == -1){
        fprintf(stderr, "[Error] clock_gettime Failed in thread %d\n", *(int*)arg);
        exit(EXIT_FAILURE);
    }

    pthread_mutex_lock(&telephone_operator_mutex);

    while(!telephone_operators){
        pthread_cond_wait(&telephone_operator_cond, &telephone_operator_mutex);
    }
    telephone_operators--;
    pthread_mutex_unlock(&telephone_operator_mutex);

    /* Generate the number of pizzas for this order*/
    numOfPizzas = getRandomInRange(&localSeed, N_ORDERLOW, N_ORDERHIGH);

    for(int i = 0; i < numOfPizzas; i++){

        unsigned int random_number = rand_r(&localSeed) % 100 + 1; // Generate a number between 1, 100
        if (random_number <= P_MARGARITA ){ // P_MARGARITA% this number is between 1 and P_MARGARITA
            numOfMargaritaPizzas++;
        } else if (random_number <= P_MARGARITA + P_PEPPERONI){ // P_PEPERRONI% this number is between P_MARGARITA AND P_PEPPERONI
            numOfPepperoniPizzas++;
        }else{
            numOfSpecialPizzas++; // P_SPECIAL% this number is between P_PEPPERONI AND 100
        }
    }
    
    /*Wait for payment*/
    sleep(getRandomInRange(&localSeed, T_PAYMENTLOW, T_PAYMENTHIGH));

    
    pthread_mutex_lock(&telephone_operator_mutex);
    telephone_operators++; // Telephone operator is available 
    pthread_cond_signal(&telephone_operator_cond);
    pthread_mutex_unlock(&telephone_operator_mutex);
    
    /* There is a P_FAIL perncetange that the order was cancelled*/
    if(rand_r(&localSeed) % 100 + 1 <= P_FAIL){

        pthread_mutex_lock(&printMutex);
        printf("Order With ID <%03d> Got Cancelled\n", *(unsigned int*)arg);
        pthread_mutex_unlock(&printMutex);

        /* Add failed order to statistics*/
        pthread_mutex_lock(&(orderStats.mutex));
        orderStats.cancelledOrders++;
        pthread_mutex_unlock(&(orderStats.mutex));
        pthread_exit(NULL);

    }
    
    pthread_mutex_lock(&printMutex);
    printf("Order With ID <%03d> Got Registered: %d Margarita, %d Pepperoni and %d Special \n", *(unsigned int*)arg, numOfMargaritaPizzas,
        numOfPepperoniPizzas, numOfSpecialPizzas);
    pthread_mutex_unlock(&printMutex);

    /* Calculate the earnings for this order and add it to the total*/
    pthread_mutex_lock(&(sales.salesMutex));
    sales.margaritaPizzasSold += numOfMargaritaPizzas;
    sales.pepperoniPizzasSold += numOfPepperoniPizzas;
    sales.specialPizzasSold += numOfSpecialPizzas;
    sales.earnings += ((numOfMargaritaPizzas * C_MARGARITA) + (numOfPepperoniPizzas * C_PEPPERONI) +(numOfSpecialPizzas * C_SPECIAL));
    pthread_mutex_unlock(&(sales.salesMutex));


    pthread_mutex_lock(&cooksMutex);
    /* Prepare the pizzas*/
    while(!cooks){  /* Same as cooks == 0*/
        pthread_cond_wait(&cooksCond, &cooksMutex); /* Wait until a cook is available to prepare the pizzas*/
    }
    cooks--;
    pthread_mutex_unlock(&cooksMutex);

    sleep(T_PREP * numOfPizzas);

    /* Bake the pizzas*/
    pthread_mutex_lock(&ovensMutex);
    while(ovens < numOfPizzas){
        pthread_cond_wait(&ovensCond, &ovensMutex); // Wait until enough ovens are availale to bake the pizzas
    }

    ovens -= numOfPizzas;   // Those ovens are not occupied
    pthread_mutex_unlock(&ovensMutex);

    pthread_mutex_lock(&cooksMutex);
    cooks++; /* Cook has put the pizzas in the oven and is now ready to handle some other order*/
    pthread_cond_broadcast(&cooksCond); /* Signal the other threads that a resource has been freed*/
    pthread_mutex_unlock(&cooksMutex);

    sleep(T_BAKE);

    /*Baking has been finished. From now on the pizzas are starting to get cold*/
    if(clock_gettime(CLOCK_REALTIME, &bake_timer_finish) == -1){
        fprintf(stderr, "[Error] clock_gettime Failed\n");
        exit(EXIT_FAILURE);
    }

    /*Pack the pizzas*/
    pthread_mutex_lock(&deliverersMutex);
    while(!deliverers){
        pthread_cond_wait(&delivererCond, &deliverersMutex); /* Wait until some deliverer is available to pack the pizzas from the ovens and
            deliver it to the customers*/
    }
    deliverers--;       /* A delivered is occupied*/
    pthread_mutex_unlock(&deliverersMutex);

    sleep(T_PACK * numOfPizzas);    /* Time it takes to pack all the pizzas*/

    
    if(clock_gettime(CLOCK_REALTIME, &packing_timer_finish) == -1){
        fprintf(stderr, "[Error] clock_gettime Failed\n");
        exit(EXIT_FAILURE);
    }

    timeUntilPackaging = packing_timer_finish.tv_sec - start_timer.tv_sec;

    pthread_mutex_lock(&printMutex);
    printf("Order With Id <%03d> Got Packaged In %ld Minutes\n", *(int*)arg, (long)timeUntilPackaging);
    pthread_mutex_unlock(&printMutex);

    pthread_mutex_lock(&ovensMutex);
    ovens += numOfPizzas;   /* Pizzas have been packed, the ovens are now available for use*/
    pthread_cond_signal(&ovensCond); /* Singal to other threads that some ovens are now available*/
    pthread_mutex_unlock(&ovensMutex);

    
    
    unsigned int deliveryTime = getRandomInRange(&localSeed, T_DELLOW, T_DEHIGH);
    
    /* Deliverer delivers pizza*/
    sleep(deliveryTime); /* Time it takes to drive to the customer's home*/
    
    /* Delivery completed*/

    if(clock_gettime(CLOCK_REALTIME, &delivery_timer_finish) == -1){
        fprintf(stderr, "[Error] clock_gettime Failed in thread %d\n", *(int*)arg);
        exit(EXIT_FAILURE);
    }

    timeUntilDelivery = delivery_timer_finish.tv_sec - start_timer.tv_sec;
    timeToCool = delivery_timer_finish.tv_sec - bake_timer_finish.tv_sec;

    pthread_mutex_lock(&printMutex);
    printf("Order With Id <%03d> Got Delivered In %ld Minutes\n", *(unsigned int*)arg,  (long)timeUntilDelivery); 
    pthread_mutex_unlock(&printMutex);

    /* Time statistics*/
    pthread_mutex_lock(&(serviceTime.mutex));
    serviceTime.sum += timeUntilDelivery;
    if(timeUntilDelivery > serviceTime.max){
        serviceTime.max = timeUntilDelivery;
    }
    pthread_mutex_unlock(&(serviceTime.mutex));

    pthread_mutex_lock(&(coolingTime.mutex));
    coolingTime.sum += timeToCool;

    if(timeToCool > coolingTime.max){
        coolingTime.max = timeToCool;
    }
    pthread_mutex_unlock(&(coolingTime.mutex));

    /* Order was successfull, increase the number of successfull ordes by one*/
    pthread_mutex_lock(&(orderStats.mutex));
    orderStats.successfulOrders++;
    pthread_mutex_unlock(&(orderStats.mutex));

    // Time until the deliverer gets back to the store
    sleep(deliveryTime);

    pthread_mutex_lock(&deliverersMutex);
    deliverers++;   // Delivery guy returned to the pizzeria, he is now free to handle another order*/
    pthread_cond_signal(&delivererCond);    /* Signal other threads that a delivery guy is available to handle other orders*/
    pthread_mutex_unlock(&deliverersMutex);

    return NULL;
}

unsigned int getRandomInRange(unsigned int* seedp, unsigned int lowest, unsigned int highest){
    return rand_r(seedp) % (highest + 1 - lowest) + lowest;
}

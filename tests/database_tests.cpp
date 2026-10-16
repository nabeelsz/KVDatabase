#include "database.h"
#include "testing.h"
#include <semaphore.h>
#include <pthread.h>


struct ThreadArgs {
    bool* ready; 
    KVDataBase* database; 
    sem_t* semaphore; 
    pthread_cond_t* cv; 
    pthread_mutex_t* cv_lock; 
}; 

void* thread_function(void* args) {
    printf("Thread initialized\n"); 
    ThreadArgs* struct_args = (ThreadArgs*)(args); 
    bool* ready = struct_args->ready; 
    KVDataBase* database = struct_args->database; 
    sem_t* semaphore = struct_args->semaphore; 
    pthread_cond_t* cv = struct_args->cv; 
    pthread_mutex_t* cv_lock = struct_args->cv_lock; 

    char key[28];  
    generate_random_cstring(key, 27);
    int value = 4; 
    pthread_mutex_lock(cv_lock); 
    while (*ready == false) {
        pthread_cond_wait(cv, cv_lock); 
    }
    pthread_mutex_unlock(cv_lock); 

    database->put(key, (void*)&value, sizeof(int)); 
    sem_post(semaphore);

    return NULL; 
}

void database_perf_tests() {
    KVDataBase database; 
    bool ready = false; 
    sem_t semaphore; 
    pthread_cond_t cv = PTHREAD_COND_INITIALIZER; 
    if (sem_init(&semaphore, 0, 0) != 0) {
        printf("ERROR: Could not initialize semaphore\n"); 
    }
    pthread_mutex_t cv_lock = PTHREAD_MUTEX_INITIALIZER; 
    ThreadArgs args = {&ready, &database, &semaphore, &cv, &cv_lock}; 
    pthread_t id; 
    // Spawn 1000 threads to do 1000 randomized PUTs once all of them are spawned
    for (int i = 0; i < 1000; i++) {
        pthread_create(&id, NULL, &thread_function, (void*)(&args)); 
    }
    ready = true; 
    pthread_cond_broadcast(&cv); 
    // Need to send this thread to sleep until all PUTS have finsihed so args don't get destructed 
    int num_done = 0; 
    while (num_done < 1000) {
        sem_wait(&semaphore); 
        num_done++; 
        printf("Num done = %d\n", num_done); 
    }
    // Once we've reached here, all threads have finished and the main thread can exit out of this function. 
    printf("PUT performance tests finished!\n"); 
}


int main() {
    database_perf_tests(); 
    return 0; 
}
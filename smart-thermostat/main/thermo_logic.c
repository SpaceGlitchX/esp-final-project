#include "thermo_logic.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


void set_setpoint(int level) {


}

void get_setpoint(void) {

}
void thermo_logic_init(void) {

    pthread_t setter, getter;

    // Create thread
    pthread_create(&setter, NULL, &set_setpoint, NULL);
    pthread_create(&getter, NULL, &get_setpoint, NULL);

    // Wait for threads to finish
    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);

}
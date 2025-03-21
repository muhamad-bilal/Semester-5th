#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

struct station {
    pthread_mutex_t lock;
    pthread_cond_t train_arrived;
    pthread_cond_t seat_taken;
    pthread_cond_t train_full;

    int waiting_passengers;
    int available_seats;
    int passengers_on_board;
};

// Initialize the station
void station_init(struct station *station) {
    pthread_mutex_init(&station->lock, NULL);
    pthread_cond_init(&station->train_arrived, NULL);
    pthread_cond_init(&station->seat_taken, NULL);
    pthread_cond_init(&station->train_full, NULL);
    station->waiting_passengers = 0;
    station->available_seats = 0;
    station->passengers_on_board = 0;
}

// Train arrives at the station
void station_load_train(struct station *station, int count) {
    pthread_mutex_lock(&station->lock);

    station->available_seats = count;

    // Notify passengers that a train has arrived
    while (station->available_seats > 0 && station->waiting_passengers > 0) {
        pthread_cond_broadcast(&station->train_arrived);
        pthread_cond_wait(&station->train_full, &station->lock);
    }

    station->available_seats = 0; // Reset seats for the next train

    pthread_mutex_unlock(&station->lock);
}

// Passenger waits for a train
void station_wait_for_train(struct station *station) {
    pthread_mutex_lock(&station->lock);

    station->waiting_passengers++;

    // Wait until a train arrives and there's a seat available
    while (station->available_seats == 0) {
        pthread_cond_wait(&station->train_arrived, &station->lock);
    }

    // Prepare to board
    station->waiting_passengers--;
    station->passengers_on_board++;
    station->available_seats--;

    pthread_mutex_unlock(&station->lock);
}

// Passenger signals they are on board
void station_on_board(struct station *station) {
    pthread_mutex_lock(&station->lock);

    station->passengers_on_board--;

    // Notify the train if it's full or all passengers have boarded
    if (station->passengers_on_board == 0 &&
        (station->available_seats == 0 || station->waiting_passengers == 0)) {
        pthread_cond_signal(&station->train_full);
    }

    pthread_mutex_unlock(&station->lock);
}

// Passenger thread function
void* passenger_thread(void* arg) {
    struct station* station = (struct station*)arg;

    station_wait_for_train(station);
    printf("Passenger: Waiting for train...\n");
    station_on_board(station);
    printf("Passenger: Boarded the train.\n");

    return NULL;
}

// Train thread function
void* train_thread(void* arg) {
    struct station* station = (struct station*)arg;

    printf("Train: Arriving with 3 seats.\n");
    station_load_train(station, 3); // Train arrives with 3 seats
    printf("Train: Departed.\n");

    return NULL;
}

int main() {
    struct station station;
    station_init(&station);

    pthread_t passengers[5];
    pthread_t train;

    // Create passenger threads
    for (int i = 0; i < 5; i++) {
        pthread_create(&passengers[i], NULL, passenger_thread, &station);
        usleep(100000); // Add slight delay to simulate real-world arrival
    }

    // Create train thread
    pthread_create(&train, NULL, train_thread, &station);

    // Wait for all threads to finish
    for (int i = 0; i < 5; i++) {
        pthread_join(passengers[i], NULL);
    }
    pthread_join(train, NULL);

    printf("Simulation complete.\n");
    return 0;
}

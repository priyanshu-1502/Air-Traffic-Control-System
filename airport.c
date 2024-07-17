#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>
#include <limits.h>

#define MAX_RUNWAYS 10
#define BACKUP_RUNWAY_CAPACITY 15000

typedef struct {
    int loadCapacity;
    sem_t runway_sem;
} Runway;

typedef struct {
    long mtype;
    int plane_id;
    int dep_airport;
    int arr_airport;
    int plane_type; 
    int num_passengers; 
    int total_weight;
} PlaneDetails;

int airport_num, num_runways;
Runway runways[MAX_RUNWAYS + 1];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *handle_departure(void *arg) {
    printf("I am inside the departure function\n");
    PlaneDetails *plane_details = (PlaneDetails *)arg;
    int runway_idx = -1;
    int min_remaining_capacity = INT_MAX;

    // Best cost logic
    for (int i = 0; i < num_runways; i++) {
        if (plane_details->total_weight <= runways[i].loadCapacity) {
            int remaining_capacity = runways[i].loadCapacity - plane_details->total_weight;
            if (remaining_capacity < min_remaining_capacity) {
                min_remaining_capacity = remaining_capacity;
                runway_idx = i;
            }
        }
    }
    

    // Backup Runaway
    if (runway_idx == -1) {
        runway_idx = num_runways;
    }

    // Acquiring the semaphore for the chosen runway
    sem_wait(&runways[runway_idx].runway_sem);

    // Boarding/loading time
    sleep(3);

    printf("Plane %d has completed boarding/loading and taken off from Runway No. %d of Airport No. %d.\n", plane_details->plane_id, runway_idx + 1, airport_num);

    
    //printf("Sending message to airtrafficcontroller\n");
    
    // Releasing the semaphore for the chosen runway
    sem_post(&runways[runway_idx].runway_sem);
    //printf("Ending This thread!\n");
    return NULL;
}

void *handle_arrival(void *arg) {
    //printf("I am inside the arrival function\n");
    PlaneDetails *plane_details = (PlaneDetails *)arg;
    int runway_idx = -1, min_weight_diff = 15000;
    int min_remaining_capacity = INT_MAX;	
	
    // Find the runway with the minimum weight difference
    /***for (int i = 0; i < num_runways; i++) {
        int weight_diff = plane_details->total_weight - runways[i].loadCapacity;
        if (weight_diff > 0 && weight_diff < min_weight_diff) {
            min_weight_diff = weight_diff;
            runway_idx = i;
        }
    }***/
    for (int i = 0; i < num_runways; i++) {
        if (plane_details->total_weight <= runways[i].loadCapacity) {
            int remaining_capacity = runways[i].loadCapacity - plane_details->total_weight;
            if (remaining_capacity < min_remaining_capacity) {
                min_remaining_capacity = remaining_capacity;
                runway_idx = i;
            }
        }
    }

    // Backup runway
    if (runway_idx == -1) {
        runway_idx = num_runways;
    }

    // Acquire the semaphore for the chosen runway
    sem_wait(&runways[runway_idx].runway_sem);

    // Simulating landing time
    sleep(2);
	
    // Simulating deboarding or unloading time
    sleep(3);
    
    printf("Plane %d has landed on Runway No. %d of Airport No. %d and has completed deboarding/unloading.\n", plane_details->plane_id, runway_idx + 1, airport_num);
    
    // Releasing the semaphore for the chosen runway
    sem_post(&runways[runway_idx].runway_sem);

    pthread_exit(NULL);
}
int main() {
    int i, runway_capacity;
    pthread_t departure_thread, arrival_thread;
    // Get airport number
    printf("Enter Airport Number: ");
    scanf("%d", &airport_num);

    // Get number of runways
    printf("Enter number of Runways: ");
    scanf("%d", &num_runways);

    printf("Enter loadCapacity of Runways (give as a space separated list in a single line): ");
    for (i = 0; i < num_runways; i++) {
        scanf("%d", &runway_capacity);
        runways[i].loadCapacity = runway_capacity;
        sem_init(&runways[i].runway_sem, 0, 1);
    }
    // Add backup runway
    runways[num_runways].loadCapacity = BACKUP_RUNWAY_CAPACITY;
    sem_init(&runways[num_runways].runway_sem, 0, 1);
    num_runways++;
    int msgid;
    msgid = msgget((key_t)12345, 0644|IPC_CREAT);
    if (msgid == -1) {
        printf("error in creating message queue\n");
        exit(1);
    }
    while (1) {
        PlaneDetails plane_details;
        if (msgrcv(msgid, (void *)&plane_details, sizeof(plane_details), 30 + airport_num, IPC_NOWAIT) != -1) {
            printf("Termination of this airprt starting...\n");
            break;
        }
        if (msgrcv(msgid, (void *)&plane_details, sizeof(plane_details), airport_num, IPC_NOWAIT) != -1) {
            //printf("Message received by the airport!\n");
            pthread_mutex_lock(&mutex);
            if (plane_details.arr_airport == airport_num) { //arrival
                //printf("Plane Arriving!\n");
                if (pthread_create(&arrival_thread, NULL, handle_arrival, &plane_details) != 0) {
                    perror("pthread_create");
                    exit(EXIT_FAILURE);
                }
                pthread_join(arrival_thread, NULL);
                // Set the message type for the destination airport
                plane_details.mtype = plane_details.arr_airport + 20;
                // Send a message to the ATC
                if (msgsnd(msgid, (void *)&plane_details, sizeof(PlaneDetails), 0) == -1) {
                    printf("Error in sending message\n");
                    exit(1);
                } else {
                    printf("Message sent to ATC!\n");
                }

            } else { //depart
                printf("Plane Departing!\n");
                if (pthread_create(&departure_thread, NULL, handle_departure, &plane_details) != 0) {
                    perror("pthread_create");
                    exit(EXIT_FAILURE);
                } else {
                    printf("Success in creating the thread!\n");
                }
                pthread_join(departure_thread, NULL);
                // Send a message to the ATC
                plane_details.mtype=20+plane_details.dep_airport;
                if (msgsnd(msgid, (void *)&plane_details, sizeof(PlaneDetails), 0) == -1) {
                    printf("Error in sending message to airtrafficcontroller\n");
                    exit(1);
                } else {
                    printf("Message sent to airtrafficcontroller!\n");
                }

            }
            pthread_mutex_unlock(&mutex);
        }
    }
    return 0;
} 

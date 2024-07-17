#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>

#define MAX_AIRPORTS 10

typedef struct {
    long mtype;
    int plane_id;
    int dep_airport;
    int arr_airport;
    int plane_type; 
    int num_passengers; 
    int total_weight;
} PlaneDetails;

int num_airports, msgid;

void *handle_plane(void *arg) {
    PlaneDetails *plane_details = (PlaneDetails *)arg;
    FILE *file;
    // Send departure request to the departure airport
    plane_details->mtype = plane_details->dep_airport;
    printf("Sending Departure request to Airport %d\n", (int)plane_details->mtype);
    if (msgsnd(msgid, plane_details, sizeof(PlaneDetails), 0) == -1) {
        perror("msgsnd");
        exit(EXIT_FAILURE);
    } else {
        printf("Message sent to the departure airport!\n");
    }

    // Wait for response from the departure airport
    while (msgrcv(msgid, plane_details, sizeof(PlaneDetails), plane_details->dep_airport + 20, 0) == -1);
    //printf("Message received from the departure airport!\n");

    // Create AirTrafficController.txt if it doesn't exist
    file = fopen("AirTrafficController.txt", "a+");
    if (file == NULL) {
        printf("Error creating AirTrafficController.txt\n");
        exit(EXIT_FAILURE);
    }

    // Append plane journey information to AirTrafficController.txt
    fprintf(file, "Plane %d has departed from Airport %d and will land at Airport %d.\n", plane_details->plane_id, plane_details->dep_airport, plane_details->arr_airport);
    fclose(file);

    sleep(30); // flight time

    // Send message to the arrival airport
    plane_details->mtype = plane_details->arr_airport;
    if (msgsnd(msgid, plane_details, sizeof(PlaneDetails), 0) == -1) {
        printf("Error in sending message to the arrival airport\n");
        exit(1);
    }

    // response from the arrival airport
    while (msgrcv(msgid, plane_details, sizeof(PlaneDetails), 20 + plane_details->arr_airport, 0) == -1);
    //printf("Message received from the arrival airport!\n");

    // Send final message to plane.c
    plane_details->mtype = 12;
    if (msgsnd(msgid, plane_details, sizeof(PlaneDetails), 0) == -1) {
        printf("Error in sending final message to plane.c\n");
        exit(1);
    } else {
        //printf("Final message sent to plane.c\n");
    }

    //free(plane_details);
}

int main() {
    int i, num_planes = 0;
    //pthread_t plane_thread;

    // Get number of airports
    printf("Enter the number of airports to be handled/managed: ");
    scanf("%d", &num_airports);

    // Set up message queue
    
    msgid = msgget((key_t)12345, 0644 | IPC_CREAT);
    if (msgid == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    while (1) {
        PlaneDetails *plane_details = (PlaneDetails *)malloc(sizeof(PlaneDetails));
        // Receive initial message from plane.c
        if (msgrcv(msgid, plane_details, sizeof(PlaneDetails), 11, IPC_NOWAIT) != -1) {
            printf("Received initial message from plane\n");
            // Create a new thread to handle the plane
            handle_plane(plane_details);
            /*if (pthread_create(&plane_thread, NULL, handle_plane, plane_details) != 0) {
                perror("pthread_create");
                exit(EXIT_FAILURE);
            }*/
            // Detach the thread to avoid memory leaks
            // pthread_detach(plane_thread);
        }
        // Check for termination message first
        if (msgrcv(msgid, (void *)plane_details, sizeof(PlaneDetails), 13, IPC_NOWAIT) != -1) {
            printf("Termination Request Received!\nStarting Termination process...\n");
            //pthread_join(plane_thread, NULL);
            for (int i = 1; i <= num_airports; i++) {
                plane_details->mtype = 30 + i;
                if (msgsnd(msgid, (void *)plane_details, sizeof(PlaneDetails), 0) == -1) {
                    printf("Error in sending message to the Airport for Termination.\n");
                    exit(1);
                }
            }
            break;
        }
        else continue;
    }
    if (msgctl(msgid, IPC_RMID, NULL) == -1) {
        perror("msgctl");
        exit(EXIT_FAILURE);
    }
    return 0;
}

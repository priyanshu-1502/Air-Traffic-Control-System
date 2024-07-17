#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>

#define MAX_PASSENGERS 10
#define CREW_WEIGHT 75
#define MAX_LUGGAGE_WEIGHT 25
#define MAX_PASSENGER_WEIGHT 100

typedef struct {
    int luggage_weight;
    int passenger_weight;
} PassengerWeight;

typedef struct {
    long mtype;
    int plane_id;
    int dep_airport;
    int arr_airport;
    int plane_type; // 0 for cargo, 1 for passenger
    int num_passengers; // for passenger planes only
    int total_weight;
} PlaneDetails;

int main() {
    int plane_id, plane_type, num_passengers, i;
    int luggage_weight[MAX_PASSENGERS], passenger_weight[MAX_PASSENGERS];
    int num_cargo_items, avg_cargo_weight;
    int dep_airport, arr_airport;
    int total_weight = 0;
    pid_t passenger_pids[MAX_PASSENGERS];
    int passenger_pipes[MAX_PASSENGERS][2];
    int msgid;

    // Get plane ID
    printf("Enter Plane ID: ");
    scanf("%d", &plane_id);

    // Get plane type
    printf("Enter Type of Plane: ");
    scanf("%d", &plane_type);

    // Set up shared data structure to store plane details
    PlaneDetails plane_details;
    plane_details.plane_id = plane_id;
    plane_details.plane_type = plane_type;
    if (plane_type == 1) { // Passenger plane
        // Get number of occupied seats
        printf("Enter Number of Occupied Seats: ");
        scanf("%d", &num_passengers);
        plane_details.num_passengers = num_passengers;

        // Create passenger processes and pipes
        for (i = 0; i < num_passengers; i++) {
            if (pipe(passenger_pipes[i]) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }

            passenger_pids[i] = fork();

            if (passenger_pids[i] == 0) { // Passenger process
                close(passenger_pipes[i][0]); // Close read end of pipe

                printf("Enter Weight of Your Luggage: ");
                scanf("%d", &luggage_weight[i]);

                printf("Enter Your Body Weight: ");
                scanf("%d", &passenger_weight[i]);
                PassengerWeight pw;

                pw.luggage_weight = luggage_weight[i];
                pw.passenger_weight = passenger_weight[i];
                write(passenger_pipes[i][1], &pw, sizeof(PassengerWeight));
                exit(EXIT_SUCCESS);
            } else if (passenger_pids[i] < 0) {
                perror("fork");
                exit(EXIT_FAILURE);
            }

            close(passenger_pipes[i][1]); // Close write end of pipe
            PassengerWeight pr;
            read(passenger_pipes[i][0], &pr, sizeof(PassengerWeight));
            total_weight += pr.luggage_weight + pr.passenger_weight;
            close(passenger_pipes[i][0]); // Close read end of pipe
            waitpid(passenger_pids[i], NULL, 0); // Wait for passenger process to end
        }

        total_weight += (CREW_WEIGHT * 7); // 2 pilots + 5 cabin crew
        plane_details.total_weight = total_weight;
    } else { // Cargo plane
        // Get number of cargo items
        printf("Enter Number of Cargo Items: ");
        scanf("%d", &num_cargo_items);

        // Get average weight of cargo items
        printf("Enter Average Weight of Cargo Items: ");
        scanf("%d", &avg_cargo_weight);

        // Calculate total weight of cargo plane
        total_weight += (num_cargo_items * avg_cargo_weight) + (CREW_WEIGHT * 2); // 2 pilots
        plane_details.total_weight = total_weight;
    }
    // Get departure and arrival airports
    printf("Enter Airport Number for Departure: ");
    scanf("%d", &dep_airport);
    plane_details.dep_airport = dep_airport;
    printf("Enter Airport Number for Arrival: ");
    scanf("%d", &arr_airport);
    plane_details.arr_airport = arr_airport;

    msgid = msgget((key_t)12345, 0644 | IPC_CREAT);
    if (msgid == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }
    plane_details.mtype = 11;
    if (msgsnd(msgid, (void *)&plane_details, sizeof(plane_details), 0) == -1) {
        perror("msgsnd");
        exit(EXIT_FAILURE);
    }
    while(1){
        if (msgrcv(msgid, (void*)&plane_details, sizeof(PlaneDetails), 12, 0) == -1) {
            perror("msgrcv");
            exit(EXIT_FAILURE);
        }
        else{
            printf("Plane %d has successfully traveled from Airport %d to Airport %d!\n", plane_id, dep_airport, arr_airport);
            break;        
        }
    }
    if (plane_type == 1) {
        for (i = 0; i < num_passengers; i++) {
            close(passenger_pipes[i][0]);
            close(passenger_pipes[i][1]);
        }
    }
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/msg.h>

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
    char choice;
    int msgid;
    // Set up message queue
    msgid = msgget((key_t)12345, 0644 | IPC_CREAT);
    if (msgid == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }
    while(1){
    printf("Do you want the Air Traffic Control System to terminate?(Y for Yes and N for No) ");
        scanf(" %c", &choice);
        PlaneDetails plane_details;
        if (choice == 'Y' || choice == 'y') {
         plane_details.mtype=13;
         if (msgsnd(msgid, &plane_details, sizeof(PlaneDetails), 0) == -1) {
             perror("msgsnd");
             exit(EXIT_FAILURE);
         } else {
             printf("Message sent to the ATC to terminate!\n");
         }
         break;
        }
    }
    return 0;
}



        // if (msgrcv(msgid, (void *)&plane_details, sizeof(plane_details), 13, IPC_NOWAIT) != -1) {
        //     printf("Termination Request Received!\nStarting Termination process...\n");
        //     for(int i=1;i<=num_airports;i++)
        //     {
        //         pthread_join(plane_thread, NULL);
        //         plane_details->mtype=30+i;
        //         if (msgsnd(msgid, (void *)&plane_details, sizeof(PlaneDetails), 0) == -1) {
        //             printf("Error in sending message to the Airport for Termination.\n");
        //             exit(1);
        //         }
        //     }
        //     break;
        // }

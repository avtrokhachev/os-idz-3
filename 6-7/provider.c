#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <stdbool.h>


#define BUFFER_SIZE 1024
#define NUMBER_OF_SMOKERS 3
#define NUMBER_OF_LOGGERS 1


char buffer[BUFFER_SIZE];
char output_buffer[BUFFER_SIZE];


// курильщик
struct Smoker {
    int id;
    int socket;
};

// логгировщик системы
struct Logger {
    int id;
    int socket;
};


// валидирует размер кол-во входных аргументов
void validate_input_arguments(int argc) {
    if (argc != 2) {
        printf("Expected 1 argument\n");
        exit(1);
    }
}


// отправляет сообщение на клиент
void send_message(int sock, char* msg) {
    if (send(sock, msg, strlen(msg), 0) < 0) {
        printf("Error occured while trying to send message.\n");
        exit(1);
    }
}


int main(int argc, char *argv[]) {
    validate_input_arguments(argc);

    // создаю сокет для сервера
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        printf("Error occured while creating socket\n");
        exit(1);
    }

    // настраиваю сокет для сервера
    int options = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &options, sizeof(options))) {
        printf("Error occured while trying to set up socket\n");
        exit(1);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(atoi(argv[1]));
    if (bind(server_socket, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
        printf("Error occured while trying to bind socket\n");
        exit(1);
    }
    if (listen(server_socket, NUMBER_OF_SMOKERS) < 0) {
        printf("Error occured while trying listen socket\n");
        exit(1);
    }
    printf("Server successfully started working on port %d\n", atoi(argv[1]));

    // ждем, пока подключатся все 3 курильщика для начала работы
    // и когда подключится логгер
    int cnt_smokers = 0;
    int cnt_loggers = 0;
    Smoker smokers[NUMBER_OF_SMOKERS];
    Logger loggers[NUMBER_OF_LOGGERS];
    while (cnt_smokers + cnt_loggers < NUMBER_OF_SMOKERS + NUMBER_OF_LOGGERS) {
        struct sockaddr_in client_addr;
        socklen_t client_length = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr*) &client_addr, &client_length);
        if (client_socket < 0) {
            printf("Error occured while trying to accept socket.\n");
            exit(1);
        }
        printf("Successfully got incoming connection\n");

        memset(buffer, 0, BUFFER_SIZE);
        if (recv(client_socket, buffer, BUFFER_SIZE - 1, 0) < 0) {
            printf("Error occured while trying to read message.\n");
            exit(1);
        }
        printf("Successfully read initial message from client\n");

        int message;
        if (sscanf(buffer, "%d", &message) != 1) {
            printf("Error occured while trying to parse client message.\n");
            close(client_socket);
            continue;
        }

        if (message < 3) {
            int smoker_item_id = message;
            smokers[smoker_item_id].id = smoker_item_id;
            smokers[smoker_item_id].socket = client_socket;
            ++cnt_smokers;
            printf("Successfully added smoker id=%d\n", smoker_item_id);
        }
        else {
            loggers[message - 3].id = message - 3;
            loggers[message - 3].socket = client_socket;
            ++cnt_loggers;
            printf("Successfully added logger id=%d\n", message - 3);
        }
        sleep(1);
    }

    printf("All smokers connected to the server\n");
    send_message(loggers[0].socket, "All smokers connected to the server\n");

    // основной цикл работы сервера
    while (true) {
        // сгенерировать новый предмет на столе и отправиль сообщения всем курильщикам об этом событии
        int smoke_item_id = rand() % 3;
        printf("Server generated smoke_item id=%d\n", smoke_item_id);
        sprintf(output_buffer, "Server generated smoke_item id=%d\n", smoke_item_id);
        send_message(loggers[0].socket, output_buffer);
        memset(buffer, 0, BUFFER_SIZE);
        sprintf(buffer, "%d", smoke_item_id);
        for (int i = 0; i < NUMBER_OF_SMOKERS; ++i) {
            send_message(smokers[i].socket, buffer);
        }
        sprintf(output_buffer, "Successfully sent messages to all smokers about smoke_item_id=%d\n", smoke_item_id);
        send_message(loggers[0].socket, output_buffer);
        printf("Successfully sent messages to all smokers about smoke_item_id=%d\n", smoke_item_id);

        // ждать ответа от курильщика, когда он закурит
        int smoking_smoker_id = -1;
        while (smoking_smoker_id == -1) {
            for (int i = 0; i < NUMBER_OF_SMOKERS; ++i) {
                memset(buffer, 0, BUFFER_SIZE);
                if (recv(smokers[i].socket, buffer, BUFFER_SIZE - 1, 0) < 0) {
                    printf("Error occured while trying to get client message.\n");
                    exit(1);
                }
                sprintf(output_buffer, "Successfully got message from the smoker id=%d\n", smokers[i].id);
                send_message(loggers[0].socket, output_buffer);
                printf("Successfully got message from the smoker id=%d\n", smokers[i].id);

                if (strstr(buffer, "smoking") != NULL) {
                    smoking_smoker_id = smokers[i].id;
                    break;
                }
            }
            sleep(1);
        }
        sprintf(output_buffer, "Smoker id=%d got component and start smoking\n", smoking_smoker_id);
        send_message(loggers[0].socket, output_buffer);
        printf("Smoker id=%d got component and start smoking\n", smoking_smoker_id);

        int time_to_sleep = 2 + (rand() % 4);
        sleep(time_to_sleep);
    }

    for (int i = 0; i < NUMBER_OF_SMOKERS; ++i) {
        close(smokers[i].socket);
    }
    close(server_socket);
    return 0;
}
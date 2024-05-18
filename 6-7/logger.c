#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024  // размер буфера

char buffer[BUFFER_SIZE];


// сервер, хранит в себе ip и порт
struct Server {
    char* ip;
    int port;
};

// валидирует размер кол-во входных аргументов
void validate_input_arguments(int argc) {
    if (argc != 3) {
        printf("Expected 2 arguments\n");
        exit(1);
    }
}

// отправляет сообщение на сервер
void send_message_to_server(int sock, char* msg) {
    if (send(sock, msg, strlen(msg), 0) < 0) {
        printf("Error occured while trying to send message.\n");
        exit(1);
    }
}


int main(int argc, char *argv[]) {
    validate_input_arguments(argc);

    // сервер, куда отправлять запросы
    Server server;
    server.ip = argv[1];
    server.port = atoi(argv[2]);

    printf("Logger started working\n");

    // устанавливаю соединение с сервером для отправки сообщений
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        printf("Error occured while creating socket\n");
        exit(1);
    }
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server.ip);
    server_addr.sin_port = htons(server.port);
    if (connect(client_socket, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
        printf("Error occured while trying to connect\n");
        exit(1);
    }

    // соединение с сервером успешно установлено
    printf("Logger successfully connected to the server %s:%d\n", server.ip, server.port);

    // отправляю сообщение на сервер с id нужного компонента, чтобы сервер знал про логгер
    memset(buffer, 0, BUFFER_SIZE);
    sprintf(buffer, "%d", 3);
    send_message_to_server(client_socket, buffer);
    printf("Logger sent first message to the server\n");

    while (true) {
        // читаю сообщение от серсера
        memset(buffer, 0, BUFFER_SIZE);
        if (recv(client_socket, buffer, BUFFER_SIZE - 1, 0) < 0) {
            printf("Error while reading message from server\n");
            exit(1);
        }
        printf("Logger got message from server: %s\n", buffer);

        sleep(1);
    }

    close(client_socket);
    return 0;
}
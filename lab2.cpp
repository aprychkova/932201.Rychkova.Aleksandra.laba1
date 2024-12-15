#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>
#include <vector>

#define PORT 8080
#define BACKLOG 1 // Нам нужно только одно подключение
//Объявление обработчика сигнала
volatile sig_atomic_t wasSigHup = 0; // Переменная для хранения состояния сигнала
void sigHupHandler(int r) {
    wasSigHup = 1;
}

// Функция для создания серверного сокета
int create_server_socket() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Ошибка при создании сокета");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Ошибка при привязке сокета");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("Ошибка при прослушивании сокета");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

int main() {
    int server_socket = create_server_socket();
    int client_socket = -1;
    fd_set fds;
    struct sigaction sa;
    sigset_t blockedMask, origSigMask;

    // Блокировка сигнала SIGHUP
    sigemptyset(&blockedMask);
    sigaddset(&blockedMask, SIGHUP);
    sigprocmask(SIG_BLOCK, &blockedMask, &origSigMask);

    // Регистрация обработчика сигнала SIGHUP
    sa.sa_handler = sigHupHandler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGHUP, NULL, &sa);
    sigaction(SIGHUP, &sa, NULL);

    // Вектор клиентов
    std::vector<int> clients;

    printf("Сервер слушает на порту %d\n", PORT);

    // Основной цикл
    while (1) {
        FD_ZERO(&fds);
        FD_SET(server_socket, &fds); // Добавляем серверный сокет

        // Добавление всех клиентских сокетов в список
        for (auto clientIt = clients.begin(); clientIt != clients.end(); clientIt++) {
            FD_SET(*clientIt, &fds);
        }

        // Вычисляем максимальный файловый дескриптор
        int max_fd = server_socket;
        for (auto clientIt = clients.begin(); clientIt != clients.end(); clientIt++) {
            if (*clientIt > max_fd) {
                max_fd = *clientIt;
            }
        }

        // Вызов pselect для ожидания событий
        int activity = pselect(max_fd + 1, &fds, NULL, NULL, NULL, &origSigMask);

        if (activity == -1) {
            if (errno == EINTR) {
                // Если сигнал был получен, обработаем его
                if (wasSigHup) {
                    printf("Обработан сигнал SIGHUP\n");
                    wasSigHup = 0; // Сброс флага сигнала
                }
            }
            else {
                perror("Ошибка при вызове pselect");
                break;
            }
        }

        // Проверка для нового соединения (серверный сокет)
        if (FD_ISSET(server_socket, &fds)) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int new_client = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
            if (new_client == -1) {
                perror("Ошибка при принятии соединения");
                continue;
            }

            // Добавляем новое соединение в список клиентов
            clients.push_back(new_client);
            printf("Новое соединение установлено\n");
        }

        // Проверка для активных соединений с клиентами
        for (auto clientIt = clients.begin(); clientIt != clients.end(); clientIt++) {
            if (FD_ISSET(*clientIt, &fds)) {
                char buffer[1024];
                ssize_t bytes_received = recv(*clientIt, buffer, sizeof(buffer), 0);
                if (bytes_received > 0) {
                    printf("Получено %ld байт от клиента\n", bytes_received);
                }
                else if (bytes_received == 0) {
                    printf("Соединение с клиентом закрыто\n");
                    close(*clientIt);
                    clientIt = clients.erase(clientIt);
                }
                else {
                    perror("Ошибка при получении данных");
                    close(*clientIt);
                    clientIt = clients.erase(clientIt);
                }
            }
        }
    }

    // Закрытие сокетов
    close(server_socket);
    return 0;
}

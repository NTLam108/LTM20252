#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>
#include <time.h>

#define PORT 8080
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024

struct client
{
    int fd;
    char id[50];
    int registered;
};

struct client clients[MAX_CLIENTS];

void get_time_str(char *buffer)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, 64, "%Y/%m/%d %I:%M:%S%p", t);
}

int parse_client_id(char *msg, char *id)
{
    // format: "client_id: client_name"
    char *colon = strchr(msg, ':');
    if (!colon)
        return 0;

    int len = colon - msg;
    strncpy(id, msg, len);
    id[len] = '\0';

    return 1;
}

void broadcast(int sender_fd, char *msg)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].fd != -1 && clients[i].fd != sender_fd && clients[i].registered)
        {
            send(clients[i].fd, msg, strlen(msg), 0);
        }
    }
}

int main()
{
    int server_fd;
    struct sockaddr_in server_addr;

    // init clients
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i].fd = -1;
    }

    // create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_fd, 10);

    struct pollfd fds[MAX_CLIENTS + 1];

    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    for (int i = 1; i <= MAX_CLIENTS; i++)
    {
        fds[i].fd = -1;
    }

    printf("Server started on port %d...\n", PORT);

    while (1)
    {
        int ret = poll(fds, MAX_CLIENTS + 1, -1);

        if (ret < 0)
        {
            perror("poll");
            break;
        }

        // check new connection
        if (fds[0].revents & POLLIN)
        {
            int client_fd = accept(server_fd, NULL, NULL);

            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (clients[i].fd == -1)
                {
                    clients[i].fd = client_fd;
                    clients[i].registered = 0;

                    fds[i + 1].fd = client_fd;
                    fds[i + 1].events = POLLIN;

                    send(client_fd,
                         "Nhap: client_id: client_name\n",
                         strlen("Nhap: client_id: client_name\n"),
                         0);
                    break;
                }
            }
        }

        // check clients
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (fds[i + 1].fd != -1 && (fds[i + 1].revents & POLLIN))
            {
                char buffer[BUFFER_SIZE];
                int n = recv(fds[i + 1].fd, buffer, BUFFER_SIZE - 1, 0);

                if (n <= 0)
                {
                    close(fds[i + 1].fd);
                    clients[i].fd = -1;
                    fds[i + 1].fd = -1;
                    continue;
                }

                buffer[n] = '\0';

                // chưa đăng ký
                if (!clients[i].registered)
                {
                    char id[50];
                    if (parse_client_id(buffer, id))
                    {
                        strcpy(clients[i].id, id);
                        clients[i].registered = 1;

                        send(clients[i].fd,
                             "Dang ky thanh cong!\n",
                             strlen("Dang ky thanh cong!\n"),
                             0);
                    }
                    else
                    {
                        send(clients[i].fd, "Sai format!\n", 13, 0);
                    }
                }
                else
                {
                    // broadcast
                    char time_str[64];
                    get_time_str(time_str);

                    char msg[BUFFER_SIZE];
                    int used = snprintf(msg, sizeof(msg), "%s %s: ",
                                        time_str, clients[i].id);

                    if (used < sizeof(msg))
                    {
                        snprintf(msg + used, sizeof(msg) - used, "%s", buffer);
                    }

                    broadcast(clients[i].fd, msg);
                }
            }
        }
    }

    close(server_fd);
    return 0;
}
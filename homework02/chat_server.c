#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/select.h>

#define PORT 8888
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

typedef struct
{
    int fd;
    char client_id[50];
    int registered;
} Client;

void broadcast(Client clients[], int sender_idx, char *message, int num_clients)
{
    char timestamp[20];
    time_t now = time(NULL);
    strftime(timestamp, sizeof(timestamp), "%Y/%m/%d %I:%M:%S%p", localtime(&now));

    char final_msg[BUFFER_SIZE + 100];
    sprintf(final_msg, "%s %s: %s", timestamp, clients[sender_idx].client_id, message);

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].fd != -1 && clients[i].registered && i != sender_idx)
        {
            send(clients[i].fd, final_msg, strlen(final_msg), 0);
        }
    }
}

int main()
{
    int listen_fd, new_fd, max_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    fd_set master_set, read_fds;
    Client clients[MAX_CLIENTS];

    // Khởi tạo danh sách client
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i].fd = -1;
        clients[i].registered = 0;
    }

    // Tạo socket
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(listen_fd, 5);

    FD_ZERO(&master_set);
    FD_SET(listen_fd, &master_set);
    max_fd = listen_fd;

    printf("Chat Server started on port %d...\n", PORT);

    while (1)
    {
        read_fds = master_set;
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0)
        {
            perror("select error");
            exit(1);
        }

        for (int i = 0; i <= max_fd; i++)
        {
            if (FD_ISSET(i, &read_fds))
            {
                if (i == listen_fd)
                {
                    // Chấp nhận kết nối mới
                    new_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addr_len);

                    int joined = 0;
                    for (int j = 0; j < MAX_CLIENTS; j++)
                    {
                        if (clients[j].fd == -1)
                        {
                            clients[j].fd = new_fd;
                            FD_SET(new_fd, &master_set);
                            if (new_fd > max_fd)
                                max_fd = new_fd;

                            char *welcome = "Vui long nhap ten theo cu phap 'client_id: client_name':\n";
                            send(new_fd, welcome, strlen(welcome), 0);
                            joined = 1;
                            break;
                        }
                    }
                    if (!joined)
                        close(new_fd);
                }
                else
                {
                    // Nhận dữ liệu từ client hiện có
                    char buf[BUFFER_SIZE];
                    int nbytes = recv(i, buf, sizeof(buf) - 1, 0);

                    int c_idx = -1;
                    for (int j = 0; j < MAX_CLIENTS; j++)
                    {
                        if (clients[j].fd == i)
                        {
                            c_idx = j;
                            break;
                        }
                    }

                    if (nbytes <= 0)
                    {
                        // Client ngắt kết nối
                        close(i);
                        FD_CLR(i, &master_set);
                        clients[c_idx].fd = -1;
                        clients[c_idx].registered = 0;
                    }
                    else
                    {
                        buf[nbytes] = '\0';
                        // Loại bỏ ký tự xuống dòng nếu có
                        buf[strcspn(buf, "\r\n")] = 0;

                        if (!clients[c_idx].registered)
                        {
                            // Xử lý đăng ký tên
                            char name[50];
                            if (sscanf(buf, "client_id: %s", name) == 1)
                            {
                                strcpy(clients[c_idx].client_id, name);
                                clients[c_idx].registered = 1;
                                char *ok = "Dang ky thanh cong! Bat dau chat.\n";
                                send(i, ok, strlen(ok), 0);
                            }
                            else
                            {
                                char *retry = "Sai cu phap. Nhap lai (client_id: client_name):\n";
                                send(i, retry, strlen(retry), 0);
                            }
                        }
                        else
                        {
                            // Chuyển tiếp tin nhắn
                            broadcast(clients, c_idx, buf, MAX_CLIENTS);
                        }
                    }
                }
            }
        }
    }
    return 0;
}
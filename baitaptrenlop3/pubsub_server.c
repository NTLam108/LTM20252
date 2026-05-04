#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>

#define PORT 9000
#define MAX_CLIENTS 64
#define MAX_TOPICS 10
#define BUFFER_SIZE 1024

typedef struct
{
    int fd;
    char topics[MAX_TOPICS][50];
    int topic_count;
} ClientInfo;

ClientInfo clients[MAX_CLIENTS];

// Hàm xóa bỏ mọi ký tự xuống dòng rác (\r \n) ở cuối chuỗi
void trim_newline(char *str)
{
    int len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r' || str[len - 1] == ' '))
    {
        str[len - 1] = '\0';
        len--;
    }
}

int main()
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    char buffer[BUFFER_SIZE];
    struct pollfd fds[MAX_CLIENTS + 1];

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i].fd = -1;
        clients[i].topic_count = 0;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 10);

    fds[0].fd = server_fd;
    fds[0].events = POLLIN;
    for (int i = 1; i <= MAX_CLIENTS; i++)
        fds[i].fd = -1;

    printf("Server chạy trên cổng %d...\n", PORT);

    while (1)
    {
        if (poll(fds, MAX_CLIENTS + 1, -1) < 0)
            break;

        if (fds[0].revents & POLLIN)
        {
            new_socket = accept(server_fd, NULL, NULL);
            for (int i = 1; i <= MAX_CLIENTS; i++)
            {
                if (fds[i].fd == -1)
                {
                    fds[i].fd = new_socket;
                    fds[i].events = POLLIN;
                    clients[i - 1].fd = new_socket;
                    clients[i - 1].topic_count = 0;
                    break;
                }
            }
        }

        for (int i = 1; i <= MAX_CLIENTS; i++)
        {
            if (fds[i].fd > 0 && (fds[i].revents & POLLIN))
            {
                int n = recv(fds[i].fd, buffer, BUFFER_SIZE - 1, 0);
                if (n <= 0)
                {
                    close(fds[i].fd);
                    fds[i].fd = -1;
                    clients[i - 1].fd = -1;
                }
                else
                {
                    buffer[n] = '\0';
                    trim_newline(buffer); // Làm sạch chuỗi ngay lập tức

                    // 1. SUB <topic>
                    if (strncmp(buffer, "SUB ", 4) == 0)
                    {
                        char *t = buffer + 4;
                        while (*t == ' ')
                            t++; // Bỏ qua khoảng trắng dư thừa

                        if (clients[i - 1].topic_count < MAX_TOPICS)
                        {
                            // Lưu vào danh sách mảng 2 chiều
                            strcpy(clients[i - 1].topics[clients[i - 1].topic_count], t);
                            clients[i - 1].topic_count++;
                            send(fds[i].fd, "OK SUB\n", 7, 0);
                        }
                    }
                    // --- XỬ LÝ UNSUB <topic> ---
                    else if (strncmp(buffer, "UNSUB ", 6) == 0)
                    {
                        char *topic_to_del = buffer + 6;
                        while (*topic_to_del == ' ')
                            topic_to_del++; // Loại bỏ khoảng trắng thừa nếu có

                        int found = 0;
                        // Duyệt danh sách các topic hiện có của client i-1
                        for (int k = 0; k < clients[i - 1].topic_count; k++)
                        {
                            if (strcmp(clients[i - 1].topics[k], topic_to_del) == 0)
                            {
                                // Xóa bằng cách ghi đè phần tử cuối cùng của danh sách vào vị trí này
                                int last_index = clients[i - 1].topic_count - 1;
                                strcpy(clients[i - 1].topics[k], clients[i - 1].topics[last_index]);

                                clients[i - 1].topic_count--; // Giảm số lượng topic đang quản lý
                                found = 1;
                                break;
                            }
                        }

                        if (found)
                        {
                            send(fds[i].fd, "OK UNSUB\n", 9, 0);
                            printf("Client %d đã hủy đăng ký topic: %s\n", fds[i].fd, topic_to_del);
                        }
                        else
                        {
                            send(fds[i].fd, "ERR - Topic not found\n", 22, 0);
                        }
                    }
                    // 2. PUB <topic> <msg>
                    else if (strncmp(buffer, "PUB ", 4) == 0)
                    {
                        char *temp = buffer + 4;
                        char *pub_topic = strtok(temp, " ");
                        char *msg = strtok(NULL, ""); // Lấy toàn bộ phần còn lại làm tin nhắn

                        if (pub_topic && msg)
                        {
                            for (int j = 0; j < MAX_CLIENTS; j++)
                            {
                                if (clients[j].fd > 0)
                                {
                                    // Duyệt qua TẤT CẢ topic mà client j đã đăng ký
                                    for (int k = 0; k < clients[j].topic_count; k++)
                                    {
                                        if (strcmp(clients[j].topics[k], pub_topic) == 0)
                                        {
                                            send(clients[j].fd, msg, strlen(msg), 0);
                                            send(clients[j].fd, "\n", 1, 0);
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
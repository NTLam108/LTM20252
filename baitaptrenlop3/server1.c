#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>
#include <ctype.h>

#define PORT 8080
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024

// Hàm mã hóa theo quy tắc
void encrypt_string(char *str)
{
    for (int i = 0; str[i] != '\0'; i++)
    {
        if (isalpha(str[i]))
        {
            if (str[i] == 'Z')
                str[i] = 'A';
            else if (str[i] == 'z')
                str[i] = 'a';
            else
                str[i] = str[i] + 1;
        }
        else if (isdigit(str[i]))
        {
            str[i] = (9 - (str[i] - '0')) + '0';
        }
    }
}

int main()
{
    int server_fd, new_socket, client_count = 0;
    struct sockaddr_in address;
    int opt = 1;
    char buffer[BUFFER_SIZE];

    // Mảng cấu trúc pollfd để quản lý các socket
    struct pollfd fds[MAX_CLIENTS + 1];

    // 1. Khởi tạo Socket Server
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    // 2. Thiết lập poll cho server_fd
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    // Khởi tạo các phần tử còn lại là -1 (trống)
    for (int i = 1; i <= MAX_CLIENTS; i++)
        fds[i].fd = -1;

    printf("Server poll đang chạy trên port %d...\n", PORT);

    while (1)
    {
        int poll_count = poll(fds, MAX_CLIENTS + 1, -1);
        if (poll_count < 0)
        {
            perror("Poll error");
            break;
        }

        // Kiểm tra xem có kết nối mới không
        if (fds[0].revents & POLLIN)
        {
            new_socket = accept(server_fd, NULL, NULL);
            client_count++;

            sprintf(buffer, "Xin chào. Hiện có %d clients đang kết nối.\n", client_count);
            send(new_socket, buffer, strlen(buffer), 0);

            // Thêm socket mới vào mảng pollfd
            for (int i = 1; i <= MAX_CLIENTS; i++)
            {
                if (fds[i].fd == -1)
                {
                    fds[i].fd = new_socket;
                    fds[i].events = POLLIN;
                    break;
                }
            }
        }

        // Kiểm tra dữ liệu từ các client hiện có
        for (int i = 1; i <= MAX_CLIENTS; i++)
        {
            if (fds[i].fd > 0 && (fds[i].revents & POLLIN))
            {
                int nbytes = recv(fds[i].fd, buffer, sizeof(buffer) - 1, 0);

                if (nbytes <= 0)
                {
                    // Client đóng kết nối hoặc lỗi
                    close(fds[i].fd);
                    fds[i].fd = -1;
                    client_count--;
                }
                else
                {
                    buffer[nbytes] = '\0';
                    // Loại bỏ ký tự xuống dòng
                    buffer[strcspn(buffer, "\r\n")] = 0;

                    if (strcmp(buffer, "exit") == 0)
                    {
                        char *msg = "Tạm biệt!\n";
                        send(fds[i].fd, msg, strlen(msg), 0);
                        close(fds[i].fd);
                        fds[i].fd = -1;
                        client_count--;
                    }
                    else
                    {
                        encrypt_string(buffer);
                        strcat(buffer, "\n");
                        send(fds[i].fd, buffer, strlen(buffer), 0);
                    }
                }
            }
        }
    }

    close(server_fd);
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Sử dụng: %s <Địa chỉ IP> <Cổng>\n", argv[0]);
        return 1;
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);

    // 1. Tạo socket
    int client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock < 0)
    {
        perror("Lỗi tạo socket");
        exit(1);
    }

    // 2. Thiết lập cấu trúc địa chỉ server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0)
    {
        perror("Địa chỉ IP không hợp lệ");
        close(client_sock);
        exit(1);
    }

    // 3. Kết nối đến server
    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Kết nối thất bại");
        close(client_sock);
        exit(1);
    }

    char welcome_msg[BUFFER_SIZE];
    int n = recv(client_sock, welcome_msg, sizeof(welcome_msg) - 1, 0);
    if (n > 0)
    {
        welcome_msg[n] = '\0'; // Kết thúc xâu
        printf("Server chào: %s\n", welcome_msg);
    }

    printf("Đã kết nối tới server %s:%d\n", ip, port);
    printf("Nhập tin nhắn (Ctrl+C để thoát):\n");

    char buffer[BUFFER_SIZE];
    while (1)
    {
        printf("> ");
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL)
            break;

        // Gửi dữ liệu tới server
        int bytes_sent = send(client_sock, buffer, strlen(buffer), 0);
        if (bytes_sent < 0)
        {
            perror("Lỗi gửi dữ liệu");
            break;
        }
    }

    close(client_sock);
    return 0;
}
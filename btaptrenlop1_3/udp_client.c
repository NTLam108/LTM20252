#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define PORT 9090
#define BUF_SIZE 1024

int main()
{
    int sockfd;
    char buffer[BUF_SIZE];
    char echo_msg[BUF_SIZE];
    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    while (1)
    {
        printf("Nhập tin nhắn: ");
        fgets(buffer, BUF_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0; // Xóa ký tự xuống dòng

        // Gửi sang server
        sendto(sockfd, buffer, strlen(buffer), 0,
               (const struct sockaddr *)&server_addr, sizeof(server_addr));

        // Nhận phản hồi
        int n = recvfrom(sockfd, echo_msg, BUF_SIZE, 0, NULL, NULL);
        echo_msg[n] = '\0';
        printf("Server phản hồi: %s\n", echo_msg);
    }

    close(sockfd);
    return 0;
}
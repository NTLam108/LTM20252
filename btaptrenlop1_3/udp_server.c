#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 9090
#define BUF_SIZE 1024

int main()
{
    int sockfd;
    char buffer[BUF_SIZE];
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // 1. Tạo socket UDP (SOCK_DGRAM)
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 2. Bind socket với port
    bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr));

    printf("UDP Echo Server đang chạy trên port %d...\n", PORT);

    while (1)
    {
        // 3. Nhận dữ liệu từ client
        int n = recvfrom(sockfd, buffer, BUF_SIZE, 0,
                         (struct sockaddr *)&client_addr, &addr_len);
        buffer[n] = '\0';

        printf("Nhận từ %s:%d: %s\n", inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port), buffer);

        // 4. Phản hồi lại chính dữ liệu đó (Echo)
        sendto(sockfd, buffer, n, 0,
               (const struct sockaddr *)&client_addr, addr_len);
    }

    close(sockfd);
    return 0;
}
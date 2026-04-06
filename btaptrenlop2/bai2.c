#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("Sử dụng: %s <port_s> <ip_d> <port_d>\n", argv[0]);
        exit(1);
    }

    int port_s = atoi(argv[1]);
    char *ip_d = argv[2];
    int port_d = atoi(argv[3]);

    int sockfd;
    struct sockaddr_in servaddr, destaddr;
    char buffer[BUF_SIZE];

    // 1. Tạo Socket UDP
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Tạo socket thất bại");
        exit(EXIT_FAILURE);
    }

    // 2. Thiết lập địa chỉ nhận (Local)
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port_s);

    // Bind socket để nhận dữ liệu trên port_s
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("Bind thất bại");
        exit(EXIT_FAILURE);
    }

    // 3. Thiết lập địa chỉ gửi (Destination)
    memset(&destaddr, 0, sizeof(destaddr));
    destaddr.sin_family = AF_INET;
    destaddr.sin_port = htons(port_d);
    if (inet_pton(AF_INET, ip_d, &destaddr.sin_addr) <= 0)
    {
        printf("Địa chỉ IP đích không hợp lệ\n");
        exit(EXIT_FAILURE);
    }

    printf("UDP Chat đang chạy... (Cổng nhận: %d, Đích: %s:%d)\n", port_s, ip_d, port_d);
    printf("Nhập tin nhắn để gửi:\n");

    fd_set readfds;
    int maxfd = (sockfd > STDIN_FILENO) ? sockfd : STDIN_FILENO;

    while (1)
    {
        // Xóa và thiết lập lại tập hợp file descriptors để select theo dõi
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds); // Theo dõi bàn phím
        FD_SET(sockfd, &readfds);       // Theo dõi socket (dữ liệu đến)

        // Chờ đợi sự kiện trên một trong các file descriptors
        int activity = select(maxfd + 1, &readfds, NULL, NULL, NULL);

        if (activity < 0)
        {
            perror("Lỗi select");
            break;
        }

        // TRƯỜNG HỢP 1: Có dữ liệu từ bàn phím (Gửi đi)
        if (FD_ISSET(STDIN_FILENO, &readfds))
        {
            fgets(buffer, BUF_SIZE, stdin);
            sendto(sockfd, buffer, strlen(buffer), 0,
                   (const struct sockaddr *)&destaddr, sizeof(destaddr));
        }

        // TRƯỜNG HỢP 2: Có dữ liệu từ mạng gửi đến (Nhận về)
        if (FD_ISSET(sockfd, &readfds))
        {
            struct sockaddr_in src_addr;
            socklen_t addr_len = sizeof(src_addr);
            int n = recvfrom(sockfd, buffer, BUF_SIZE, 0,
                             (struct sockaddr *)&src_addr, &addr_len);

            buffer[n] = '\0'; // Kết thúc chuỗi
            printf("\033[0;32m[Nhận từ %s]: %s\033[0m", inet_ntoa(src_addr.sin_addr), buffer);
        }
    }

    close(sockfd);
    return 0;
}
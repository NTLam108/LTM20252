#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("Sử dụng: %s <port> <remote_ip> <remote_port>\n", argv[0]);
        return 1;
    }

    int my_port = atoi(argv[1]);
    char *remote_ip = argv[2];
    int remote_port = atoi(argv[3]);

    int sockfd;
    struct sockaddr_in my_addr, remote_addr;
    char buffer[BUFFER_SIZE];

    // 1. Tạo UDP Socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 2. Thiết lập địa chỉ của mình và Bind vào cổng chờ (port)
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(my_port);

    if (bind(sockfd, (const struct sockaddr *)&my_addr, sizeof(my_addr)) < 0)
    {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // 3. Thiết lập địa chỉ máy nhận (remote)
    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(remote_port);
    if (inet_pton(AF_INET, remote_ip, &remote_addr.sin_addr) <= 0)
    {
        printf("Địa chỉ IP từ xa không hợp lệ.\n");
        close(sockfd);
        return 1;
    }

    // 4. Sử dụng poll để quản lý nhập liệu từ bàn phím và dữ liệu từ socket
    struct pollfd fds[2];

    // fds[0]: Theo dõi bàn phím
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    // fds[1]: Theo dõi Socket
    fds[1].fd = sockfd;
    fds[1].events = POLLIN;

    printf("Đang chờ trên cổng %d. Gửi đến %s:%d\n", my_port, remote_ip, remote_port);
    printf("Nhập tin nhắn và nhấn Enter để gửi:\n-------------------------------\n");

    while (1)
    {
        // chặn chương trình
        int ret = poll(fds, 2, -1);
        if (ret < 0)
        {
            perror("Poll error");
            break;
        }

        // Kiểm tra sự kiện từ bàn phím và gửi tin
        if (fds[0].revents & POLLIN)
        {
            if (fgets(buffer, BUFFER_SIZE, stdin))
            {
                // Gửi nội dung vừa nhập tới địa chỉ remote
                sendto(sockfd, buffer, strlen(buffer), 0,
                       (const struct sockaddr *)&remote_addr, sizeof(remote_addr));
            }
        }

        // Nhận tin và in ra màn hình
        if (fds[1].revents & POLLIN)
        {
            struct sockaddr_in src_addr;
            socklen_t addr_len = sizeof(src_addr);
            int n = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0,
                             (struct sockaddr *)&src_addr, &addr_len);
            if (n > 0)
            {
                buffer[n] = '\0';
                printf("[Nhận từ %s]: %s", inet_ntoa(src_addr.sin_addr), buffer);
            }
        }
    }

    close(sockfd);
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {AF_INET, htons(8080), inet_addr("127.0.0.1")};

    connect(sock, (struct sockaddr *)&addr, sizeof(addr));

    // Dữ liệu mẫu chia làm nhiều lần gửi
    char *data[] = {
        "SOICTSOICT012345678901234567890123456789012345",
        "6789SOICTSOICTSOICT012345678901234567890123456",
        "7890123456789012345678901234567890123456789012",
        "3456789SOICTSOICT01234567890123456789012345678"};

    for (int i = 0; i < 4; i++)
    {
        send(sock, data[i], strlen(data[i]), 0);
        usleep(100000); // Nghỉ một chút để giả lập truyền tải
    }

    close(sock);
    return 0;
}
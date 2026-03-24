#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define TARGET "0123456789"
#define TARGET_LEN 10
#define BUF_SIZE 1024

int count_occurrences(const char *haystack, int len)
{
    int count = 0;
    for (int i = 0; i <= len - TARGET_LEN; i++)
    {
        if (strncmp(&haystack[i], TARGET, TARGET_LEN) == 0)
        {
            count++;
        }
    }
    return count;
}

int main()
{
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {AF_INET, htons(8080), INADDR_ANY};
    bind(server_sock, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_sock, 5);

    int client_sock = accept(server_sock, NULL, NULL);

    char remainder[TARGET_LEN] = {0}; // Lưu 9 ký tự cuối của lần trước
    int remainder_len = 0;
    int total_count = 0;
    char buffer[BUF_SIZE];
    char process_buf[BUF_SIZE + TARGET_LEN];

    while (1)
    {
        int bytes_received = recv(client_sock, buffer, BUF_SIZE, 0);
        if (bytes_received <= 0)
            break;

        // 1. Ghép phần dư lần trước với dữ liệu mới
        memcpy(process_buf, remainder, remainder_len);
        memcpy(process_buf + remainder_len, buffer, bytes_received);
        int current_len = remainder_len + bytes_received;

        // 2. Đếm số lần xuất hiện
        int found = count_occurrences(process_buf, current_len);
        total_count += found;

        // 3. Cập nhật phần dư cho lần sau (lưu 9 ký tự cuối)
        if (current_len >= TARGET_LEN - 1)
        {
            remainder_len = TARGET_LEN - 1;
            memcpy(remainder, process_buf + current_len - remainder_len, remainder_len);
        }
        else
        {
            remainder_len = current_len;
            memcpy(remainder, process_buf, remainder_len);
        }

        printf("Số lần xuất hiện hiện tại: %d\n", total_count);
    }

    printf("\nTổng cộng tìm thấy: %d lần.\n", total_count);
    close(client_sock);
    close(server_sock);
    return 0;
}
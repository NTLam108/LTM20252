#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

int main()
{
    int server_sock, client_sock;
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(addr);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    bind(server_sock, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_sock, 5);

    printf("Server đang đợi kết nối...\n");
    client_sock = accept(server_sock, (struct sockaddr *)&addr, &addr_size);

    // 1. Nhận tên thư mục
    unsigned char path_len;
    recv(client_sock, &path_len, 1, 0);
    char path[256] = {0};
    recv(client_sock, path, path_len, 0);
    printf("Thư mục: %s\n", path);

    // 2. Nhận số lượng tập tin
    int file_count;
    recv(client_sock, &file_count, sizeof(int), 0);

    // 3. Nhận chi tiết từng file
    for (int i = 0; i < file_count; i++)
    {
        unsigned char name_len;
        recv(client_sock, &name_len, 1, 0);

        char file_name[256] = {0};
        recv(client_sock, file_name, name_len, 0);

        long file_size;
        recv(client_sock, &file_size, sizeof(long), 0);

        printf("%s – %ld bytes\n", file_name, file_size);
    }

    close(client_sock);
    close(server_sock);
    return 0;
}
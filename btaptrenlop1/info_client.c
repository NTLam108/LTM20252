#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#ifndef DT_REG
#define DT_REG 8
#endif

#define SERVER_IP "127.0.0.1"
#define PORT 8080

int main()
{
    int client_sock;
    struct sockaddr_in server_addr;
    char path[256];

    // Lấy thư mục hiện tại
    getcwd(path, sizeof(path));

    client_sock = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(PORT);

    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Kết nối thất bại");
        return 1;
    }

    // 1. Gửi độ dài và tên thư mục
    unsigned char path_len = strlen(path);
    send(client_sock, &path_len, 1, 0);
    send(client_sock, path, path_len, 0);

    // 2. Đếm và gửi số lượng tập tin
    int file_count = 0;
    DIR *d = opendir(".");
    struct dirent *dir;
    while ((dir = readdir(d)) != NULL)
    {
        if (dir->d_type == DT_REG)
            file_count++;
    }
    send(client_sock, &file_count, sizeof(int), 0);

    // 3. Gửi thông tin từng tập tin
    rewinddir(d);
    while ((dir = readdir(d)) != NULL)
    {
        if (dir->d_type == DT_REG)
        {
            unsigned char name_len = strlen(dir->d_name);
            struct stat st;
            stat(dir->d_name, &st);
            long file_size = st.st_size;

            send(client_sock, &name_len, 1, 0);             // 1 byte độ dài tên
            send(client_sock, dir->d_name, name_len, 0);    // Tên file
            send(client_sock, &file_size, sizeof(long), 0); // 8 bytes kích thước
        }
    }

    closedir(d);
    close(client_sock);
    printf("Đã gửi dữ liệu thành công.\n");
    return 0;
}
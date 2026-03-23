#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("Sử dụng: %s <Cổng> <File_Xau_Chao> <File_Ghi_Du_Lieu>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    char *hello_file = argv[2];
    char *output_file = argv[3];

    // 1. Tạo socket
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0)
    {
        perror("Lỗi tạo socket");
        exit(1);
    }

    // 2. Thiết lập địa chỉ và Bind
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Lỗi Bind");
        close(server_sock);
        exit(1);
    }

    // 3. Listen
    if (listen(server_sock, 5) < 0)
    {
        perror("Lỗi Listen");
        exit(1);
    }

    printf("Server đang đợi kết nối ở cổng %d...\n", port);

    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);

        if (client_sock < 0)
        {
            perror("Lỗi Accept");
            continue;
        }

        printf("Client kết nối từ: %s\n", inet_ntoa(client_addr.sin_addr));

        // 4. Gửi xâu chào từ file
        FILE *f_hello = fopen(hello_file, "r");
        if (f_hello != NULL)
        {
            char hello_buf[BUFFER_SIZE];
            while (fgets(hello_buf, BUFFER_SIZE, f_hello) != NULL)
            {
                send(client_sock, hello_buf, strlen(hello_buf), 0);
            }
            fclose(f_hello);
        }
        else
        {
            char *msg = "Chào mừng bạn đến với server!\n";
            send(client_sock, msg, strlen(msg), 0);
        }

        // 5. Nhận dữ liệu và ghi vào file output
        FILE *f_out = fopen(output_file, "a"); // "a" để ghi nối tiếp
        if (f_out == NULL)
        {
            perror("Không thể mở file ghi dữ liệu");
        }
        else
        {
            char buffer[BUFFER_SIZE];
            int bytes_received;
            while ((bytes_received = recv(client_sock, buffer, BUFFER_SIZE - 1, 0)) > 0)
            {
                buffer[bytes_received] = '\0';
                fprintf(f_out, "%s", buffer);
                fflush(f_out); // Đảm bảo dữ liệu được ghi ngay xuống đĩa
            }
            fclose(f_out);
        }

        printf("Client đã ngắt kết nối.\n");
        close(client_sock);
    }

    close(server_sock);
    return 0;
}
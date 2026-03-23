#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

typedef struct
{
    char mssv[20];
    char ho_ten[50];
    char ngay_sinh[12];
    float diem_tb;
} Student;

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Sử dụng: %s <Cổng> <File_Log>\n", argv[0]);
        return 1;
    }

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(atoi(argv[1]));

    bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_sock, 5);

    printf("Server sinh viên đang chạy ở cổng %s...\n", argv[1]);

    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);

        Student sv;
        int bytes_received = recv(client_sock, &sv, sizeof(Student), 0);
        if (bytes_received > 0)
        {
            // Lấy thời gian hiện tại
            time_t t = time(NULL);
            struct tm *tm_info = localtime(&t);
            char time_str[20];
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

            char *client_ip = inet_ntoa(client_addr.sin_addr);

            // In ra màn hình
            printf("%s %s %s %s %s %.2f\n", client_ip, time_str, sv.mssv, sv.ho_ten, sv.ngay_sinh, sv.diem_tb);

            // Ghi vào file log
            FILE *f = fopen(argv[2], "a");
            if (f)
            {
                fprintf(f, "%s %s %s %s %s %.2f\n", client_ip, time_str, sv.mssv, sv.ho_ten, sv.ngay_sinh, sv.diem_tb);
                fclose(f);
            }
        }
        close(client_sock);
    }
    return 0;
}
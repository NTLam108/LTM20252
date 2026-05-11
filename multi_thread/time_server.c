#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/wait.h>

#define PORT 8888
#define BUFFER_SIZE 1024

// Hàm xử lý tín hiệu để dọn dẹp tiến trình con (tránh zombie process)
void sigchld_handler(int s)
{
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

// Hàm lấy thời gian theo định dạng
void get_formatted_time(char *format, char *output)
{
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);

    if (strcmp(format, "dd/mm/yyyy") == 0)
    {
        strftime(output, BUFFER_SIZE, "%d/%m/%Y", tm_info);
    }
    else if (strcmp(format, "dd/mm/yy") == 0)
    {
        strftime(output, BUFFER_SIZE, "%d/%m/%y", tm_info);
    }
    else if (strcmp(format, "mm/dd/yyyy") == 0)
    {
        strftime(output, BUFFER_SIZE, "%m/%d/%Y", tm_info);
    }
    else if (strcmp(format, "mm/dd/yy") == 0)
    {
        strftime(output, BUFFER_SIZE, "%m/%d/%y", tm_info);
    }
    else
    {
        strcpy(output, "ERROR: Invalid format");
    }
}

void process_client(int client_sock)
{
    char buffer[BUFFER_SIZE];
    while (1)
    {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);

        if (bytes_received <= 0)
            break;

        // Xóa ký tự xuống dòng nếu có
        buffer[strcspn(buffer, "\r\n")] = 0;

        char cmd[20], fmt[50];
        // Kiểm tra tính đúng đắn của lệnh: GET_TIME [format]
        int count = sscanf(buffer, "%s %s", cmd, fmt);

        char response[BUFFER_SIZE];
        if (count == 2 && strcmp(cmd, "GET_TIME") == 0)
        {
            get_formatted_time(fmt, response);
        }
        else
        {
            strcpy(response, "ERROR: Invalid command syntax. Use: GET_TIME [format]");
        }

        strcat(response, "\n");
        send(client_sock, response, strlen(response), 0);
    }
    close(client_sock);
    exit(0);
}

int main()
{
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(client_addr);

    // Tạo socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind và Listen
    bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_sock, 10);

    // Thiết lập bộ dọn dẹp tiến trình con
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    printf("Time Server đang chạy tại port %d...\n", PORT);

    while (1)
    {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_size);
        if (client_sock < 0)
            continue;

        if (fork() == 0)
        { // Tiến trình con
            close(server_sock);
            process_client(client_sock);
        }
        else
        { // Tiến trình cha
            close(client_sock);
        }
    }

    close(server_sock);
    return 0;
}
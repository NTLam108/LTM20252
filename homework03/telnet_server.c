#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>

#define MAX_CLIENTS 64
#define BUFFER_SIZE 1024

typedef struct
{
    int fd;
    int authenticated; // 0: chưa, 1: đã login
    char username[64];
} ClientState;

// Hàm kiểm tra tài khoản từ file
int check_login(const char *user, const char *pass)
{
    FILE *f = fopen("databases.txt", "r");
    if (!f)
        return 0;

    char f_user[64], f_pass[64];
    while (fscanf(f, "%s %s", f_user, f_pass) != EOF)
    {
        if (strcmp(user, f_user) == 0 && strcmp(pass, f_pass) == 0)
        {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

// Hàm gửi nội dung file out.txt cho client
void send_file_content(int client_fd, const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (!f)
    {
        send(client_fd, "Error executing command\n", 24, 0);
        return;
    }
    char buffer[BUFFER_SIZE];
    while (fgets(buffer, sizeof(buffer), f))
    {
        send(client_fd, buffer, strlen(buffer), 0);
    }
    fclose(f);
}

int main()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {AF_INET, htons(8888), INADDR_ANY};

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 5);

    struct pollfd fds[MAX_CLIENTS];
    ClientState clients[MAX_CLIENTS];

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        fds[i].fd = -1;
        clients[i].authenticated = 0;
    }

    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    printf("Server listening on port 8888...\n");

    while (1)
    {
        int ret = poll(fds, MAX_CLIENTS, -1);
        if (ret < 0)
            break;

        // Chấp nhận kết nối mới
        if (fds[0].revents & POLLIN)
        {
            int client_fd = accept(server_fd, NULL, NULL);
            for (int i = 1; i < MAX_CLIENTS; i++)
            {
                if (fds[i].fd == -1)
                {
                    fds[i].fd = client_fd;
                    fds[i].events = POLLIN;
                    clients[i].authenticated = 0;
                    send(client_fd, "User: ", 6, 0);
                    break;
                }
            }
        }

        // Xử lý dữ liệu từ client
        for (int i = 1; i < MAX_CLIENTS; i++)
        {
            if (fds[i].fd != -1 && (fds[i].revents & POLLIN))
            {
                char buf[BUFFER_SIZE];
                int n = recv(fds[i].fd, buf, sizeof(buf) - 1, 0);

                if (n <= 0)
                {
                    close(fds[i].fd);
                    fds[i].fd = -1;
                    continue;
                }

                buf[n] = '\0';
                // Xóa ký tự xuống dòng (CRLF)
                strtok(buf, "\r\n");

                if (!clients[i].authenticated)
                {
                    // Xử lý đăng nhập đơn giản: nhận "user pass"
                    char user[64], pass[64];
                    if (sscanf(buf, "%s %s", user, pass) == 2)
                    {
                        if (check_login(user, pass))
                        {
                            clients[i].authenticated = 1;
                            send(fds[i].fd, "Login success! Enter command:\n", 30, 0);
                        }
                        else
                        {
                            send(fds[i].fd, "Wrong user/pass. Try again (user pass): ", 40, 0);
                        }
                    }
                    else
                    {
                        send(fds[i].fd, "Format: user pass\n", 18, 0);
                    }
                }
                else
                {
                    // Đã login -> Thực thi lệnh
                    char cmd[BUFFER_SIZE + 20];
                    sprintf(cmd, "%s > out.txt", buf);
                    system(cmd);

                    send_file_content(fds[i].fd, "out.txt");
                    send(fds[i].fd, "\nCommand executed. Next command: ", 33, 0);
                }
            }
        }
    }

    return 0;
}
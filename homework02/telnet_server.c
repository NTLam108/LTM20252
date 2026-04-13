#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>

#define PORT 8888
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

typedef struct
{
    int fd;
    int authenticated;
} Client;

// Hàm kiểm tra tài khoản từ file
int check_login(char *user, char *pass)
{
    FILE *f = fopen("accounts.txt", "r");
    if (f == NULL)
        return 0;

    char f_user[50], f_pass[50];
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

// Hàm gửi file kết quả cho client
void send_file_content(int client_fd, char *filename)
{
    FILE *f = fopen(filename, "r");
    if (f == NULL)
    {
        send(client_fd, "Error executing command.\n", 25, 0);
        return;
    }
    char buffer[BUFFER_SIZE];
    while (fgets(buffer, sizeof(buffer), f) != NULL)
    {
        send(client_fd, buffer, strlen(buffer), 0);
    }
    fclose(f);
}

int main()
{
    int listen_fd, max_fd;
    struct sockaddr_in server_addr;
    fd_set master_set, read_fds;
    Client clients[MAX_CLIENTS];

    for (int i = 0; i < MAX_CLIENTS; i++)
        clients[i].fd = -1;

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(listen_fd, 5);

    FD_ZERO(&master_set);
    FD_SET(listen_fd, &master_set);
    max_fd = listen_fd;

    printf("Telnet Server is running on port %d...\n", PORT);

    while (1)
    {
        read_fds = master_set;
        select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        for (int i = 0; i <= max_fd; i++)
        {
            if (FD_ISSET(i, &read_fds))
            {
                if (i == listen_fd)
                {
                    int new_fd = accept(listen_fd, NULL, NULL);
                    for (int j = 0; j < MAX_CLIENTS; j++)
                    {
                        if (clients[j].fd == -1)
                        {
                            clients[j].fd = new_fd;
                            clients[j].authenticated = 0;
                            FD_SET(new_fd, &master_set);
                            if (new_fd > max_fd)
                                max_fd = new_fd;
                            send(new_fd, "Welcome! Please login (user pass):\n", 35, 0);
                            break;
                        }
                    }
                }
                else
                {
                    char buf[BUFFER_SIZE];
                    int nbytes = recv(i, buf, sizeof(buf) - 1, 0);
                    if (nbytes <= 0)
                    {
                        close(i);
                        FD_CLR(i, &master_set);
                        for (int j = 0; j < MAX_CLIENTS; j++)
                            if (clients[j].fd == i)
                                clients[j].fd = -1;
                    }
                    else
                    {
                        buf[nbytes] = '\0';
                        buf[strcspn(buf, "\r\n")] = 0;

                        int c_idx = -1;
                        for (int j = 0; j < MAX_CLIENTS; j++)
                            if (clients[j].fd == i)
                                c_idx = j;

                        if (clients[c_idx].authenticated == 0)
                        {
                            char user[50], pass[50];
                            if (sscanf(buf, "%s %s", user, pass) == 2 && check_login(user, pass))
                            {
                                clients[c_idx].authenticated = 1;
                                send(i, "Login successful! Enter command:\n", 33, 0);
                            }
                            else
                            {
                                send(i, "Login failed. Try again (user pass):\n", 37, 0);
                            }
                        }
                        else
                        {
                            char cmd[BUFFER_SIZE + 50], outfile[20];
                            sprintf(outfile, "out%d.txt", i);
                            sprintf(cmd, "%s > %s 2>&1", buf, outfile);

                            system(cmd);
                            send_file_content(i, outfile);
                            unlink(outfile); // Xóa file tạm sau khi gửi
                            send(i, "\nDone. Next command:\n", 21, 0);
                        }
                    }
                }
            }
        }
    }
    return 0;
}
/*******************************************************************************
 * @file    non_blocking_server.c
 * @brief   Mô tả ngắn gọn về chức năng của file
 * @date    2026-03-31 07:10
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <ctype.h>

typedef enum
{
    STATE_WANT_NAME,
    STATE_WANT_MSSV,
    STATE_DONE
} ClientState;

typedef struct
{
    int fd;
    ClientState state;
    char fullname[100];
    char mssv[20];
} ClientContext;

void to_lower(char *str)
{
    for (; *str; ++str)
        *str = tolower(*str);
}
void generate_email(char *fullname, char *mssv, char *result)
{
    char name_parts[10][20];
    int count = 0;

    // Tách tên theo khoảng trắng
    char temp_name[100];
    strcpy(temp_name, fullname);
    char *token = strtok(temp_name, " \n\r");
    while (token != NULL && count < 10)
    {
        strcpy(name_parts[count++], token);
        token = strtok(NULL, " \n\r");
    }

    if (count < 2 || strlen(mssv) < 8)
    {
        strcpy(result, "Du lieu khong hop le!");
        return;
    }

    // 1. Lấy tên chính (phần tử cuối)
    char last_name[20];
    strcpy(last_name, name_parts[count - 1]);
    to_lower(last_name);

    // 2. Lấy chữ cái đầu của họ và tên đệm
    char initials[10] = "";
    for (int i = 0; i < count - 1; i++)
    {
        char first_char[2] = {tolower(name_parts[i][0]), '\0'};
        strcat(initials, first_char);
    }

    // 3. Xử lý MSSV (Lấy 2 số đầu của khóa và 4 số cuối)
    // Ví dụ: 20225204 -> 22 và 5204
    char year_part[3] = {mssv[2], mssv[3], '\0'};
    char id_part[5] = {mssv[4], mssv[5], mssv[6], mssv[7], '\0'};

    sprintf(result, "%s.%s%s%s@sis.hust.edu.vn", last_name, initials, year_part, id_part);
}

int main()
{
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1)
    {
        perror("socket() failed");
        return 1;
    }

    // Chuyen socket listener sang non-blocking
    unsigned long ul = 1;
    ioctl(listener, FIONBIO, &ul);

    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)))
    {
        perror("setsockopt() failed");
        close(listener);
        return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)))
    {
        perror("bind() failed");
        close(listener);
        return 1;
    }

    if (listen(listener, 5))
    {
        perror("listen() failed");
        close(listener);
        return 1;
    }

    // Server is now listening for incoming connections
    printf("Server is listening on port 8080...\n");

    ClientContext clients[64];
    int nclients = 0;

    char buf[256];
    int len;

    while (1)
    {
        // Chap nhan ket noi
        int client = accept(listener, NULL, NULL);
        if (client == -1)
        {
            if (errno == EWOULDBLOCK)
            {
                // Loi do dang cho ket noi
                // Bo qua
            }
            else
            {
                // Loi khac
            }
        }
        else
        {
            printf("New client accepted: %d\n", client);
            clients[nclients].fd = client;
            clients[nclients].state = STATE_WANT_NAME;
            nclients++;
            ul = 1;
            ioctl(client, FIONBIO, &ul);

            char *q1 = "Nhap Ho Ten của sinh vien: ";
            send(client, q1, strlen(q1), 0);
        }

        // Nhan du lieu tu cac client
        for (int i = 0; i < nclients; i++)
        {
            len = recv(clients[i].fd, buf, sizeof(buf) - 1, 0);
            if (len > 0)
            {
                buf[len] = 0;
                strtok(buf, "\n\r");
                if (clients[i].state == STATE_WANT_NAME)
                {
                    strcpy(clients[i].fullname, buf);
                    clients[i].state = STATE_WANT_MSSV;

                    char *q2 = "Nhap MSSV (8 chu so): ";
                    send(clients[i].fd, q2, strlen(q2), 0);
                }
                else if (clients[i].state == STATE_WANT_MSSV)
                {
                    strcpy(clients[i].mssv, buf);

                    char email[100];
                    generate_email(clients[i].fullname, clients[i].mssv, email);

                    char response[200];
                    sprintf(response, "=> Ket qua email la: %s\n", email);
                    send(clients[i].fd, response, strlen(response), 0);

                    // dong ket noi lai
                    close(clients[i].fd);
                    clients[i] = clients[nclients - 1];
                    nclients--;
                    i--;
                }
            }
            else if (len == 0 || (len < 0 && errno != EWOULDBLOCK))
            {
                close(clients[i].fd);
                clients[i] = clients[nclients - 1];
                nclients--;
                i--;
            }
        }

        usleep(10000);
    }
    close(listener);
    return 0;
}
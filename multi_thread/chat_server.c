#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

#define PORT 9999
#define MAX_CLIENTS 100
#define BUFFER_SIZE 2048

// Cấu trúc lưu trữ thông tin mỗi client
typedef struct
{
    int socket;
    char name[50];
} client_t;

client_t *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// Hàm lấy thời gian hiện tại định dạng yyyy/mm/dd hh:mm:ssAM/PM
void get_timestamp(char *buffer)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, 30, "%Y/%m/%d %I:%M:%S%p", t);
}

// Gửi tin nhắn tới tất cả client trừ người gửi
void broadcast(char *message, int sender_fd)
{
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i] && clients[i]->socket != sender_fd)
        {
            if (send(clients[i]->socket, message, strlen(message), 0) < 0)
            {
                perror("ERROR: send failed");
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// Hàm xử lý riêng cho mỗi luồng client
void *handle_client(void *arg)
{
    char buffer[BUFFER_SIZE];
    char name_buf[100];
    int client_fd = *((int *)arg);
    free(arg);

    // Bước 1: Yêu cầu định danh client_id: client_name
    while (1)
    {
        char *msg = "Vui long nhap ten theo cú pháp (client_id: client_name): ";
        send(client_fd, msg, strlen(msg), 0);

        int receive = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (receive <= 0)
        {
            close(client_fd);
            return NULL;
        }
        buffer[receive] = '\0';
        buffer[strcspn(buffer, "\r\n")] = 0; // Xóa ký tự xuống dòng

        // Kiểm tra định dạng bằng sscanf
        if (sscanf(buffer, "client_id: %s", name_buf) == 1)
        {
            break; // Đúng định dạng
        }
    }

    // Lưu thông tin client vào danh sách
    client_t *cli = (client_t *)malloc(sizeof(client_t));
    cli->socket = client_fd;
    strcpy(cli->name, name_buf);

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (!clients[i])
        {
            clients[i] = cli;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    printf("Client '%s' da tham gia phong chat.\n", cli->name);

    // Bước 2: Nhận tin nhắn và chuyển tiếp
    while (1)
    {
        int receive = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (receive <= 0)
            break;

        buffer[receive] = '\0';
        buffer[strcspn(buffer, "\r\n")] = 0;

        if (strlen(buffer) > 0)
        {
            char timestamp[30];
            char final_msg[BUFFER_SIZE + 100];
            get_timestamp(timestamp);

            // Định dạng: 2026/05/11 02:00:00PM abc: xin chao
            sprintf(final_msg, "%s %s: %s\n", timestamp, cli->name, buffer);
            broadcast(final_msg, client_fd);
        }
    }

    // Xóa client khi ngắt kết nối
    printf("Client '%s' da thoat.\n", cli->name);
    close(client_fd);
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i] && clients[i]->socket == client_fd)
        {
            clients[i] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    free(cli);
    return NULL;
}

int main()
{
    int server_fd, *new_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        return 1;
    }

    listen(server_fd, 10);
    printf("Chat Server dang chay tai port %d...\n", PORT);

    while (1)
    {
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);

        pthread_t tid;
        new_sock = malloc(sizeof(int));
        *new_sock = client_fd;

        // Tạo luồng mới cho mỗi client kết nối vào
        if (pthread_create(&tid, NULL, handle_client, (void *)new_sock) != 0)
        {
            perror("Could not create thread");
            free(new_sock);
        }
        pthread_detach(tid); // Tự động giải phóng tài nguyên luồng khi kết thúc
    }

    return 0;
}
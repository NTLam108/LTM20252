#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define DB_FILE "database.txt"
#define BUFFER_SIZE 1024

// Hàm kiểm tra đăng nhập
int check_login(char *user, char *pass)
{
    FILE *f = fopen(DB_FILE, "r");
    if (f == NULL)
        return 0;

    char db_user[50], db_pass[50];
    while (fscanf(f, "%s %s", db_user, db_pass) != EOF)
    {
        if (strcmp(user, db_user) == 0 && strcmp(pass, db_pass) == 0)
        {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

// Hàm gửi nội dung file kết quả về client
void send_file_content(int client_sock, char *filename)
{
    FILE *f = fopen(filename, "r");
    if (f == NULL)
        return;

    char buffer[BUFFER_SIZE];
    while (fgets(buffer, sizeof(buffer), f) != NULL)
    {
        send(client_sock, buffer, strlen(buffer), 0);
    }
    fclose(f);
}

void *client_handler(void *arg)
{
    int client_sock = *(int *)arg;
    free(arg);
    char buffer[BUFFER_SIZE];
    char user[50], pass[50];

    // Bước 1: Yêu cầu đăng nhập
    while (1)
    {
        char *msg = "Nhap user pass (format: user pass): ";
        send(client_sock, msg, strlen(msg), 0);

        int bytes = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0)
        {
            close(client_sock);
            return NULL;
        }

        buffer[bytes] = '\0';
        if (sscanf(buffer, "%s %s", user, pass) == 2)
        {
            if (check_login(user, pass))
            {
                send(client_sock, "Dang nhap thanh cong!\n", 22, 0);
                break;
            }
        }
        send(client_sock, "Sai tai khoan. Thu lai!\n", 24, 0);
    }

    // Bước 2: Nhận lệnh và thực thi
    while (1)
    {
        char *cmd_prompt = "\nServer> ";
        send(client_sock, cmd_prompt, strlen(cmd_prompt), 0);

        int bytes = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0)
            break;

        buffer[bytes] = '\0';
        buffer[strcspn(buffer, "\r\n")] = 0; // Xóa ký tự xuống dòng

        if (strlen(buffer) > 0)
        {
            // Khai báo kích thước lớn hơn một chút để an toàn
            char sys_cmd[BUFFER_SIZE + 128];
            char tmp_file[64];

            // Tạo file tạm
            sprintf(tmp_file, "out_%d.txt", client_sock);

            // Sử dụng snprintf để bảo vệ bộ đệm
            int written = snprintf(sys_cmd, sizeof(sys_cmd), "%s > %s 2>&1", buffer, tmp_file);

            // Kiểm tra xem lệnh có bị cắt cụt do buffer quá nhỏ không
            if (written >= sizeof(sys_cmd))
            {
                char *err_msg = "Loi: Lenh qua dai!\n";
                send(client_sock, err_msg, strlen(err_msg), 0);
            }
            else
            {
                system(sys_cmd);
                send_file_content(client_sock, tmp_file);
                unlink(tmp_file);
            }
        }
    }

    close(client_sock);
    return NULL;
}

int main()
{
    int server_sock;
    struct sockaddr_in server_addr;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        return 1;
    }

    listen(server_sock, 5);
    printf("Telnet Server đang chạy tại cổng %d...\n", PORT);

    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len);

        pthread_t tid;
        int *new_sock = malloc(sizeof(int));
        *new_sock = client_sock;
        pthread_create(&tid, NULL, client_handler, new_sock);
        pthread_detach(tid);
    }

    return 0;
}
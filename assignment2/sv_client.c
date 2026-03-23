#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

// Định nghĩa cấu trúc dữ liệu sinh viên
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
        printf("Sử dụng: %s <IP Server> <Cổng>\n", argv[0]);
        return 1;
    }

    int client_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr);

    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Kết nối thất bại");
        return 1;
    }

    Student sv;
    printf("--- Nhập thông tin sinh viên ---\n");
    printf("MSSV: ");
    scanf("%s", sv.mssv);
    getchar(); // Xóa bộ đệm enter
    printf("Họ tên: ");
    fgets(sv.ho_ten, sizeof(sv.ho_ten), stdin);
    sv.ho_ten[strcspn(sv.ho_ten, "\n")] = 0;
    printf("Ngày sinh (YYYY-MM-DD): ");
    scanf("%s", sv.ngay_sinh);
    printf("Điểm trung bình: ");
    scanf("%f", &sv.diem_tb);

    // Gửi sang server
    send(client_sock, &sv, sizeof(Student), 0);
    printf("Đã gửi dữ liệu thành công!\n");

    close(client_sock);
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUF_SIZE 1024
#define SERVER_IP "127.0.0.1"

void log_action(const char *msg) {
    printf("%s\n", msg);
}

void send_file(int client_fd) {
    char filename[256];
    char buf[BUF_SIZE];

    printf("Enter the file name (with .txt): ");
    scanf("%s", filename);

    write(client_fd, filename, strlen(filename));

    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        perror("File not found");
        return;
    }

    while (1) {
        int n = fread(buf, 1, BUF_SIZE, fp);
        if (n <= 0) break;
        write(client_fd, buf, n);
    }
    fclose(fp);
}

void receive_response(int client_fd) {
    char resp[128];
    int n = read(client_fd, resp, sizeof(resp));
    if (n > 0) {
        resp[n] = '\0';
        log_action(resp);
    }
}

int main() {
    int client_fd;
    struct sockaddr_in server_addr;

    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <dirent.h>

#define PORT 8080
#define BUF_SIZE 1024
#define SECRET_DIR "client/secrets/"
#define OUTPUT_DIR "client/"
#define SERVER_IP "127.0.0.1"

int connect_server() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("Socket creation failed"); return -1; }
    struct sockaddr_in serv = {0};
    serv.sin_family = AF_INET;
    serv.sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER_IP, &serv.sin_addr) <= 0) { perror("Invalid address"); close(sock); return -1; }
    if (connect(sock, (struct sockaddr*)&serv, sizeof(serv)) < 0) { perror("Connect failed"); close(sock); return -1; }
    return sock;
}

void send_file(int sock) {
    char filename[256];
    printf("Enter the file name (with .txt): ");
    scanf("%255s", filename);
    write(sock, filename, strlen(filename) + 1);

    char path[512], buf[BUF_SIZE];
    snprintf(path, sizeof(path), "%s%s", SECRET_DIR, filename);
    FILE *fp = fopen(path, "rb");
    if (!fp) { perror("File open error"); return; }
    size_t n;
    while ((n = fread(buf, 1, BUF_SIZE, fp)) > 0) write(sock, buf, n);
    fclose(fp);
}

void receive_response(int sock) {
    char resp[BUF_SIZE];
    int r = recv(sock, resp, sizeof(resp)-1, 0);
    if (r > 0) {
        resp[r] = '\0';
        printf("Server: %s\n", resp);
    }
}

int main() {
    int choice;
    while (1) {
        printf("===========\n");
        printf(" Image Decoder Client \n");
        printf("===========\n");
        printf("1. Send input file to server\n");
        printf("2. Download file from server\n");
        printf("3. List input files\n");
        printf("4. Exit\n");
        printf(">> ");
        if (scanf("%d", &choice) != 1) break;
        getchar();

        if (choice == 1) {
            int sock = connect_server();
            if (sock < 0) continue;
            send_file(sock);
            receive_response(sock);
            close(sock);
        } else if (choice == 2) {
            char filename[256];
            printf("Enter filename to download (e.g., 1234567890.jpg): ");
            scanf("%255s", filename);
            getchar();
            int sock = connect_server();
            if (sock < 0) continue;
            char cmd[BUF_SIZE];
            snprintf(cmd, sizeof(cmd), "DOWNLOAD|%s", filename);
            write(sock, cmd, strlen(cmd));
            char outpath[512];
            snprintf(outpath, sizeof(outpath), "%s%s", OUTPUT_DIR, filename);
            FILE *fp = fopen(outpath, "wb");
            if (!fp) { perror("Open output error"); close(sock); continue; }
            int n; char buf[BUF_SIZE];
            while ((n = recv(sock, buf, BUF_SIZE, 0)) > 0) fwrite(buf, 1, n, fp);
            fclose(fp);
            printf("Downloaded: %s\n", filename);
            close(sock);
        } else if (choice == 3) {
            printf("Available .txt files in %s:\n", SECRET_DIR);
            system("ls client/secrets/*.txt");
        } else if (choice == 4) {
            break;
        } else {
            printf("Invalid option\n");
        }
        printf("\n");
    }
    return 0;
}


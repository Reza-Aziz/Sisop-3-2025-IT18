#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#define PORT 8080
#define BUF_SIZE 1024
#define SAVE_DIR "database/"
#define LOG_FILE "server.log"

void log_action(const char *msg) {
    FILE *f = fopen(LOG_FILE, "a");
    if (!f) return;
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char ts[20];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", tm);
    fprintf(f, "[%s] %s\n", ts, msg);
    fclose(f);
}

void daemonize() {
    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);
    if (setsid() < 0) exit(EXIT_FAILURE);
    umask(0);
}

void receive_and_convert(int client_fd) {
    char filename[256], buf[BUF_SIZE], logmsg[300];
    int n = read(client_fd, filename, sizeof(filename));
    if (n <= 0) return;
    filename[n] = '\0';
    snprintf(logmsg, sizeof(logmsg), "RECEIVED %s", filename);
    log_action(logmsg);

    mkdir(SAVE_DIR, 0755);
    char path[512];
    snprintf(path, sizeof(path), "%s%s", SAVE_DIR, filename);
    FILE *fp = fopen(path, "wb"); if (!fp) return;
    while ((n = read(client_fd, buf, BUF_SIZE)) > 0) {
        fwrite(buf, 1, n, fp);
        if (n < BUF_SIZE) break;
    }
    fclose(fp);

    char jpgpath[512];
    snprintf(jpgpath, sizeof(jpgpath), "%s%s.jpg", SAVE_DIR, filename);
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "convert %s %s", path, jpgpath);
    system(cmd);
    snprintf(logmsg, sizeof(logmsg), "CONVERTED %s.jpg", filename);
    log_action(logmsg);
    remove(path);

    char resp[128];
    snprintf(resp, sizeof(resp), "Text decrypted and saved as %s.jpg", filename);
    write(client_fd, resp, strlen(resp));
    close(client_fd);
}

int main() {
    daemonize();
    mkdir(SAVE_DIR, 0755);
    int lf = open(LOG_FILE, O_CREAT|O_APPEND, 0644);
    if (lf>=0) close(lf);
    log_action("SERVER STARTED");

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);
    bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    listen(sock, 5);

    while (1) {
        int cfd = accept(sock, NULL, NULL);
        if (cfd<0) continue;
        if (!fork()) {
            receive_and_convert(cfd);
            exit(0);
        }
        close(cfd);
    }
    return 0;
}
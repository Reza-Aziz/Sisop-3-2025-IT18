#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#define MAX_ORDERS 100
#define SHM_KEY 1234

typedef struct {
    char name[64];
    char address[128];
    char type[32];
    int is_active; 
} Order;

Order (*orders)[MAX_ORDERS];

void tulis_log(const char *agent, const char *nama, const char *alamat) {
    FILE *log = fopen("delivery.log", "a");
    if (!log) {
        perror("Gagal membuka log file");
        return;
    }

    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%d/%m/%Y %H:%M:%S", tm_info);

    fprintf(log, "[%s] [AGENT %s] Reguler package delivered to %s in %s\n",
            time_buf, agent, nama, alamat);
    fclose(log);
}

void deliver(char *nama) {
    int found = 0;
    char *user = getenv("USER");
    if (!user) user = "UNKNOWN";

    for (int i = 0; i < MAX_ORDERS; i++) {
        if ((*orders)[i].is_active == 1 &&
            strcmp((*orders)[i].name, nama) == 0 &&
            strstr((*orders)[i].type, "Reguler")) { 

            (*orders)[i].is_active = 0;
            tulis_log(user, (*orders)[i].name, (*orders)[i].address);
            printf("Berhasil mengirim paket untuk %s.\n", nama);
            found = 1;
            break;
        }
    }

    if (!found) {
        printf("Data dengan nama \"%s\" dan jenis Reguler tidak ditemukan atau sudah dikirim.\n", nama);
    }
}


void status(char *nama) {
    FILE *log = fopen("delivery.log", "r");
    if (!log) {
        perror("Gagal membuka log file");
        return;
    }

    char line[256];
    int found = 0;

    while (fgets(line, sizeof(line), log)) {
        if (strstr(line, nama)) {
          
            char *agent_ptr = strstr(line, "[AGENT ");
            if (agent_ptr) {
                char agent[64];
                sscanf(agent_ptr, "[AGENT %[^]]", agent);
                printf("Status for %s: Delivered by Agent %s\n", nama, agent);
                found = 1;
            }
        }
    }

    if (!found) {
        printf("Status for %s: Not found or not delivered yet.\n", nama);
    }

    fclose(log);
}


void list_log() {
    FILE *log = fopen("delivery.log", "r");
    if (!log) {
        perror("Gagal membuka log file");
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), log)) {
        printf("%s", line);
    }

    fclose(log);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  %s -deliver [Nama]\n", argv[0]);
        fprintf(stderr, "  %s -status [Nama]\n", argv[0]);
        fprintf(stderr, "  %s -list\n", argv[0]);
        return 1;
    }

    int shmid = shmget(SHM_KEY, sizeof(Order) * MAX_ORDERS, 0666);
    if (shmid == -1) {
        perror("Gagal membuka shared memory");
        return 1;
    }

    orders = shmat(shmid, NULL, 0);
    if (orders == (void *)-1) {
        perror("Gagal attach shared memory");
        return 1;
    }

    if (strcmp(argv[1], "-deliver") == 0 && argc == 3) {
        deliver(argv[2]);
    } else if (strcmp(argv[1], "-status") == 0 && argc == 3) {
        status(argv[2]);
    } else if (strcmp(argv[1], "-list") == 0) {
        list_log();
    } else {
        fprintf(stderr, "Perintah tidak dikenali.\n");
    }

    shmdt(orders);
    return 0;
}

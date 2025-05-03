#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#define MAX_ORDERS 100
#define MAX_LINE 256
#define SHM_KEY 1234

typedef struct {
    char name[64];
    char address[128];
    char type[32];
    int is_active; 
} Order;

Order (*orders)[MAX_ORDERS];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void download_csv() {
    system("curl -L -o delivery_order.csv \"https://drive.usercontent.google.com/u/0/uc?id=1OJfRuLgsBnIBWtdRXbRsD2sG6NhMKOg9&export=download\"");
}

int load_csv_to_shm() {
    FILE *file = fopen("delivery_order.csv", "r");
    if (!file) {
        perror("Gagal membuka file CSV");
        return -1;
    }

    char line[MAX_LINE];
    int idx = 0;

    fgets(line, sizeof(line), file);

    while (fgets(line, sizeof(line), file) && idx < MAX_ORDERS) {
        char *nama = strtok(line, ",");
        char *alamat = strtok(NULL, ",");
        char *jenis = strtok(NULL, "\n");

        if (nama && alamat && jenis) {
            strcpy((*orders)[idx].name, nama);
            strcpy((*orders)[idx].address, alamat);
            strcpy((*orders)[idx].type, jenis);
            (*orders)[idx].is_active = 1;
            idx++;
        }
    }

    fclose(file);
    return idx;
}

void tulis_log(const char *agent, const char *nama, const char *alamat) {
    FILE *log = fopen("delivery.log", "a");
    if (!log) return;

    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%d/%m/%Y %H:%M:%S", tm_info);

    fprintf(log, "[%s] [AGENT %s] Express package delivered to %s in %s\n",
            time_buf, agent, nama, alamat);
    fclose(log);
}

void* agent_worker(void *arg) {
    char *agent = (char *)arg;

    for (int i = 0; i < MAX_ORDERS; i++) {
        pthread_mutex_lock(&mutex);
        if ((*orders)[i].is_active && strstr((*orders)[i].type, "Express")) {
            (*orders)[i].is_active = 0;
            char nama[64], alamat[128];
            strcpy(nama, (*orders)[i].name);
            strcpy(alamat, (*orders)[i].address);
            pthread_mutex_unlock(&mutex);

            tulis_log(agent, nama, alamat);
            sleep(1); 
        } else {
            pthread_mutex_unlock(&mutex);
        }
    }

    return NULL;
}

int main() {
    int shmid = shmget(SHM_KEY, sizeof(Order) * MAX_ORDERS, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("Gagal membuat shared memory");
        return 1;
    }

    orders = shmat(shmid, NULL, 0);
    if (orders == (void *)-1) {
        perror("Gagal attach shared memory");
        return 1;
    }

    download_csv();
    int total = load_csv_to_shm();
    printf("Loaded %d order(s) ke shared memory.\n", total);

    pthread_t agents[3];
    char *agent_names[] = {"A", "B", "C"};
    for (int i = 0; i < 3; i++) {
        pthread_create(&agents[i], NULL, agent_worker, agent_names[i]);
    }

    for (int i = 0; i < 3; i++) {
        pthread_join(agents[i], NULL);
    }

    shmdt(orders);
    return 0;
}

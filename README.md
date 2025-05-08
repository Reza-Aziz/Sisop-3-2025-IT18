# Sisop-3-2025-IT18
# Soal 1
1. Import library
<pre>
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
 </pre>
 
2. Fungsi untuk mencatat aktivitas server ke file log.
 <pre>
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
 </pre>
*Membuka file log dengan mode append ("a")
*Mendapatkan waktu saat ini dan memformatnya
*Menulis pesan log dengan format: [timestamp] pesan
*Menutup file log setelah selesai

3. Fungsi untuk mengubah proses menjadi daemon (berjalan di background).
<pre>
 void daemonize() {
    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);
    if (setsid() < 0) exit(EXIT_FAILURE);
    umask(0);
}
    </pre>

<pre>
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
               </pre>


# Soal 2
delivery_agent
1. Import library 
<pre>
 #include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
</pre>
2. Menentukan tipe data dan thread
<pre>
 typedef struct {
    char name[64];
    char address[128];
    char type[32];
    int is_active; 
} Order;
 Order (*orders)[MAX_ORDERS];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
</pre>
3. Function download
<pre>
 void download_csv() {
    system("curl -L -o delivery_order.csv \"https://drive.usercontent.google.com/u/0/uc?id=1OJfRuLgsBnIBWtdRXbRsD2sG6NhMKOg9&export=download\"");
}
</pre>
4. Function untuk membaca dan mengirimkan seluruhnya e shared memory
<pre>
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
</pre>
5. Menuliskan ke log
<pre>
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
</pre>
6. Function untuk melakukan pengiriman untuk data bertipe express
<pre>
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
</pre>
7. Function main
<pre>
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

</pre>
dispatcher
1. Import libraary dan definisi
<pre>
 #include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#define MAX_ORDERS 100
#define SHM_KEY 1234
</pre>
2. Deklarasi tipe data dan order
<pre>
 typedef struct {
    char name[64];
    char address[128];
    char type[32];
    int is_active; 
} Order;

Order (*orders)[MAX_ORDERS];
</pre>
3. Function menuliskan log
<pre>
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
</pre>
4. Function --deliver untuk mengirimkan data satu persatu dari shared memory
<pre>
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
</pre>
5. Function --status
<pre>
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
</pre>
6. Function untuk menampilkan seluruh log
<pre>
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
</pre>
7. Fungsi main
<pre>
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

</pre>
# Soal 3
# Soal 4
Di dunia yang kacau, seorang hunter bernama Sung Jin Woo bereinkarnasi dan menjadi seorang admin. Pekerjaan ini mempunyai sebuah sistem yang bisa melakukan tracking pada seluruh aktivitas dan keadaan seseorang. Sayangnya, model yang diberikan oleh Bos-nya sudah sangat tua sehingga program tersebut harus dimodifikasi agar tidak ketinggalan zaman.

Program ini terdiri dari dua file, yaitu:
  * system.c: Program utama sebagai admin system yang mengelola hunter dan dungeon menggunakan shared memory
  * hunter.c: Program client hunter untuk login, melihat dungeon, melakukan raid, dan bertarung antar hunter

## 4.1 system.c
1. Import Library dan Define Constant
   ```c
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <unistd.h>
    #include <sys/ipc.h>
    #include <sys/shm.h>
    #include <time.h>

    #define MAX_HUNTERS 100
    #define MAX_DUNGEONS 100
    #define MAX_USERNAME 100
    #define MAX_NOTIFICATIONS 100

2. Struktur Data
   <pre>
   struct Hunter {
      char username[MAX_USERNAME];
      int level;
      int exp;
      int atk;
      int hp;
      int def;
      int banned;
   }; 
   
   struct Dungeon {
       char name[100];
       int min_level;
       int exp;
       int atk;
       int hp;
       int def;
       key_t shm_key;
   };

   struct SystemData {
       struct Hunter hunters[MAX_HUNTERS];
       int num_hunters;

       struct Dungeon dungeons[MAX_DUNGEONS];
       int num_dungeons;

       char notifications[MAX_NOTIFICATIONS][256];
       int current_notification_index; 
   };</pre>
   * struct Hunter menyimpan data statistik setiap hunter
   * struct Dungeon Menyimpan data dungeon beserta shared memory key unik
   * struct SystemData menyimpan semua informasi global dari sistem (hunters, dungeons, notifications)

3. Tampilkan Hunter
   <pre>
   void tampilkan_hunters() {
       printf("\n=== Daftar Hunter ===\n");
       for (int i = 0; i < system_data->num_hunters; i++) {
           struct Hunter h = system_data->hunters[i];
           printf("%d. %s | Level: %d | EXP: %d | ATK: %d | HP: %d | DEF: %d | %s\n",
                  i+1, h.username, h.level, h.exp, h.atk, h.hp, h.def,
                  h.banned ? "BANNED" : "ACTIVE");
       }
    }</pre>
    * Melakukan loop semua hunter
    * Menampilkan data lengkapnya dan status active/banned

4. Tampilkan Dungeon
   <pre>
   void tampilkan_dungeons() {
       printf("\n=== Daftar Dungeon ===\n");
       for (int i = 0; i < system_data->num_dungeons; i++) {
       struct Dungeon d = system_data->dungeons[i];
           printf("%d. %s | Min Level: %d | EXP: %d | ATK: %d | HP: %d | DEF: %d\n",
                  i+1, d.name, d.min_level, d.exp, d.atk, d.hp, d.def);
       }
    } </pre>
    * Melakukan loop semua dungeon
    * Menampilkan data lengkapnya

5. Buat Dungeon
   <pre>
   void buat_dungeon() {
       if (system_data->num_dungeons >= MAX_DUNGEONS) {
           printf("Dungeon penuh!\n");
           return;
       }

       struct Dungeon d;
       while (getchar() != '\n');
       printf("Nama Dungeon: ");
       scanf("%[^\n]", d.name);

       printf("Minimum Level: ");
       scanf("%d", &d.min_level);

       d.exp       = (rand() % 151) + 150;
       d.atk       = (rand() % 51) + 100;
       d.hp        = (rand() % 51) + 50;
       d.def       = (rand() % 26) + 25;
       d.shm_key   = rand();

       system_data->dungeons[system_data->num_dungeons++] = d;
       snprintf(system_data->notifications[++system_data->current_notification_index],
            256, "Dungeon baru tersedia: %s", d.name);

        printf("Dungeon berhasil dibuat!\n");
   } </pre>
   * Cek apakah dungeon sudah penuh
   * Minta input nama dungeon
   * Input minimum level
   * Generate nilai-nilai stat secara acak
   * Tambahkan notifikasi dungeon baru

6. Ban dan Unban Hunter
   <pre>
   void ban_hunter() {
       tampilkan_hunters();
       printf("Pilih nomor hunter yang akan diban: ");
       int idx; scanf("%d", &idx); idx--;

       if (idx >= 0 && idx < system_data->num_hunters) {
           system_data->hunters[idx].banned = 1;
           printf("Hunter %s telah diban.\n", system_data->hunters[idx].username);
       } else {
           printf("Indeks tidak valid.\n");
       }
   }

   void unban_hunter() {
       tampilkan_hunters();
       printf("Pilih nomor hunter yang akan diunban: ");
       int idx; scanf("%d", &idx); idx--;

       if (idx >= 0 && idx < system_data->num_hunters) {
           system_data->hunters[idx].banned = 0;
           printf("Hunter %s telah diunban.\n", system_data->hunters[idx].username);
       } else {
           printf("Indeks tidak valid.\n");
       }
   } </pre>
   * Input nomor hunter lalu ubah statusnya banned/acticve
   * Unban juga sama tapi set banned = 0

7. Reset Stat Hunter
   <pre>
   void reset_hunter_stat() {
       tampilkan_hunters();
       printf("Pilih nomor hunter yang akan direset: ");
       int idx; scanf("%d", &idx); idx--;

       if (idx >= 0 && idx < system_data->num_hunters) {
            struct Hunter *h = &system_data->hunters[idx];
            h->level = 1;
            h->exp = 0;
            h->atk = 10;
            h->hp = 100;
            h->def = 5;
            printf("Stat hunter %s telah direset.\n", h->username);
        } else {
            printf("Indeks tidak valid.\n");
        }
   } </pre>
   * Kembalikan semua nilai ke  default/stat awal

8. Fungsi main
   <pre>
   int main() {
        key_t key = get_system_key();
        int shmid = shmget(key, sizeof(struct SystemData), IPC_CREAT | 0666);
        if (shmid == -1) {
            perror("shmget");
            exit(1);
        }

        system_data = (struct SystemData *) shmat(shmid, NULL, 0);
        if (system_data == (void *) -1) {
            perror("shmat");
            exit(1);
        }

        srand(time(NULL));

        if (system_data->num_hunters == 0 && system_data->num_dungeons == 0 && system_data->current_notification_index == 0> memset(system_data, 0, sizeof(struct SystemData));
        }

        int choice;
        do {
            printf("\n=== Menu System ===\n");
            printf("1. Lihat Hunter\n");
            printf("2. Lihat Dungeon\n");
            printf("3. Buat Dungeon\n");
            printf("4. Ban Hunter\n");
            printf("5. Unban Hunter\n");
            printf("6. Reset Hunter Stat\n");
            printf("7. Exit\n");
            printf("Pilihan: ");
            scanf("%d", &choice);

            switch (choice) {
                case 1: tampilkan_hunters(); break;
                case 2: tampilkan_dungeons(); break;
                case 3: buat_dungeon(); break;
                case 4: ban_hunter(); break;
                case 5: unban_hunter(); break;
                case 6: reset_hunter_stat(); break;
                case 7: return 0;
                default: printf("Pilihan tidak valid.\n");
            }
        } while (1);



        return 0;
   } </pre>
   * Akses dan map shared memory
   * Tampilkan menu admin
   * Loop menu sistem hingga exit

## 4.2 hunter.c
1. Import Library dan Define Constant
   ```c
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <unistd.h>
   #include <sys/ipc.h>
   #include <sys/shm.h>
   #include <pthread.h>

   #define MAX_HUNTERS 100
   #define MAX_DUNGEONS 100
   #define MAX_USERNAME 100
   #define MAX_NOTIFICATIONS 100

2. Struktur Data
   <pre>
   struct Hunter {
      char username[MAX_USERNAME];
      int level;
      int exp;
      int atk;
      int hp;
      int def;
      int banned;
   }; 
   
   struct Dungeon {
       char name[100];
       int min_level;
       int exp;
       int atk;
       int hp;
       int def;
       key_t shm_key;
   };

   struct SystemData {
       struct Hunter hunters[MAX_HUNTERS];
       int num_hunters;

       struct Dungeon dungeons[MAX_DUNGEONS];
       int num_dungeons;

       char notifications[MAX_NOTIFICATIONS][256];
       int current_notification_index; 
   };</pre>
   * struct Hunter menyimpan data statistik setiap hunter
   * struct Dungeon Menyimpan data dungeon beserta shared memory key unik
   * struct SystemData menyimpan semua informasi global dari sistem (hunters, dungeons, notifications)

3. Thread Notifikasi
   <pre>
   void *notification_thread(void *arg) {
       int last_index = -1;
       while (1) {
           if (show_notification && system_data->current_notification_index != last_index) {
               printf("\n[Notifikasi] %s\n", system_data->notifications[system_data->current_notification_index]);
               last_index = system_data->current_notification_index;
           }
           sleep(3);
       }
       return NULL;
   } </pre>
   * Thread berjalan terus
   * Jika ada notifikasi baru (current_notification_index berubah), akan ditampilkan ke layar
     
4. Register Hunter
   <pre>
   void register_hunter() {
       if (system_data->num_hunters >= MAX_HUNTERS) {
            printf("Jumlah hunter sudah penuh!\n");
            return;
        }

        struct Hunter h;
        printf("Masukkan username: ");
        scanf("%s", h.username);
        h.level = 1;
        h.exp = 0;
        h.atk = 10;
        h.hp = 100;
        h.def = 5;
        h.banned = 0;

        system_data->hunters[system_data->num_hunters++] = h;
        printf("Registrasi berhasil.\n");
   } </pre>
   * Meminta input username
   * Menambahkan hunter baru ke shared memory dengan stat awal

5. Login Hunter
   <pre>
   int login_hunter() {
       char name[MAX_USERNAME];
       printf("Masukkan username: ");
       scanf("%s", name);
       for (int i = 0; i < system_data->num_hunters; i++) {
           if (strcmp(system_data->hunters[i].username, name) == 0) {
               if (system_data->hunters[i].banned) {
                   printf("Hunter dibanned!\n");
                   return -1;
               }
               current_hunter_index = i;
               printf("Login berhasil sebagai %s\n", name);
               return i;
           }
       }
       printf("Username tidak ditemukan!\n");
       return -1;
   } </pre>
   * Mencari hunter berdasarkan username
   * Login gagal jika hunter sedang dibanned

6. Lihat Dungeon
   <pre>
   void lihat_dungeon() {
       printf("\n=== Daftar Dungeon ===\n");
       struct Hunter *h = &system_data->hunters[current_hunter_index];
       for (int i = 0; i < system_data->num_dungeons; i++) {
           struct Dungeon d = system_data->dungeons[i];
           if (h->level >= d.min_level) {
               printf("%d. %s (Min Lv %d, EXP %d, ATK %d, HP %d, DEF %d)\n",
                   i+1, d.name, d.min_level, d.exp, d.atk, d.hp, d.def);
           }
       }
   } </pre>
   * Menampilkan dungeon yang tersedia sesuai kecukupan level hunter

7. Raid Dungeon
   <pre>
   void raid_dungeon() {
       lihat_dungeon();
       printf("Pilih dungeon: ");
       int pilih;
       scanf("%d", &pilih);
       pilih--;

       if (pilih < 0 || pilih >= system_data->num_dungeons) {
           printf("Pilihan tidak valid.\n");
           return;
       }

       struct Hunter *h = &system_data->hunters[current_hunter_index];
       struct Dungeon *d = &system_data->dungeons[pilih];

       if (h->level < d->min_level) {
           printf("Level belum cukup!\n");
           return;
       }

       int monster_hp = d->hp;
       int player_hp = h->hp;
       int dmg_to_monster = h->atk - d->def;
       int dmg_to_player = d->atk - h->def;
       if (dmg_to_monster <= 0) dmg_to_monster = 1;
       if (dmg_to_player <= 0) dmg_to_player = 1;

       while (monster_hp > 0 && player_hp > 0) {
           monster_hp -= dmg_to_monster;
           if (monster_hp <= 0) break;
           player_hp -= dmg_to_player;
       }

       if (monster_hp <= 0 && player_hp > 0) {
           printf("Menang! Dapat EXP %d\n", d->exp);
           h->exp += d->exp;
           while (h->exp >= 500) {
               h->level++;
               h->exp -= 500;
               h->atk += 10;
               h->hp += 10;
               h->def += 10;
               printf("Naik level! Level: %d\n", h->level);
           }

           } else {
                printf("Kalah!\n");
           }
   } </pre>
   * Memilih dungeon yang ingin di-raid
   * Validasi level hunter dan pilihan dungeon
   * Hitung damage hunter dan monster

8. Battle Hunter
   <pre>
   void battle_hunter() {
       printf("\n=== Pilih Lawan ===\n");
       for (int i = 0; i < system_data->num_hunters; i++) {
           if (i != current_hunter_index && !system_data->hunters[i].banned) {
               printf("%d. %s\n", i+1, system_data->hunters[i].username);
           }
       }

       printf("Pilih lawan: ");
       int idx;
       scanf("%d", &idx);
       idx--;

       if (idx < 0 || idx >= system_data->num_hunters || idx == current_hunter_index) {
           printf("Pilihan tidak valid!\n");
           return;
       }

       struct Hunter *p = &system_data->hunters[current_hunter_index];
       struct Hunter *e = &system_data->hunters[idx];

       int p_hp = p->hp, e_hp = e->hp;
       int p_dmg = p->atk - e->def, e_dmg = e->atk - p->def;
       if (p_dmg <= 0) p_dmg = 1;
       if (e_dmg <= 0) e_dmg = 1;

       while (p_hp > 0 && e_hp > 0) {
           e_hp -= p_dmg;
           if (e_hp <= 0) break;
           p_hp -= e_dmg;
       }

       if (p_hp > 0 && e_hp <= 0) {
           printf("Kamu menang! EXP +50\n");
           p->exp += 50;
           while (p->exp >= 500) {
               p->level++;
               p->exp -= 500;
               p->atk += 10;
               p->hp += 10;
               p->def += 10;
               printf("Naik level! Level: %d\n", p->level);
           }
       } else {
           printf("Kamu kalah!\n");
       }
   }  </pre>
   * Pilih hunter lain yang tidak dibanned sebagai lawan
   * Hitung damage antar hunter

9. Fungsi Main
    <pre>
    int main() {
       key_t key = get_system_key();
       int shmid = shmget(key, sizeof(struct SystemData), 0666);
       if (shmid == -1) {
           perror("shmget");
           exit(EXIT_FAILURE);
       }

       system_data = shmat(shmid, NULL, 0);
       if (system_data == (void *) -1) {
           perror("shmat");
           exit(EXIT_FAILURE);
       }

       pthread_t tid;
       int  notif_started = 0;

       int awal;
       do {
           printf("\n=== Menu Awal ===\n");
           printf("1. Register\n");
           printf("2. Login\n");
           printf("3. Exit\n");
           printf("Pilihan: ");
           scanf("%d", &awal);

           if (awal == 1) register_hunter();
           else if (awal == 2) {
               if (login_hunter() != -1) {
                   if(!notif_started) {
                      pthread_create(&tid, NULL, notification_thread, NULL);
                      notif_started = 1;
                   }

                   int menu;
                   do {
                       printf("\n=== Menu Utama ===\n");
                       printf("1. Lihat Dungeon\n");
                       printf("2. Raid Dungeon\n");
                       printf("3. Battle\n");
                       printf("4. Notifikasi\n");
                       printf("5. Logout\n");
                       printf("Pilihan: ");
                       scanf("%d", &menu);

                       if (menu == 1) lihat_dungeon();
                       else if (menu == 2) raid_dungeon();
                       else if (menu == 3) battle_hunter();
                       else if (menu == 4) {
                           show_notification = !show_notification;
                           printf("Notifikasi %s\n", show_notification ? "diaktifkan" : "dimatikan");
                       }
                       else if (menu == 5) {
                           current_hunter_index = -1;
                           break;
                       }
                   }    while (1);
               }
           }
       } while (awal != 3);
       return 0;
    } </pre>
    * Akses shared memory buatan system.c
    * Menampilkan fitur hunter

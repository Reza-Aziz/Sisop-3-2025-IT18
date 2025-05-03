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
};

key_t get_system_key() {
    return ftok("system.c", 1);
}

struct SystemData *system_data;
int current_hunter_index = -1;
int show_notification = -1;

void *notification_thread(void *arg) {
    int last_index = 1;
    while (1) {
        if (show_notification && system_data->current_notification_index != last_index) {
            printf("\n[Notifikasi] %s\n", system_data->notifications[system_data->current_notification_index]);
            last_index = system_data->current_notification_index;
        }
        sleep(3);
    }
    return NULL;
}

void register_hunter() {
    if (system_data->num_hunters >= MAX_HUNTERS) {
        printf("Jumlah hunter penuh!\n");
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
}

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
}

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
}

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
}

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
}

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
                } while (1);
            }
        }
    } while (awal != 3);

    return 0;
}

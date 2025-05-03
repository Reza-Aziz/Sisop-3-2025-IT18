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

void *notification_thread(void *arg) {
    int last_index = system_data->current_notification_index;
    while (1) {
        if (system_data->current_notification_index != last_index) {
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

    struct Hunter new_hunter;
    printf("Masukkan username: ");
    scanf("%s", new_hunter.username);

    new_hunter.level = 1;
    new_hunter.exp = 0;
    new_hunter.atk = 10;
    new_hunter.hp = 100;
    new_hunter.def = 5;
    new_hunter.banned = 0;

    system_data->hunters[system_data->num_hunters++] = new_hunter;
    printf("Hunter berhasil diregistrasi.\n");
}

int login() {
    char uname[MAX_USERNAME];
    printf("Masukkan username: ");
    scanf("%s", uname);

    for (int i = 0; i < system_data->num_hunters; i++) {
        if (strcmp(system_data->hunters[i].username, uname) == 0) {
            if (system_data->hunters[i].banned) {
                printf("Hunter anda dibanned.\n");
                return -1;
            }
            current_hunter_index = i;
            return i;
        }
    }

    printf("Username tidak ditemukan.\n");
    return -1;
}

void list_dungeons() {
    printf("\n=== Daftar Dungeon ===\n");
    struct Hunter *h = &system_data->hunters[current_hunter_index];
    for (int i = 0; i < system_data->num_dungeons; i++) {
        struct Dungeon d = system_data->dungeons[i];
        if (h->level >= d.min_level) {
            printf("%d. %s | Min Level: %d | EXP: %d | ATK: %d | HP: %d | DEF: %d\n",
                   i+1, d.name, d.min_level, d.exp, d.atk, d.hp, d.def);
        }
    }
}

void raid_dungeon() {
    list_dungeons();
    printf("Pilih nomor dungeon: ");
    int idx;
    scanf("%d", &idx);
    idx--;

    if (idx < 0 || idx >= system_data->num_dungeons) {
        printf("Pilihan tidak valid.\n");
        return;
    }

    struct Hunter *h = &system_data->hunters[current_hunter_index];
    struct Dungeon *d = &system_data->dungeons[idx];

    if (h->level < d->min_level) {
        printf("Level anda belum cukup!\n");
        return;
    }

    int damage_to_monster = h->atk - d->def;
    int damage_to_player = d->atk - h->def;

    if (damage_to_monster <= 0) damage_to_monster = 1;
    if (damage_to_player <= 0) damage_to_player = 1;

    int monster_hp = d->hp;
    int player_hp = h->hp;

    while (monster_hp > 0 && player_hp > 0) {
        monster_hp -= damage_to_monster;
        if (monster_hp <= 0) break;
        player_hp -= damage_to_player;
    }

    if (monster_hp <= 0 && player_hp > 0) {
        printf("Kamu menang! EXP +%d\n", d->exp);
        h->exp += d->exp;
        while (h->exp >= 500) {
            h->level++;
            h->exp -= 500;
            h->atk += 10;
            h->hp += 10;
            h->def += 10;
            printf("Naik level! Level sekarang: %d\n", h->level);
        }
    } else {
        printf("Kamu kalah!\n");
    }
}

void battle_hunter() {
    printf("\n=== Daftar Hunter ===\n");
    for (int i = 0; i < system_data->num_hunters; i++) {
        if (i != current_hunter_index && !system_data->hunters[i].banned) {
            printf("%d. %s\n", i+1, system_data->hunters[i].username);
        }
    }

    printf("Pilih nomor hunter lawan: ");
    int idx;
    scanf("%d", &idx);
    idx--;

    if (idx < 0 || idx >= system_data->num_hunters || idx == current_hunter_index) {
        printf("Pilihan tidak valid.\n");
        return;
    }

    struct Hunter *player = &system_data->hunters[current_hunter_index];
    struct Hunter *enemy = &system_data->hunters[idx];

    int damage_to_enemy = player->atk - enemy->def;
    int damage_to_player = enemy->atk - player->def;

    if (damage_to_enemy <= 0) damage_to_enemy = 1;
    if (damage_to_player <= 0) damage_to_player = 1;

    int player_hp = player->hp;
    int enemy_hp = enemy->hp;

    while (player_hp > 0 && enemy_hp > 0) {
        enemy_hp -= damage_to_enemy;
        if (enemy_hp <= 0) break;
        player_hp -= damage_to_player;
    }

    if (player_hp > 0 && enemy_hp <= 0) {
        printf("Kamu menang! EXP +50\n");
        player->exp += 50;
        while (player->exp >= 500) {
            player->level++;
            player->exp -= 500;
            player->atk += 10;
            player->hp += 10;
            player->def += 10;
            printf("Naik level! Level sekarang: %d\n", player->level);
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
        exit(1);
    }

    system_data = (struct SystemData *) shmat(shmid, NULL, 0);
    if (system_data == (void *) -1) {
        perror("shmat");
        exit(1);
    }

    pthread_t notif_thread;
    pthread_create(&notif_thread, NULL, notification_thread, NULL);

    int choice;
    do {
        printf("\n=== Menu Hunter ===\n");
        printf("1. Register\n");
        printf("2. Login\n");
        printf("3. Lihat Dungeon\n");
        printf("4. Raid Dungeon\n");
        printf("5. Battle\n");
        printf("6. Exit\n");
        printf("Pilihan: ");
        scanf("%d", &choice);

        if (choice == 1) register_hunter();
        else if (choice == 2) login();
        else if (choice == 3 && current_hunter_index != -1) list_dungeons();
        else if (choice == 4 && current_hunter_index != -1) raid_dungeon();
        else if (choice == 5 && current_hunter_index != -1) battle_hunter();
        else if (choice == 6) break;
        else printf("Silakan login terlebih dahulu.\n");
    } while (1);

    return 0;
}
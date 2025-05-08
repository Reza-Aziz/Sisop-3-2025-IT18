# Sisop-3-2025-IT18
# Soal 1
# Soal 2
# Soal 3
# Soal 4
Di dunia yang kacau, seorang hunter bernama Sung Jin Woo bereinkarnasi dan menjadi seorang admin. Pekerjaan ini mempunyai sebuah sistem yang bisa melakukan tracking pada seluruh aktivitas dan keadaan seseorang. Sayangnya, model yang diberikan oleh Bos-nya sudah sangat tua sehingga program tersebut harus dimodifikasi agar tidak ketinggalan zaman.

Program ini terdiri dari dua file, yaitu:
  * system.c: Program utama sebagai admin system yang mengelola hunter dan dungeon menggunakan shared memory
  * hunter.c: Program client hunter untuk login, melihat dungeon, melakukan raid, dan bertarung antar hunter

## 4.1 system.c
1. Header
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

3. Fungsi Tampilkan Hunter
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

 4. Fungsi Tampilkan Dungeon
    <pre>
    void tampilkan_dungeons() {
        printf("\n=== Daftar Dungeon ===\n");
        for (int i = 0; i < system_data->num_dungeons; i++) {
            struct Dungeon d = system_data->dungeons[i];
            printf("%d. %s | Min Level: %d | EXP: %d | ATK: %d | HP: %d | DEF: %d\n",
                   i+1, d.name, d.min_level, d.exp, d.atk, d.hp, d.def);
        }
    }
    </pre>
    * Melakukan loop semua dungeon
    * Menampilkan data lengkapnya

 5. Fungsi Buat Dungeon
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
    }
    </pre>
    * Cek apakah dungeon sudah penuh
    * Minta input nama dungeon
    * Input minimum level
    * Generate nilai-nilai stat secara acak
    * Tambahkan notifikasi dungeon baru

    
   


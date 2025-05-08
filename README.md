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
       int current_notification_index; </pre>
   * struct Hunter menyimpan data statistik setiap hunter
   * struct Dungeon Menyimpan data dungeon beserta shared memory key unik
   * struct SystemData menyimpan semua informasi global dari sistem (hunters, dungeons, notifications)

   3. Fungsi Tampilkan Hunter
   


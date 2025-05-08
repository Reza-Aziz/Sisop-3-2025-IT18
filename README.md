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
1. Struktur Data
<pre>
struct Hunter {
    char username[MAX_USERNAME];
    int level;
    int exp;
    int atk;
    int hp;
    int def;
    int banned;
}; </pre>
* Struktur untuk menyimpan data statistik setiap hunter
* Begitu juga dengan struct dungeon yang menimpan data dungeon beserta shared memory key unik


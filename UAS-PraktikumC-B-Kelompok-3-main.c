#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FILE_BINARY "keuangan.bin"
#define FILE_CSV "laporan_bulanan.csv"
#define FILE_TXT "laporan_bulanan.txt"
#define MAX_TRANSAKSI 1000

typedef struct {
    int id;
    char tanggal[15];   // Format: YYYY-MM-DD
    char kategori[30];  // Contoh: Makan, Transportasi, Gaji
    char tipe[15];      // "Pemasukan" atau "Pengeluaran"
    double nominal;
} CatatanKeuangan;

/* --- Fungsi Utilitas --- */

void dapatkan_tanggal_hari_ini(char *buffer, size_t ukuran) {
    time_t sekarang = time(NULL);
    struct tm *info_waktu = localtime(&sekarang);
    strftime(buffer, ukuran, "%Y-%m-%d", info_waktu);
}

int sama_ignore_case(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

void baca_baris(char *buffer, size_t ukuran) {
    if (fgets(buffer, (int)ukuran, stdin) != NULL) {
        buffer[strcspn(buffer, "\r\n")] = '\0';
    }
}

/* --- Operasi File Binary (keuangan.bin) --- */

int muat_semua_transaksi(CatatanKeuangan daftar[], int maks) {
    FILE *fp = fopen(FILE_BINARY, "rb");
    if (fp == NULL) {
        return 0; // File belum ada (belum ada transaksi)
    }

    int jumlah = 0;
    while (jumlah < maks &&
           fread(&daftar[jumlah], sizeof(CatatanKeuangan), 1, fp) == 1) {
        jumlah++;
    }

    fclose(fp);
    return jumlah;
}

int dapatkan_id_berikutnya(void) {
    CatatanKeuangan daftar[MAX_TRANSAKSI];
    int jumlah = muat_semua_transaksi(daftar, MAX_TRANSAKSI);
    if (jumlah == 0) {
        return 1;
    }
    return daftar[jumlah - 1].id + 1;
}

int simpan_transaksi_baru(const CatatanKeuangan *catatan) {
    FILE *fp = fopen(FILE_BINARY, "ab");
    if (fp == NULL) {
        perror("Gagal membuka file binary");
        return 0;
    }

    size_t tertulis = fwrite(catatan, sizeof(CatatanKeuangan), 1, fp);
    fclose(fp);
    return tertulis == 1;
}

/* --- Tampilan & Ekspor Laporan (CSV / TXT) --- */

void tampilkan_riwayat(const char *filter_bulan) {
    CatatanKeuangan daftar[MAX_TRANSAKSI];
    int jumlah = muat_semua_transaksi(daftar, MAX_TRANSAKSI);

    if (jumlah == 0) {
        printf("\n[Info] Belum ada riwayat transaksi di %s.\n", FILE_BINARY);
        return;
    }

    double total_pemasukan = 0.0;
    double total_pengeluaran = 0.0;
    int ditemukan = 0;

    printf("\n===========================================================================\n");
    if (filter_bulan != NULL && strlen(filter_bulan) > 0) {
        printf(" RIWAYAT TRANSAKSI BULAN: %s\n", filter_bulan);
    } else {
        printf(" SEMUA RIWAYAT TRANSAKSI\n");
    }
    printf("===========================================================================\n");
    printf("%-4s | %-12s | %-12s | %-18s | %15s\n",
           "ID", "Tanggal", "Tipe", "Kategori", "Nominal (Rp)");
    printf("---------------------------------------------------------------------------\n");

    for (int i = 0; i < jumlah; i++) {
        if (filter_bulan != NULL && strlen(filter_bulan) > 0) {
            if (strncmp(daftar[i].tanggal, filter_bulan, 7) != 0) {
                continue;
            }
        }

        printf("%-4d | %-12s | %-12s | %-18s | %15.2f\n",
               daftar[i].id,
               daftar[i].tanggal,
               daftar[i].tipe,
               daftar[i].kategori,
               daftar[i].nominal);

        if (sama_ignore_case(daftar[i].tipe, "Pemasukan")) {
            total_pemasukan += daftar[i].nominal;
        } else {
            total_pengeluaran += daftar[i].nominal;
        }
        ditemukan++;
    }

    if (ditemukan == 0) {
        printf(" Tidak ada transaksi pada periode tersebut.\n");
    }

    printf("===========================================================================\n");
    printf("Total Pemasukan   : Rp %.2f\n", total_pemasukan);
    printf("Total Pengeluaran : Rp %.2f\n", total_pengeluaran);
    printf("Saldo Bersih      : Rp %.2f\n", total_pemasukan - total_pengeluaran);
    printf("===========================================================================\n");
}

void ekspor_ke_csv(const CatatanKeuangan daftar[], int jumlah, const char *filter_bulan) {
    FILE *fp = fopen(FILE_CSV, "w");
    if (fp == NULL) {
        perror("Gagal membuat file CSV");
        return;
    }

    double total_pemasukan = 0.0;
    double total_pengeluaran = 0.0;

    fprintf(fp, "ID,Tanggal,Tipe,Kategori,Nominal\n");

    for (int i = 0; i < jumlah; i++) {
        if (filter_bulan != NULL && strlen(filter_bulan) > 0) {
            if (strncmp(daftar[i].tanggal, filter_bulan, 7) != 0) {
                continue;
            }
        }

        fprintf(fp, "%d,%s,%s,\"%s\",%.2f\n",
                daftar[i].id,
                daftar[i].tanggal,
                daftar[i].tipe,
                daftar[i].kategori,
                daftar[i].nominal);

        if (sama_ignore_case(daftar[i].tipe, "Pemasukan")) {
            total_pemasukan += daftar[i].nominal;
        } else {
            total_pengeluaran += daftar[i].nominal;
        }
    }

    fprintf(fp, "\n,,,Total Pemasukan,%.2f\n", total_pemasukan);
    fprintf(fp, ",,,Total Pengeluaran,%.2f\n", total_pengeluaran);
    fprintf(fp, ",,,Saldo Bersih,%.2f\n", total_pemasukan - total_pengeluaran);

    fclose(fp);
    printf("[Sukses] Laporan CSV berhasil diekspor ke '%s'.\n", FILE_CSV);
}

void ekspor_ke_txt(const CatatanKeuangan daftar[], int jumlah, const char *filter_bulan) {
    FILE *fp = fopen(FILE_TXT, "w");
    if (fp == NULL) {
        perror("Gagal membuat file TXT");
        return;
    }

    double total_pemasukan = 0.0;
    double total_pengeluaran = 0.0;

    fprintf(fp, "===========================================================================\n");
    fprintf(fp, " LAPORAN REKAPITULASI KEUANGAN (%s)\n",
            (filter_bulan && strlen(filter_bulan) > 0) ? filter_bulan : "Semua Periode");
    fprintf(fp, "===========================================================================\n");
    fprintf(fp, "%-4s | %-12s | %-12s | %-18s | %15s\n",
            "ID", "Tanggal", "Tipe", "Kategori", "Nominal (Rp)");
    fprintf(fp, "---------------------------------------------------------------------------\n");

    for (int i = 0; i < jumlah; i++) {
        if (filter_bulan != NULL && strlen(filter_bulan) > 0) {
            if (strncmp(daftar[i].tanggal, filter_bulan, 7) != 0) {
                continue;
            }
        }

        fprintf(fp, "%-4d | %-12s | %-12s | %-18s | %15.2f\n",
                daftar[i].id,
                daftar[i].tanggal,
                daftar[i].tipe,
                daftar[i].kategori,
                daftar[i].nominal);

        if (sama_ignore_case(daftar[i].tipe, "Pemasukan")) {
            total_pemasukan += daftar[i].nominal;
        } else {
            total_pengeluaran += daftar[i].nominal;
        }
    }

    fprintf(fp, "===========================================================================\n");
    fprintf(fp, "Total Pemasukan   : Rp %.2f\n", total_pemasukan);
    fprintf(fp, "Total Pengeluaran : Rp %.2f\n", total_pengeluaran);
    fprintf(fp, "Saldo Bersih      : Rp %.2f\n", total_pemasukan - total_pengeluaran);
    fprintf(fp, "===========================================================================\n");

    fclose(fp);
    printf("[Sukses] Laporan teks berhasil diekspor ke '%s'.\n", FILE_TXT);
}

/* --- Penanganan Argumen CLI --- */

void tampilkan_bantuan_cli(const char *nama_program) {
    printf("Penggunaan:\n");
    printf("  Mode Interaktif : %s\n", nama_program);
    printf("  Quick-Add CLI   : %s --tambah <Pemasukan|Pengeluaran> <Kategori> <Nominal> [YYYY-MM-DD]\n",
           nama_program);
    printf("Contoh:\n");
    printf("  %s --tambah Pengeluaran Makan 25000\n", nama_program);
}

int proses_argumen_cli(int argc, char *argv[]) {
    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        tampilkan_bantuan_cli(argv[0]);
        return 0;
    }

    if (strcmp(argv[1], "--tambah") != 0) {
        fprintf(stderr, "[Error] Argumen tidak dikenal: %s\n", argv[1]);
        tampilkan_bantuan_cli(argv[0]);
        return 1;
    }

    if (argc < 5 || argc > 6) {
        fprintf(stderr, "[Error] Format --tambah tidak sesuai.\n");
        tampilkan_bantuan_cli(argv[0]);
        return 1;
    }

    CatatanKeuangan catatan;
    catatan.id = dapatkan_id_berikutnya();

    // Validasi & normalisasi tipe transaksi
    if (sama_ignore_case(argv[2], "Pemasukan")) {
        strcpy(catatan.tipe, "Pemasukan");
    } else if (sama_ignore_case(argv[2], "Pengeluaran")) {
        strcpy(catatan.tipe, "Pengeluaran");
    } else {
        fprintf(stderr, "[Error] Tipe harus 'Pemasukan' atau 'Pengeluaran'.\n");
        return 1;
    }

    // Kategori
    strncpy(catatan.kategori, argv[3], sizeof(catatan.kategori) - 1);
    catatan.kategori[sizeof(catatan.kategori) - 1] = '\0';

    // Validasi nominal
    char *sisa = NULL;
    catatan.nominal = strtod(argv[4], &sisa);
    if (sisa == argv[4] || *sisa != '\0' || catatan.nominal <= 0) {
        fprintf(stderr, "[Error] Nominal tidak valid: %s\n", argv[4]);
        return 1;
    }

    // Tanggal (opsional di argv[5], default hari ini)
    if (argc == 6) {
        strncpy(catatan.tanggal, argv[5], sizeof(catatan.tanggal) - 1);
        catatan.tanggal[sizeof(catatan.tanggal) - 1] = '\0';
    } else {
        dapatkan_tanggal_hari_ini(catatan.tanggal, sizeof(catatan.tanggal));
    }

    if (simpan_transaksi_baru(&catatan)) {
        printf("[Sukses] Transaksi tersimpan ke %s:\n", FILE_BINARY);
        printf("  ID: %d | Tanggal: %s | %s | %s | Rp %.2f\n",
               catatan.id, catatan.tanggal, catatan.tipe, catatan.kategori, catatan.nominal);
        return 0;
    }

    return 1;
}

/* --- Menu Interaktif --- */

void menu_tambah_interaktif(void) {
    CatatanKeuangan catatan;
    char input[64];

    catatan.id = dapatkan_id_berikutnya();

    printf("\n--- Tambah Transaksi Baru ---\n");
    printf("Tipe (1 = Pemasukan, 2 = Pengeluaran): ");
    baca_baris(input, sizeof(input));
    if (strcmp(input, "1") == 0 || sama_ignore_case(input, "Pemasukan")) {
        strcpy(catatan.tipe, "Pemasukan");
    } else if (strcmp(input, "2") == 0 || sama_ignore_case(input, "Pengeluaran")) {
        strcpy(catatan.tipe, "Pengeluaran");
    } else {
        printf("[Error] Pilihan tipe tidak valid.\n");
        return;
    }

    printf("Kategori (misal: Makan, Gaji, Kos)   : ");
    baca_baris(catatan.kategori, sizeof(catatan.kategori));
    if (strlen(catatan.kategori) == 0) {
        strcpy(catatan.kategori, "Umum");
    }

    printf("Nominal (Rp)                         : ");
    baca_baris(input, sizeof(input));
    catatan.nominal = strtod(input, NULL);
    if (catatan.nominal <= 0) {
        printf("[Error] Nominal harus lebih besar dari 0.\n");
        return;
    }

    char default_tgl[15];
    dapatkan_tanggal_hari_ini(default_tgl, sizeof(default_tgl));
    printf("Tanggal [YYYY-MM-DD, Enter = %s]: ", default_tgl);
    baca_baris(input, sizeof(input));
    if (strlen(input) == 0) {
        strcpy(catatan.tanggal, default_tgl);
    } else {
        strncpy(catatan.tanggal, input, sizeof(catatan.tanggal) - 1);
        catatan.tanggal[sizeof(catatan.tanggal) - 1] = '\0';
    }

    if (simpan_transaksi_baru(&catatan)) {
        printf("[Sukses] Transaksi ID #%d berhasil ditambahkan!\n", catatan.id);
    }
}

void menu_ekspor_interaktif(void) {
    CatatanKeuangan daftar[MAX_TRANSAKSI];
    int jumlah = muat_semua_transaksi(daftar, MAX_TRANSAKSI);

    if (jumlah == 0) {
        printf("\n[Info] Tidak ada data transaksi untuk diekspor.\n");
        return;
    }

    char filter_bulan[15];
    char pilihan[10];

    printf("\n--- Ekspor Laporan Rekapitulasi ---\n");
    printf("Filter bulan (YYYY-MM, kosongkan untuk semua): ");
    baca_baris(filter_bulan, sizeof(filter_bulan));

    printf("Format ekspor (1 = CSV, 2 = TXT, 3 = Keduanya): ");
    baca_baris(pilihan, sizeof(pilihan));

    if (strcmp(pilihan, "1") == 0) {
        ekspor_ke_csv(daftar, jumlah, filter_bulan);
    } else if (strcmp(pilihan, "2") == 0) {
        ekspor_ke_txt(daftar, jumlah, filter_bulan);
    } else if (strcmp(pilihan, "3") == 0) {
        ekspor_ke_csv(daftar, jumlah, filter_bulan);
        ekspor_ke_txt(daftar, jumlah, filter_bulan);
    } else {
        printf("[Error] Pilihan format tidak valid.\n");
    }
}

int main(int argc, char *argv[]) {
    // Jika dijalankan dengan argumen terminal (contoh: ./program --tambah ...)
    if (argc > 1) {
        return proses_argumen_cli(argc, argv);
    }

    // Jika dijalankan tanpa argumen, masuk ke mode menu interaktif
    char pilihan[10];
    while (1) {
        printf("\n=== APLIKASI PENCATAT KEUANGAN PRIBADI ===\n");
        printf("1. Tambah Transaksi\n");
        printf("2. Lihat Semua Riwayat & Saldo\n");
        printf("3. Filter Riwayat per Bulan (YYYY-MM)\n");
        printf("4. Ekspor Laporan Bulanan (CSV / TXT)\n");
        printf("0. Keluar\n");
        printf("Pilih menu: ");
        baca_baris(pilihan, sizeof(pilihan));

        if (strcmp(pilihan, "1") == 0) {
            menu_tambah_interaktif();
        } else if (strcmp(pilihan, "2") == 0) {
            tampilkan_riwayat(NULL);
        } else if (strcmp(pilihan, "3") == 0) {
            char bulan[15];
            printf("Masukkan bulan (contoh: 2026-11): ");
            baca_baris(bulan, sizeof(bulan));
            tampilkan_riwayat(bulan);
        } else if (strcmp(pilihan, "4") == 0) {
            menu_ekspor_interaktif();
        } else if (strcmp(pilihan, "0") == 0) {
            printf("Terima kasih. Data tersimpan aman di %s.\n", FILE_BINARY);
            break;
        } else {
            printf("[Error] Menu tidak tersedia.\n");
        }
    }

    return 0;
}

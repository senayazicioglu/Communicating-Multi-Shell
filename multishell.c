/**
Sena Betül Yazıcıoğlu - 20120205061 
Alihan Durmuş - 20120205028

Multishell kısmını genel olarak birlikte tamamladık. Fakat sonunda Sena kalan eksiklikleri tamamladı ve hataları çözdü.

Bu kod, çoklu shell uygulamasını gerçekleştirir. Kod, birden fazla alt işlemde, her biri bir xterm penceresinde çalışan singleshell uygulamasını çalıştırır. Çalıştırılan her shellin çıktısı paylaşılan bellek bölgesinde birleştirilir ve log dosyasına yazılır. Ayrıca, her bir shell işlemi bir PID ile izlenir.*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>

#define MY_FILE_SIZE 1024
#define MY_SHARED_FILE_NAME "/sharedlogfile"
#define MAX_SHELL 10
#define DEFAULT_NSHELL 2

/*100 millisaniye*/
#define XTERM_WAIT_TIME 100000  

/* global değişkenler tanımlanır */
char *addr = NULL; /* mmap adresi */
int fd = -1; /* paylaşılan dosya nesnesi için fd tanımlandı */

/* fonksiyon prototipleri */
int initmem();
void run_shell(int index);

int main(int argc, char **argv) {

    int nshell;
    pid_t pid[MAX_SHELL];
    int i;
    time_t now;
    struct tm *tm_info;
    char datetime[20];
    FILE *log_file;

    /* bu kısımda ChatGPT'den yardım aldık*/
    /* komut satırı argümanlarını ayrıştır */
    if (argc < 2) {
        nshell = DEFAULT_NSHELL; /* herhangi bir argüman sağlanmazsa, varsayılan shell sayısını ayarla */
    } else {
        nshell = atoi(argv[1]); /* ilk komut satırı argümanını tamsayıya dönüştür */
        if (nshell < 1 || nshell > MAX_SHELL) {
            fprintf(stderr, "Error: Number of shells must be between 1 and %d\n", MAX_SHELL);
            exit(1);
        }
    }

    /* Paylaşılan belleği başlat */
    if (initmem() != 0) {
        fprintf(stderr, "Error: Failed to initialize shared memory\n");
        exit(1);
    }

    /* bu kısımda ChatGPT'den yardım aldık*/
    /* nshell örnekleri oluştur */
    for (i = 0; i < nshell; i++) {
        pid[i] = fork(); /* Yeni işlem oluştur */
        if (pid[i] < 0) { /* hata */
            perror("Error: fork");
            exit(1);
        } else if (pid[i] == 0) { /* child process */
            run_shell(i);
            exit(0);
        } else { /* parent process */
            printf("Created shell #%d with pid %d\n", i, pid[i]);
            usleep(XTERM_WAIT_TIME); /* Bir sonraki xterm'e başlamadan önce bekle*/
        }
    }

    /* child process'lerin bitmesini bekle */
    for (i = 0; i < nshell; i++) {
        waitpid(pid[i], NULL, 0);
    }

    /* log dosyasına yaz */
    now = time(NULL);
    tm_info = localtime(&now);
    strftime(datetime, sizeof(datetime), "%Y%m%d_%H%M%S", tm_info);
    char filename[50];
    sprintf(filename, "shelllog-%s.txt", datetime);
    log_file = fopen(filename, "w");
    if (log_file == NULL) {
        perror("Error: multishell: fopen");
        exit(1);
    }
    fprintf(log_file, "%s", addr);
    fclose(log_file);

    /* paylaşılan hafızayı serbest bırak */
    munmap(addr, MY_FILE_SIZE);
    close(fd);

    return 0;
}

/* paylaşılan hafızayı başlat */
int initmem() {
    fd = shm_open(MY_SHARED_FILE_NAME, O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (fd < 0) {
        perror("Error: multishell: shm_open");
        return -1;
    }
    if (ftruncate(fd, MY_FILE_SIZE) == -1) {
        perror("Error: multishell: ftruncate");
        return -1;
    }
    addr = mmap(NULL, MY_FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        perror("Error: multishell: mmap");
        return -1;
    }
    return 0;
}

/* tek bir shell örneği çalıştır */
void run_shell(int index) {
  
    char command[256];
    sprintf(command, "xterm -hold -e './singleshell %d'", index + 1);
    int result = system(command);
  
    if (result == -1) {
        perror("Error: multishell: system");
    }
}

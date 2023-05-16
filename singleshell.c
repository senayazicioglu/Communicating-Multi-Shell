/** 
Sena Betül Yazıcıoğlu - 20120205061 
Alihan Durmuş - 20120205028

Singleshell kısmını genel olarak birlikte tamamladık. Fakat sonunda Alihan kalan eksiklikleri tamamladı ve hataları çözdü.

Bu kod, kullanıcının girdiği komutları, çıktıları bir dosyaya ve stdout'a yazdırır.
Program ayrıca tarih ve zaman bilgilerini de dosyaya, paylaşılan bellek alanına yazdırır. 
*/
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
#include <errno.h>
/* Girilen komutların maksimum boyutu */
#define INBUF_SIZE 256  

/* Dosya boyutu ve adı (multishell.c ile aynı olmalıdır) */
#define MY_FILE_SIZE 1024
#define MY_SHARED_FILE_NAME "/sharedlogfile"

char *addr = NULL; /* Paylaşılan bellek alanı için pointer tanımlandı */
int fd = -1; /* Paylaşılan bellek alanının dosya tanımlayıcısı */

int initmem() { /* Paylaşılan bellek alanını başlatır ve dosya boyutunu ayarlar */

    fd = shm_open(MY_SHARED_FILE_NAME, O_RDWR, S_IRUSR | S_IWUSR); /*Dosya MY_SHARED_FILE_NAME adıyla O_RDWR modunda açılır ve fd değişkenine atanır.*/
    if (fd < 0){
        perror("singleshell.c:fd:line31");
        exit(1);
    }

  /*Dosya boyutu, ftruncate() fonksiyonu kullanılarak ayarlanır */
    if (ftruncate(fd, MY_FILE_SIZE) == -1) {
        perror("ftruncate");
        exit(1);
    }
  /*mmap() fonksiyonu kullanılarak paylaşılan bellek alanına bir pointer (addr) atanır.
  Bellek alanı, PROT_READ | PROT_WRITE ile okuma ve yazma işlemleri için korunur.
  Bellek alanı, MAP_SHARED seçeneğiyle diğer process'lerle paylaşılabilir hale getirilir.*/
    addr = mmap(NULL, MY_FILE_SIZE,
                PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  /*Eğer bellek alanı oluşturulamazsa, program hata verir ve sonlanır.
Fonksiyon başarılı bir şekilde tamamlanırsa, 0 döndürülür */
    if (addr == MAP_FAILED){
        perror("singleshell.c:mmap:");
        close(fd);
        exit(1);
    }
    return 0;
}

/*Bu fonksiyon verilen mesajı log.txt dosyasına ekler.*/
int write_to_logfile(const char *message)
{
  
    FILE *fp;
    fp = fopen("log.txt", "a");/*Dosya, "a" (append) modunda açılır ve mesaj sonuna bir newline karakteri eklenerek yazdırılır */
    if (fp == NULL) {
        perror("fopen");
        return -1;
    }

    /* Mesajı log dosyasına yazdır */
    fputs(message, fp);
    fputc('\n', fp);
    fclose(fp);

  /*Fonksiyon başarılı bir şekilde tamamlanırsa, 0 döndürülür */
    return 0;
}

int main(int argc, char *argv[])
{
    
    initmem(); /* Paylaşılan bellek alanını başlat */
    int n = 0; /* Başlangıç tarih ve zamanını dosyaya ve paylaşılan bellek alanına yazdırmak için sayaç*/
    pid_t pid;
    int status;
    char inbuf[INBUF_SIZE]; /* Kullanıcının girdiği komutu almak için buffer*/
    FILE *fp;
    fp = fopen("log.txt", "a");
    if (fp == NULL) {
        perror("fopen");
        return -1;
    }
   
  
    int pipefd[2];

    while (1) {  /* İlk çalıştırma için tarih ve zaman bilgilerini dosyaya ve paylaşılan bellek alanına yaz */
	
	if (n==0){
	    fprintf(fp,"\n Başlangıç Tarih ve Zaman: \n"); /* log dosyasına başlangıç tarihi yazdırılır*/
	    fclose(fp); /*dosya kapatılır*/
	    time_t current_time;
            time(&current_time);
            char *time_str = ctime(&current_time);
            time_str[strlen(time_str) - 1] = '\0'; /* son karakteri kaldır */
            fprintf(fp, "%s\n", time_str); /*log dosyasına yazdır*/
            strncat(addr, "\nBaşlangıç Tarih ve Zaman: \n",30); /*paylaşılan bellek alanına başlangıç tarih ve zamanı yazdırılır*/
            strncat(addr, time_str, strlen(time_str)); /*paylaşılan bellek alanına zaman dizisi log dosyasına yazdırılır*/
	    
            write_to_logfile(addr); /*log dosyasına yazdırma fonksiyonu çağrılır*/
	    n++; /*n değişkeni 1 arttırılır. Bu sayede başlangıç tarihi ve zamanı sadece bir kere yazdırılır.*/
 	}
        printf("myshell> ");
        fgets(inbuf, INBUF_SIZE, stdin);
        
        /* girilen komutların başına kendi process id'sini ekliyoruz */
        char pid_str[16];
        snprintf(pid_str, sizeof(pid_str), "\nParent ID: %d \n ", getpid());
        strncat(addr, pid_str, sizeof(pid_str));

              if (strcmp(inbuf, "exit\n") == 0) {
            /* Program exit ile sonlandığında tarih ay gün saat yazdırır */
            fprintf(fp,"\nÇıkış Tarih ve Zaman: ");
            time_t current_time;
            time(&current_time);
            char *time_str = ctime(&current_time);
	    time_str[strlen(time_str) - 1] = '\0'; /* son karakteri kaldır */
            fprintf(fp, "%s\n", time_str); /* log dosyasına yazdır */
            strncat(addr, "\nÇıkış Tarih ve Zaman: ", 28);
            strncat(addr, time_str, strlen(time_str));
            write_to_logfile(addr); /*Program dosyaya exit ile sonlandığında tarih ay gün saat yazdırır */
            exit(EXIT_SUCCESS);
        }


   
      /* shell komutlarını ve çıktılarını dosyaya ve stdout'a yazdırıyoruz -> bu kısımda ChatGPT'den yardım aldık */
        if (pipe(pipefd) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
        
        pid = fork();
        if (pid == 0) {
            /* Çocuk işlem, komutu çalıştır */
            char *args[INBUF_SIZE / 2 + 1]; /* Maksimum argüman sayısı için yeterli alan ayırdık */
            int argc = 0;
            char *token = strtok(inbuf, " \n\t");
            while (token) {
                args[argc++] = token;
                token = strtok(NULL, " \n\t");
            }
            args[argc] = NULL;

             
            /* Çıktıyı pipe'a yönlendir -> bu kısımda ChatGPT'den yardım aldık */
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);

            /* execvp() kullanarak komutu çalıştır */
            if (execvp(args[0], args) == -1) {
                perror("Error executing command");
                strcpy(addr, "Error executing command\n");
                write_to_logfile(addr);
                exit(EXIT_FAILURE);
            }
        } else if (pid < 0) {
            /* Fork hatası */
            perror("Fork error");
            exit(EXIT_FAILURE);
        } else {
            /* Ebeveyn işlem, çocuk işlemin tamamlanmasını bekle ve çıktıları yazdır */
            char buffer[256];
            ssize_t nbytes;

        close(pipefd[1]); /* pipe'ın yazma ucu kapatılır */

            /* pipe'dan gelen veriyi buffer'a okur */
          
        while ((nbytes = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {

            buffer[nbytes] = '\0'; /* son karakter null karakteri yapılır */

            /* log dosyasına child ID'si ve buffer'daki veri yazdırılır */
            snprintf(pid_str, sizeof(pid_str), "\nChild ID: %d \n ", pid);
            strncat(addr, pid_str, sizeof(pid_str));
            strncat(addr, buffer, sizeof(buffer));
            write_to_logfile(addr);

            /* buffer'daki veri stdout'a yazdırılır */
            printf("%s", buffer);
        }

        close(pipefd[0]); /* pipe'ın okuma ucu kapatılır */
        waitpid(pid, &status, 0); /* child işlemi tamamlanana kadar beklenir */
    }
}

/* Paylaşılan bellek alanı kapatılır ve bellek serbest bırakılır */
munmap(addr, MY_FILE_SIZE);

/* Paylaşılan bellek alanı dosyası kapatılır */
close(fd);

return 0;
}

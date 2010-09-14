#ifndef SIMU_STDIO_H
#define SIMU_STDIO_H

#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif

FILE *simu_fopen(const char *path, const char *mode);
FILE *simu_fdopen(int fildes, const char *mode);
FILE *simu_freopen(const char *path, const char *mode, FILE *stream);
int simu_fclose(FILE *fp);
int simu_fcloseall (void);

int simu_fflush(FILE *stream);
void simu_clearerr(FILE *stream);
int simu_feof(FILE *stream);
int simu_ferror(FILE *stream);
int simu_fileno(FILE *stream);

int simu_fseek(FILE *stream, long offset, int whence);
long simu_ftell(FILE *stream);
int simu_fgetpos(FILE *stream, fpos_t *pos);
int simu_fsetpos(FILE *stream, const fpos_t *pos);
void simu_rewind(FILE *stream);

int simu_setvbuf(FILE *stream, char *buf, int mode, size_t size);

size_t simu_fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t simu_fwrite(const void *ptr, size_t size, size_t nmemb,
		   FILE *stream);
int simu_fputc(int c, FILE *stream);
int simu_fputs(const char *s, FILE *stream);
int simu_fgetc(FILE *stream);
char* simu_fgets(char *s, int size, FILE *stream);
int simu_ungetc(int c, FILE *stream);

int simu_remove (const char *pathname);

#ifdef __cplusplus
}
#endif

#endif /* SIMU_STDIO_H */

#ifndef SIMU_STDIO_H
#define SIMU_STDIO_H

#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif

FILE *dce_fopen (const char *path, const char *mode);
FILE *dce_fdopen (int fildes, const char *mode);
FILE *dce_freopen (const char *path, const char *mode, FILE *stream);
int dce_fclose (FILE *fp);
int dce_fclose_unconditional (FILE *fp);
int dce_fcloseall (void);

int dce_fflush (FILE *stream);
void dce_clearerr (FILE *stream);
int dce_feof (FILE *stream);
int dce_ferror (FILE *stream);
int dce_fileno (FILE *stream);

int dce_fseek (FILE *stream, long offset, int whence);
long dce_ftell (FILE *stream);
int dce_fgetpos (FILE *stream, fpos_t *pos);
int dce_fsetpos (FILE *stream, const fpos_t *pos);
void dce_rewind (FILE *stream);

int dce_setvbuf (FILE *stream, char *buf, int mode, size_t size);

size_t dce_fread (void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t dce_fwrite (const void *ptr, size_t size, size_t nmemb,
                   FILE *stream);
int dce_fputc (int c, FILE *stream);
int dce_fputs (const char *s, FILE *stream);
int dce_fgetc (FILE *stream);
char* dce_fgets (char *s, int size, FILE *stream);
int dce_ungetc (int c, FILE *stream);

int dce_remove (const char *pathname);

#ifdef __cplusplus
}
#endif

#endif /* SIMU_STDIO_H */

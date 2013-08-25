#ifndef SIMU_STDIO_H
#define SIMU_STDIO_H

#include <stdio.h>
#include <stdio_ext.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

FILE * dce_fopen (const char *path, const char *mode);
FILE * dce_fopen64 (const char *path, const char *mode);
FILE * dce_fdopen (int fildes, const char *mode);
FILE * dce_freopen (const char *path, const char *mode, FILE *stream);
int dce_fclose (FILE *fp);
int dce_fclose_unconditional (FILE *fp);
int dce_fclose_onexec (FILE *file);
int dce_fcloseall (void);

int dce_fflush (FILE *stream);
void dce_clearerr (FILE *stream);
int dce_feof (FILE *stream);
int dce_ferror (FILE *stream);
int dce_fileno (FILE *stream);

int dce_fseek (FILE *stream, long offset, int whence);
int dce_fseeko (FILE *stream, off_t offset, int whence);
long dce_ftell (FILE *stream);
off_t dce_ftello (FILE *stream);
int dce_fgetpos (FILE *stream, fpos_t *pos);
int dce_fsetpos (FILE *stream, const fpos_t *pos);
void dce_rewind (FILE *stream);

int dce_setvbuf (FILE *stream, char *buf, int mode, size_t size);
void dce_setbuf (FILE *stream, char *buf);
void dce_setbuffer (FILE *stream, char *buf, size_t size);
void dce_setlinebuf (FILE *stream);

size_t dce_fread (void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t dce_fwrite (const void *ptr, size_t size, size_t nmemb,
                   FILE *stream);
int dce_fputc (int c, FILE *stream);
int dce_fputs (const char *s, FILE *stream);
int dce_fgetc (FILE *stream);
char* dce_fgets (char *s, int size, FILE *stream);
int dce_ungetc (int c, FILE *stream);

int dce_remove (const char *pathname);

int dce_printf (const char *format, ...);
int dce_getchar (void);
int dce__IO_getc (FILE *stream);
int dce_putchar (int __c);
int dce__IO_putc (int __c, FILE *__stream);
int dce_puts (const char *__s);

void dce_perror (const char *s);

int dce___printf_chk (int __flag, __const char *__restrict __format, ...);
int dce___vfprintf_chk (FILE *__restrict __stream, int __flag,
                        __const char *__restrict __format, _G_va_list __ap);
int dce___fprintf_chk (FILE *__restrict __stream, int __flag,
                       __const char *__restrict __format, ...);
int dce___snprintf_chk (char *__restrict __s, size_t __n, int __flag,
                        size_t __slen, __const char *__restrict __format, ...);

void dce___fpurge (FILE *stream);
size_t dce___fpending (FILE *stream);

int dce_asprintf (char **strp, const char *fmt, ...);
int dce_vasprintf (char **strp, const char *fmt, va_list ap);
int dce_vsnprintf (char *s, size_t si, const char *f, va_list ap);
int dce___vsnprintf_chk (char *__restrict __s, size_t __n, int __flag,
                                               size_t __slen,
                         __const char *__restrict __format, _G_va_list __ap);


#ifdef __cplusplus
}
#endif

#endif /* SIMU_STDIO_H */

#ifndef DCE_MMAN_H
#define DCE_MMAN_H


#ifdef __cplusplus
extern "C" {
#endif

void * dce_mmap (void *start, size_t length, int prot, int flags,
                 int fd, off_t offset);
void * dce_mmap64 (void *start, size_t length, int prot, int flags,
                   int fd, off64_t offset);
int dce_munmap (void *start, size_t length);

#ifdef __cplusplus
}
#endif

#endif /* DCE_MMAN_H */

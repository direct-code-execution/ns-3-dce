#ifndef SIMU_MMAN_H
#define SIMU_MMAN_H


#ifdef __cplusplus
extern "C" {
#endif

void *simu_mmap (void *start, size_t length, int prot, int flags,
		 int fd, off_t offset);
void *simu_mmap64 (void *start, size_t length, int prot, int flags,
		   int fd, off64_t offset);
int simu_munmap (void *start, size_t length);

#ifdef __cplusplus
}
#endif

#endif /* SIMU_MMAN_H */

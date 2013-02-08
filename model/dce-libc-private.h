#ifndef DCE_LIBC_PRIVATE_H
#define DCE_LIBC_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

struct cpu_features;

extern const struct cpu_features * __get_cpu_features (void);

const struct cpu_features * dce___get_cpu_features (void);

#ifdef __cplusplus
}
#endif


#endif /* DCE_LIBC_PRIVATE_H */

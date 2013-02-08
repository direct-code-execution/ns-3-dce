#ifndef DCE_IF_H
#define DCE_IF_H


#ifdef __cplusplus
extern "C" {
#endif

unsigned dce_if_nametoindex (const char *ifname);
char * dce_if_indextoname (unsigned ifindex, char *ifname);


#ifdef __cplusplus
}
#endif

#endif /* DCE_IF_H */

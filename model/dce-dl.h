
void *dce_dlopen(const char *filename, int flag);
void *dce_dlsym(void *handle, const char *symbol);
char *dce_dlerror (void);
int dce_dlclose(void *handle);

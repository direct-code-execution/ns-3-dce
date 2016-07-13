DCE(time_t, time, (time_t *t));
DCE(struct tm *, gmtime, (const time_t *timep));
DCE(struct tm * , localtime, (const time_t *timep));
DCE(char * , ctime, (const time_t *timep));
DCE(char * , asctime, (const struct tm *tm));
DCE(int , clock_gettime, (clockid_t which_clock, struct timespec *tp));
DCE(int , sysinfo, (struct sysinfo *info));
DCE(void , tzset, (void));
DCE(int , clock_getres, (clockid_t c, struct timespec *r));
DCE(int , utime, (const char *filename, const struct utimbuf *times));

DCE(int , timer_create,(clockid_t clockid, struct sigevent *sevp, timer_t *timerid));
DCE(int , timer_settime, (int timerid, int flags, 
    const struct itimerspec *new_value, struct itimerspec *old_value));
DCE(int , timer_gettime,(int timerid, struct itimerspec *cur_value));
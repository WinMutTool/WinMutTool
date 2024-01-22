static long default_usec = 0;
void set_default_timer(long usec) { default_usec = usec; }
long get_default_timer() { return default_usec; }

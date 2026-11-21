#ifndef SA1Z_ZTHREADS_H
#define SA1Z_ZTHREADS_H
int zthreads_selftest(void);
int zthreads_check(void);
void zgame_start(void (*entry)(void));
int zgame_step(void);
void native_vsync(void);
#endif

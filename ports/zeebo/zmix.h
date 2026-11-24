/* zmix.h — mixer de som por software (ver zmix.c) */
#ifndef ZMIX_H
#define ZMIX_H

#include <AEEShell.h>

#define ZMIX_STREAMS 2 /* 0: música, 1: voz / áudio de filme */
#define ZMIX_VOICES 16

int zmix_start(IShell *shell);
void zmix_shutdown(void);
int zmix_active(void);
void zmix_stop_all(void);
void zmix_master(int gain); /* 0..256 */

/* ADX de um arquivo (a partir de base, len bytes) ou de memória (copiada) */
int zmix_stream_file(int slot, const char *name, int base, int len, int gain);
int zmix_stream_mem(int slot, const unsigned char *adx, int len, int gain);
void zmix_stream_stop(int slot);
void zmix_stream_gain(int slot, int gain); /* 0..256 */
int zmix_stream_playing(int slot);

/* efeito PCM 16 bits mono; devolve o índice da voz ou -1 */
int zmix_voice(int id, int prio, const short *pcm, unsigned n, int rate, int loop_s,
               int loop_e, int gl, int gr);
void zmix_voice_stop(int id);

unsigned zmix_reads(void);

#endif

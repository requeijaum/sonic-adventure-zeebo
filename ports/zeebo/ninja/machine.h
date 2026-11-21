/* machine.h — shim ARM do header SH-4 do decomp.
 *
 * O único uso no caminho de draw é o PREF da store queue. No Zeebo não há
 * store queue do PowerVR: o sink GLES1 ignora o endereço, então o prefetch
 * vira no-op. */
#ifndef MACHINE_H
#define MACHINE_H

#define prefetch(p) ((void)(p))

#endif /* MACHINE_H */

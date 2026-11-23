/* fatores de mistura do PVR (códigos do Ninja, 0..7) no Zeebo. O buffer de
 * cor do Zeebo é RGB565, sem alfa de destino (Developer Guide): DST_ALPHA
 * (6) vale 1 e 1-DST_ALPHA (7) vale 0. */
#ifndef ZBLEND_H
#define ZBLEND_H

static inline int zblend_code(int code)
{
    code &= 7;
    if (code == 6) {
        return 1; /* ONE */
    }
    if (code == 7) {
        return 0; /* ZERO */
    }
    return code;
}

#endif

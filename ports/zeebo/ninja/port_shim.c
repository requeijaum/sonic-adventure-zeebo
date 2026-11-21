/* port_shim.c — o mínimo de src/system/port.h que njDraw.c usa.
 *
 * No port Dreamcast, present_record guarda o draw para o quadro e
 * port_wide_* implementa o 16:9. No Z1 não há replay nem 16:9: o draw passa
 * direto e nada é espremido. */

#include <sg_xpt.h>

#include "system/port.h"

Int present_record(enum present_kind kind, const void *what, Int count, Int a,
                   Float pri, Uint32 attr)
{
    (void)kind;
    (void)what;
    (void)count;
    (void)a;
    (void)pri;
    (void)attr;
    return TRUE;
}

Int present_sprite(enum present_kind kind, const void *what, Int count, Int a,
                   Float pri, Uint32 attr)
{
    return present_record(kind, what, count, a, pri, attr);
}

/* njSprite.c define port_wide_x/squeezes/aspect; aqui ficam o pivô e o
 * estado do 16:9 que ele lê */
Float port_wide_anchor = 0.0F;
Int port_wide_cover = FALSE;

Int port_wide_covers(const Float *x, Int count, Int stride)
{
    (void)x;
    (void)count;
    (void)stride;
    return FALSE;
}


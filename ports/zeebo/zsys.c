/* zsys.c — timer e pad do Shinobi sobre o BREW.
 *
 * syTmr*: o jogo mede tempo pelo contador da CPU (1,28 us por conta). Aqui a
 * conta sai de GETUPTIMEMS (1 ms = 781,25 contas), contada desde syTmrInit.
 *
 * pd*: o applet entrega EVT_KEY/EVT_KEY_RELEASE a zsys_key; pdExecPeripheral-
 * Server calcula on/off/press/release por frame, como o servidor do Katana.
 * Só a porta A0 existe (PDD_PORT_A0). Sem analógico: eixos ficam em 0. */

#include <AEEStdLib.h>
#include <AEEVCodes.h>

#include <kamui.h>
#include <shinobi.h>

#include "zstrip.h"

/* --- timer --- */

static uint32 tmr_base;

void syTmrInit(void)
{
    tmr_base = GETUPTIMEMS();
}

void zlog(const char *what, const char *name, int value);

/* ms de BREW para o perfil do .mod (lá GETUPTIMEMS não existe) */
unsigned int zprof_ms(void)
{
    return GETUPTIMEMS();
}

Uint32 syTmrGetCount(void)
{
    zlog("poll", "syTmrGetCount", (int)__builtin_return_address(0));
    uint32 ms = GETUPTIMEMS() - tmr_base;

    /* ms * 781.25 = ms * 3125 / 4, sem estourar 32 bits por ~15 min */
    return (ms / 4) * 3125u + (ms % 4) * 781u;
}

Uint32 syTmrDiffCount(Uint32 count1, Uint32 count2)
{
    return count2 - count1;
}

Uint32 syTmrCountToMicro(Uint32 count)
{
    return (count / 25u) * 32u + (count % 25u) * 32u / 25u; /* *1.28 */
}

Uint32 syTmrMicroToCount(Uint32 micro)
{
    return (micro / 32u) * 25u + (micro % 32u) * 25u / 32u; /* /1.28 */
}

/* --- pad --- */

static PDS_PERIPHERAL pad;
static Uint32 held;
/* toque: aperto e solta no mesmo intervalo de leitura ainda contam um quadro */
static Uint32 latched;

static Uint32 key_bit(uint16 key)
{
    switch (key) {
    case AVK_UP:     return PDD_DGT_KU;
    case AVK_DOWN:   return PDD_DGT_KD;
    case AVK_LEFT:   return PDD_DGT_KL;
    case AVK_RIGHT:  return PDD_DGT_KR;
    case AVK_SELECT: return PDD_DGT_TA;
    /* o controle do Zeebo (e o zeebx: Z/Enter) manda o botão 1 como
     * AVK_CONFIRM (0xe064); sem Start no controle, ele vale A e Start */
    case 0xe064:     return PDD_DGT_TA | PDD_DGT_ST;
    case AVK_CLR:    return PDD_DGT_TB;
    case AVK_SOFT1:  return PDD_DGT_ST;
    case AVK_SOFT2:  return PDD_DGT_TX;
    case AVK_SEND:   return PDD_DGT_TY;
    default:         return 0;
    }
}

void zsys_key(int down, uint16 key)
{
    Uint32 bit = key_bit(key);

    if (down) {
        zlog("pad", bit != 0 ? "tecla" : "tecla sem botao", key);
    }

    if (down) {
        held |= bit;
        latched |= bit;
    } else {
        held &= ~bit;
    }
}

void pdInitPeripheral(Sint32 logic, void *recvbuf, void *sendbuf)
{
    (void)logic;
    (void)recvbuf;
    (void)sendbuf;
    held = 0;
    latched = 0;
    pad.id = PDD_DEV_CONTROLLER;
    /* o jogo só lê a porta se o suporte disser controle (src/system/pad.c) */
    pad.support = PDD_DEV_CONTROLLER;
    pad.old = 0;
    pad.on = pad.off = pad.press = pad.release = 0;
}

void pdExecPeripheralServer(void)
{
    Uint32 now = held | latched;

    latched = 0;
    pad.id = PDD_DEV_CONTROLLER;
    /* o Dreamcast reporta botão pressionado como bit 0 em `on`/1 em `off`?
     * Não: `on` é o mask dos pressionados (ver comentário em shinobi.h) */
    pad.press = now & ~pad.old;
    pad.release = pad.old & ~now;
    pad.on = now;
    pad.off = ~now;
    pad.old = now;
}

const PDS_PERIPHERAL *pdGetPeripheral(Uint32 port)
{
    zlog("poll", "pdGetPeripheral", (int)__builtin_return_address(0));
    /* como no DC, toda porta tem um registro; as sem controle ficam zeradas
     * (o campo escreve per_info[1]->press sem olhar) */
    static PDS_PERIPHERAL empty[3];

    if (port == PDD_PORT_A0) {
        return &pad;
    }
    if (port == PDD_PORT_B0) return &empty[0];
    if (port == PDD_PORT_C0) return &empty[1];
    if (port == PDD_PORT_D0) return &empty[2];
    return 0;
}

const NJS_PERIPHERAL *njGetPeripheral(Uint32 port)
{
    return pdGetPeripheral(port);
}

/* --- ponte para a cena de teste --- */

static int pad_x, pad_y;
static Uint32 t_first, t_last;
static int t_frames, t_bad;

void zsys_frame(void)
{
    Uint32 now = syTmrGetCount();

    pdExecPeripheralServer();
    if (pad.on & PDD_DGT_KL) pad_x -= 4;
    if (pad.on & PDD_DGT_KR) pad_x += 4;
    if (pad.on & PDD_DGT_KU) pad_y -= 4;
    if (pad.on & PDD_DGT_KD) pad_y += 4;
    if (t_frames == 0) {
        t_first = now;
    } else if (now < t_last) {
        t_bad = 1; /* o contador voltou */
    }
    t_last = now;
    t_frames++;
}

int zsys_pad_x(void) { return pad_x; }
int zsys_pad_y(void) { return pad_y; }

/* 1 se o contador subiu ~1,28 us por conta a ~35 fps (entre 20 e 60 ms/frame) */
int zsys_timer_ok(void)
{
    Uint32 micro;

    if (t_frames < 10 || t_bad) {
        return 1; /* ainda sem amostra: não acusa */
    }
    micro = syTmrCountToMicro(t_last - t_first);
    micro /= (Uint32)(t_frames - 1);
    return micro >= 20000u && micro <= 60000u;
}

/* --- TMU do SH-4, contador 1 (src/system/timer.c) -------------------------
 * O jogo escreve TCNT1 = 12.500.000 e lê a contagem regressiva a 12,5 MHz
 * para medir o frame. Aqui a escrita marca o instante e a leitura devolve o
 * valor escrito menos o tempo passado (syTmrGetCount: 781,25 contas por ms;
 * 12.500 tiques do TMU por ms). */
Uint8 ztmu_tstr;
Uint32 ztmu_tcor1;
Uint16 ztmu_tcr1;
static Uint32 tcnt1_value;
static Uint32 tcnt1_written;
static Uint32 tcnt1_at;

Uint32 *ztmu_tcnt1_ref(void)
{
    Uint32 now = syTmrGetCount();

    if (tcnt1_value != tcnt1_written) { /* o jogo escreveu desde a última leitura */
        tcnt1_written = tcnt1_value;
        tcnt1_at = now;
    } else {
        /* contas do syTmr (1,28 us) para tiques do TMU (0,08 us): x16 */
        Uint32 ticks = (now - tcnt1_at) * 16u;

        tcnt1_value = ticks < tcnt1_written ? tcnt1_written - ticks : 0;
        tcnt1_written = tcnt1_value;
        tcnt1_at = now;
    }
    return &tcnt1_value;
}

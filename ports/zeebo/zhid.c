/* controle do Zeebo pelo IHID: os 16 botoes e os 2 analogicos.
 * EVT_KEY so entrega a cruz, o 1 e poucos botoes; o SA1 anda pelo analogico
 * e usa os gatilhos para a camera. Sem IHID (ou sem controle ligado) tudo
 * volta ao que o EVT_KEY ja da. Fonte: SDK do BREW 4 (AEEIHID.h,
 * AEEIHIDDevice.h, AEEHIDDevice_Joystick.h). */
#include <AEEStdLib.h>
#include <AEEShell.h>
#include <AEEClassIDs.h>
#include "AEEHID.bid"
#include "AEEIHID.h"
#include "AEEIHIDDevice.h"
#include "AEEHIDDevice_Joystick.h"

#include "zhid.h"

void zlog(const char *what, const char *name, int value);

#define NAXES 4 /* LX, LY, RX, RY */

static IHID *hid;
static IHIDDevice *dev;
static int is_boom;            /* o controle e um Boomerang (zboom.c) */
static zboom_t boom;
static int retry;              /* leituras ate tentar abrir o controle de novo */
static unsigned int btn_state; /* bit por botao, na ordem de ZHID_* */
/* valor de cada eixo (nX,nY,nZ,nRx,nRy,nRz) e o campo que cada um ocupa */
static int axis_field[NAXES] = {-1, -1, -1, -1};
static int axis_min[6], axis_max[6];

static int field_of(const AEEHIDPositionInfo *p, int i)
{
    switch (i) {
    case 0: return p->nX;
    case 1: return p->nY;
    case 2: return p->nZ;
    case 3: return p->nRx;
    case 4: return p->nRy;
    default: return p->nRz;
    }
}

static int bit_of(int uid)
{
    switch ((unsigned int)uid) {
    case AEEUID_HIDJoystick_DPad_Up:              return ZHID_UP;
    case AEEUID_HIDJoystick_DPad_Down:            return ZHID_DOWN;
    case AEEUID_HIDJoystick_DPad_Left:            return ZHID_LEFT;
    case AEEUID_HIDJoystick_DPad_Right:           return ZHID_RIGHT;
    case AEEUID_HIDJoystick_Start:                return ZHID_START;
    case AEEUID_HIDJoystick_Back:                 return ZHID_BACK;
    case AEEUID_HIDJoystick_Button_1:             return ZHID_B1;
    case AEEUID_HIDJoystick_Button_2:             return ZHID_B2;
    case AEEUID_HIDJoystick_Button_3:             return ZHID_B3;
    case AEEUID_HIDJoystick_Button_4:             return ZHID_B4;
    case AEEUID_HIDJoystick_Left_Shoulder_Upper:  return ZHID_ZL;
    case AEEUID_HIDJoystick_Left_Shoulder_Lower:  return ZHID_L2;
    case AEEUID_HIDJoystick_Right_Shoulder_Upper: return ZHID_ZR;
    case AEEUID_HIDJoystick_Right_Shoulder_Lower: return ZHID_R2;
    default:                                      return -1;
    }
}

/* abre o primeiro controle ligado; 0 se ainda nao ha nenhum */
static int open_device(void)
{
    int n = 0;
    int h[4];
    AEEHIDPositionInfo ax, lo, hi;
    AEEHIDDeviceInfo info;
    int want[NAXES] = {AEEUID_HIDJoystick_LeftThumb_X,
                       AEEUID_HIDJoystick_LeftThumb_Y,
                       AEEUID_HIDJoystick_RightThumb_X,
                       AEEUID_HIDJoystick_RightThumb_Y};
    int i, f;

    if (IHID_GetConnectedDevices(hid, AEEUID_HID_Joystick_Device, h, 4, &n)
            != SUCCESS || n < 1) {
        return 0;
    }
    if (IHID_CreateDevice(hid, h[0], &dev) != SUCCESS) {
        dev = 0;
        zlog("hid", "CreateDevice falhou", 0);
        return 0;
    }
    MEMSET(&info, 0, sizeof(info));
    is_boom = IHIDDevice_GetDeviceInfo(dev, &info) == SUCCESS
              && info.wVendorID == ZBOOM_VENDOR
              && info.wProductID == ZBOOM_PRODUCT;
    zboom_reset(&boom);
    MEMSET(&ax, 0, sizeof(ax));
    MEMSET(&lo, 0, sizeof(lo));
    MEMSET(&hi, 0, sizeof(hi));
    IHIDDevice_GetAxesInfo(dev, &ax);
    IHIDDevice_GetMinPositionInfo(dev, &lo);
    IHIDDevice_GetMaxPositionInfo(dev, &hi);
    for (f = 0; f < 6; f++) {
        axis_min[f] = field_of(&lo, f);
        axis_max[f] = field_of(&hi, f);
        for (i = 0; i < NAXES; i++) {
            if (field_of(&ax, f) == want[i]) {
                axis_field[i] = f;
            }
        }
    }
    zlog("hid", is_boom ? "Boomerang aberto" : "controle aberto", h[0]);
    return 1;
}

void zhid_open(IShell *shell)
{
    if (hid || !shell) {
        return;
    }
    if (ISHELL_CreateInstance(shell, AEECLSID_HID, (void **)&hid) != SUCCESS) {
        hid = 0;
        zlog("hid", "sem IHID", 0);
        return;
    }
    if (!open_device()) {
        zlog("hid", "sem controle", 0); /* zhid_poll tenta de novo */
    }
}

/* ha um controle IHID (com Start e Back proprios)? */
int zhid_ativo(void)
{
    return dev != 0;
}

/* o controle aberto e um Boomerang? zboom.c ja entrega os botoes no sentido do Dreamcast
 * (1 = A, 2 = B, HOME+1/2 = X/Y): nao passam pelo pareamento do Z-Pad em zsys.c */
int zhid_boomerang(void)
{
    return dev != 0 && is_boom;
}

void zhid_close(void)
{
    if (dev) {
        IHIDDevice_Release(dev);
        dev = 0;
    }
    if (hid) {
        IHID_Release(hid);
        hid = 0;
    }
    btn_state = 0;
    is_boom = 0;
    retry = 0;
}

/* contadores da entrada: o relatorio da janela so imprime o que mudou */
static int in_polls, in_polls_axis, in_btn_changes;
static int in_last_lx, in_last_ly, in_last_rx, in_last_ry, in_axis_seen;
static unsigned in_last_btn;

static int norm(int axis, const AEEHIDPositionInfo *p)
{
    int f = axis_field[axis];
    int span, v;

    if (f < 0) {
        return 0;
    }
    span = axis_max[f] - axis_min[f];
    if (span <= 0) {
        return 0;
    }
    /* 0..span -> -128..127, como o analogico do Dreamcast */
    v = (int)(((long long)(field_of(p, f) - axis_min[f]) * 255) / span) - 128;
    if (v > -16 && v < 16) {
        v = 0;
    }
    if (v < -128) {
        v = -128;
    }
    if (v > 127) {
        v = 127;
    }
    return v;
}

#define ZHID_EVENTS 64 /* eventos de botao lidos por leitura */
#define ZHID_FALHAS 120 /* leituras de posicao sem resposta ate dar o controle por perdido */

unsigned int zhid_poll(int *lx, int *ly, int *rx, int *ry)
{
    AEEHIDButtonInfo b;
    AEEHIDPositionInfo p;
    uint32 ts = 0;
    boolean lost = FALSE;
    int events;
    static int failures;

    *lx = *ly = *rx = *ry = 0;
    if (!dev) {
        /* o Z-Pad e o Boomerang sao sem fio: podem ligar depois do jogo */
        if (hid && --retry <= 0) {
            retry = 60;
            open_device();
        }
        return 0;
    }
    if (is_boom) {
        MEMSET(&p, 0, sizeof(p));
        if (IHIDDevice_GetPositionState(dev, &p) != SUCCESS) {
            if (++failures >= ZHID_FALHAS) {
                zlog("hid", "controle sumiu", failures);
                IHIDDevice_Release(dev);
                dev = 0;
                btn_state = 0;
                is_boom = 0;
                failures = 0;
                retry = 1;
            }
            return 0;
        }
        failures = 0;
        return zboom_update(&boom, (const unsigned int *)(void *)&p, lx, ly);
    }
    /* no maximo ZHID_EVENTS por leitura: um aparelho que despeje eventos sem parar nao pode
     * prender o jogo neste laço */
    for (events = 0; events < ZHID_EVENTS &&
         IHIDDevice_GetNextButtonEvent(dev, &b, &ts, &lost) == SUCCESS; events++) {
        int bit = bit_of(b.nButtonUID);

        if (bit >= 0) {
            if (b.nState) {
                btn_state |= 1u << bit;
            } else {
                btn_state &= ~(1u << bit);
            }
        }
    }
    MEMSET(&p, 0, sizeof(p));
    if (IHIDDevice_GetPositionState(dev, &p) != SUCCESS) {
        /* o controle sem fio sumiu (pilha, fora de alcance): depois de ZHID_FALHAS leituras
         * seguidas sem resposta solta tudo, para nao deixar botao preso, e volta a procurar */
        if (++failures >= ZHID_FALHAS) {
            zlog("hid", "controle sumiu", failures);
            IHIDDevice_Release(dev);
            dev = 0;
            btn_state = 0;
            is_boom = 0;
            failures = 0;
            retry = 1;
            return 0;
        }
    } else {
        failures = 0;
        *lx = norm(0, &p);
        *ly = norm(1, &p);
        *rx = norm(2, &p);
        *ry = norm(3, &p);
        in_polls++;
        if ((*lx | *ly | *rx | *ry) != 0) {
            in_polls_axis++;
            in_last_lx = *lx; in_last_ly = *ly; in_last_rx = *rx; in_last_ry = *ry;
        }
    }
    if (btn_state != in_last_btn) {
        in_btn_changes++;
        in_last_btn = btn_state;
    }
    return btn_state;
}

extern void zlogd(const char *what, const char *name, int value);
/* a conversao do jogo: pad_sticks (ang, stroke em bits de float) so existe com o jogo linkado */
extern struct { int ang; unsigned stroke; } pad_sticks[] __attribute__((weak));

void zhid_report(void)
{
    static int prev[9];
    static unsigned windows;
    int now[9], i;
    static const char *const names[9] = {
        "eixo ativo (0/1)", "mudancas de botao", "botoes (mascara)", "ultimo lx", "ultimo ly",
        "ultimo rx", "ultimo ry", "jogo: stroke x1000", "jogo: angulo (0..65535)"};
    union {
        unsigned u;
        float f;
    } s;

    /* leituras e leituras com eixo crescem a cada quadro: so a cada 8 janelas (16 s) */
    if ((windows++ & 7) == 0) {
        zlogd("tin", "leituras do controle (total)", in_polls);
        zlogd("tin", "leituras com eixo (total)", in_polls_axis);
    }
    s.u = pad_sticks != 0 ? pad_sticks[0].stroke : 0;
    now[0] = (in_last_lx | in_last_ly | in_last_rx | in_last_ry) != 0 &&
             in_polls_axis != 0 && in_polls_axis != in_axis_seen;
    in_axis_seen = in_polls_axis;
    now[1] = in_btn_changes; now[2] = (int)in_last_btn;
    now[3] = in_last_lx; now[4] = in_last_ly; now[5] = in_last_rx; now[6] = in_last_ry;
    now[7] = (int)(s.f * 1000.0f);
    now[8] = pad_sticks != 0 ? pad_sticks[0].ang : 0;
    for (i = 0; i < 9; i++) {
        if (now[i] != prev[i]) {
            zlogd("tin", names[i], now[i]);
        }
        prev[i] = now[i];
    }
}

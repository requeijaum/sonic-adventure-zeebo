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

void zhid_open(IShell *shell)
{
    int n = 0;
    int h[4];
    AEEHIDPositionInfo ax, lo, hi;
    int want[NAXES] = {AEEUID_HIDJoystick_LeftThumb_X,
                       AEEUID_HIDJoystick_LeftThumb_Y,
                       AEEUID_HIDJoystick_RightThumb_X,
                       AEEUID_HIDJoystick_RightThumb_Y};
    int i, f;

    if (hid || !shell) {
        return;
    }
    if (ISHELL_CreateInstance(shell, AEECLSID_HID, (void **)&hid) != SUCCESS) {
        hid = 0;
        zlog("hid", "sem IHID", 0);
        return;
    }
    if (IHID_GetConnectedDevices(hid, AEEUID_HID_Joystick_Device, h, 4, &n)
            != SUCCESS || n < 1) {
        zlog("hid", "sem controle", n);
        return; /* mantem o hid: vale tentar de novo em zhid_poll */
    }
    if (IHID_CreateDevice(hid, h[0], &dev) != SUCCESS) {
        dev = 0;
        zlog("hid", "CreateDevice falhou", 0);
        return;
    }
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
    zlog("hid", "controle aberto", h[0]);
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
}

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

unsigned int zhid_poll(int *lx, int *ly, int *rx, int *ry)
{
    AEEHIDButtonInfo b;
    AEEHIDPositionInfo p;
    uint32 ts = 0;
    boolean lost = FALSE;

    *lx = *ly = *rx = *ry = 0;
    if (!dev) {
        return 0;
    }
    while (IHIDDevice_GetNextButtonEvent(dev, &b, &ts, &lost) == SUCCESS) {
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
    if (IHIDDevice_GetPositionState(dev, &p) == SUCCESS) {
        *lx = norm(0, &p);
        *ly = norm(1, &p);
        *rx = norm(2, &p);
        *ry = norm(3, &p);
    }
    return btn_state;
}

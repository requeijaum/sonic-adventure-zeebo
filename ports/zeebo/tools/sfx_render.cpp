// sfx_render.cpp — renderiza os efeitos sonoros do jogo no host, com o backend de som do PC
// (engine/pc/sound.cpp, que faz o papel do MASARU.DRV no AICA), e os grava como IMA ADPCM
// mono para o IMedia do BREW tocar sem decodificar nada no ARM.
//
//   sfx_render DISCO SAIDA [arquivo.mlt ...]
//
// Cada MLT (Multi Unit: 'SMLT', registros de bancos SMPB/SMSB/SFPB...) vira SAIDA/<nome>.zsf:
//   'ZSF1', n (u32), n x { kind u8 (0 programa/nota, 1 sequencia), bank u8, number u8, flags u8,
//   rate u32, offset u32, size u32 }, depois os .wav (IMA ADPCM, mono)
//   flags bit 0: o som ainda soava no fim do clipe (laco: repetir o ultimo trecho)
// Uma nota de programa usa a tecla 60 em velocidade 127, como sound_slot_play do jogo.
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <vector>

#include "sound.h"

namespace {

constexpr uint32_t RATE_IN = SA_PC_SOUND_RATE; /* taxa do backend */
constexpr uint32_t RATE_OUT = 22050;
constexpr uint32_t MAX_SECONDS = 12;
constexpr uint32_t MAX_SEQ_SECONDS = 30;

uint32_t le32(const uint8_t *p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }

sa_audio_command midi(uint32_t port, uint32_t status, uint32_t d1, uint32_t d2)
{
    sa_audio_command c{};
    c.kind = SA_AUDIO_MIDI;
    c.key.id = port;
    c.value = static_cast<int32_t>(status | d1 << 8 | d2 << 16);
    return c;
}

/* IMA ADPCM: tabelas padrao */
const int STEP[89] = {7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707,
    1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845,
    8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
    32767};
const int INDEX[16] = {-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8};

struct Enc {
    int pred = 0, idx = 0;
    uint8_t code(int s)
    {
        int step = STEP[idx], diff = s - pred, c = 0;
        if (diff < 0) { c = 8; diff = -diff; }
        int d = step >> 3;
        if (diff >= step) { c |= 4; diff -= step; d += step; }
        if (diff >= step >> 1) { c |= 2; diff -= step >> 1; d += step >> 1; }
        if (diff >= step >> 2) { c |= 1; d += step >> 2; }
        pred += (c & 8) ? -d : d;
        pred = pred > 32767 ? 32767 : pred < -32768 ? -32768 : pred;
        idx += INDEX[c];
        idx = idx < 0 ? 0 : idx > 88 ? 88 : idx;
        return (uint8_t)c;
    }
};

void put16(std::vector<uint8_t> &b, uint32_t v) { b.push_back(v); b.push_back(v >> 8); }
void put32(std::vector<uint8_t> &b, uint32_t v) { put16(b, v & 0xffff); put16(b, v >> 16); }

/* .wav IMA ADPCM mono, blocos de 256 bytes (505 amostras) */
std::vector<uint8_t> wav_ima(const std::vector<int16_t> &pcm)
{
    constexpr uint32_t ALIGN = 256, PER = (ALIGN - 4) * 2 + 1;
    std::vector<uint8_t> data;
    for (size_t at = 0; at < pcm.size(); at += PER) {
        Enc e;
        e.pred = pcm[at];
        data.push_back(e.pred & 255); data.push_back((e.pred >> 8) & 255);
        data.push_back(0); data.push_back(0);
        uint8_t nib[PER - 1] = {};
        for (uint32_t i = 1; i < PER; i++) {
            int s = at + i < pcm.size() ? pcm[at + i] : pcm[pcm.size() - 1];
            nib[i - 1] = e.code(s);
        }
        for (uint32_t i = 0; i + 1 < PER; i += 2) data.push_back(nib[i] | nib[i + 1] << 4);
        data.resize(((data.size() - 1) / ALIGN + 1) * ALIGN); /* so por seguranca */
    }
    std::vector<uint8_t> w;
    uint32_t blocks = (uint32_t)(data.size() / ALIGN);
    w.insert(w.end(), {'R', 'I', 'F', 'F'}); put32(w, 4 + 8 + 20 + 12 + 8 + (uint32_t)data.size());
    w.insert(w.end(), {'W', 'A', 'V', 'E', 'f', 'm', 't', ' '}); put32(w, 20);
    put16(w, 0x11); put16(w, 1); put32(w, RATE_OUT); put32(w, RATE_OUT * ALIGN / PER);
    put16(w, ALIGN); put16(w, 4); put16(w, 2); put16(w, PER);
    w.insert(w.end(), {'f', 'a', 'c', 't'}); put32(w, 4); put32(w, (uint32_t)pcm.size());
    w.insert(w.end(), {'d', 'a', 't', 'a'}); put32(w, (uint32_t)data.size());
    w.insert(w.end(), data.begin(), data.end());
    (void)blocks;
    return w;
}

struct Clip { uint8_t kind, bank, number, flags; std::vector<uint8_t> wav; double seconds; };

/* renderiza ate o silencio (ou o maximo); devolve mono em RATE_OUT. sustained: ainda soava */
std::vector<int16_t> run(pc::Sound &s, uint32_t max_seconds, bool &sustained)
{
    std::vector<int16_t> mono;
    std::vector<int16_t> buf(2 * 441);
    uint32_t quiet = 0, heard = 0, frames = 0;
    double acc = 0.0, pos = 0.0;
    uint32_t phase = 0;
    int16_t last[2] = {0, 0};
    sustained = false;
    while (frames < RATE_IN * max_seconds) {
        s.render(buf.data(), 441);
        frames += 441;
        int peak = 0;
        for (int i = 0; i < 441; i++) {
            int m = (buf[2 * i] + buf[2 * i + 1]) / 2;
            if (abs(m) > peak) peak = abs(m);
            /* decimacao por media (RATE_IN -> RATE_OUT) com acumulador de fase */
            acc += m; pos += 1.0; phase += RATE_OUT;
            if (phase >= RATE_IN) {
                phase -= RATE_IN;
                mono.push_back((int16_t)(acc / pos)); acc = 0; pos = 0;
            }
        }
        (void)last;
        if (getenv("SFX_DEBUG") && frames < 441 * 12) fprintf(stderr, " chunk peak %d\n", peak);
        if (peak > 16) { heard = 1; quiet = 0; } else quiet += 441;
        if (heard && quiet > RATE_IN / 5) break;   /* 0,2 s de silencio depois de soar */
        if (!heard && quiet > RATE_IN * 3) break;  /* nunca soou */
    }
    sustained = frames >= RATE_IN * max_seconds;
    while (!mono.empty() && abs(mono.back()) < 8) mono.pop_back();
    return mono;
}

bool process(const std::string &path, const std::string &out)
{
    FILE *f = fopen(path.c_str(), "rb");
    if (!f) return false;
    std::vector<uint8_t> d;
    uint8_t tmp[65536];
    size_t n;
    while ((n = fread(tmp, 1, sizeof tmp, f)) > 0) d.insert(d.end(), tmp, tmp + n);
    fclose(f);
    if (d.size() < 0x20 || memcmp(d.data(), "SMLT", 4) != 0) return false;
    uint32_t count = le32(&d[8]);
    struct Rec { char tag[5]; uint32_t num, addr, size, off, len; };
    std::vector<Rec> recs;
    for (uint32_t i = 0; i < count && 0x20 + 0x20 * (i + 1) <= d.size(); i++) {
        const uint8_t *p = &d[0x20 + 0x20 * i];
        Rec r{}; memcpy(r.tag, p, 4);
        r.num = le32(p + 4); r.addr = le32(p + 8); r.size = le32(p + 12);
        r.off = le32(p + 16); r.len = le32(p + 20);
        recs.push_back(r);
    }
    std::vector<Clip> clips;
    std::vector<std::pair<uint32_t, uint32_t>> seqs; /* banco, quantidade */
    std::vector<std::pair<uint32_t, uint32_t>> progs;
    for (auto &r : recs) {
        if (r.off + r.len > d.size()) continue;
        if (!strcmp(r.tag, "SMSB") && r.len > 12) seqs.push_back({r.num, le32(&d[r.off + 12])});
        if (!strcmp(r.tag, "SMPB") && r.len > 0x40) {
            /* tabela de programas em +0x30, contagem em +0x28 do banco (visto nos dados) */
            progs.push_back({r.num, le32(&d[r.off + 0x14])});
        }
    }
    auto fresh = [&](pc::Sound &s) {
        for (auto &r : recs) {
            if (r.off + r.len > d.size()) continue;
            sa_audio_bank b{};
            b.kind = !strcmp(r.tag, "SMPB") ? SA_BANK_PROGRAMS : !strcmp(r.tag, "SMSB") ? SA_BANK_SEQUENCES : SA_BANK_OTHER;
            b.number = r.num; b.address = r.addr; b.bytes = r.len; b.data = &d[r.off];
            sa_result lr = s.load(b);
            if (getenv("SFX_DEBUG")) fprintf(stderr, "load %s bank %u addr %x len %x -> %d\n", r.tag, r.num, r.addr, r.len, (int)lr);
        }
    };
    pc::Sound s;
    if (!s.create()) { fprintf(stderr, "create falhou\n"); return false; }
    fresh(s);
    uint32_t total = 0;
    for (auto &pb : progs) {
        for (uint32_t p = 0; p < pb.second && p < 128; p++) {
            sa_audio_command c[] = {midi(0, 0xb3, 32, pb.first), midi(0, 0xc3, p, 0), midi(0, 0xb3, 7, 127),
                                    midi(0, 0xb3, 10, 64), midi(0, 0x93, 60, 127)};
            s.submit(c, 5);
            bool sus;
            auto pcm = run(s, MAX_SECONDS, sus);
            sa_audio_command off = midi(0, 0x83, 60, 0);
            s.submit(&off, 1);
            if (pcm.size() < 64) continue;
            Clip k{0, (uint8_t)pb.first, (uint8_t)p, (uint8_t)(sus ? 1 : 0), wav_ima(pcm), (double)pcm.size() / RATE_OUT};
            clips.push_back(std::move(k)); total++;
        }
    }
    for (auto &sb : seqs) {
        for (uint32_t q = 0; q < sb.second && q < 256; q++) {
            sa_audio_command c{};
            c.kind = SA_AUDIO_SEQUENCE; c.key.id = 2; c.sound = (int32_t)(sb.first << 8 | q); c.priority = 0;
            s.submit(&c, 1);
            bool sus;
            auto pcm = run(s, MAX_SEQ_SECONDS, sus);
            sa_audio_command st{}; st.kind = SA_AUDIO_SEQUENCE_STOP; st.key.id = 2;
            s.submit(&st, 1);
            if (pcm.size() < 64) continue;
            Clip k{1, (uint8_t)sb.first, (uint8_t)q, (uint8_t)(sus ? 1 : 0), wav_ima(pcm), (double)pcm.size() / RATE_OUT};
            clips.push_back(std::move(k)); total++;
        }
    }
    s.destroy();
    std::vector<uint8_t> o = {'Z', 'S', 'F', '1'};
    put32(o, (uint32_t)clips.size());
    uint32_t off = 8 + 16 * (uint32_t)clips.size();
    double secs = 0;
    for (auto &k : clips) {
        o.push_back(k.kind); o.push_back(k.bank); o.push_back(k.number); o.push_back(k.flags);
        put32(o, RATE_OUT); put32(o, off); put32(o, (uint32_t)k.wav.size());
        off += (uint32_t)k.wav.size(); secs += k.seconds;
    }
    for (auto &k : clips) o.insert(o.end(), k.wav.begin(), k.wav.end());
    FILE *w = fopen(out.c_str(), "wb");
    if (!w) return false;
    fwrite(o.data(), 1, o.size(), w);
    fclose(w);
    printf("%s: %u clipes, %.1f s, %zu bytes\n", path.c_str(), total, secs, o.size());
    return true;
}

} // namespace

int main(int argc, char **argv)
{
    if (argc < 3) { fprintf(stderr, "uso: sfx_render DISCO SAIDA [arq.mlt...]\n"); return 2; }
    std::string disc = argv[1], out = argv[2];
    for (int i = 3; i < argc; i++) {
        std::string stem = argv[i];
        stem = stem.substr(0, stem.rfind('.'));
        process(disc + "/" + argv[i], out + "/" + stem + ".zsf");
    }
    return 0;
}

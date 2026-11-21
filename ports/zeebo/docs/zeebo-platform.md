# Zeebo platform — hardware / OS / API facts (for deciding a graphics target)

Scope: authoritative facts for the Zeebo console (Tectoy + Qualcomm, Brazil/Mexico, 2009–2011),
with sources. Primary source is the **official Zeebo Developer Guide v0.97 (May 2009)**, which is
the console vendor's own spec and the most reliable document we have. Where public sources
disagree with it, that is called out explicitly.

---

## 1. CPU / SoC

| Item | Value | Source |
|---|---|---|
| SoC | Qualcomm **MSM7201A** (+ RTR6280 RF, PM7540 PMIC) | Zeebo Developer Guide §2.1, §3.1 |
| App/GPU CPU | **ARM11** running at **528 MHz** ("ARM11 / QDSP-5") | Dev Guide §2.1, §3.1; Wikipedia; CdrInfo press kit |
| Modem CPU | ARM9 / QDSP-4 (integrated in the MSM) | Dev Guide §2.1, §3.1 |
| DSP | QDSP-5 applications DSP, QDSP-4 modem DSP | Dev Guide §3.1 |
| Auxiliary | Mobile Display Processor (MDP) drives TV-out | Dev Guide §3.1 |
| ISA | **ARMv6** (ARM11 core). Instruction set is ARMv6 + Thumb; ARMv6 has no NEON. | Derived from ARM11; MAME drives it as `arm11_cpu_device` |
| Interconnect | 32-bit bus at 133 MHz | Dev Guide §3.1 |
| Floating point | **No hardware FPU** — the ARM11 here has no VFP in this configuration; floats are emulated in software and are slow. Use fixed point. | Dev Guide §3.2.2 |

Notes:
- The QDSP-5 here is not the "Snapdragon" branding; MSM7201A predates Snapdragon marketing but is
  the same generation as the MSM7xxx family (Adreno 130 era). The ARM11 in MSM7201A is an ARM1136J-S
  class core (ARMv6, no VFP/NEON in the main applications core).
- Fixed point format: `GLfixed` = **s15.16**, 32-bit signed (Dev Guide §3.2.2).

Sources:
- Zeebo Developer Guide 0.97 — https://www.tripleoxygen.net/files/devices/zeebo/doc/ZeeboDeveloperGuide0.97.pdf
- Wikipedia Zeebo — https://en.wikipedia.org/wiki/Zeebo
- CdrInfo/GDC 2009 press spec — https://cdrinfo.com/d7/content/zeebo-unveils-videogame-console-emerging-markets
- MAME skeleton driver — https://github.com/mamedev/mame/blob/master/src/mame/skeleton/zeebo_qualcomm_adreno130.cpp

---

## 2. GPU / graphics API — **the critical answer**

| Item | Value | Source |
|---|---|---|
| GPU | **Qualcomm Adreno 130** (rebrand of **ATI Imageon 3D**; Qualcomm bought ATI's Imageon team) | Dev Guide §2.1, §3.1; Wikipedia *Adreno*/*Imageon* |
| 2D/display | MDP (Mobile Display Processor), concurrent with 3D | Dev Guide §3.1 |
| Graphics API | **OpenGL ES 1.0 + most of OpenGL ES 1.1**, Common **and** Common-Lite profiles | Dev Guide §2.1, §7, §7.4 |
| Shaders / GLSL | **None. There is no shader support and no ES 2.0 support anywhere in the platform or SDK.** | Dev Guide (no shader/ES2 symbol exists in the entire 119-page document); Adreno 130 = fixed-function |
| Pipeline | **Fixed-function only**; hardware transform (vertex transform engine) + hardware texture combining; **no hardware lighting** | Dev Guide §7.4.1, §7.1.10 |

**Definitive answer to the critical question: the Zeebo is OpenGL ES 1.x fixed-function only.
There is no OpenGL ES 2.0 and no GLSL/shader path.** Qualcomm's own product line confirms this:
the Adreno 130 is explicitly documented as a *fixed-function graphics pipeline* supporting
"OpenGL ES 1.1, OpenVG 1.1, EGL 1.3, Direct3D Mobile, SVGT 1.2, DirectDraw, GDI". The **Adreno 200
(ATI Imageon Z430)** was Qualcomm's *first* OpenGL ES 2.0 GPU; it shipped in MSM7x27/QSD8x50, a
different SoC than the MSM7201A used by Zeebo. No homebrew or custom ES 2.0 driver exists for the
Adreno 130, and the two known Zeebo emulators (Infuse, Zeemu) implement ES 1.x / software paths only.

Sources:
- Zeebo Developer Guide §2.1/§7 (OpenGL ES 1.0+ Common Profile; ES 1.1 via extensions) — link above
- Wikipedia Qualcomm Adreno — https://en.wikipedia.org/wiki/Qualcomm_Adreno ("Adreno 130 is rebrand
  of Imageon 3D"; fixed-function; "Adreno 130 … supports OpenGL ES 1.1, OpenVG 1.1, EGL 1.3, D3D Mobile")
- Wikipedia Imageon — https://en.wikipedia.org/wiki/Imageon (Z430 = first Imageon certified for ES 2.0,
  later rebranded Adreno 200)
- Qualcomm "3D Graphics and SpeqG Update" (Imageon 3D core, MSM7xxx) — https://docslib.org/doc/5009769/
- PhoneDB MSM7201A — https://phonedb.net/index.php?m=processor&id=183&c=qualcomm_msm7201a&d=detailed_specs

### 2.1 Concrete graphics limits and features (from the Dev Guide)

These are the numbers that actually constrain a port. All from Chapter 7 unless noted.

- **Max texture size: 1024 × 1024**, dimensions **must be power-of-two**. (§7.1.3)
- **Texture cache: 8 KB**; best when an entire texture fits (e.g. 64×64×16bpp, 32×128). (§7.1.3)
- **Texture compression: ATITC only** (AMD/ATI `GL_COMPRESSED_RGB_ATI_TC` /
  `GL_COMPRESSED_RGBA_ATI_TC`, i.e. the Khronos `AMD_compressed_ATC_texture` set). §8.2.
  - Ratios: **6:1** RGB (24→4 bpp), **4:1** RGBA (32→8 bpp).
  - Authoring tools named: QXTextureConverter, The Compressonator, ATI_Compress. §8.2.
  - **No PVRTC** (that is PowerVR/Imagination, not Adreno/Imageon) and **no ETC**.
- **Frame buffer: 16-bit RGB565**, double-buffered. Optional **16-bit depth** and **4-bit stencil**. (§7.1.1, §7.4.1)
- **Blending**: alpha test + alpha blending, no measurable cost. (§7.1.9, §7.4.1)
- **Multisample / edge AA**: not listed as supported; only "32-bit precision in pixel pipe" and
  dithering. Assume **no hardware MSAA**. (§7.4.1)
- **Two texture units** + texture crossbar; **multitexturing** and `ARB_texture_env_combine` /
  `ARB_texture_env_dot3` in hardware. (§7.1.3, §7.2, §7.4.1)
- **Mip-mapping in hardware**: 10 mip levels, per-pixel LOD. Recommended filters:
  minify `GL_LINEAR_MIPMAP_NEAREST`, magnify `GL_LINEAR`. **Trilinear is not accelerated**
  (`GL_LINEAR_MIPMAP_LINEAR` is a big perf hit). Dual-texture filtering is nearest/bilinear only. (§7.1.3, §7.4.1)
- **No rendering to textures** ("Rendering to Textures — No"). (§7.2)
- **No hardware OpenGL ES lighting** ("Lighting supported in SW"; "no hardware support for OpenGL ES
  lighting"). Use multitexture lightmaps / dot3. (§7.1.10, §7.4.1)
- **Per-vertex fog** (interpolated); also "per fragment fog" listed. (§7.1.5, §7.4.1)
- **Vertex skinning with up to two matrices** (`OES_matrix_palette` = Yes). (§7.2, §7.4.1)
- **VBOs** (`ARB_vertex_buffer_object`) supported and strongly recommended. (§7.2, §8.1)
- **Point sprites**, `OES_point_size_array` supported. (§7.2)
- **Not supported**: `SGIS_generate_mipmap`, `OES_matrix_get`, `ARB_point_parameters`, user clip
  planes (clipping is software). (§7.2, §7.4.1)
- **Only one EGL context; one 3D app at a time.** (§7.4.2)
- **Binning / tile-based rendering**: the GPU can't fit its buffers in 256 KB SRAM, so it renders in
  **bins** — 2 bins for QVGA, **8 bins for VGA (640×480)**. Frame/Z/stencil live in EBI RAM and are
  copied into SRAM for the GPU. This is architecturally a *tiling/deferred* design, conceptually
  similar to the Dreamcast's PVR2 tile binning. (§3.1.2–3.1.3)

### 2.2 Reported performance (sources conflict — flagged)

- **Zeebo Developer Guide §2.1**: polygon performance **1.6 M triangles/s**; textured 3D fill rate
  "**63 M polygons (2 textures)**".
- **Qualcomm "3D Graphics and SpeqG Update" slide** (Imageon 3D in MSM7x00): peak **4 M tris/s**,
  **133 M pixels/s**.
- **Giant Bomb** repeats "4 million triangles/second"; **Wikipedia/uvlist** repeat the Dev Guide's
  1.6 M tris/s.

=> **Conflict**: 1.6 M vs 4 M tris/s. The safest reading is ~1.5–2 M tris/s *sustained* on Zeebo
(the Dev Guide, which is console-specific, says 1.6 M), with 4 M being the silicon's idealized peak.
Either way this is roughly comparable to a Dreamcast (~1.5–3 M polygons/s visible). Fill rate is the
bigger constraint (640×480×60 ≈ 18 M px/frame; 63 M px/s ≈ 3.5 backgrounds/overdraw).

---

## 3. Memory, storage, display

| Item | Value | Source |
|---|---|---|
| RAM | **160 MB total**: 128 MB DDR SDRAM in the MCP + **32 MB stacked DDR in the MSM7201A** (SMI) | Dev Guide §2.1, §3.1 |
| GPU-visible internal SRAM | 256 KB total (banks 128 KB + 64 KB + 64 KB) | Dev Guide §3.1 |
| Storage | **1 GB NAND flash** (1 GB eNAND, some sources say "128 MB NAND in MCP" for boot + 1 GB eNAND user) | Dev Guide §2.1; Wikipedia |
| App heap limit | **32 MB** per app; the game binary is fully loaded into the heap first | Dev Guide §5.1 |
| Stack | **32 KB** default (IThread can use heap-backed larger stacks) | Dev Guide §5.1 |
| Save data | Games must save under `/mod/<game>/udata/`; **max 64 KB** save data | Dev Guide §11.3, §5.1 |
| Video out | **VGA 640×480, 4:3**, composite RCA; PAL-M and NTSC | Dev Guide §2.1, §11.2 |
| Refresh | **60 Hz** (NTSC) / 50 Hz (PAL-M); MAME models 60 Hz | MAME driver |
| Safe area | Reserve 4–8 px border on all sides for CRT overscan | Dev Guide §11.2 |

Important: despite 160 MB of physical RAM, **a single BREW applet only gets ≤ 32 MB of heap** and a
32 KB stack. That is the number a port must budget against, not 160 MB.

---

## 4. OS / SDK / toolchain / distribution

| Item | Value | Source |
|---|---|---|
| OS / runtime | **Qualcomm BREW 4.0.2** (BREW 4.0.3 SDK also acceptable) | Dev Guide §1.2, §2.1, §4 |
| BREW later name | "BREW MP" (post-4); BREW 4.0.2 SP19 add-on usable | O3 Labs wiki (PT) |
| App model | BREW applets = **DLL modules** built as ARM code, dynamically loaded; one main 3D app at a time | Dev Guide §4 |
| Graphics integration | BREW wrappers `EGL_1x.c`, `GLES_1x.c`, `GLES_ext.c` over standard EGL/GL; `EGL_Init`/`GLES_Init` | Dev Guide §7.3 |
| Compiler | Official: **ARM RealView Developer Suite (RVDS) 2.2+**. Community: **GNU GCC** (Sourcery G++ Lite ARM EABI), plus SDK makefiles | Dev Guide §11.1; O3 Labs wiki |
| ELF→BREW module | **`elf2mod`** (`elf2mod.x`, `elf2mod.exe`) converts a normal ELF into a BREW `.mod` | O3 Labs wiki |
| Other tools | BREW MIF Editor (`.mif`), BREW Resource Editor, QXEngine / QXTextureConverter, Adreno Profiler | Dev Guide §2.4; forum tutorial |
| SDK packages | `ZeeboSDKPackage-0.93.zip` (36 MB) and `ZeeboSDKPackage-1.2.4.zip` (50 MB) mirrored publicly; original at `zeeboinc.com/zip/...` | tripleoxygen file index |
| Emulator/simulator | BREW simulator (Windows only) with a Zeebo device pack (`GEC_Device.dpk`); **no Linux simulator** | O3 Labs wiki; forum tutorial |
| Distribution | Digital only, **OTA over ZeeboNet** (3G HSUPA, fallback EDGE/GPRS), tied to carrier (Claro BR / Telcel MX). No cartridges/discs. | CdrInfo; Wikipedia |
| Game size policy | Low ≤ 8 MB, Standard ≤ 25 MB, Premium ≤ 50 MB (offline/preload for larger) | Dev Guide §2.3 |
| On-device file layout | `/mod/<game>/` with the game's `.mod`, `.mif`, resources and `udata/` | Dev Guide §11.3 |

Note on packaging: games are **BREW modules** (`.mod`) + a **`.mif`** class descriptor + a
**`.sig`** signature (and optionally `.bar` resource barrels) — not a simple ZIP. The public SDK
package is distributed as a ZIP, but the installed artifact is a signed BREW module set.

### 4.1 DRM / signing

- BREW uses **two RSA-1024 signatures**: one for the developer, one for "TRUE BREW" (TBT) testing.
  The `.mif` is encrypted/tied to the device **ESN/IMEI**, and `.sig` files gate installation, to
  prevent sideloading. (LPC Wiki; Dev Guide §11.4)
- To run your own build on hardware you must **generate a per-console test `.sig`** via Qualcomm's
  developer extranet and load it with **BREW AppLoader** over USB (console in "USB Download mode").
  (Dev Guide §11.4, §11.6)
- Commercial release requires **NSTL TBT certification + Zeebo Inc. QA** before signing/submission.
  (Dev Guide §12)
- There is community knowledge of bypasses (patched firmware / signature-check removal, ESN
  spoofing), but **no official path to run unsigned binaries**; assume a signature/DRM barrier for
  homebrew. (LPC Wiki)

### 4.2 Networking

ZeeboNet over 3G (HSUPA), scaling to 2.5G EDGE / 2G GPRS; transparent to the user, no plan needed.
Console ID is the **IMEI**. SMS server-initiated only. (CdrInfo; Dev Guide §2.1)

---

## 5. Audio

| Item | Value | Source |
|---|---|---|
| Subsystem | Qualcomm **CMX** synthesis + wavetable; **QAudioFX** 3D audio | Dev Guide §9 |
| Encoded formats | **PCM (linear), IMA ADPCM, QCP/QCELP, MP3** | Dev Guide §9.1 |
| MIDI formats | **MIDI (SP-MIDI/GM1/GM2 SMF), XMF/mXMF, PMD (CMX), DLS** | Dev Guide §9.1.2 |
| Polyphony | **72-voice** MIDI total; 512 KB wavetable, 44 kHz | Dev Guide §2.1, §9.4 |
| Concurrency | **4 MIDI/PMD objects + 4 encoded objects simultaneously**; formats mixable | Dev Guide §9.2, §9.4 |
| ADX | **Not supported.** (ADX is CRI's Dreamcast/PS format.) | — |
| FM synthesis | Not described; DSP is wavetable/encoded only | Dev Guide §9 |

Audio resource cost is tiny: each ADPCM/PCM/QCP object ≈ **2% of DSP load**, and audio is fully
offloaded from the ARM11 and does **not** compete with the Adreno 130 (Dev Guide §9). 3D audio
(position/rolloff/reverb/doppler) is available for encoded objects via QAudioFX (§9.3).

**Implication:** Dreamcast ADX music must be **decoded and re-encoded** (to MP3 for streaming music,
or IMA ADPCM / QCP for SFX). Plan for a 4-stream music/SFX budget, not the Dreamcast's 64 voices.

Sources: Dev Guide Ch. 9; Wikipedia (8-channel audio summary).

---

## 6. Input (Z-Pad)

- **D-Pad** (4-way), **two analog sticks**, **four face buttons (1–4)**, **two shoulder triggers
  (ZL/ZR)**, **Home button** — 7 buttons + D-pad + 2 sticks total (Wikipedia / Grokipedia).
- Developer Guide naming: D-pad; **B1/B2** = left/right analog; **C = 1/2/3/4**; **D1/D2** =
  left/right triggers; **E = Home**. UI navigation uses A, B1, 1–4, Home; B2/D1/D2 are game-only.
- Connection: **USB HID**; up to **2 controllers** (P1/P2). Rumble API exists but the shipped Z-Pad
  **does not support rumble**. Remapping via `AEEHIDButtons.c` / `AEEHIDThumbsticks.c`.
  Home generates `AVK_CLR`; right-stick click → `AVK_SELECT`.
- Optional **"Boomerang"** motion accessory (accelerometer) for Zeebo Sports titles.

Sources: Dev Guide §2.2.1, §6; Wikipedia Zeebo; Grokipedia; StrategyWiki.

---

## 7. Modern homebrew / dev / emulation

- **Infuse** (Tuxality) — "Qualcomm BREW subsystem reimplementation and Zeebo high-level emulator",
  clean-room, dynarmic ARM JIT. Claims playable Double Dragon, Crash Nitro Kart 3D, Zeebo Family
  Pack. **Graphics supported are "OpenGL ES 1.x" / software paths — no ES 2.0.** Ports for
  Windows/macOS/Linux/Steam Deck/Haiku. https://tuxality.net/projects/infuse_zeebo_emulator
- **Zeemu** (mrpostiga) — experimental Zeebo emulator: BREW HLE, ARM core, input, audio, and
  "software rendering and **OpenGL ES-style** graphics paths". https://github.com/mrpostiga/zeemu
- **MAME** — a skeleton `zeebo_qualcomm_adreno130` driver exists (boots NAND bootstrap; no GPU).
  https://github.com/mamedev/mame/blob/master/src/mame/skeleton/zeebo_qualcomm_adreno130.cpp
- **SDK/tooling & docs** are mirrored at https://www.tripleoxygen.net/files/devices/zeebo/doc/
  (`ZeeboSDKPackage-0.93.zip`, `ZeeboSDKPackage-1.2.4.zip`, Dev Guide PDF, BREW OEM API PDF).
- **Community build recipe** (Linux): BREW 4.0.2 SP19 platform files + `elf2mod.x`, GCC Sourcery G++
  Lite ARM EABI, hand-rolled Makefile; test on hardware because there is no Linux BREW simulator.
  https://www.tripleoxygen.net/wiki/console/zeebo/ambiente_de_desenvolvimento
- **Is ES 2.0 actually available?** **No.** Every one of these projects models only ES 1.x / fixed
  function / software rendering. The Adreno 130 hardware is fixed-function, so ES 2.0 is not a
  software-configuration choice — it is physically absent.

---

## 8. Recommendation

### 8.1 Best graphics API to target on Zeebo

**Target OpenGL ES 1.1 fixed-function (Common profile). Do not target ES 2.0.**

Justification:
1. The Adreno 130 is a **fixed-function GPU**; ES 2.0/GLSL is not present in the hardware or the
   Zeebo/BREW SDK, and no driver or emulator exposes it. ES 2.0 is not achievable on the real device.
2. ES 1.x fixed function is a **near-exact conceptual match** for a Dreamcast-era renderer: hardware
   transform, per-vertex lighting/color, a hardware texture-combine stage, mipmapping, alpha blend,
   depth test, and 2 texture units.
3. Use the features the Zeebo actually accelerates: **VBOs, multitexture + `ARB_texture_env_combine`
   / `ARB_texture_env_dot3`, `OES_matrix_palette` (≤2 matrices), point sprites, `OES_draw_texture`**.
4. Avoid what it lacks: **no render-to-texture, no ES lighting, no trilinear in HW, no MSAA, max
   1024×1024 POT textures, single EGL context.** Architect the renderer around that.

### 8.2 What maps cleanly from a Dreamcast game (PVR2 / shader-less fixed pipeline / VQ textures / ADX)

**Conceptual mappings (not proven one-to-one equivalence):**
- **Fixed-function raster pipeline without shaders** → ES1.x state/material adapter.
- **SH-4-computed transforms/lighting** → retain CPU semantics or use qualified
  GLES vertex-transform paths. PVR2 is not a hardware T&L engine.
- **Per-vertex lighting and precomputed vertex colors** → ES 1.1 `glColor`/`GL_LIGHTING`-free vertex
  color and lightmap/dot3 combining. No shaders were needed on Dreamcast either.
- **Multitexture/specular/bump effects** → investigate texture combine/dot3;
  PVR2 bump encoding and sorting are not automatically equivalent.
- **Tile/bin-based rendering model** → Zeebo also bins (8 bins at VGA), so a deferred/tiled
  renderer's assumptions are not alien.
- **640×480 output, RGB565 frame buffer, mipmapped textures, alpha blending, fog, backface culling** →
  all directly supported.
- **Vertex color / diffuse+specular** → ES 1.1 color pipeline.

**Must be replaced / emulated:**
- **Textures: PVR "PVR textures" (twiddled / VQ / PAL4/8)** → **re-encode** to ATITC
  (`GL_COMPRESSED_RGB/RGBA_ATI_TC`) or uncompressed RGB565/RGBA4444/5551. Note PVR ≠ PVRTC; the Zeebo
   does **not** support PVRTC or ETC. Offline decode is a CPU/storage tradeoff;
   existing runtime decoding remains possible within measured budgets.
- **Render-to-texture effects** (mirrors, reflections, some SA effects) → **removed or faked**; the
  Zeebo has *no* rendering-to-texture. Replace with pre-baked textures, planar projection, or
  `glCopyTexImage2D` tricks (frame buffer → texture copy, which is merely 16-bit-format-sensitive).
  *(This is the single biggest gameplay/visual redesign item.)*
- **Hardware per-pixel lighting / any shader-like effects** → soften to vertex lighting + lightmaps;
  Zeebo has **no ES lighting** and no shaders.
- **Skinning with more than 2 bone matrices** → the Zeebo matrix palette is documented as **up to two
  matrices**; higher-bone vertex skinning must be pre-computed (bake) or split.
- **Textures larger than 1024×1024 or NPOT** → downscale and pad to POT.
- **Trilinear/mip filtering assumptions** → use `GL_LINEAR_MIPMAP_NEAREST`; trilinear is unaccelerated.
- **Multisample/edge AA** → drop it; use a 640×480 RGB565 target with no MSAA.
- **ADX audio** → application-decoded PCM WAV/raw PCM via IMedia is a possible
  endpoint; MP3/IMA re-encoding is optional. CMX documents **four MIDI objects
  plus four encoded objects**, not four music plus four SFX streams. Decoder
  allocation, software mixing and firmware support require physical tests;
  see `audio-backend-spike.md`. AICA synthesis is a separate missing component.
- **PVR2's hardware T&L note:** the Dreamcast's PVR2 (CLX2) does **not** actually contain programmable/
  hardware T&L — vertex transform/lighting runs on the **SH-4 CPU** (the PVR2 has a Triangle Setup FPU
  and the tile/ISP/TSP pipeline). So a Dreamcast engine already computes transforms/lighting in
  software; that code port to ARM11 is straightforward. The real mapping risk is **performance**
  (200 MHz SH-4 vs 528 MHz ARM11 but no FPU on Zeebo → fixed-point math) and **fill-rate/overdraw**,
  not API mismatch.
- **CPU/ISA & endianness**: SH-4 (little-endian, FPU) → ARMv6 soft-float. Fixed-point
  is useful for qualified renderer paths, not an unproven replacement of guest
  floating-point gameplay. Validate alignment and **32 KB stack / 32 MB heap**.
- **Disc/GD-ROM + save (VMU)** → Zeebo has no disc; assets load from NAND / `.mod` and saves go to
  `/mod/<game>/udata/` capped at **64 KB**.

**Bottom line:** the graphics target is **OpenGL ES 1.1 fixed-function** — it is the only option and
it happens to be a close architectural cousin of the Dreamcast's shader-less PVR2 pipeline. The hard
porting work is not the API; it is (a) **no render-to-texture**, (b) **texture re-encoding** off VQ,
(c) **≤2-matrix skinning and no ES lighting** (bake effects), (d) **fixed-point, 32 MB heap, 32 KB
stack**, and (e) **ADX decode/PCM streaming and MLT synthesis** under measured
CMX/application budgets. No physical audio/throughput proof exists yet.

---

## 9. Source list (URLs)

Official / vendor
- Zeebo Developer Guide v0.97 (May 2009, 119 pp.) —
  https://www.tripleoxygen.net/files/devices/zeebo/doc/ZeeboDeveloperGuide0.97.pdf
  (mirror: https://docslib.org/doc/375540/zeebo-developer-guide-gaming-for-the-next-billion)
- Zeebo SDK / docs file index — https://www.tripleoxygen.net/files/devices/zeebo/doc/
- BREW OEM API Reference for MSM — https://www.tripleoxygen.net/files/devices/zeebo/doc/BREWOemAPIReferenceforMSM.pdf

Hardware / specs
- Wikipedia Zeebo — https://en.wikipedia.org/wiki/Zeebo
- Wikipedia Qualcomm Adreno — https://en.wikipedia.org/wiki/Qualcomm_Adreno
- Wikipedia Imageon — https://en.wikipedia.org/wiki/Imageon
- PhoneDB MSM7201A — https://phonedb.net/index.php?m=processor&id=183&c=qualcomm_msm7201a&d=detailed_specs
- CdrInfo GDC 2009 spec sheet — https://cdrinfo.com/d7/content/zeebo-unveils-videogame-console-emerging-markets
- Grokipedia Zeebo (Z-Pad / Boomerang) — https://grokipedia.com/page/Zeebo
- MAME Zeebo driver — https://github.com/mamedev/mame/blob/master/src/mame/skeleton/zeebo_qualcomm_adreno130.cpp

Graphics / texture compression
- Khronos `AMD_compressed_ATC_texture` — https://registry.khronos.org/OpenGL/extensions/AMD/AMD_compressed_ATC_texture.txt
- Qualcomm "3D Graphics and SpeqG Update" (Imageon 3D perf) — https://docslib.org/doc/5009769/
- Qualcomm Adreno OpenGL ES Dev Guide (background) — https://docslib.org/doc/11771559/

OS / SDK / DRM / homebrew
- LPC Wiki: Qualcomm BREW (DRM/signing) — https://lpcwiki.miraheze.org/wiki/Qualcomm_BREW
- O3 Labs: dev environment (BREW MP SDK, elf2mod, GCC) — https://www.tripleoxygen.net/wiki/console/zeebo/ambiente_de_desenvolvimento
- Forum: SDK availability / packages — https://assemblergames.org/viewtopic.php?t=60997
- Infuse emulator — https://tuxality.net/projects/infuse_zeebo_emulator
- Zeemu emulator — https://github.com/mrpostiga/zeemu

Dreamcast (for the mapping)
- Sega Retro: Dreamcast technical specifications — https://segaretro.org/Sega_Dreamcast/Technical_specifications
- Copetti: Dreamcast Architecture — https://classic.copetti.org/writings/consoles/dreamcast/
- Dreamcast Hardware Specification Outline (PVR2/HOLLY/AICA) —
  https://segaretro.org/images/8/8b/Dreamcast_Hardware_Specification_Outline.pdf
- Dreamcast texture formats (VQ/twiddled/PAL) — https://dreamcast.wiki/Texture_Formats
- PowerVR2 / Tile Accelerator overview (mc.pp.se) — https://mc.pp.se/dc/pvr.html

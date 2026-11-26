# APIs gráficas do Zeebo (OpenGL ES / EGL) e o que isso significa para o port SA1

Data: 2026-11-25. Origem: leitura de documentação, headers do SDK e código (nenhum teste em
hardware). Tudo marcado **NÃO VERIFICADO** depende de um console real.

## 1. Resumo

- O Zeebo é um Qualcomm MSM7201A com GPU Adreno 130 (ATI Imageon). O 3D é **OpenGL ES 1.0
  Common / Common-Lite**; as "principais funcionalidades do 1.1" vêm por **extensões**
  (Developer Guide 0.97, cap. 7).
- O SDK declara bem mais extensões do que o guia garante. **Um dump feito num console real**
  (§3b, homebrew do Pedro/Hardwarelandia) resolveu isso: `GL_VERSION` = `OpenGL ES 1.0-CM`; **não há**
  `OES_framebuffer_object`, `glGenerateMipmapOES`, `OES_query_matrix` nem cube map; **há**
  `ARB_texture_mirrored_repeat`, `OES_compressed_paletted_texture` e blend minmax/subtract/separate.
- **Confirmado no aparelho (25/11 22:50): `AEEIID_GLES11` = NULL e `AEEIID_GLES10` = NON-NULL.** Logo `glTexParameteri` e
  `glTexEnvi`, as únicas 2 funções do port que o wrapper do SDK só chama com a interface 1.1, **não fazem nada no console**
  (§5). Limites: textura de 1024, 2 unidades, mipmap até o nível 10.
- O port usa só 36 funções GL, todas de ponto fixo (variantes `x`), e todas do header ES 1.0,
  **exceto** `glTexParameteri` e `glTexEnvi` (só no header 1.1). Risco concreto, ver §5.

## 2. Fontes

- Developer Guide 0.97 (`~/projects/zeebo-emulator/research/docs/sdk-extract/ZeeboDeveloperGuide0.97.md`):
  cap. 7 (3D), §7.2 Tabela 7-1, Apêndice B (lista de API suportada).
- Headers do SDK (`.../BREW-Simulator-brew_sdk/brew_sdk/sdk/inc/gles/`): `gles_1_0/gl.h`,
  `gles_1_1/gl.h`, `glESext.h`, `EGLext.h`, `egl.h`, `glQUALCOMM.h` (só inclui os dois de
  extensão), `gl_extensions.h` (obsoleto, idem).
- Wiki do Zeebo (`~/projects/zeebo-wiki/z33b0tek/pages/8-video.md`) e docs do emulador
  (`~/projects/zeebx-emu/docs/implementacao/06-video-3d.md`): extensões que os jogos retail
  procuram, decodificadas dos módulos.
- Código do emulador (`~/projects/zeebx-emu-copytex/src/machine/gl.rs`, `src/video/gles.rs`).
  Há dois checkouts do zeebx: `zeebx-emu` (branch `speedhack`) e `zeebx-emu-copytex` (usado nos
  testes desta sessão); a varredura do §8 foi feita neles e pode diferir entre si.
- Fonte do wrapper do SDK: `.../sdk/src/GLES_1x.c`, `.../sdk/inc/GLES_1x_support.h`.

## 3. O que o guia garante (Tabela 7-1)

| Funcionalidade | Extensão | Zeebo |
|---|---|---|
| Buffer Objects | ARB_vertex_buffer_object | Sim |
| Draw Texture | OES_draw_texture | Sim |
| Matrix Palette | OES_matrix_palette | Sim |
| Multitexture | - | Sim |
| Point Size Array | OES_point_size_array | Sim |
| Point Sprites | OES_point_sprite | Sim |
| Texture Combine | ARB_texture_env_combine | Sim |
| Texture Dot3 | ARB_texture_env_dot3 | Sim |
| Mipmap automático | SGIS_generate_mipmap | **Não** |
| Matrix Get | OES_matrix_get | **Não** |
| Point Parameters | ARB_point_parameters | **Não** |
| Render para textura | - | **Não** |
| Planos de corte do usuário | - | **Não** |
| Consultas de estado | - | Parcial ("Some") |

Mais: framebuffer 565 (16 bits), depth 16 bits, stencil 4 bits opcional (§7.1.1). O guia
desaconselha a extensão "Color Buffer" (sem sincronização 2D/3D no MSM7201A) e recomenda
`glDrawTex*` para blit de textura na tela (§7.1.2). O guia manda usar os wrappers
`EGL_1x.h`/`GLES_1x.h`, que escolhem a interface mais alta disponível; criar `AEECLSID_GL`/`EGL`
direto por `ISHELL_CreateInstance` está obsoleto (§7.3).

## 3b. Dump do console real (25/11/2026, Pedro/Hardwarelandia)

Arquivo bruto: `zeebo-gl-extensions-dump-hardwarelandia.txt` (nesta pasta). Gerado por um homebrew rodando num Zeebo.

- `GL_VENDOR` QUALCOMM, Inc.; `GL_RENDERER` **Q3Dimension MSM7500** 01.02.37 0 4.0.0; `GL_VERSION` **OpenGL ES 1.0-CM**.
  (O guia fala em MSM7201A/Adreno 130; o renderer diz MSM7500. Não sei explicar a diferença.)
- `EGL_VERSION` 2.07.050110.34681. `EGL_EXTENSIONS`: `EGL_QUALCOMM_COLOR_BUFFER`, `get_color_buffer`, `EGL_EXT_swap_control`,
  `surface_scale`, `surface_rotate`, `surface_transparency`, `surface_color_key`, `surface_overlay`.
- `GL_EXTENSIONS` (25): `ARB_texture_env_combine`, `ARB_texture_env_crossbar`, `ARB_texture_env_dot3`,
  **`ARB_texture_mirrored_repeat`**, `ARB_vertex_buffer_object`, `ATI_extended_texture_coordinate_data_formats`,
  `ATI_imageon_misc`, `ATI_texture_compression_atitc`, `EXT_blend_equation_separate`, `EXT_blend_func_separate`,
  `EXT_blend_minmax`, `EXT_blend_subtract`, `EXT_stencil_wrap`, `OES_byte_coordinates`,
  **`OES_compressed_paletted_texture`**, `OES_draw_texture`, `OES_fixed_point`, `OES_matrix_palette`,
  `OES_point_size_array`, `OES_point_sprite`, `OES_read_format`, `OES_single_precision`, `OES_vertex_buffer_object`,
  `QUALCOMM_vertex_buffer_object`.

**Ausente no console**, apesar de declarado em `glESext.h` ou anunciado pelo zeebx: `OES_framebuffer_object` (e portanto
`glGenerateMipmapOES`), `OES_query_matrix`, `OES_texture_cube_map`, `OES_texture_mirrored_repeat` (existe a versão **ARB**),
`OES_blend_*` (existem as **EXT**), `GL_QUALCOMM_IMAGEON`, point parameters, `EGL_QUALCOMM_get_power_level` e
`get_color_buffer_info`. Isso bate com a Tabela 7-1 do guia (sem render para textura, sem mipmap automático, sem matrix get).

### Respostas do Pedro (25/11, 22:43–22:50; dumps 2 e 3)

Arquivos: `zeebo-gl-extensions-dump-2-hardwarelandia.txt` e `...-dump-3-hardwarelandia.txt`. A lista de `GL_EXTENSIONS` é idêntica nos 3 dumps.

| Pergunta | Resposta do console |
|---|---|
| `AEEIID_GLES10` em `GLES_Init`/QEGL | **NON-NULL** |
| `AEEIID_GLES11` em `GLES_Init`/QEGL | **NULL** |
| `GL_MAX_TEXTURE_SIZE` | **1024** |
| `GL_MAX_TEXTURE_UNITS` | **2** |
| Nível máximo de mipmap a partir do tamanho máximo | **10** (1024 → níveis 0 a 10) |
| `MIRRORED_REPEAT` com `GL_LINEAR` em textura retangular | **não testado**: dump 2 = `UNKNOWN (GLES11 NULL)`; dump 3 = `SKIPPED SAFE MODE (GLES10 render test)`. O teste de renderização em GLES10 que o Pedro tentou **deu watchdog no console**; a causa não foi isolada (pode ser o teste, não o recurso) |

`GL_MAX_TEXTURE_UNITS = 2` bate com o `GL_COMBINE` de 2 unidades do zeebx (§8b). `GL_MAX_VERTEX_UNITS` não foi perguntado nesta rodada.

## 4. O que os headers do SDK declaram além disso

`glESext.h` (1.009 linhas) declara estas extensões, **sem prova de que o driver as implemente**:

- `GL_OES_framebuffer_object` (`glGenFramebuffersOES`, `glFramebufferTexture2DOES`,
  `glRenderbufferStorageOES`, `glCheckFramebufferStatusOES`...) e `glGenerateMipmapOES`;
- `GL_OES_query_matrix` (`glQueryMatrixxOES`);
- `GL_OES_texture_cube_map`, `GL_OES_texture_mirrored_repeat`, `GL_ARB_texture_mirrored_repeat`;
- `GL_ATI_compressed_texture_atitc` (compressão ATC), `GL_ATI_extended_texture_coordinate_data_formats`;
- `GL_OES_blend_subtract`, `blend_func_separate`, `blend_equation_separate`, `GL_EXT_blend_minmax`,
  `stencil_wrap`;
- vertex buffer com nome Qualcomm (`glBindBufferQUALCOMM`, `glBufferDataQUALCOMM`,
  `glGenBuffersQUALCOMM`...) e listas de malha da ATI (`glMeshListATI`,
  `glDrawVertexBufferObjectATI`);
- `GL_QUALCOMM_IMAGEON`.

(Quais dessas existem no console: ver §3b. Ausentes: framebuffer object, query matrix, cube map, OES_blend_*, QUALCOMM_IMAGEON.)

`EGLext.h`: `EGL_QUALCOMM_surface_scale`, `_rotate`, `_overlay`, `_color_key`, `_transparency`,
`EGL_QUALCOMM_get_color_buffer` (+ `_info`), `EGL_QUALCOMM_get_power_level`,
`EGL_EXT_swap_control` (`eglSwapIntervalOES`), `eglCreateCompositeSurfaceQUALCOMM`.

O que jogos retail usam (wiki, decodificado de módulos; não é dump de hardware): blit por
`GL_OES_draw_texture`, matrix palette, alvos fora da tela por `GL_OES_framebuffer_object`,
vertex buffers com nome Qualcomm, ATITC, e as extensões EGL de superfície. Eles procuram por
**substring** em `glGetString(GL_EXTENSIONS)` / `eglQueryString(EGL_EXTENSIONS)` os nomes
`GL_OES_draw_texture`, `GL_ATI_imageon_misc`, `GL_ATI_texture_compression_atitc`,
`ARB_vertex_buffer_object` e `EGL_QUALCOMM_surface_scale`; sem eles desistem de inicializar.

## 5. Interfaces 1.0 e 1.1 e o wrapper do SDK (fonte primária)

Segundo a wiki há duas gerações de vtable: **legada** (IGL 80 slots, IEGL 28) e **ES 1.1**
(IGL 148, IEGL 31). O port compila contra os wrappers `EGL_1x.c`/`GLES_1x.c` do SDK 4.0.

Lido no código do wrapper (`GLES_1x.c`, `GLES_1x_support.h`):

- `GLES_Init` pede ao objeto QEGL **duas** interfaces separadas, `AEEIID_GLES10` e
  `AEEIID_GLES11`; qualquer uma pode vir NULL.
- As funções comuns (macros `GLCL_*`/`GLCM_*`) tentam 1.1, depois 1.0, depois a IGL legada.
- As funções só do 1.1 (macros `GL11_FUNC_*`, entre elas `glTexParameteri`, `glTexEnvi`) fazem
  `if (GPIGLES11 != NULL) chama;` e **não fazem nada** caso contrário: sem erro e sem fallback.

**Confirmado no console real (dump do Pedro, 25/11 22:50): `AEEIID_GLES11` é NULL.** Das 36 funções GL do port, o wrapper só
exige a interface 1.1 em duas (`glTexParameteri` e `glTexEnvi`, macro `GL11_FUNC_3_ARGS`); as outras 34 caem na 1.0.
Consequência para o port: como o console não responde `AEEIID_GLES11`, as 6 chamadas
`glTexParameteri` e a `glTexEnvi` de `zstrip.c` viram **no-ops silenciosos**. Os filtros ficariam
no padrão do driver (a spec do GL ES manda `NEAREST_MIPMAP_LINEAR` para o filtro de redução, o
que sem mipmaps deixa a textura incompleta; a wiki diz "linear" para o estado inicial do
console, o que seria inofensivo). **Não verificado:** o filtro padrão que o driver
usa quando essas chamadas não fazem nada (se for o da spec, as texturas sem mipmap podem sair incompletas). `glTexParameterx`/`glTexEnvx` têm caminho 1.0.

## 6. Uso real do port (36 funções GL, `ports/zeebo/*.c`)

`glLoadIdentity, glEnable, glDisable, glTexImage2D, glMatrixMode, glTexParameteri, glEnableClientState,
glBlendFunc, glOrthox, glDrawArrays, glDepthFunc, glClearDepthx, glLoadMatrixx, glClear,
glGenTextures, glFogx, glVertexPointer, glPushMatrix, glPopMatrix, glDepthMask, glDeleteTextures,
glColorPointer, glClearColor, glBindTexture, glViewport, glTranslatex, glTexSubImage2D, glTexEnvi,
glTexCoordPointer, glScalex, glMultMatrixx, glGetString, glFrustumx, glFogxv,
glDisableClientState, glAlphaFuncx`.

- Só ponto fixo (`x`), coerente com o ARM11 sem VFP.
- `glGetString(GL_EXTENSIONS)` é lido uma vez (`zstrip.c:1444`) e cada nome vai para o log com
  a categoria `tgl` (no zeebx saem 5: `GL_OES_draw_texture GL_OES_query_matrix
  GL_ATI_imageon_misc GL_ATI_texture_compression_atitc GL_ARB_vertex_buffer_object`).
- Só os arquivos de topo de `ports/zeebo/` chamam GL (conferido por busca recursiva).
- Sem VBO, sem draw_texture, sem framebuffer object, sem clip planes, sem point sprites.
- Só `glTexParameteri` (6 chamadas) e `glTexEnvi` (1) saem do ES 1.0.

## 7. Pendências (GL/GLES do port) — revistas com o dump do console (§3b)

1. **`glTexParameteri`/`glTexEnvi` → `glTexParameterx`/`glTexEnvx`** (1.0 core). **Defeito confirmado:** `AEEIID_GLES11` é NULL
   no console (dump de 25/11), então as 7 chamadas de `zstrip.c` (6 + 1) não faziam nada lá. **FEITO em 25/11 (árvore de trabalho,
   sem commit):** `zstrip.c` troca as 7 chamadas (um comentário explica o porquê). Gates com o zeebx sem o modo console:
   752/752 objetos, duplicatas 0, `host-selftest` PASSED, `novfp` 0 VFP, `hash_ref` IGUAL (`DEBUG_TITLE=0`); `.mod` normal
   `AUDIO=1` = `cbac8a97` (26.481.500 B; o do alpha era `47d4c85c`). Os 7z do alpha **não** têm a correção.
2. **Wrap/clamp/flip de textura:** não chegam ao GL (`GL_REPEAT`+`GL_LINEAR` fixos). **Há caminho:** `GL_CLAMP_TO_EDGE` é core do
   ES 1.0 e o console tem **`GL_ARB_texture_mirrored_repeat`** (`GL_MIRRORED_REPEAT` = `0x8370`), que cobre o FLIP do DC;
   ajustar com `glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S/T, ...)`. Não implementado. **Cuidado:** o teste do Pedro com
   `MIRRORED_REPEAT` em GLES10 deu watchdog no console (causa não isolada); tratar como não provado.
3. **Mipmaps ausentes** (D1): **não há** `glGenerateMipmapOES` nem `SGIS_generate_mipmap` no console. Caminho único: enviar cada
   nível com `glTexImage2D(level, ...)`; `assets/pvr.c` hoje decodifica só o nível-base de TWIDDLED+MIPMAP e VQ+MIPMAP (os níveis
   já estão no PVR do DC). Filtro de redução com mipmap (`GL_LINEAR_MIPMAP_LINEAR`) é core do ES 1.0. Limites medidos: textura de até 1024, 2 unidades, até o nível 10.
4. **`kmRenderTexture` é STUB** (reflexos): **não há** `OES_framebuffer_object` no console. Sobra a cópia do framebuffer
   (`glCopyTexImage2D`/`glCopyTexSubImage2D`, core 1.0, presentes na vtable do wrapper); o zeebx as despacha só em `upstream/development`
   (PR #137, só nível 0), não nos checkouts locais (§8).
5. **Mapeamento de ambiente (`USE_ENV`):** não implementado; sem texgen no ES, teria que ser na CPU.
6. **`kmSetUserClipping` é STUB** (TODO `glScissor`); não há `glClipPlane`.
7. **Sprites sobre o 3D e contornos pretos** (D2): causa provável lida no código, **não testada** (teste de `CLAMP_TO_EDGE` inconclusivo).
8. **Oportunidades não avaliadas** (o console tem): `OES_compressed_paletted_texture` (texturas PAL4/PAL8 do DC direto, menos RAM que
   RGBA); `EXT_blend_subtract`/`blend_func_separate` (modos de mistura do DC); `ARB_vertex_buffer_object` (hoje o port usa arrays de
   cliente); `OES_draw_texture` (blit 2D). Nenhuma tem medida de ganho.

## 8. O que o zeebx (emulador) implementa, comparado ao dump do console

Base: `upstream/development` (`ZeebxTeam/zeebx-emu`, `c73500bf`, 24/11 23:00), lida em cópia extraída (nenhum checkout foi alterado).
`upstream/master` está parado em `2aea72c7` (19/11); os PRs aprovados nos últimos dias (#136 `SetSurfaceScale`, #137
`glCopyTexImage2D`/`glCopyTexSubImage2D`, #138 teclas do controle no CLI) entraram em `development` em 24/11 21:16. Checkouts locais:
`zeebx-emu` (branch `speedhack`, base de 08/10) está **387 commits atrás** de `development`; `zeebx-emu-copytex` (branch
`profiling-tools-tmp`, base de 19/11, usado nos testes desta sessão) está **22 atrás**, sem os PRs acima, e tem 3 commits próprios
(perfilador de PC, relógio virtual, `--state`). Entre o `copytex` e `development` o código gráfico quase não difere: `gl.rs` ganha
+207 linhas (a cópia de textura) e `rasterizer.rs` perde 38 (instrumentação do `copytex`). **A análise abaixo vale para os dois.**
(A primeira versão desta seção, e a varredura de funções GL de antes, usaram o checkout `speedhack`, mais antigo; as conclusões
foram refeitas em `development`.)

| Do dump do console | zeebx (`development`) |
|---|---|
| `OES_draw_texture`, `ARB/OES/QUALCOMM_vertex_buffer_object` | **implementado** |
| `ATI_texture_compression_atitc`, `OES_compressed_paletted_texture` | **implementado** (`atc.rs`, `paltex.rs`) |
| `ARB_texture_env_combine` | **implementado** no caminho de GPU (`gpu.rs`, `GL_COMBINE`); software não verificado |
| `ARB_texture_env_dot3` | **só no caminho de GPU** (`gpu.rs:2362`, `DOT3_RGB/RGBA`); nenhuma ocorrência no rasterizador por software |
| `ARB_texture_env_crossbar` | **implementado** como fonte do `GL_COMBINE`: texel da unidade 0 ou 1 e cor primária (`gpu.rs:2369-2374`) |
| `ARB_texture_mirrored_repeat` | **ausente**: o rasterizador por software só trata `CLAMP_TO_EDGE` e repete o resto (`rasterizer.rs:522`); a GPU repassa o valor ao GL do host (não testei) |
| `EXT_blend_equation_separate/func_separate/minmax/subtract` | **só responde sucesso**, sem efeito (`misc.rs:184`, `:353`) |
| `OES_matrix_palette`, `OES_point_size_array`, `OES_point_sprite` | **só responde sucesso**, sem efeito (`misc.rs:187`, `:441`; o código diz que não há o que desenhar para ponto) |
| `ATI_imageon_misc` | anunciada; a doc do zeebx diz que quase tudo responde "consegui" sem fazer nada |
| `OES_byte_coordinates` | **implementado** (arrays `GL_BYTE`, `gl.rs:1131`, `:1175`) |
| `OES_fixed_point`, `OES_single_precision` | funções `x` e `f` despachadas (`gl.rs`); o port usa só as `x` |
| `EXT_stencil_wrap` | **ausente** (nenhuma ocorrência de `INCR_WRAP`/`DECR_WRAP`) |
| `OES_read_format`, `ATI_extended_texture_coordinate_data_formats` | **ausente** (sem `IMPLEMENTATION_COLOR_READ_*`; sem o nome da extensão) |
| EGL `surface_scale`, `get_color_buffer`, `swap_control` | implementado; `rotate`, `transparency`, `color_key`, `overlay` respondem sucesso sem efeito |

Outros achados em `development`:
- **`glCopyTexImage2D`/`glCopyTexSubImage2D` agora existem** (`gl.rs:472`): copiam o quadro para a textura, **só o nível 0**
  (`gles_copy_tex_sub_image` retorna se `level != 0`). Isto torna os reflexos por cópia de framebuffer testáveis no zeebx, depois de atualizar.
- **Framebuffer objects são falsos**: ids fictícios que respondem "completo", e o desenho vai para a tela; `GenerateMipmapOES` é no-op
  (`misc.rs:346-358`). O console **não** tem essas extensões, então um uso delas no port passaria no zeebx e falharia no aparelho.
- **O zeebx anuncia `GL_OES_query_matrix`**, que o console não anuncia, e deixa de fora cerca de 20 extensões que o console tem.
- **Não dá para reproduzir um console só ES 1.0 no zeebx**: `egl.rs:483` responde `AEEIID_GLES10` e `AEEIID_GLES11` com o mesmo
  objeto. Logo o risco de `glTexParameteri`/`glTexEnvi` virarem no-op (§5) **é invisível no emulador**; só o aparelho mostra.
- Mipmaps: o rasterizador guarda os níveis e usa o padrão da spec `GL_NEAREST_MIPMAP_LINEAR` (`rasterizer.rs:506`). Como o port
  hoje manda `glTexParameteri` (com `AEEIID_GLES11` presente no zeebx), o filtro fica `GL_LINEAR`; num aparelho onde a chamada
  não faz nada, o filtro padrão do driver (spec: `NEAREST_MIPMAP_LINEAR`, textura incompleta sem mipmaps) **poderia** deixar
  texturas sem desenho. **Hipótese, não verificada.**
- No-ops de propósito: `Hint`, `LineWidth`, `PolygonOffset`, `Finish` (`machine/mod.rs:696-704`). Sem despacho nenhum:
  `glClipPlane*`, `glLogicOp`, `glPointSize`, `glMultiTexCoord4`, `glCompressedTexSubImage2D`, `glIsTexture`, `glGetFloatv`.
  O port não usa nenhuma dessas.

### Modo console do zeebx: `ZEEBX_CONSOLE_GL=1` (feito em 25/11, branch local `feat/console-gl-parity`, sem commit)

Opt-in; **desligado por padrão** e sem mudança de comportamento quando desligado. Base: `zeebx-emu-copytex` @ `a4c85ed4`.
O que faz quando ligado:

- `QueryInterface(AEEIID_GLES11)` falha (`ECLASSNOTSUPPORT`) e `AEEIID_GLES10` responde: o wrapper do SDK deixa `GPIGLES11` NULL e as
  `GL11_FUNC_*` (`glTexParameteri`, `glTexEnvi`) viram no-op, como no console. (`IGLES11` herda `IGLES10`; o mesmo objeto serve.)
- `glGetString` devolve o vendor, o renderer, a versão (`OpenGL ES 1.0-CM`) e as 24 extensões do dump do console.
- O rasterizador por software guarda e compara a profundidade em **16 bits**, arredondada para o mais próximo (o hardware pode
  truncar: reproduz a precisão, não os bits). Também o valor de `glClearDepth`. O caminho de GPU (`gpu.rs`) **não** é quantizado.

Arquivos: `src/video/gles.rs` (`console_gl()`, `depth16()`, strings do console, 3 testes), `src/video/rasterizer.rs` (3 laços de
preenchimento e o `glClear`), `src/machine/egl.rs` (`QueryInterface`), `src/machine/gl.rs` (`GetString`). Para reconstruir o binário
é preciso `cargo build --release --offline -p zeebx-cli` (só `cargo build --release` não atualiza `target/release/zeebx`).

Medido (25/11, fluxo de título + 8 `start`, 140 s virtuais, mesmos 4.045 quadros em todas as execuções):

| Execução | `TexParameter*` que chegam ao emulador | `TexEnv*` | último quadro |
|---|---|---|---|
| `.mod` antigo (`47d4c85c`), normal | 3.610 `TexParameteri` | 1 `TexEnvi` | `ce453765` |
| `.mod` antigo, **console** | **0** | **0** | `b2228cbe` |
| `.mod` novo (`cbac8a97`), normal | 3.610 `TexParameterx` | 1 `TexEnvx` | `ce453765` |
| `.mod` novo, **console** | 3.610 `TexParameterx` | 1 `TexEnvx` | `b2228cbe` |

- Reproduz o defeito do console: o `.mod` antigo perde as 3.610 + 1 chamadas; o novo as mantém (`x` passa pelo caminho 1.0).
- Com o modo desligado o binário novo dá o **mesmo quadro e os mesmos hashes** do binário anterior (`ce453765`).
- O modo console muda **3.638 de 307.200 pixels** (1,2%) do último quadro, em 83 das 192 células de 40×40, concentrados na faixa
  inferior (y 360–440). Um olhar visual sobre o recorte aponta uma porta/vitrine onde duas camadas quase coplanares disputam os pixels
  com 16 bits (leitura por modelo de visão; não conferi pixel a pixel). É o tipo de artefato que o `f32` escondia.
- **Limite:** o último quadro do `.mod` antigo e o do novo são idênticos no modo console. O rasterizador não implementa a regra da
  spec de textura incompleta (filtro de redução padrão `NEAREST_MIPMAP_LINEAR` sem mipmaps), então o zeebx **não mostra** o efeito
  visual de perder o `glTexParameteri`; só prova que a chamada deixa de acontecer. O efeito real no console continua desconhecido.
- Testes: `cargo test --release --offline --lib` = 624 passam, 1 falha (`an_overlay_only_save_joins_the_list_without_dropping_the_rest`,
  já falhava no `upstream/master`), mais os 3 testes novos.

### Fidelidade do rasterizador em relação ao console (`development`)

- **Profundidade em ponto flutuante**: `depth: Vec<f32>` normalizado em `[0, 1]` (`rasterizer.rs:1190`); o console tem **depth de 16 bits**
  (Dev Guide §7.1.1). Z-fighting, decals e a faixa de profundidade do nosso passe 2D (D2: `SCREEN_NEAR = -16`) **não se reproduzem** do mesmo jeito
  no emulador. Hipótese, não medida no aparelho.
- **Cor em 8 bits por canal** (`color: Vec<[u8; 4]>`), convertida para RGB565 só na apresentação; o console desenha em 565. Banding e
  mistura com alfa podem sair diferentes. Não vi dithering no código (busca por `dither` sem resultado).
- **Textura**: perspectiva correta, LOD por pixel com filtro entre níveis (`sample_lod`, `rasterizer.rs:568`) e `GL_COMBINE` completo;
  o hardware (Q3Dimension) pode filtrar de outro jeito. Névoa por distância no espaço do olho, com opção do jogador para desligar
  (`set_fog_allowed`). Stencil de 8 bits (o console anuncia 4 bits opcionais no guia).
- **Dois caminhos** (software `rasterizer.rs`, 6.454 linhas, e placa/GPU `gpu.rs`, 5.373 linhas) devem dar o mesmo quadro, mas o
  `GL_MIRRORED_REPEAT` só teria chance de funcionar no segundo.

**Conclusão:** o zeebx não é um espelho do console para `MIRRORED_REPEAT`, blend subtract/minmax, matrix palette, mipmaps
automáticos e a interface 1.0-only. Para testar FLIP e blend no emulador seria preciso implementá-los lá (o código do zeebx
não será mexido por enquanto).

## 8b. Relatório de cobertura do zeebx — datas e commits

Gerado em 25/11/2026 por `git log` em `ZeebxTeam/zeebx-emu` (remoto `upstream`, depois de `git fetch` em 25/11). Datas = data do commit
no `git` (relógio do repositório). Linhas de código citadas valem para `upstream/development` @ `c73500bf`.

| Referência | Commit | Data | Assunto |
|---|---|---|---|
| `upstream/master` | `2aea72c7` | 19/11/2026 | Merge pull request #134 from ZeebxTeam/development |
| `upstream/development` (a "dev") | `c73500bf` | 24/11/2026 | feat(settings): add option to use game names from ROM files instead of Z-Wheel titles |
| `zeebx-emu-copytex` (`profiling-tools-tmp`) | `a4c85ed4` | 24/11/2026 | feat(cli): --state loads a .zbxstate, in run and in session (base `73f5fba1`, 19/11; 22 commits atrás de `development`) |
| `zeebx-emu` (`speedhack`) | `7ef4c761` | 08/10/2026 | Underclock documentado como placebo, junto ao overclock removido (387 commits atrás) |
| `origin/master` (fork) | `d7a73d34` | 11/09/2026 | corrige despacho de eventos e superficie do roller da Z-Wheel |

Método: para cada item, o primeiro commit de `upstream/development` que acrescenta a string/símbolo (`git log -S`). Quando o
resultado é **`87312d75` (08/09/2026, "initial commit para organization (migrado)")**, o recurso já existia no primeiro commit do
repositório migrado e o histórico anterior não está disponível; isso **não** é a data em que foi escrito. Duas reorganizações
movem arquivos e falseiam buscas por caminho: `923715ea` (11/09, "Agrupa a saída de vídeo em video/") e `49d15549` (11/09, "Cria
src/machine/"); a tradução para inglês (`d9b789c5` machine e `a8295990` video, 06/11) renomeou identificadores e arquivos
(`diversos.rs` virou `misc.rs`).

| Item do dump do console | Cobertura no zeebx | Commit | Data | Onde |
|---|---|---|---|---|
| `OES_draw_texture` | implementado | `87312d75` | 08/09 | `gl.rs`, `rasterizer.rs` |
| `ARB_vertex_buffer_object`, `OES_`/`QUALCOMM_vertex_buffer_object` | implementado | `daa18c5c` (ARB, "e o Prey Evil roda") | 12/09 | `gl.rs`, `misc.rs:184+` |
| `ATI_texture_compression_atitc` | decodifica; anunciada | `87312d75` (`atc.rs`); anúncio `3fc21789` | 08/09; 10/09 | `atc.rs`, `gl.rs` |
| `OES_compressed_paletted_texture` | implementado | `87312d75` | 08/09 | `paltex.rs` |
| `ARB_texture_env_combine` | implementado (GPU e rasterizador) | `e2cb591e` "Duas unidades de textura, o GL_COMBINE…"; endurecido por `a8217722` e `aa84c35e` | 18/09; 14/10 | `gpu.rs`, `rasterizer.rs:157+` |
| `ARB_texture_env_crossbar` | implementado como fonte do combine | `e2cb591e` | 18/09 | `gpu.rs:2369-2374` |
| `ARB_texture_env_dot3` | só no caminho de GPU | `e2cb591e` | 18/09 | `gpu.rs:2362` |
| `ARB_texture_mirrored_repeat` | **ausente** (software só `CLAMP_TO_EDGE`/`REPEAT`) | `87312d75` (`wrap` já existia assim) | 08/09 | `rasterizer.rs:522` |
| Mipmaps (níveis, LOD) | implementado, filtro `NEAREST_MIPMAP_LINEAR` por padrão | `20f24c65` "mipmap, que o palco pede e nós jogávamos fora" | 10/09 | `rasterizer.rs:478-550,568` |
| Névoa (`glFog*`) | implementado, com chave para desligar | `6d4f43e6` | 16/09 | `rasterizer.rs:403+` |
| `EXT_blend_equation_separate`/`func_separate`/`minmax`/`subtract` | **só responde sucesso** | `87312d75` | 08/09 | `misc.rs:184-187`, `:353` |
| `OES_matrix_palette` | **só responde sucesso** | `b48b6964` "entrega o IGLES11Ext por QueryInterface" | 21/09 | `misc.rs:441` |
| `OES_point_size_array`, `OES_point_sprite` | **só sucesso / sem efeito** | `87312d75` | 08/09 | `misc.rs:187`, `gl.rs:457` |
| `OES_byte_coordinates` | implementado | `87312d75` | 08/09 | `gl.rs:1131,1175` |
| `EXT_stencil_wrap`, `OES_read_format`, `ATI_extended_texture_coordinate_data_formats` | **ausentes** | n/a | n/a | sem ocorrência no código |
| EGL `surface_scale` | implementado | `87312d75`; correção do retorno `a9e24950` (PR #136) | 08/09; 23/11 | `egl.rs` |
| EGL `swap_control`, `COLOR_BUFFER` | implementados | `87312d75` | 08/09 | `egl.rs`, `machine/mod.rs` |
| EGL `get_color_buffer` (`GetColorBufferQUALCOMM`) | implementado | `3fc21789` "anunciar o ATITC, resolver nomes egl…" | 10/09 | `egl.rs`, `machine/mod.rs` |
| EGL `rotate`/`transparency`/`color_key`/`overlay` | só responde sucesso | `87312d75` | 08/09 | `egl.rs` |
| `glCopyTexImage2D`/`glCopyTexSubImage2D` | implementado, **só nível 0** | `4154815b` (23/11), merge PR #137 `b98e2e20` (24/11) | 23–24/11 | `gl.rs:472`, `:814` |

Itens que o zeebx anuncia ou faz e o console **não** tem:

| Item | Commit | Data | Observação |
|---|---|---|---|
| `GL_OES_query_matrix` anunciada | `b5534447` "O Quake 2 entra nas fases: a matriz consultada é a de verdade…" | 29/09 | não está no dump do console |
| Framebuffer objects falsos, `GenerateMipmapOES` no-op | `0832a559` "entrega o IGLES11ExtPak" | 21/09 | `misc.rs:346-358`; ids fictícios, desenho vai para a tela |
| `AEEIID_GLES10` e `AEEIID_GLES11` com o mesmo objeto | `87312d75` | 08/09 | `egl.rs:483`; impede simular o console só 1.0 |
| `glGetString(GL_EXTENSIONS)` com 5 nomes | `87312d75`; nomes acrescentados por `daa18c5c`, `3fc21789`, `b5534447` | 08/09–29/09 | `gl.rs` |

Mudanças de gráfico relevantes entre a base do `copytex` (19/11) e `development`: **apenas** o PR #137 (`4154815b`, `gl.rs`
+207 linhas) e o PR #136 (`a9e24950`). Outras mudanças gráficas anteriores à base: `8c0ad90e` (27/10, normais pela transposta da
inversa), `9583a2f7` (16/10, nomes de textura/buffer reaproveitados), `d7c49768` (12/10, quadro apresentado numa "frente"),
`a02d8923` (09/10, a placa desliga a tesoura e respeita o filtro).

**Limites deste relatório:** "implementado" quer dizer que o código despacha e age sobre o desenho; **não** foi testado com o
port SA1 nem comparado com o hardware. As datas são de commit, não de revisão. O histórico anterior ao commit inicial migrado
(`87312d75`) é desconhecido. O arquivo bruto do console está em `zeebo-gl-extensions-dump-hardwarelandia.txt`.

## 9. O que ainda falta provar (revisto em 25/11 22:50)

Respondido pelo Pedro: `AEEIID_GLES11` é NULL e `AEEIID_GLES10` é NON-NULL; `GL_MAX_TEXTURE_SIZE` 1024; `GL_MAX_TEXTURE_UNITS` 2; mipmap até o nível 10.

1. **`MIRRORED_REPEAT` com `GL_LINEAR` em textura retangular**: não testado. O teste de render em GLES10 deu watchdog; pedir ao Pedro para
   repetir **sem** renderizar (só `glTexParameterx(...MIRRORED_REPEAT)` e `glGetError`), ou com uma textura quadrada pequena, e logar antes de cada chamada.
2. **Filtro padrão do driver** quando `glTexParameteri` não faz nada: pedir um homebrew que desenhe uma textura sem mipmap **sem** chamar
   `glTexParameter*` e outra com `glTexParameterx(MIN_FILTER, GL_LINEAR)`, e fotografe.
3. `GL_MAX_VERTEX_UNITS`, `GL_MAX_LIGHTS`, `GL_STENCIL_BITS`/`GL_DEPTH_BITS` reais, e se o depth é de 16 bits (Dev Guide §7.1.1).
4. Onde o `zlog` escreve no console (o port já loga as extensões como `tgl`, mas isso nunca foi lido num aparelho).
5. Por que o renderer diz `Q3Dimension MSM7500` e o guia diz MSM7201A/Adreno 130.

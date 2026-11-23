# Lições do port Sonic Mania → Dreamcast para o SA1 → Zeebo

Fonte estudada (só leitura, nada copiado): `~/projects/sonic-mania-dreamcast`
(Sonic-Mania-Decompilation, branch `sf94/dreamcast-kallistios-pvr`, submódulo RSDKv5).
Arquivos lidos: `platforms/KallistiOS.cmake`,
`RSDKv5/RSDK/Graphics/KallistiOS/KallistiOSRenderDevice.cpp`, `Graphics/{Drawing,Scene3D,Sprite}.cpp`,
`Scene/Scene.cpp`, `Core/Math.cpp`, `dreamcast/*.sh` (pipeline de assets).

Alvo: SH-4 + PowerVR2 (16 MB RAM, 8 MB VRAM) de lá; aqui ARM11 528 MHz **sem VFP** + Adreno 130
(Developer Guide 0.97). Cada item diz o que o Mania faz, o que vale para nós **conferido no nosso
código**, e como validar. "Inferência" = não medido.

## O que eles fazem e o que vale para o Zeebo

| # | Técnica no Mania/DC | Situação no nosso port (verificado) | Ação / validação |
|---|---|---|---|
| 1 | **Cache de estado**: `PreparePrimitive` só emite novo cabeçalho de polígono se mudou tipo, paleta, tinta ou textura (`KallistiOSRenderDevice.cpp:868`) | O passe **opaco** já agrupa tudo num único `glDrawArrays` (`opaque_flush`, `zstrip.c:217`). O passe **translúcido** faz `glEnable/glBindTexture/glBlendFunc` em todo strip, sem comparar com o anterior (`zstrip.c:378-387`) | Pular chamadas quando textura/blend/habilitação não mudam, só no translúcido. Barato, risco baixo. Medir: contagem de chamadas GL por quadro (log do zeebx) |
| 2 | **Texturas em 16 bpp / paletizadas / VQ** (sheets VQ ARGB1555, sprites PAL8); `LoadVQSpriteSheet` (`Sprite.cpp:910`) | Texturas do PVR do DC são decodificadas para **RGBA8888** (`pvr_decode_rgba8888` → `GL_RGBA/GL_UNSIGNED_BYTE`, `zstrip.c:696-704`) | Subir em 16 bpp (`GL_UNSIGNED_SHORT_4_4_4_4/5_5_5_1/5_6_5`, que o DC já usa): metade da memória e da banda. O guia diz que o cache de textura é de 8 KB e recomenda 565 para cópias. Medir RAM e tempo de upload; ATITC (4 bpp, guia 8.2) é o próximo passo |
| 3 | **Lista sem blend vs. com blend**: tudo que não mistura vai para a lista punch-through (sem ordenação), só o blend real vai para a TR (`BeginScene`, `KallistiOSRenderDevice.cpp:623-660`). O Mania também trocou a ordenação das listas de entidades por `std::sort` nos builds KallistiOS, "the faster" (`Object.cpp:779`) | Nosso passe translúcido ordena por inserção O(n²) até 1024 strips (`zstrip.c:355-361`). `GL_ALPHA_TEST` só aparece desabilitado (`zstrip.c:724`); **não verifiquei onde caem os recortes** (texturas com alpha 0/1) | Primeiro descobrir quais strips são recorte; se der, `GL_ALPHA_TEST` no passe opaco em vez de blend. Trocar a inserção por ordenação melhor (contagem/radix) só com medida do tempo do `zstrip_flush` |
| 4 | **Profundidade crescente em vez de ordenar**: `GetDepth()` soma 0,0001 por sprite. Obs.: o `pvr_init` deles deixa o autosort **ligado** (campo "autosort disabled? 0", linha ~207), então a profundidade ordena a camada e o hardware ordena o translúcido | Ordenamos por CPU | Para o 2D (HUD, sprites) z incremental com depth test pode dispensar a ordenação; translúcido ainda precisa de ordem |
| 5 | **Buffer TR com guarda e degradação** (`safe_pvr_vertbuf_*`, `trExhausted`; círculo troca `INK_BLEND` por opaco "para não estourar o TR") | `TLIST_STRIPS 1024` fixo: o que acontece ao estourar? (não conferido) | Garantir que estourar descarte com aviso, nunca corrompa; manter contador no `FPSHUD` |
| 6 | **Paletas "sujas" só sobem no início da cena**; hardware diferido lê a paleta na hora de renderizar | Equivalente: `glTexSubImage2D` no meio do quadro (animação de textura) | Em renderizador por binning (Adreno, 8 bins no VGA) alterar textura que já foi usada neste quadro é risco/stall. Agrupar uploads no começo do quadro. Inferência |
| 7 | **Compilação**: `-Os` geral, **`-O3` só nos arquivos quentes** (Math, Drawing, Scene3D, Collision, Scene), `-flto`, `-ffast-math`, `-fipa-pta` | O código do jogo (744 objetos) já compila com `OPT ?= -O3 -fno-tree-vectorize` (`game.mk:275`); os arquivos do port (`zstrip.c`, `zkamui.c`, `zgame.c`...) saem em `-Os` (`Makefile:83`), sem LTO | Experimentar `-O2/-O3` em `zstrip.c` (caminho quente por quadro) e `-flto` no conjunto. Medir tamanho (`.mod` 36 MB > orçamento) e tempo. `-ffast-math` só se não houver float emulado em caminho quente |
| 8 | **Matemática**: tabelas de seno/cosseno (256/512/1024), `fsrra`/`invf` do sh4zam no lugar de divisão | ARM11 sem VFP: cada float é chamada de biblioteca (gate `novfp.sh`: 0 instruções VFP) | Reforça: caminho quente em ponto fixo, divisão evitada com tabela de recíproco. Conferir se há `sinf/cosf/sqrtf` no quadro (perfil por função) |
| 9 | **Vértices uma vez só**: modelos *stripificados* offline (`mania-mesh-optimizer`/meshoptimizer), transforma só os vértices únicos num buffer de rascunho `stripXformVerts[2048]` alinhado a 32 bytes; estrutura de vértice cabe numa linha de cache (`Scene3D.hpp:103`) | Nossos modelos vêm do formato NJ (chunk); transformação fica na GPU via `glVertexPointer(GL_FIXED)` | Alinhar a 32 B as estruturas quentes (linha de cache do ARM1136 = 8 palavras = 32 B e há `PLD`, conferido no TRM `DDI0211H` local) e usar `__builtin_prefetch`.  O ganho no ARM não foi medido |
| 10 | **Rejeição antecipada**: um produto escalar decide se o tile/objeto é descartado antes de transformar (`Scene.cpp:~4683`) | O driver do Zeebo faz o clipping **por software** (guia 7.4.1); não há descarte prévio no nosso lado (`zkamui.c`: cull é stub) | Descartar objetos/strips totalmente fora da tela antes de enviar; no guia "trivial rejection" é de hardware, mas o clip é do ARM11 |
| 11 | **Pré-assar** o que se repete: piso 3D inteiro em 1 textura 256×256 e **um quad com UV repetido** em vez de centenas de tiles (`Scene.cpp:52`); LOD de mapa em textura | SA1 tem água/chão em tiras | Procurar sequências de quads com mesma textura; substituir por `GL_REPEAT` |
| 12 | **Caminho rápido para o caso simples** (deform 1:1 → retângulos alinhados, sem divisão por tile, `Scene.cpp:3016`) | — | Padrão geral: detectar o caso comum e pular a matemática genérica |
| 13 | **Saída em 320×240**: com `DO_480=0` (padrão, `KallistiOSRenderDevice.hpp:5`) o Mania usa o **modo de vídeo `DM_320x240`** da própria Dreamcast e `displayWidth/Height = 320×240` (`.cpp:221-229, 260-266`); só o renderizador por software usa 640×480. Sem dithering e sem FSAA; framebuffer RGB565. **Não escala via hardware para 640×480** (corrijo: a primeira versão deste doc dizia isso) | No Zeebo a saída é só VGA 640×480 (guia 3.1.3), por isso nosso `DYNRES` precisa de `surface_scale` | Confirma que renderizar menos pixels é a política deles, mas o mecanismo é diferente do nosso. Medir no aparelho |
| 14 | **Dados pré-convertidos no host**: sprites → `.dtex` (pvrtex), SFX → ADPCM (`wav2adpcm`), vídeo → MPEG1 (doc do repo), malhas → stripificadas; música: o script `music_step_1_ogg_to_pcm.sh` converte para PCM, mas o `BUILD-DREAMCAST.md:94` diz ADPCM (divergência não resolvida). **Datapack lido em streaming do disco** em vez de 207 MB na RAM (commit `99ff825c`) | `zgdfs` lê do disco; `.mod` 36 MB | Streaming de assets grandes; converter texturas/áudio no build, não no aparelho |
| 15 | **Barra de tempo de CPU na borda do vídeo** (`vid_border_color`) como profiler grátis | `FPSHUD` mostra só médias | Opcional: barras por fase (jogo / montar strips / flush / swap) no overlay |
| 16 | **Memória**: GC + desfragmentação do pool de armazenamento; saves comprimidos (LZFX) | RAM livre mín. medido só no zeebx | Medir pico de RAM no aparelho (`FPSHUD` M) antes de mexer |

## O que não revisei / não se aplica

- Não li: áudio (`Audio.cpp`, tabelas de interpolação linear; o Mania usa o AICA da DC), decodificador de vídeo (`mpeg.c`, `pl_mpeg.h`, theora), colisão e atualização de entidades (`Collision.cpp`, `Object.cpp` fora do `std::sort`), `DrawingLegacy.cpp` (desenrolamento de laço) e o renderizador por software inteiro. Podem ter outras ideias úteis.
- Específico do hardware DC, sem equivalente no Zeebo: *store queues* / render direto (`pvr_dr_*`), buffer TR por DMA com dupla bufferização, compressão VQ nativa do PVR, palette banks do PVR (o Adreno tem outros formatos; paletizadas só se o driver oferecer: não verifiquei).
- O doc cita arquivo e linha só do que li; números de "ganho" são inferência, nenhum medido.

## Prioridade sugerida (impacto esperado / custo)

1. Item 2 (texturas 16 bpp): maior efeito em RAM e banda, mudança localizada em `zstrip.c`.
2. Item 1 (cache de estado): trivial, sem risco visual.
3. Item 3 (alpha test + ordenação): mexe na ordem de desenho, exige comparar quadros.
4. Item 7 (`-O2/-O3` em `zstrip.c`, LTO): rápido de tentar; observar tamanho.
5. Item 10 (descarte prévio): ganho depende do clipping por software; medir.

Nada disso foi medido no aparelho. O zeebx não serve para decidir ganho de desempenho (o relógio
virtual não cobra custo de GL/pixels); decidir com `FPSHUD=1` no Zeebo real.

## Não copiado

Estudo de técnica apenas. O código do Mania segue a licença do projeto dele; não trazemos trechos,
só as ideias (cache de estado, 16 bpp, `-O3` seletivo, pré-assar, descarte antecipado).

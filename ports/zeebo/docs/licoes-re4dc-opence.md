# Lições de re4dc (RE4 Dreamcast) e OpenCE (Halo CE)

Só leitura (24/11/2026); nada foi copiado. Fontes: `lamb2k/re4dc`, branch
`dreamcast-port`, `port/dreamcast/{README.md, docs/D367_ASSET_PIPELINE.md,
docs/D367_THIRTY_FPS_ROUTE.md, tools/d367/README.md}` (li o README, o
cabeçalho do roadmap de 30 fps e as seções 4.1-4.4 do pipeline; **não** li o
resto dos ~150 documentos nem o código C++), e `OpenCommunityEdition/OpenCE`
(`README.md`, `AGENTS.md`, `port/android/README.md`, um commit do branch
`android-resolution-scaling`).

**OpenCE não é um port de Dreamcast**: é o Halo CE (decomp do Xbox 2342) para
Linux, Windows, Android e web. Nenhum arquivo ou branch menciona Dreamcast, KOS
ou SH-4 (grep + `git ls-remote`, 39 branches).

## re4dc: o que se aplica ao SA1/Zeebo

| # | Técnica deles | Aqui | Estado |
|---|---|---|---|
| 1 | **Pacing**: a lógica mantém o relógio fixo; quando atrasa, roda o tick inteiro **sem desenhar** (`PACE_CAP`, `pace.cpp`) | O SA1 separa `exec` e `disp` por tarefa; pular o `disp` em ticks atrasados poupa o desenho que roda na CPU de 528 MHz (a "jogo" do `vprof` inclui as duas coisas) | hipótese: falta confirmar o laço de tarefas e medir o quanto do "jogo" é `disp` |
| 2 | **Modelo de custo por contagem** (`costmodel.toml`: ms = base + c·registros + c·strips + ...) e a conclusão de que ~12 de 18,7 ms são custo fixo que nenhuma escolha de asset alcança | Os contadores do V8 (`desenhos`, `estado repetido`) são a matéria-prima para ajustar um modelo parecido no aparelho | só com aparelho |
| 3 | **Pular trabalho só de render** que a lógica não lê (grade de água), com portão "trace de lógica idêntico" (STRICT/MUST-IDENTICAL) | Hoje o gate é o `md5` do último quadro; um hash de estado de jogo por quadro (posição do jogador, `game_mode`) seria um gate mais forte e o zeebx é determinístico | ideia nova (V13) |
| 4 | **Fator do console** (1,155 fora, 1,03 dentro): calibrar o modelo do emulador contra medidas de hardware | Uma medida de ms/quadro no aparelho convertida em instruções/quadro do zeebx daria o fator de conversão | só com aparelho |
| 5 | **Texturas**: 16 bpp twiddled padrão, VQ (1/8 do tamanho), PAL4/PAL8 em atlas; escolhe a resolução pela densidade de UV e pela distância mínima (se a ampliação < 1/2, metade do tamanho não se vê) | Hoje o port decodifica tudo para RGBA8888 (`zstrip.c:578`). O Zeebo **suporta ATITC** (guia 0.97, 8.2: 4:1 para RGBA, 6:1 para RGB, "melhor comportamento de cache de textura"); o zeebx implementa ATITC e `OES_compressed_paletted_texture` (`src/machine/gl.rs:692`, `src/video/paltex.rs`) | achado novo (V14) |
| 6 | Pasta de evidência com `verdict.json` por rodada (`fullplay.sh`: 0 passa, 1 falha, 2 erro de ferramenta) | Nosso `CHECK_GAME=1` falha desde antes (U5) | já em U5 |
| 7 | Link enxuto (sem padding do script do KOS, ajudantes de um overlay só dentro do overlay) e piso de heap como gate | Já temos overlays; o piso de RAM livre sai no `vprof` (`mem minimo livre KB`) mas o zeebx não representa os 32 MB do aparelho | sem ação |

## OpenCE: o que se aplica

| # | Técnica | Aqui |
|---|---|---|
| 1 | Camada de plataforma que implementa as APIs do console (D3D8 sobre GL) e compila o código do jogo quase sem mudança | É o que o port já faz (shim BREW/Kamui) |
| 2 | **Escala de resolução escolhida pelo jogador** (original, 25%, 50%, 75%, nativo; desenha essa fração e o blit amplia), commit `ab7c3c7` | Nosso `DYNRES` é fixo na compilação; poderia virar opção de menu/arquivo de configuração |
| 3 | PGO (`pgo/`) e LTO no `configure.py` | O port compila sem LTO e sem PGO; só testamos `-O2/-O3` (V3) |
| 4 | CI que compila todas as plataformas a cada push, com verificação de ABI (`android_abi_check.py`) | Nossos gates são manuais (`make check`, `novfp.sh`, "744 de 744") |

## Itens novos

- **V13** Hash de estado por quadro (posição do jogador, `game_mode`, contador de quadros) no log, como gate de otimização: compara dois builds no zeebx.
- **V14** Texturas ATITC (e, se o aparelho aceitar, PAL4/PAL8): menos memória e banda; precisa de codificador offline (o guia cita `QXTextureConverter`, no SDK) e muda a imagem (pede aprovação, como o V4). O guia não diz que o Adreno 130 aceita `OES_compressed_paletted_texture`; só o zeebx o implementa.
- **V15** Pular `disp` em ticks atrasados (item 1 da tabela re4dc); começar confirmando o laço de tarefas.

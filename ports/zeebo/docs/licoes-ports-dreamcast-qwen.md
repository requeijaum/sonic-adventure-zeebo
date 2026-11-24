# Lista de ports de Dreamcast (sessão do Qwen): o que serve e o que não serve

Verificado em 24/11/2026. Fonte: `~/Downloads/chat-export-1795483063190.json` (5 perguntas, uma resposta final
com ~90 repositórios em 7 blocos cronológicos). Nada foi copiado; só leitura.

## Como foi feito (e o que não foi)

- **Metadados:** `gh api repos/...` para 60 repositórios da lista (e alguns que já havíamos estudado): existência,
  data de criação e do último push, licença, tamanho, linguagem. **59 existem; 1 não existe**:
  `maryncrt/ultimate-mortal-kombat-3-decompiled` (404).
- **Código lido de verdade:** clone raso de 23 repositórios. Li README e arquivos-chave só de (e, em `sonic-r`, o cabeçalho de `profiler.c` e `render_track_dc.c`): `jnmartin84/oot-dc`,
  `sf64-dc`, `sonic-r`, `CaptainKoffski/naomi-dc-port-kit`, `cfp2dreamcast`, `MoneyShift64/cruisn-usa-dreamcast`
  e `hotel-mario-dreamcast` (README). `GLdc`, `sh4zam`, `SonicCD-DC`, `Sonic1-DC`, `senkosp2dreamcast` e `cfp2dreamcast` (só `CLAUDE.md`) foram
  clonados antes da escrita e só tiveram contagem de arquivos, de código e de arquivos de IA (`CLAUDE.md`,
  `AGENTS.md`). `mk64-dc`, `wipeout-dc`, `halflife_dc`, `outrunners-port`, `rayverse-dc`, `wings64-dc`, `ssb64-dc`,
  `kigold-recomp`, `resident-evil-dreamcast-port`, `clayfighter...` e `manic-karts...` só terminaram de clonar
  depois que o documento foi escrito. **Não li a implementação de nenhum deles.**
- **Falhou:** dispararam-se 4 subagentes para ler os repos em paralelo; todos caíram em 4 s com HTTP 400 do
  Copilot (`model not available for integrator "vscode-chat"`). Por isso a leitura abaixo é só a minha, e curta.
- **Datas do Qwen:** a resposta diz que usa a data de release quando existe e senão a do último push (`[repo]`).
  O GitHub mostra as duas: por exemplo `sf64-dc` foi criado em 2025-08-08 e `mk64-dc` em 2025-08-06; a data que o
  Qwen dá para `sf64-dc` ("06/dez") não bate com a criação, e não verifiquei se é a de uma release. Não usei
  nenhuma data dele como fato.

## O que se confirmou

- **Autores com fluxo de IA declarado no próprio repo:** `sonic-r` (README: "Ghidra e Claude Code", validado contra
  sessões ao vivo do WinDbg do executável original), `cruisn-usa-dreamcast` ("escrito com a ajuda do Claude",
  "centenas de horas de teste em Dreamcast real"), `oot-dc` ("LLMs para o intérprete Fast3D do PowerVR, o sistema
  de arquivos, o mixer de áudio"), `hotel-mario-dreamcast` (seção "AI Disclosure"), `naomi-dc-port-kit`/`cfp2dreamcast`/
  `senkosp2dreamcast` (`CLAUDE.md`) e `sh4zam` (`AGENTS.md` + `for-agents.md`).
- **Os ports NAOMI→DC de CaptainKoffski não são ports de código-fonte:** o próprio `CLAUDE.md` diz "static binary
  conversion (no source code)" (carrega o binário do arcade com shims da BIOS). Não dá para chamá-los de decomp.
- **OpenCE (que você citou antes) não é Dreamcast**; já estava registrado em `licoes-re4dc-opence.md`.
- **Não confirmei:** "Halo CE com só a SH4DER-PVR pública", "GTA 3 em `dca3-game`", "FFX sem repositório", datas de
  lançamento de cada port.

## Útil para o port do SA1 no Zeebo

| # | Técnica | Onde (lido) | Aplicação | Estado |
|---|---|---|---|---|
| 1 | **Gravar uma vez e repetir a malha estática**: o cenário de Cruis'n era "interpretar a display list e traduzir" todo quadro, o que custava mais que desenhar. O port grava, na primeira vez, vértices em espaço do modelo, triângulos e estado de cada trecho, e depois só transforma os vértices e copia os triângulos visíveis. A gravação cai se a malha é recompilada ou se a textura muda (hash). Pool de 1 MB | `cruisn-usa-dreamcast/port/gfx_replay.h:1-35` | O port do SA1 reconstrói os strips de cada modelo a cada quadro pelo `zstrip`/`zlive`. Candidato forte no cenário estático (landtable). **Hipótese**: falta medir quanto de "jogo" é decodificação de strips | novo, V16 |
| 2 | **Cache de estado por "sujo"**: o cabeçalho de polígono do PVR só é recompilado quando muda o estado de profundidade (marcado "dirty") | `sf64-dc/src/gfx/gfx_pvr.c:67-75` | É o que fizemos no V1 (`STATECACHE`); confirma a abordagem | feito (`de85276`) |
| 3 | **Textura direto em 16 bpp**: o front-end decodifica N64 já em ARGB1555/4444, sem conversão de pixel; só preenche até potência de 2 e copia | `sf64-dc/src/gfx/gfx_pvr.c:78-83` | Reforça o V4 (hoje RGBA8888) e o V14 (ATITC, que o Zeebo suporta) | aberto, pede aprovação |
| 4 | **Cache de textura paletizada por id (CI4)** e tamanho fixo da tabela de texturas (1024) | `sf64-dc/src/gfx/gfx_pvr.c:83`, `gfx_retro_dc.c` (só o comentário do cabeçalho) | Só vale se o Adreno aceitar texturas paletizadas (não confirmado) | baixa |
| 5 | **Validar contra o original ao vivo**: Sonic R foi comparado com o executável de 1998 sob o WinDbg e o único desvio restante foi x87 80-bit contra IEEE | `sonic-r/README.md` ("AI Disclosure") | Nosso equivalente é o `hash_gate.sh`; sem VFP o ARM11 usa soft-float, então o ruído de x87 não existe, mas o de SH-4 contra IEEE pode existir | conceito |
| 6 | **Regras de documento para agentes**: toda afirmação de hardware com citação, "o código do emulador vale mais que wiki", registrar toda instalação de ferramenta (`docs/kb/tooling.md`), `00-status.md` como ponto de partida | `cfp2dreamcast/CLAUDE.md` | Já fazemos quase tudo no roadmap; o que falta é o log de ferramentas instaladas | pequeno |
| 7 | **Layout derivado do ROM em vez de passo de build do alvo**: "no N64 build step, no MIPS toolchain" | `oot-dc/README.md` | É o que o `gen_*.py` já faz com o `.mod` | já temos |
| 8 | **Pool de páginas pequenas reciclado quando o quadro que as desenhou já foi mostrado** (256 B; listas de 31 comandos; vértices de 16 em 16) | `cruisn-usa-dreamcast/src/meshcomp.c:1-30` | Pensar nisso só se formos compilar malhas em tempo de execução (junto do item 1) | baixa |

## Inútil para o nosso caso

| O quê | Por quê |
|---|---|
| Conversão estática NAOMI→DC (`naomi-dc-port-kit`: `shims/`, `loader/`, mapa de endereços, BIOS) | É para rodar binário de arcade no Dreamcast; nosso port é código-fonte C em outro console |
| `sh4zam` (SIMD/FIPR do SH-4), store queues, listas do PVR, DMA do TA, `sq_fast_cpy` | Específico do SH-4/PVR; o ARM11 não tem equivalente |
| GLdc, KallistiOS, AICA, VMU, GD-ROM, `mkdcdisc`, builders de CDI | Plataforma Dreamcast |
| `repl_tex.c` (14.232 linhas de textura de substituição embutida) e os dados de arte | Dado de um jogo, não técnica |
| Plataformas de arcade e emuladores citados (Flycast instrumentado, Atomiswave) | Não temos arcade; o zeebx faz o papel do emulador |
| Repositórios sem código de port na prática (instalador, empacotamento, roadmap: `DCRSDK/Upcoming-Projects` de 5 KB) | Nada a aprender |
| Datas de lançamento e ordem cronológica do Qwen | Mistura criação, último push e notícia; não serve de fonte |

## O que muda no roadmap

- **V16 (novo, hipótese):** gravar e repetir malhas estáticas do SA1 (itens 1 e 8), começando por medir no zeebx quantas
  instruções por quadro vão em decodificar e traduzir strips de modelos que não mudam.
- **V15 corrigido (laço de tarefas lido):** no SA1 o desenho acontece **dentro** do `step` de cada tarefa
  (`stage_run_step` chama `task_step_all`; `task_draw_all` só roda nos quadros de pausa e de erro do cartão,
  `src/stage/stage.c:2662/2714`). Não existe um passe `disp` separado para pular em ticks atrasados. Pular desenho
  exigiria um interruptor no destino do desenho do port (`zstrip`/`zlive`) que descarte os envios enquanto a lógica
  roda. No log de uma execução do zeebx o jogo roda 2 passos por quadro e 2 vsyncs no menu inicial e 1 passo por
  quadro e 1 vsync de `game_mode 1` em diante, sem mudar mais até o gameplay (`game_mode 4`).

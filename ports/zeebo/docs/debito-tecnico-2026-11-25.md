# Débito técnico — sessão de 25/11/2026

Estado: levantamento. Nada aqui foi corrigido ainda, salvo onde marcado. Cada item diz o que
foi **medido**, o que é **hipótese** e como verificar.

Referência de build: `sa1n.mod` 26.583.004 B, SHA `5bf174e2`, com os 249 `.ovl` do mesmo build
(instalado no fullset às 12:19). Branch `zeebo-port`.

## D1. Mipmapping ausente (shimmering)

- **Medido:** só o nível-base é decodificado (`assets/pvr.h`: "mipmapped types retornam apenas o
  nível-base") e só o nível 0 vai ao GL (`zstrip_tex_new`/`tex16_upload` em `zstrip.c`), com
  filtros `GL_LINEAR`/`GL_LINEAR`. Sem cadeia de mips, mudar o filtro não tem efeito.
- **Medido (log da sessão do usuário):** 1.981 de 4.776 uploads eram de texturas mipmapped no
  original (`TWIDDLED+MIPMAP` 1.414, `VQ+MIPMAP` 567). São uploads acumulados (inclui reenvios), não
  texturas distintas. Os outros 59% (2.795: twiddled, VQ, retângulo e "modo 0") não têm cadeia
  no PVM.
- **Fonte primária:** Guia do Desenvolvedor 0.97 §7.1.3: mipmap em hardware; ideal
  `GL_LINEAR_MIPMAP_NEAREST` (min) e `GL_LINEAR` (mag); trilinear não suportado em hardware
  (`GL_LINEAR_MIPMAP_LINEAR` cai muito de desempenho); `SGIS_generate_mipmap` = No (tabela de
  extensões), então o port precisa fornecer os níveis.
- **Contexto de TV:** saída composta RCA, NTSC/PAL-M; o MDP reamostra os 640×480 (§3.1.6, §11.2);
  >90% do mercado-alvo era CRT. Que a reamostragem agrava o cintilar é hipótese, não medida.
- **Custo:** +33% de memória de textura. Texturas vivas: 10.208 KB RGBA8888 nominal (5.104 KB em
  16 bpp); o acréscimo seria ~1,7 MB (16 bpp) a ~3,4 MB (RGBA). Onde o driver aloca isso no
  console: **não sei**.
- **Plano:**
  1. Fiel ao Dreamcast: decodificar os mips nativos (um `pvr_decode_rgba8888` por nível) e
     enviar com `glTexImage2D(level n)`; min `GL_LINEAR_MIPMAP_NEAREST` só nas texturas com
     cadeia.
  2. Opcional: gerar mips por filtro de caixa nas texturas 3D sem cadeia; excluir HUD, sprites
     e UI (mistura de alfa, `alpha_bleed`).
- **Arquivos:** `assets/pvr.c`, `ztexture.c`, `zstrip.c`. Exige atualizar `hash_ref`
  (`tools/hash_ref.sh update`) e dizer por quê no commit.
- **Verificar:** `ttex` (memória viva), contagem de uploads por modo, comparação de quadros no
  rasterizador do `zeebx` (tem `sample_lod`; não é o filtro do Adreno).

## D2. Efeitos com contorno preto em vez de alpha e sobrepostos ao 3D

- **Reportado pelo usuário e anterior ao build atual:** sprites de explosão/efeito com contorno
  preto e desenhados por cima de geometria 3D.
- **Medido** (estado `slot-2` de Windy Valley restaurado com `zeebx session --state`, 3.000
  draws com `ZEEBX_DRAWLOG`): 222 quads de 4 vértices com mistura e **sem depth test**,
  intercalados entre draws com depth. Também 120 quads opacos de 4 vértices (75 com a textura
  418: 69 com 2 triângulos e 6 com 0), 2 draws aditivos (`1+1`) e só 6 draws com teste de alfa.
- **Causa provável dos sprites sobre o 3D (lida no código, não testada):**
  - Os billboards (`njDrawSprite3D`) chegam como vértices em tela com `z = 1/w`
    (`sprite_project` em `engine/dc/sa1/sprite.c`).
  - Os modelos 3D escrevem profundidade `(1 - 1/vz)/2` (`build_proj`, `zlive.c`): ~0,33 a 0,5.
  - O passe 2D usa `glOrthox` com `SCREEN_NEAR = -16` (`zstrip.c`): a profundidade de um sprite
    3D vira `16/17 - z/17` (0,92 a 0,94), sempre atrás de qualquer geometria 3D. Por isso o
    passe 2D roda sem depth test (`screen_depth_apply`) e todo sprite sai por cima da cena.
  - O comentário de `zscene_game_frame` ainda descreve o ortho antigo `-1..1`, que casava
    exatamente com o 3D (profundidade `(1 - z)/2`); o alargamento para -16 (pausa e HUD) quebrou
    essa equivalência. O comentário está desatualizado.
  - **Esboço de correção (não implementado):** só para strips de sprite 3D, ortho `-1..1`,
    `GL_LEQUAL`, `glDepthMask` desligado. HUD e pausa ficam como estão. Falta um flag que diga
    ao `zstrip` quando o strip vem de `sprite_draw_3d` (hoje ele não chega; a origem é
    `engine/sa1/sprites.c`). Validar com `hash_ref` (update e explicar), HUD, pausa e preview.
- **Hipóteses para o contorno preto (não confirmadas):** halo de alfa reto com filtragem
  bilinear (RGB dos texels transparentes) ou textura classificada como opaca (`tex_opq` é
  calculado na criação da textura; pode ficar velho se a textura for atualizada depois).
  `alpha_bleed` em `ztexture.c` já cobre texels de alfa < 128.
- **Verificar:** ver quais texturas os 222 quads usam e se alguma é uma das opacas; capturar a
  mesma cena com um build experimental (ver D9: estados não restauram após mudança de módulo).

## D3. Defeitos visuais antigos sem causa isolada

Sino de Speed Highway (corpo verde/laranja, badalo dourado liso), painel preto/vermelho/branco
de Station Square (aparece em 3 screenshots), mola vermelha ("bouncer") e rodas dos carros.
O usuário confirmou que já existiam antes; nenhum foi comparado com uma referência do
Dreamcast. Candidatos: mesma classe de D2 (alfa/blend), formato de textura ("modo 0" teve 33
uploads no log da sessão do usuário), UV/material trocado.

## D4. Quadros do `hash_ref`: −1,7% sem causa medida

3.036 quadros apresentados contra 3.088 da referência anterior em 100 s com as mesmas teclas.
Pode ser desempenho do jogo ou o ritmo de quadros do emulador. A Esther está testando um fix de
pacing em `src/speed/pace.rs` do `zeebx` (combinado: **não mexer no código do `zeebx` agora**).
Depois do fix, rodar `hash_ref.sh check`; se mudar, regenerar e explicar.

## D5. Z-Pad: "hold" não verificado

O mapeamento dos botões foi corrigido (`ef1df42`, `zhid.c`: `Button_1`↔`Button_2`), verificado
só pelas máscaras (`b1..b4` → 64/128/256/512). Não foi testado o comportamento de segurar o
botão. Suspeita não medida: em quadros com 2 passos (`passos por quadro` = 2) a borda de
`press` pode ser vista duas vezes, pois `pdExecPeripheralServer` roda uma vez por quadro.

## D6. Menu de debug (ZL+ZR+Cima por 3 s)

Correção: o combo usa os ombros SUPERIORES (`ZHID_ZL`/`ZHID_ZR`, `Left/Right_Shoulder_Upper`,
`ZDEBUG_COMBO = TL|TR|KU` em `zsys.c`), não L2/R2 como este título dizia antes.

Só foi exercitado no título. Falta testar dentro de uma fase e confirmar o reinício.

## D7. Cobertura da varredura de linhas do menu de debug

- **Medido** (`$TMPDIR/sweep_co_summary.txt`): 16 linhas (3, 6, 9, 12, 15, 18, 21, 24, 28, 32,
  35, 40, 46, 58, 62, 66), 0 faults, mas com um `.mod` de 30.772.188 B, isto é, **antes** do
  mapa `zdc_ram` por trechos (`35e7ee9`).
- **Com o mapa novo só foi medido** o boot de 30 s (0 faults, modos 0→1→8) e o fluxo até o
  menu/título. **Nenhuma linha de fase foi varrida com o `.mod` de `5bf174e2`.** Isso é o risco
  principal em aberto do mapa novo (a falha original do `pc=0` apareceu só em linhas de menu).
- Total de linhas do menu: ~72 pela skill; não reverificado. Script de varredura: ainda não
  existe (o comando que o criaria foi bloqueado por falta de aprovação).

## D8. Build: objetos velhos após mudar `zrebase.h`

- **Corrigido (erro meu):** a versão anterior deste relatório dizia que só 243 de 752 objetos
  tinham `.d`. **Falso.** Medido agora: 4.088 `.d` para 4.097 `.o` em `build/zeebo/game/obj`, e
  todos os 4.000 `.d` examinados citam `zrebase.h`. As dependências existem.
- **Causa dos objetos velhos que vimos: não verificada.** Hipóteses: cópias reescritas em
  `build/zeebo/game/rebased` (`$(RB)`) fora do alcance dos `.d`, ou a regra de programas/
  overlays sem `-MMD`. Investigar antes de culpar o `make`.
- **Feito (não commitado):** `tools/build.sh` para se um objeto for mais velho que
  `zrebase.h` (`ZREBASE_OK=1` ignora). Verificado com um arquivo de referência no futuro. É
  uma salvaguarda barata; a premissa do comentário no script ("só 243 de 752 têm .d") está
  errada e deve ser reescrita antes do commit.
- **Feito (não commitado):** selftest de `zrebase.c` cobre o ponteiro de fim de `WORK_AREA`
  (`0x0cf00000`) e os limites dos trechos. Boot de 30 s: 0 faults.

## D9. Estados de jogo e instrumentação

Um save state só restaura com o mesmo módulo; qualquer mudança no mapa do módulo invalida os
estados antigos. O `.mod` em `build/zeebo/` (SHA `11b6c7a4`, 26.481.164 B, ~100 KB menor que o
instalado) provavelmente inclui as edições não commitadas de outra sessão (`zshinobi.c`,
`stubnames.txt`); é inferência pela diferença de tamanho, **não verificada**. Não instalar nem
commitar. O `.mod` instalado no fullset continua sendo o de `5bf174e2`.

## D10. RAM (candidatos, **não reverificados nesta sessão**)

- Itens citados em conversa anterior: um buffer de diálogo compartilhado (~0,5 MB; o nome
  `dialog_pixels` **não existe** no código: procurar `message_window.pixels`/`bitmap`) e três
  buffers de textura de 256 KB (não localizados agora).
- A área de 1 MB entre `MOTION` e `STREAM`: uso não medido.
- Pico residual do `zdc_ram`: 4,45 MB tocados (medido em sessão anterior). Compactar dados
  residentes exigiria descompressão sob demanda (compressão só reduz o `.mod` em disco).

## D11. Higiene

- `/` a 100% (4,1 GB livres); `$TMPDIR` com 4,9 GB (`wp` 1,4 GB, `gl` 1,1 GB, `zb` 214 MB).
  Limpar só os arquivos próprios e com aprovação; `build/zeebo/keep/` e `game_o2/` não.
- Arquivos não rastreados: `sa1n.bmp`, `todo.md`, `ports/zeebo/tests/vmu_build_test.py`,
  `ports/zeebo/tools/vmu_build.py` (de outra sessão; não commitar).

## D12. Abertos e correções de afirmações anteriores

- O `zeebx session` imprime "classes the game requested that we do not have — 0x0106e415":
  classe BREW pedida e ausente no emulador; origem não investigada (sem ocorrência em
  `zeebo_demoscene/vendor` nem no `src` do `zeebx-emu-copytex`).
- Retratação: afirmei antes que a sessão do usuário registra "11 `sfx ausentes`". A busca por
  `sfx ausente` no log dá 0 ocorrências; essa contagem não tem base.
- Aprovações pendentes: commit de `zrebase.c` e `build.sh`; varredura de linhas.
- As leituras de screenshots e miniaturas foram feitas por modelo de visão, sem referência do
  Dreamcast; tratar como indícios.

## D13. Regressão ou antigo? (comparação com 23/11) — verificação opcional

Pedido do usuário: comparar com "anteontem" (23/11). **Não há screenshots nem save states de
23/11** (os mais antigos são de 24/11 15:55), então a comparação foi só por histórico de código
(`git log -S`), mais um screenshot de 24/11 15:55 lido por modelo de visão.

- **Sprites sobre o 3D: regressão provável, de 24/11 22:40 (`a5f8fa3`, "restaura movimentos e
  preview da selecao").** Antes: passe 2D com `glOrthox` near -1 / far 1, `GL_DEPTH_TEST`
  ligado e `GL_LEQUAL` (mesma profundidade do 3D). Depois: `SCREEN_NEAR = -16` (pausa e HUD usam
  z = 9..15) e depth test desligado por padrão (`screen_depth_test = 0`). Troca feita de
  propósito para consertar a pausa e o preview; custou a profundidade dos efeitos.
  Correção candidata: D2 (ortho -1..1 + `GL_LEQUAL`, só para sprites 3D).
- **Contorno preto: provavelmente não é regressão.** O port nunca definiu wrap/clamp de textura
  (`GL_TEXTURE_WRAP` só aparece nos commits de DYNRES de 23/11, `4e5d70b` e `1770e94`, e não existe
  na árvore atual). O screenshot de 24/11 15:55 (Emerald Coast) já mostra, segundo a visão, blocos
  de borda dura nas partículas de água. Não se exclui `5bc07fa` (24/11 12:14, translúcidos pelo
  caminho GL, adiados e ordenados), que é anterior a esse screenshot.
- **Experimento `GL_CLAMP_TO_EDGE` (feito e revertido):** texturas com alfa ganhavam clamp. As
  linhas 6 e 40 do menu de debug não mostram efeitos no quadro final, então o teste não decide
  nada. O ruído do mesmo build rodado duas vezes é de ~2.300 px, o que torna inútil a contagem de
  pixels. `zstrip.c` voltou ao HEAD.
- **Verificação opcional (adiada por decisão do usuário):** compilar o commit anterior ao
  `a5f8fa3` (ou ao `5bc07fa`) numa árvore à parte e rodar a mesma cena com efeitos
  (explosão, anéis). Custo: ~9 GB de objetos (o disco tem ~9 GB livres) e compilação longa; só com
  autorização. Alternativa mais barata: aplicar a correção de D2 e validar com uma cena de efeitos
  gravada no build novo (os estados antigos não restauram após mudar o módulo).

## D14. O `.mod` embute o caminho do usuário (`__FILE__` do `dbgprintf`) — resolver depois

Achado na auditoria do alpha (25/11): `strings -a sa1n.mod | grep /home/` dá **10 ocorrências**
de caminho absoluto: 7 de `/home/rafaelfrequiao/projects/sonic-adventure-zeebo/...` e 3 de
`/home/rafaelfrequiao/projects/zeebo_demoscene/...` (outro projeto), todas vindas do `__FILE__`
usado pelo `dbgprintf` (ex.: `ports/zeebo/zlibc.c:124`). Os `.ovl` não têm nenhuma. Os dois
7z do alpha (completo e lite) não têm saves (`sa1vmu*`), logs nem `.env`: o vazamento se limita
ao nome de usuário e de pastas de projeto.

- **Conserto:** compilar com `-ffile-prefix-map=$PWD=.` (e o equivalente para o caminho do
  `zeebo_demoscene`) nos `CFLAGS` do applet (`Makefile`) e do jogo (`game.mk`). Efeito colateral
  desejável: build reproduzível entre máquinas.
- **Custo:** muda o SHA do `.mod`, então pede rebuild, gates (752/752, `duplicatas: 0`,
  `host-selftest`, `novfp`, `hash_ref` — o `hash_ref` pode precisar de refazer o `.ref` se algo
  deslocar), nova passada de testes (menu de debug, VMU, menus a 30 fps) e **recompactar os
  dois 7z** (completo e lite), com novo SHA-256 em cada um.
- **Decisão adiada pelo usuário** (pacote só para testers internos): fazer antes de qualquer
  distribuição fora do grupo interno.
- **Não verificado:** se há outras strings de caminho além de `/home/` (ex.: `/mnt/`, `/tmp/`)
  nos `.mod`/`.ovl`; conferir com `strings` depois do conserto.

## Prioridade sugerida

1. D7 (varrer as fases com o mapa novo): é o maior risco em aberto; D8 já tem salvaguarda.
2. D1 passo 1 (mips nativos): maior ganho visual, risco médio de memória.
3. D2: a parte de profundidade tem causa e esboço de correção (acima); o contorno preto e D3
   seguem como hipótese.
4. D4/D5/D6: dependem de testes no `zeebx` e no hardware.

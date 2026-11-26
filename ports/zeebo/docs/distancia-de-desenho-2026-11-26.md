# Distancia de desenho: como objetos aparecem e somem, e a medicao (26/11/2026)

## Quem decide o que e desenhado

1. **Terreno** - `world_land_list_visible` (`src/collision/world.c`) monta a lista de entradas a desenhar
   (`DRAWN` = 1024). Esfera a frente da camera de raio `clip/2` mais o raio da entrada; `clip` e o do terreno
   (`LAND_CLIP`) ou o far do `njClipZ`. Entradas `LAND_FAR` usam sempre o far e nao entram na escala. A lista e so de
   desenho; a colisao nao a usa.
2. **Cada modelo** - `model_in_view` (`zlive.c`) e `sphere_in_window`/`draw_in_view_any_step` (`src/draw/model.c`):
   near/far pelo centro e pelo raio. Modelos com NO_CLIP_CHECK (fundo) nao passam por aqui.
3. **Objetos de gameplay** - `placement` (`src/object/placement.c`): a task so nasce com jogador/camera perto
   (`PLACE_RANGE` 400 ao quadrado, `KIND_RANGE` por tipo, `KIND_ALWAYS`) e sai com folga (`LEAVE_MARGIN`). E logica,
   nao desenho: **nao** foi escalada.
4. **Far por fase** - `njClipZ` (-3000 padrao; -2000 em 8 chamadas, -1000 em 7, -600, -1600, -5000, -10000 em outras).
   Nao ha LOD de modelo: o jogo so escolhe o que desenha.

## O que foi implementado

`drawdist.cfg` na pasta do jogo (inteiro 10..100; sem o arquivo, 100 = como no Dreamcast) define `zdraw_scale`,
que multiplica o clip do terreno, o far de `model_in_view` e o far de `draw_in_view`. Colisao, `placement` e
`LAND_FAR` ficam. Lido na primeira passagem por `model_in_view`. Log `tdist` (pct, entradas do terreno por quadro).
Com 100%: `hash_ref` IGUAL, 752/752, `host-selftest`, `novfp`.

## Medicao (`tools/dd_matrix.sh`, `tools/dd_summary.py`)

29 linhas do menu de debug x 100/75/50/25%, 45 s de fase por execucao (relogio virtual do zeebx, ~30 fps
travado nas fases pesadas, entao a metrica util e o uso da thread do jogo, `busy`, e o `zlive`). Media das janelas
de 2 s com a fase ativa. Colunas: entradas do terreno desenhadas por quadro; `busy` % da janela; ms/quadro; quadros
de 3+ vsyncs; desenhos e us de `zlive` por janela.

| linha | pct | janelas | terreno/quadro | busy % | ms/quadro | quadros 3+ vsync | desenhos | zlive us |
|---|---|---|---|---|---|---|---|---|
| 0 | 100 | 21 | 53.5 | 27.5 | 33.0 | 0.4 | 1921 | 130155 |
| 0 | 75 | 21 | 53.4 | 27.5 | 33.0 | 0.4 | 1919 | 129374 |
| 0 | 50 | 21 | 53.4 | 27.5 | 33.0 | 0.4 | 1918 | 128812 |
| 0 | 25 | 21 | 52.0 | 27.0 | 33.0 | 0.4 | 1913 | 126778 |
| 3 | 100 | 21 | 75.1 | 58.8 | 35.6 | 0.4 | 7371 | 262895 |
| 3 | 75 | 21 | 28.2 | 53.9 | 35.6 | 0.2 | 7129 | 216011 |
| 3 | 50 | 21 | 24.0 | 53.3 | 35.6 | 0.1 | 7092 | 208977 |
| 3 | 25 | 21 | 15.4 | 50.8 | 35.6 | 0.1 | 6499 | 182687 |
| 6 | 100 | 21 | 36.4 | 68.2 | 33.0 | 0.1 | 8912 | 254487 |
| 6 | 75 | 21 | 32.5 | 67.5 | 33.0 | 0.1 | 8928 | 249807 |
| 6 | 50 | 21 | 20.8 | 65.0 | 33.0 | 0.0 | 8928 | 223579 |
| 6 | 25 | 21 | 13.3 | 61.5 | 33.0 | 0.1 | 8074 | 192879 |
| 9 | 100 | 21 | 13.7 | 72.2 | 48.5 | 32.8 | 4452 | 148428 |
| 9 | 75 | 21 | 11.9 | 71.4 | 48.5 | 32.6 | 3927 | 137414 |
| 9 | 50 | 21 | 10.2 | 71.0 | 48.4 | 32.6 | 3667 | 134819 |
| 9 | 25 | 21 | 6.5 | 69.4 | 48.1 | 31.9 | 2980 | 113424 |
| 12 | 100 | 21 | 20.8 | 59.0 | 34.7 | 5.4 | 1872 | 215033 |
| 12 | 75 | 21 | 10.8 | 64.9 | 34.7 | 5.5 | 1623 | 234985 |
| 12 | 50 | 21 | 6.7 | 64.5 | 34.8 | 5.5 | 1431 | 225966 |
| 12 | 25 | 21 | 4.8 | 64.6 | 34.9 | 5.8 | 1390 | 224704 |
| 15 | 100 | 21 | 103.7 | 74.4 | 47.0 | 4.8 | 5209 | 350053 |
| 15 | 75 | 21 | 95.3 | 72.4 | 42.8 | 4.8 | 5396 | 329911 |
| 15 | 50 | 21 | 86.4 | 71.9 | 47.1 | 4.7 | 5169 | 327227 |
| 15 | 25 | 21 | 82.3 | 70.7 | 43.4 | 4.7 | 5362 | 331521 |
| 18 | 100 | 21 | 38.6 | 60.0 | 34.5 | 1.9 | 3931 | 277556 |
| 18 | 75 | 21 | 23.4 | 59.1 | 34.0 | 2.1 | 3918 | 267508 |
| 18 | 50 | 21 | 17.2 | 57.5 | 34.0 | 2.1 | 3620 | 251254 |
| 18 | 25 | 21 | 12.4 | 54.4 | 34.5 | 1.9 | 3212 | 231752 |
| 21 | 100 | 21 | 8.3 | 37.6 | 32.9 | 0.0 | 7675 | 82794 |
| 21 | 75 | 21 | 6.6 | 36.9 | 32.9 | 0.0 | 7655 | 76888 |
| 21 | 50 | 21 | 5.3 | 36.8 | 32.9 | 0.0 | 7620 | 75447 |
| 21 | 25 | 21 | 5.0 | 36.7 | 32.9 | 0.0 | 7652 | 75855 |
| 24 | 100 | 22 | 13.5 | 73.0 | 36.0 | 0.2 | 7793 | 166703 |
| 24 | 75 | 22 | 10.5 | 72.5 | 36.0 | 0.2 | 7793 | 161174 |
| 24 | 50 | 22 | 9.5 | 72.2 | 36.0 | 0.2 | 7695 | 157616 |
| 24 | 25 | 21 | 6.1 | 70.0 | 33.0 | 0.1 | 7587 | 149071 |
| 28 | 100 | 21 | 35.1 | 72.8 | 33.3 | 0.2 | 3708 | 145812 |
| 28 | 75 | 21 | 29.1 | 72.1 | 33.3 | 0.1 | 3690 | 138831 |
| 28 | 50 | 21 | 25.8 | 71.7 | 33.3 | 0.2 | 3671 | 135361 |
| 28 | 25 | 21 | 16.8 | 70.8 | 33.3 | 0.1 | 3567 | 122989 |
| 32 | 100 | 21 | 44.4 | 42.7 | 33.0 | 0.0 | 11063 | 198840 |
| 32 | 75 | 21 | 37.3 | 41.8 | 32.9 | 0.0 | 11143 | 186773 |
| 32 | 50 | 21 | 32.5 | 40.5 | 32.9 | 0.0 | 11032 | 172347 |
| 32 | 25 | 21 | 20.9 | 38.1 | 32.9 | 0.0 | 10833 | 151728 |
| 35 | 100 | 21 | 48.7 | 78.0 | 33.6 | 1.0 | 14268 | 368892 |
| 35 | 75 | 21 | 48.5 | 78.0 | 33.6 | 1.0 | 14267 | 368844 |
| 35 | 50 | 21 | 48.2 | 78.1 | 33.6 | 1.0 | 14263 | 369785 |
| 35 | 25 | 21 | 47.2 | 78.0 | 33.6 | 1.0 | 14258 | 368952 |
| 36 | 100 | 21 | 27.8 | 53.6 | 34.3 | 1.1 | 7336 | 270195 |
| 36 | 75 | 21 | 27.5 | 53.7 | 34.3 | 1.2 | 7343 | 271835 |
| 36 | 50 | 21 | 27.4 | 53.7 | 34.3 | 1.2 | 7340 | 271953 |
| 36 | 25 | 21 | 27.0 | 53.6 | 34.3 | 1.2 | 7330 | 273396 |
| 37 | 100 | 21 | 56.6 | 78.0 | 34.4 | 0.9 | 11972 | 200341 |
| 37 | 75 | 21 | 56.4 | 78.0 | 34.4 | 1.0 | 11972 | 201629 |
| 37 | 50 | 21 | 56.4 | 78.0 | 34.3 | 1.0 | 12018 | 204029 |
| 37 | 25 | 21 | 56.0 | 78.0 | 34.3 | 1.0 | 12018 | 201895 |
| 38 | 100 | 22 | 82.0 | 41.8 | 36.5 | 0.3 | 5054 | 201015 |
| 38 | 75 | 22 | 80.6 | 41.5 | 36.5 | 0.3 | 5046 | 200474 |
| 38 | 50 | 22 | 78.9 | 41.5 | 36.5 | 0.2 | 5034 | 198470 |
| 38 | 25 | 22 | 70.7 | 40.4 | 36.5 | 0.2 | 4989 | 190723 |
| 39 | 100 | 21 | 0.0 | 15.0 | 34.5 | 0.8 | 1421 | 43287 |
| 39 | 75 | 21 | 0.0 | 15.0 | 34.5 | 0.8 | 1421 | 43550 |
| 39 | 50 | 21 | 0.0 | 8.9 | 29.6 | 0.5 | 1106 | 26974 |
| 39 | 25 | 21 | 0.0 | 12.9 | 34.5 | 0.8 | 1348 | 27112 |
| 40 | 100 | 21 | 76.3 | 44.2 | 33.0 | 0.4 | 1746 | 155267 |
| 40 | 75 | 21 | 68.7 | 43.0 | 33.0 | 0.3 | 1751 | 145603 |
| 40 | 50 | 21 | 57.0 | 41.7 | 33.0 | 0.4 | 1509 | 130002 |
| 40 | 25 | 21 | 35.7 | 39.0 | 32.9 | 0.0 | 1456 | 105230 |
| 46 | 100 | 21 | 50.3 | 52.5 | 33.0 | 0.1 | 11957 | 204163 |
| 46 | 75 | 21 | 45.7 | 52.1 | 33.0 | 0.0 | 11957 | 201195 |
| 46 | 50 | 21 | 39.1 | 51.4 | 33.0 | 0.0 | 11902 | 194011 |
| 46 | 25 | 21 | 26.3 | 50.0 | 33.0 | 0.0 | 11780 | 181332 |
| 52 | 100 | 22 | 98.1 | 40.1 | 34.9 | 0.1 | 478 | 185641 |
| 52 | 75 | 22 | 98.0 | 40.2 | 34.8 | 0.1 | 470 | 184832 |
| 52 | 50 | 22 | 97.9 | 40.1 | 34.9 | 0.1 | 478 | 185613 |
| 52 | 25 | 22 | 97.4 | 40.0 | 34.9 | 0.1 | 476 | 185074 |
| 58 | 100 | 21 | 67.7 | 79.3 | 33.0 | 0.0 | 5041 | 401314 |
| 58 | 75 | 21 | 56.8 | 78.1 | 33.0 | 0.0 | 4979 | 393066 |
| 58 | 50 | 21 | 35.9 | 74.8 | 33.0 | 0.0 | 4682 | 360385 |
| 58 | 25 | 21 | 23.9 | 72.7 | 33.0 | 0.0 | 4564 | 338559 |
| 62 | 100 | 21 | 253.2 | 76.8 | 33.0 | 0.0 | 8396 | 342727 |
| 62 | 75 | 21 | 215.1 | 73.4 | 33.0 | 0.1 | 8402 | 312332 |
| 62 | 50 | 21 | 117.2 | 65.9 | 34.5 | 0.1 | 8197 | 226318 |
| 62 | 25 | 21 | 47.1 | 54.4 | 34.5 | 0.1 | 8197 | 112402 |
| 65 | 100 | 22 | 54.2 | 52.7 | 17.9 | 0.2 | 7415 | 245510 |
| 65 | 75 | 22 | 36.3 | 49.2 | 17.8 | 0.2 | 7413 | 212166 |
| 65 | 50 | 22 | 23.4 | 46.3 | 18.0 | 0.2 | 7193 | 184146 |
| 65 | 25 | 22 | 14.7 | 42.9 | 18.0 | 0.2 | 6012 | 148600 |
| 66 | 100 | 23 | 0.0 | 27.9 | 35.2 | 0.1 | 11091 | 196556 |
| 66 | 75 | 23 | 0.0 | 27.9 | 35.2 | 0.1 | 11091 | 196267 |
| 66 | 50 | 23 | 0.0 | 27.7 | 35.0 | 0.1 | 11035 | 194373 |
| 66 | 25 | 23 | 0.0 | 27.6 | 35.0 | 0.1 | 11035 | 194924 |
| 67 | 100 | 23 | 0.0 | 44.2 | 34.7 | 0.1 | 11043 | 330175 |
| 67 | 75 | 23 | 0.0 | 44.2 | 34.7 | 0.1 | 11043 | 330357 |
| 67 | 50 | 23 | 0.0 | 44.2 | 34.7 | 0.1 | 11044 | 330268 |
| 67 | 25 | 23 | 0.0 | 44.2 | 34.7 | 0.1 | 11044 | 330419 |
| 68 | 100 | 11 | 0.0 | 6.8 | 36.0 | 0.2 | 9694 | 0 |
| 68 | 75 | 11 | 0.0 | 6.8 | 36.0 | 0.2 | 9696 | 0 |
| 68 | 50 | 11 | 0.0 | 6.8 | 36.0 | 0.2 | 9695 | 0 |
| 68 | 25 | 11 | 0.0 | 6.7 | 36.0 | 0.2 | 9710 | 0 |
| 69 | 100 | 21 | 39.0 | 29.4 | 35.0 | 0.0 | 2539 | 60354 |
| 69 | 75 | 21 | 39.0 | 29.3 | 34.9 | 0.0 | 2537 | 60844 |
| 69 | 50 | 21 | 37.0 | 29.2 | 35.0 | 0.0 | 2539 | 60063 |
| 69 | 25 | 21 | 36.0 | 29.2 | 35.0 | 0.0 | 2539 | 59180 |
| 70 | 100 | 14 | 17.0 | 19.5 | 38.2 | 0.5 | 2286 | 29980 |
| 70 | 75 | 14 | 17.0 | 19.1 | 38.1 | 0.5 | 1635 | 30100 |
| 70 | 50 | 14 | 17.0 | 19.5 | 38.2 | 0.5 | 2286 | 29817 |
| 70 | 25 | 14 | 16.0 | 19.2 | 38.2 | 0.5 | 2286 | 27537 |
| 71 | 100 | 21 | 27.0 | 30.6 | 33.0 | 0.1 | 1158 | 66982 |
| 71 | 75 | 21 | 27.0 | 30.6 | 33.0 | 0.1 | 1158 | 66801 |
| 71 | 50 | 21 | 27.0 | 30.6 | 33.0 | 0.1 | 1158 | 67028 |
| 71 | 25 | 21 | 24.0 | 30.4 | 33.0 | 0.1 | 1158 | 62710 |
| 72 | 100 | 21 | 8.0 | 29.2 | 32.9 | 0.0 | 2058 | 88652 |
| 72 | 75 | 21 | 8.0 | 29.2 | 32.9 | 0.0 | 2058 | 88577 |
| 72 | 50 | 21 | 8.0 | 29.1 | 32.9 | 0.0 | 2059 | 89510 |
| 72 | 25 | 21 | 8.0 | 29.2 | 32.9 | 0.0 | 2058 | 88263 |

## Leitura

- Ganham muito (terreno grande): linha 62 (busy 76,8 -> 54,4; terreno 253 -> 47), 65 (52,7 -> 42,9), 58, 3, 6, 18, 40.
  Em geral 25% corta 40 a 80% das entradas do terreno e 5 a 25 pontos de `busy`; 75% ganha pouco (1 a 4 pontos).
- Nao mudam (o custo esta em outro lugar): 35, 37, 52, 66, 67, 69-72 e 0 - fases com terreno em `LAND_FAR`, sem
  terreno, ou dominadas por objetos/translucidos.
- Linha 12 vai contra a tendencia (59 -> 64,6): a execucao nao e deterministica entre pcts (tempo de jogo/camera
  diferem), entao diferencas de ~5 pontos sao ruido. Nao medi repeticao.
- **Falha independente do pct:** linha 68 acusa 2 faults com 100/75/50/25 (ja existia, nao e da distancia de desenho).
  Reproduz 4 de 4: a linha 68 abre o Sand Hill (sandboard, `SANDBOARD.PRS`, `SL1C0B.bin`) e o guest cai a ~35 s com
  `fault at 0xffffeea5 pc 0xba76c lr 0xba8ec` (callback 0x104b4 sem retorno). Pelas duas formas de mapear o pc
  (`pc - 0x10200` cai em `pose_node`, `bl njPushMatrix`; `pc - 0x10048` cai em `pose_get_matrix`) o caminho e o de
  `pose.c` (`pose_draw_player`), mas as duas leituras nao fecham com uma instrucao que acesse memoria: causa NAO
  isolada. Proximo passo: salvar state perto da queda e ler `player->anim.action` e a tabela `actions` do sandboard.
- Nao medi o custo visual (popping). Nao verifiquei nada em hardware real; `busy` e do relogio virtual do zeebx.

## Proximo passo (nao feito)

Modo dinamico: subir/descer entre 100/75/50/25 pelo `busy` e pelos quadros de 3+ vsyncs (histerese ~4 s). O
`placement` pode ganhar escala propria, mas muda a jogabilidade e fica fora desta etapa.

# Otimizacoes propostas para o port (2026-11-24)

Base de medicao: **Chaos 0** (`DEBUG_TITLE=35`), 80 s de zeebx, **2443 quadros
apresentados**, 23,1 bi de instrucoes. Perfil por amostragem de PC (652 mil amostras) e
histograma de chamadas do guest. Nada aqui esta implementado; cada item diz como provar o
ganho.

## Onde o tempo vai hoje

Perfil (Chaos 0, build de 24/11 a noite):

```
soft-float            63,7% das amostras
sa_ftrv                7,7%   FTRV generico (multiplicacao de matriz)
sprite_view            7,3%   4 cantos de sprite (expansao direta, ja sem FTRV)
f__njInvertMatrix      5,4%   inversa de matriz nos welds
draw_meshset_gl        3,8%
sqrtf                  3,8%
polygon_find_heights   3,7%   colisao
njCalcPoint            3,7%
shz_xmtrx_ftrv_        3,5%
njRotateXYZ            3,1%
njCalcVector           3,1%
surface_upload         3,1%   (so carregamento)
zstrip_model_vertex    3,0%
mesh_edge_side         2,2%   colisao
world_list_near        2,2%   colisao
sa_fipr                2,0%
njRotateX/Y            3,4%
polygon_sweep          1,7%   colisao
```

Chamadas de API do guest por quadro (2443 quadros):

```
DrawArrays        486   (221 do zstrip, ~265 do zlive)
BindTexture       337
LoadMatrixx       397
MatrixMode        154
DepthMask          65
Enable/Disable     55/53
LoadIdentity       10
aee_GetUpTimeMS   396
texturas vivas no stage: 520
```

## A. Chamadas de driver (o que domina no console, invisivel no zeebx)

O zeebx rasteriza na CPU do PC: o custo de uma chamada GL aparece como trabalho no host,
nao como instrucao do guest. Estes itens nao rendem instrucoes no zeebx; rendem no
Adreno 130 (driver + binning). Medem-se pela **contagem de chamadas**, que e deterministica.

### A1. Agrupar strips opacos por textura

- **Evidencia:** 337 `BindTexture` por quadro com 520 texturas vivas. O guia (secao de
  texturas) documenta um **cache de 8 KB**, cujo encaixe otimo e 64x64x16bpp / 32x128:
  quase nenhuma textura nossa cabe, entao cada troca e um miss garantido.
- **Acao:** na flush, ordenar os strips opacos por `texId` (estavel dentro do grupo, para
  nao mexer na ordem relativa). Translucidos ja sao ordenados por z e nao entram.
- **Ganho esperado:** binds de 337/quadro para a ordem do numero de texturas ativas.
- **Risco:** ordem de opacos com Z-buffer 16 bits nao muda a imagem, exceto z-fighting.
- **Como medir:** contagem de `BindTexture` no histograma; instrucoes; e diff de imagem
  (md5 do BMP final + mean abs diff de 30 quadros) para provar que nada mudou de cor.
- **Esforco:** pequeno (o radix do zstrip ja existe; e outra chave).

### A2. Juntar strips contiguos num `DrawArrays`

- **Evidencia:** 486 draws por quadro, 221 deles do zstrip; cada meshset pequeno vira uma
  chamada.
- **Acao:** concatenar strips vizinhos com o mesmo estado (textura, blend, depth mask) num
  unico array antes de submeter.
- **Ganho esperado:** centenas de draws por quadro viram dezenas.
- **Risco:** medio (mexer na montagem dos arrays e nos contadores de estado).
- **Como medir:** contagem de `DrawArrays`; diff de imagem; `hash_ref` (vai mudar de
  alinhamento, entao comparar as marcas iniciais).

### A3. Cache da matriz de modelo (LoadMatrixx)

- **Evidencia:** 397 `LoadMatrixx` por quadro. A projecao ja tem cache
  (`proj_differs()`); a modelview nao. Efeitos, particulas e sprites repetem a mesma
  matriz do objeto anterior.
- **Acao:** guardar os 16 valores da ultima modelview carregada e pular o `LoadMatrixx`
  quando forem iguais.
- **Risco:** baixo. **Medir:** contagem de `LoadMatrixx`.

### A4. Reduzir trocas de estado por ordenacao

- **Evidencia:** 65 `DepthMask`, 55 `Enable`, 53 `Disable` por quadro, com shadow ja
  funcionando (`gl_dmask`, `gl_blend`, `gl_tex2d`): o numero e de trocas reais, causadas
  pela alternancia opaco/translucido.
- **Acao:** desenhar todos os opacos antes dos translucidos (e os deferidos do zlive ja
  estao em lote); revisar se o zlive submete intercalado.
- **Risco:** baixo. **Medir:** contagem das tres chamadas.

## B. CPU do jogo (o que o zeebx mede)

### B1. Inversa de matriz nos welds (`f__njInvertMatrix`, 5,4%)

- **Evidencia:** 126 chamadas de `njInvertMatrix` partem de `path_editor_reorder`, 62 de
  `event_motion_weld`, 62 de `event_motion_weld_pose`, 32 de `event_weld`.
- **Acao:** cachear a inversa por matriz de origem (comparar 12 floats) ou explorar que
  weld rigido so precisa da transposta da rotacao.
- **Ganho esperado:** ate ~4% das instrucoes. **Risco:** baixo com cache por comparacao.

### B2. Multiplicacao de matriz (`sa_ftrv` 7,7% + `shz_xmtrx_ftrv_` 3,5%)

- **Evidencia:** o maior bloco restante depois do `sprite_view`. Sobrou de `njGetMatrix` ->
  `shz_xmtrx_mul4x4_colsf_` e das pilhas de matriz (`njPushMatrix`/`njScale`/
  `njRotateXYZ`).
- **Acao:** eliminar multiplicacoes de identidade (se a matriz empilhada e identidade, o
  `mul4x4` e copia) e nao recalcular `njDetMatrix` quando a matriz nao mudou.
- **Risco:** baixo se cada atalho for provado no host (como foi o `zam_turn`).

### B3. Colisao (`polygon_find_heights`, `mesh_edge_side`, `world_list_near`,
`polygon_sweep` = ~10%)

- **Evidencia:** ja documentado no roadmap: um laco de soft-float por entrada, sem laco
  redundante; o custo esta no numero de chamadas por quadro.
- **Acao:** grade espacial por area (ou cache do ultimo poligono por consulta, que
  aproveita a coerencia do movimento do Sonic).
- **Risco:** medio (mexer em colisao muda o comportamento do jogo; precisa A/B com a
  mesma sequencia de teclas e comparar posicoes/estado, nao so imagem).

### B4. `sqrtf` 3,8%

- **Acao:** identificar os chamadores (histograma por endereco de retorno, como foi feito
  com `sa_ftrv`) antes de mexer. Se for normalizacao de vetor em colisao, o item B3
  resolve junto.

### B5. `aee_GetUpTimeMS` 396 por quadro

- **Evidencia:** o port so chama `GETUPTIMEMS` algumas vezes por tique; o volume vem do
  jogo consultando `syTmrGetCount`.
- **Risco:** cachear por tique derruba a resolucao do timer do engine (que roda a 60 Hz);
  pode mudar o ritmo do jogo. **Investigar o uso antes**, nao e ganho livre.

## C. Textura e banda (so no console)

### C1. ATITC

- 6:1 para RGB (24 -> 4 bpp) e 4:1 para RGBA (32 -> 8 bpp), confirmado no guia. Reduz a
  pegada e melhora o cache de 8 KB. **Bloqueio conhecido:** nao ha codificador no host; o
  `TEX16=1` atual e so 16 bits linear.

### C2. Texturas menores onde nao se nota

- Com cache de 8 KB (64x64x16bpp otimo), uma textura 256x256 e miss garantido em cada
  acesso. Reduzir resolucao de texturas de objetos pequenos/distantes e mais barato que
  ATITC e nao depende de encoder.
- **Como medir:** nao ha medicao no zeebx (rasterizador de CPU); julgar por imagem e pela
  contagem de uploads (`ttex`).

## D. Descartadas (com motivo)

- **`surface_scale`/DYNRES:** o gargalo e CPU do jogo, nao fill; ja implementado atras de
  flag e desligado por decisao (DYNRES=0).
- **VBO com `GL_LIGHTING`/skinning:** nao usamos `GL_LIGHTING`; iluminacao e por software
  no jogo, como o guia manda.
- **Limite de 65.535 vertices por mesh:** **nao existe no guia** (procurei 65535, 65.535 e
  "16-bit index"); nenhum meshset nosso chega perto disso.
- **Recompressao de texturas (deflate/UCL/zstd):** ja medido e descartado; PVM e VQ e
  quase nao comprime.
- **PRS mais rapido:** `tprs` mediu ~23 MB/s e o carregamento nao e o gargalo do quadro.

## Fatos do guia usados aqui (e uma correcao)

Fonte: `zeebo-emulator/research/docs/sdk-extract/ZeeboDeveloperGuide0.97.md`.

- Cache de textura **8 KB**, otimo 64x64x16bpp / 32x128 (secao de texturas). E oficial,
  nao "relato de forum".
- **1.6M triangulos/s** (linha 238) e oficial.
- Teto de **35 fps** para evitar flicker (linha 3504); >30 fps e possivel em VGA (linha
  526).
- ATITC 6:1 / 4:1; clipping e iluminacao por software; VBO sem aceleracao com
  `GL_LIGHTING` ou matrix-palette skinning.
- O relatorio do DeepSeek (dump em `deepseek-chat-etapas-render-3d-gles1.md`, resposta 15)
  erra ao dizer que 1.6M/s nao e oficial, que a meta era 30 fps, e ao atribuir ao guia um
  limite de 65.535 vertices.

## Resultados medidos (2026-11-24, noite)

**A3 (cache da modelview) — FEITO.** `model_flush` guarda os 16 valores e o
`zstrip_model_begin` compara durante a copia que ja fazia (sem segunda passada; a flag e
consumida quando carrega, senao o mesmo modelo flushado duas vezes carregava de novo).

```
                antes        depois
LoadMatrixx     968.657      635.484   (-34%, -136/quadro)
instrucoes      23.100 M     23.139 M  (+0,17%)
```

**A2 (juntar strips translucidos) — MEDIDO E DESLIGADO.** Dois strips vizinhos no pool com
o mesmo estado entram num `DrawArrays` so e economizam 8,1% das chamadas (824.465 ->
757.656 em 60 s), com **zero** custo de instrucoes. Mas o strip concatenado gera o
triangulo de ligacao entre o ultimo vertice de um e os dois primeiros do outro: sem
vertices degenerados isso desenha geometria espuria (10.659 pixels diferentes no mesmo
quadro). Fica atras de `Z_TJOIN` (0 por padrao) ate inserir degenerados, o que exige copia
no pool.

**Instrumentacao que ficou:** `vprof binds de textura / texturas distintas / runs
translucidos / vertices translucidos / strips unidos` por janela (o ultimo so aparece quando
`Z_TJOIN=1`, porque com a uniao desligada ele e sempre 0). Texturas distintas usa
hash aberto de 512 (a varredura linear custava 1,7% do guest).

**Descoberta que muda a prioridade:** o batching opaco **ja funciona** (6 draws por
quadro); 155 dos 161 draws do zstrip sao translucidos, um por strip, e a ordem por z
impede agrupar. O alvo de A1/A2 nao e o opaco.

## Ordem sugerida

1. **A3** (cache de modelview) e **A1** (agrupar por textura): menor risco, maior efeito
   no console, mediveis por contagem.
2. **A2** (juntar draws) depois de A1, porque compartilham a mesma reorganizacao.
3. **B1** (inversa com cache) e **B2** (identidade no mul4x4): ganho de CPU medivel no
   zeebx, risco baixo.
4. **B3/B4** (colisao e sqrtf) exigem investigacao antes.
5. **C1/C2** so quando houver encoder ATITC ou decisao sobre reduzir texturas.

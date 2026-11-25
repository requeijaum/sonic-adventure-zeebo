# Débito técnico — LoD e distância de desenho (25/11/2026)

Levantamento por leitura de código e uma conta (sem compilar nem rodar). Nada foi alterado.
Cada item marca o que foi **lido**, o que foi **calculado** e o que é **hipótese**.

## Resumo

1. **LoD:** não existe sistema de LoD no port nem no decomp (busca pela palavra inteira `lod` em `src/`,
   `modules/`, `engine/` e `ports/zeebo/`: 0 ocorrências). O jogo cuida de distância nos
   próprios objetos (`src/object/*.c`, `draw_in_view`). A varredura não foi exaustiva.
2. **Distância de desenho:** vem do jogo (`njClipZ`); o port respeita near/far só na CPU.
3. **Precisão de Z:** o mapeamento de profundidade do port usa ~1/6 dos 65.535 níveis de um
   Z de 16 bits (cálculo, não medido no console).

## D1. Distância de desenho (near/far)

**Lido** (valores de `njClipZ(near, far)` no decomp; `sdk/ninja/njClipZ.c` nega os dois e
limita near ≥ 1 e far ≤ 65535, então o guardado é a distância positiva):

| Onde | near / far |
| --- | --- |
| `src/main.c:651` (padrão) | -3 / -3000 |
| `modules/stg01/beach.c:287` e `modules/stg07/ruin.c:264` | -3 / -2000 |
| `modules/stg05/mountain.c:757` e `:795` | -3 / -50000 e -3 / -5000 |
| `modules/stg12/shelter.c` | -3 / `VIEW_FAR` e `START_FAR` |
| `modules/b_egm3/eggmobile3.c:46` | -1 / -2000 |

**Lido** (como o port aplica):
- `ports/zeebo/zlive.c` `model_in_view`: descarta o modelo se
  `cz <= near - r` ou `cz >= far + r` (só o z do centro e o raio).
- `engine/sa1/ninja.c` (`SA_DRAW_OUT`), `engine/sa1/sprites.c` e `engine/sa1/live.c`: mesmo
  critério para modelos, sprites e malhas.
- `build_proj` (`zlive.c`): `z_clip = -1`, `w = vz`; ou seja `z_ndc = -1/vz`. **Não há plano far
  no GL**: nada é cortado por triângulo, o corte por distância acontece por modelo.

**Consequências (hipóteses, não medidas):**
- Um modelo grande cujo centro está dentro do far aparece inteiro além dele; um cujo centro
  passa de `far + r` some de uma vez. O Dreamcast cortava por polígono no hardware.
- **Em aberto (não verificado):** existem dois pares de variáveis no ELF,
  `_nj_near_clip_`/`_nj_far_clip_` (as que `zlive.c` lê) e `__nj_near_clip_`/`__nj_far_clip_`
  (`ports/zeebo/ninja/nj_sprite.c`), e o `njClipZ` traduzido usa células de dados próprias
  (`d_njClipZ__njClipZ__near_clip`/`far_clip`). Não confirmei que `njClipZ` escreve nas que
  `zlive` lê. Se não escrever, a distância de desenho do port seria a padrão de `njInitSystem`
  (1 / 60000), não a de cada fase. Verificar com um log de `_nj_far_clip_` em Beach e Mountain
  antes de qualquer conclusão desta seção.
- Fog por vértice só existe no caminho GL de `zlive` (nota da skill, não reverificada agora):
  objetos fora desse caminho não esmaecem com a distância e o corte pode aparecer.

## D2. Precisão do buffer de profundidade

**Calculado** para o caminho dos lotes 3D (`build_proj`, matriz de projeção no GL), com n = 3,
f = 3000 e Z de 16 bits linear em `(z_ndc + 1) / 2`. O caminho de vértices em tela (passe 2D,
`glOrthox` -16..1) usa outro mapeamento e **não foi calculado**:

| Distância z | Passo por nível (port) | Passo com mapeamento cheio n..f |
| --- | --- | --- |
| 100 | 0,31 | 0,05 |
| 300 | 2,75 | 0,46 |
| 1000 | 30,5 | 5,1 |
| 3000 | 274,7 | 45,7 |

- O port usa só **10.912 dos 65.535 níveis**: `-1/z` vai de `-0,333` (z = 3) a ~0 (z = 3000), e o
  intervalo de `z_ndc` aproveitado é 1/6 do total. Com far = 50000 (Red Mountain) a fração
  usada continua ~1/6, e o passo a z = 10000 chega a ~3.000 unidades.
- **Hipótese:** isso causaria z-fighting à distância (decalques, sombras, bordas de plataformas)
  no console. Não verifiquei a precisão de profundidade do rasterizador do `zeebx`, então não sei
  se ele esconderia o problema. **Não medido em hardware.**
- **Candidato a correção:** escolher `p[10]` e `p[14]` em `build_proj` para que
  `z_ndc = c1 - c2 / vz` cubra `[-1, 1]` entre near e far (`c2 = 2 / (1/n - 1/f)`,
  `c1 = 2 (1/n) / (1/n - 1/f) - 1`, com o sinal que mantém `GL_GEQUAL`). Risco: o passe 2D e o
  modo misto de `preview_draw` usam o mesmo esquema de profundidade (ver
  `zstrip_screen_depth`); regenerar `hash_ref` e validar HUD, pausa e seleção de personagem.

## D3. Sem LoD de geometria nem de textura

- **Geometria:** não há troca de modelo por distância no port; o jogo escolhe o que desenha.
  Ideia barata, não medida: descartar modelos cujo raio projetado seja < 1 px em `model_in_view`
  (uma multiplicação e uma comparação), para reduzir os ~486 `DrawArrays` por quadro medidos em
  Chaos 0. Muda o `hash_ref`.
- **Textura:** sem mipmaps (ver `debito-tecnico-2026-11-25.md`, D1); o LoD por pixel do
  hardware nunca atua, o que agrava o shimmering à distância.

## D4. O que falta medir

1. Contadores por quadro de modelos descartados por near, por far e por tamanho (em
   `zlive.c`, nome de log com `v`/`t` para não gastar cota de `zlog`).
2. Em Red Mountain (far = 50000) e na praia (far = 2000): z-fighting visível em `zeebx` com
   Z de 16 bits forçado? Não sei se o `zeebx` oferece isso; verificar no código antes de
   propor.
3. Comparar o corte por modelo com o do Dreamcast numa cena de referência (mesmo ponto e
   câmera), antes de chamar qualquer diferença de regressão.

## Prioridade sugerida

1. D4.1 (contadores): barato, sem mudar o desenho.
2. D2 (mapeamento de Z): maior ganho potencial; precisa de teste em hardware.
3. D3 (descarte por tamanho): ganho de desempenho, com impacto visual a medir.

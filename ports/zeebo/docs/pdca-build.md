# PDCA — tempo de build do port

Objetivo: tirar do caminho o tempo de build (cada iteração custava 7–10 min,
com o jogo, o letreiro, o Boomerang e o áudio esperando por isso).

## Ciclo 1 (23/11/2026)

### P — Plan

**Medida de partida** (ideapad, 16 núcleos, `make -C ports/zeebo -f game.mk mod`
sem nenhuma mudança de código; `tools/buildtime.sh` mede cada receita):

| Fase | Antes |
| --- | --- |
| `gen_traps.py` (`readelf -rW` em um objeto de 100 MB, saída em texto) | **~380 s** |
| `gen_ovl.py extract` (242 overlays) | 20 s |
| `link` (conferir 744 objetos, `ld -r`, `full.o`) | 15 s |
| `make all` recursivo dentro do `link` | 11 s |
| `integrate.py` (`nm` em 100 MB) | 5 s |
| `objcopy --weaken` | 2,7 s |
| `elf2mod` (wine) | 2,2 s |
| `ld` do `sa1g.elf` e do `ovl_extract.elf` | 2,3 s |
| **Total medido com o `gen_traps` antigo** | **453 s (7,5 min)** |

Hipóteses, ordenadas por ganho esperado:
1. `gen_traps` só precisa dos nomes-alvo das relocações de chamada; ler o ELF direto
   dispensa o texto do `readelf`.
2. `gen_ovl` perde tempo em Python: `Elf.sym()` varre 600 mil símbolos a cada uma das
   486 chamadas, e o `__init__` decodifica nome por nome.
3. `JOBS ?= 2` com 16 núcleos: só pesa em compilação a frio.
4. `make` sem dependências de cabeçalho (`-MMD`): editar um `.h` não recompila nada.
5. Falha silenciosa: o `.mod` pode não ser gerado e ninguém percebe.

Meta: build de código comum em **menos de 1 min**.

### D — Do

| Mudança | Arquivo | Resultado |
| --- | --- | --- |
| `gen_traps` lê as tabelas `.rel` direto (ELF32 LE) | `tools/gen_traps.py` | **453 s → 1,6 s** |
| `gen_ovl`: tabela de símbolos em dicionário + `iter_unpack` | `tools/gen_ovl.py` | `extract` **20 s → 1,2 s** |
| `gen_ovl`: cache de compressão PRS por conteúdo (`game/ovlcache`) | `tools/gen_ovl.py` | evita recomprimir overlays iguais |
| `.mod` apagado antes e conferido depois do `elf2mod` | `game.mk` | falha vira erro explícito |
| `JOBS ?= 8` | `game.mk` | compilação a frio mais rápida |
| `-MMD -MP` + `-include` dos `.d` | `game.mk` | dependência de cabeçalho (ver Check) |
| `build.sh` serializa com `flock` | `tools/build.sh` | duas sessões não pisam no mesmo `.o` |
| `buildtime.sh` mede cada fase | `tools/buildtime.sh` | medição repetível |

### C — Check

| Cenário | Antes | Depois |
| --- | --- | --- |
| Sem mudança (`mod`) | 453 s | **40 s** |
| Um `.c` do jogo (`src/stage/stage.c`) | ~7,5 min (estimado: o `gen_traps` antigo roda em qualquer mudança) | **42 s** |
| Um `.c` do applet (`ztexture.c`) | ~7,5 min (estimado, idem) | **42 s** |

Equivalência (as mudanças não podem alterar o resultado):
- `ztraps.c` gerado pelo `gen_traps` novo e pelo antigo: **mesmo md5** (`414b8c…`), 30 funções, 273 dados.
- 242 de 242 `.ovl` byte a byte iguais antes e depois da mudança do `gen_ovl`.

Achados do Check:
- **Header não rastreado:** `touch sdk/include/ninja.h` não recompilou nenhum objeto (0 arquivos `.d`
  em `obj/`). O `-MMD` só passa a valer para cada objeto depois que ele for recompilado uma vez.
  Para cobrir todos de uma vez é preciso uma recompilação a frio (apagar os `.o`), que **não fiz
  sem autorização**.
- **Corrida entre sessões:** enquanto eu media, outra sessão do Hermes (id diferente) rodou
  `make mod` na mesma árvore, e o meu `integrate` falhou com `full.o is empty`. Repetido sem a
  outra, passou. Daí o `build.sh` com `flock`. Quem usar `make` direto não é protegido.
- Meta atingida: 453 s → 40–42 s, cerca de 11 vezes mais rápido.

### A — Act

Padronizar:
- Usar `ports/zeebo/tools/build.sh [DEBUG_TITLE=0]` em vez de `make` direto.
- Medir com `tools/buildtime.sh` antes de mexer no build de novo.

Ciclo 2 (o piso agora é ~40 s; sobram):
1. `make all` recursivo dentro do `link` (11 s): virar pré-requisito do mesmo make.
2. `link` (15 s): `game.o` é refeito duas vezes (receita `link` e regra `game.o`) e `full.o`,
   `full_w.o`, `full_weak.o` são cópias de 100 MB cada. Cortar uma cópia e uma passagem.
3. `integrate.py` (5 s) e `objcopy --weaken` (2,7 s): cache por hash do `full.o`.
4. Recompilação a frio para ativar os `.d` (pede autorização) e medir `-j8` real.
5. Variantes (`DEBUG_TITLE`, `AUDIO`) como alvos que compartilham os objetos e relinkam só o
   necessário (`sa1g_debug`, `sa1g_normal`, `sa1g_audio` num comando).
6. `lld` no lugar do `ld` do binutils (o ELF tem script de ligação próprio; testar antes).

Riscos: o cache de overlays é por conteúdo e inclui o hash do `prscomp`, então mudar o compressor
invalida sozinho; o `gen_traps` novo só aceita ELF32 little-endian com `.rel` (para o ARM atual
basta; falha com mensagem se não for).

# ROADMAP extra — otimizações a partir da sessão DeepSeek (23/11/2026)

Fonte: `deepseek-sa1-zeebo-sessao.md` (20 turnos, exportado do link compartilhado).
O DeepSeek não é fonte primária. Cada afirmação abaixo foi conferida contra o
**Zeebo Developer Guide 0.97** (`zeebo-emulator/research/docs/sdk-extract/ZeeboDeveloperGuide0.97.md`),
contra o nosso código e contra medidas nossas. Onde não deu para conferir, está dito.

## 1. Veredito por afirmação

| Afirmação do DeepSeek | Veredito | Evidência |
| --- | --- | --- |
| Quadros presos a 35 FPS | Confirmado, já feito | Guia, linha 3504; `app.c:26` `SA1Z_FPS 35` |
| Usar `GETUPTIMEMS()`, não `GETTIMEMS()` | Confirmado, já feito | Guia linha 3498-3500; o port só usa `GETUPTIMEMS` |
| Float não é acelerado por hardware; preferir ponto fixo (GLfixed s15.16) | Confirmado | Guia §3.2.2. Nosso build é soft-float, 0 instruções VFP |
| Um único contexto EGL | Confirmado, já feito | Guia ("Multiple EGL contexts are not supported"); `brew_egl.c` cria um |
| ATITC suportado | Confirmado, **não usado** | Guia linhas 1582-1625: RGB 24 -> 4 bpp (6:1) com `GL_COMPRESSED_RGB_ATI_TC`; RGBA 32 -> 8 bpp (4:1) com `GL_COMPRESSED_RGBA_ATI_TC`. `ztexture.c` converte para 16 bits |
| Ao suspender, soltar todas as instâncias IHID | Confirmado, já feito | Guia linha 3506; `sa1z_Parar` chama `zhid_close`, `EVT_APP_RESUME` reabre |
| Até 4 objetos de áudio codificados + 4 MIDI ao mesmo tempo | Confirmado, **muda o projeto do som** | Guia §5.1.4 / 9.2.1 |
| Desalinhamento no ARM dá dado errado ou lentidão | Confirmado | Guia §5.2. Teste nosso: GCC 14 com `-mcpu=arm1136j-s` já gera `ldrb` para `packed`; só casts manuais geram `ldr` desalinhado. Auditoria rápida nas fontes do port: nenhum `*(T*)(base+off)` |
| Controle sem rumble | **Não confirmado** (irrelevante para o port) | Guia §6.3.5 documenta `IHIDDevice_Rumble`; só diz que devolve `AEE_EUNSUPPORTED` se o aparelho não suportar. Não afirma que o Z-Pad não tem |
| "Cache de textura de 8 KB" no Adreno 130 | **Não confirmado** | Não está no guia. O guia fala de IMEM (SRAM de 256 KB: 128+64+64) usada pela GPU. O número de 8 KB veio de um artigo de terceiros |
| 160 MB de RAM para o app | **Contradiz a nossa medida** | Guia: 128 MB + 32 MB empilhados no aparelho, mas o heap do app é limitado (30-32 MB livres, relato dos devs; já no orçamento) |
| Zeebo precisa de fonte japonesa embutida | **Irrelevante** | O port não chama nenhuma API de texto do BREW (`grep IDISPLAY_DrawText`: nada); o SA1 desenha o texto com as próprias texturas |
| `IMenuCtl` para os menus | **Irrelevante** | O SA1 tem os próprios menus; o debug usa `DEBUG_TITLE` |
| Level select pelos endereços `8C7649F0`/`8C75B740` | **Irrelevante** | Temos a decomp e o `DEBUG_TITLE=N`; endereços são do binário do DC |
| Atalhos L+R+Cima/Direita/Baixo do port GCC (debug, 16:9, 60 FPS) | Ideia aproveitável | Agora há gatilhos pelo IHID (W22); 60 FPS não vale no Zeebo (teto de 35) |
| RTC existe mas pode voltar a 1980 | **Não verificado** | O guia não menciona RTC (0 ocorrências). `GETTIMESECONDS` é a época de 1980 do BREW (documentado pela SDK) |
| Fixed-point "obrigatório" no ARM11 | Meio-certo | Recomendado pelo guia, mas já medimos que o gargalo da linha 68 não é float (ver §3) |

## 2. Achado colateral (bug real encontrado ao conferir o RTC)

`syRtcGetDate` era um stub gerado que devolvia 0 **sem preencher a data**. O jogo lia
lixo da pilha: o id de cada Chao (`src/chao/chao.c:208`) e a data dos saves
(`src/save/card.c`). Corrigido em `zsys.c` com `GETJULIANDATE(GETTIMESECONDS())`
(dia da semana ajustado: BREW 0 = segunda, DC 0 = domingo). Build conferido no zeebx.

## 3. Otimizações priorizadas

| # | Item | Ganho esperado | Esforço | Estado |
| --- | --- | --- | --- | --- |
| O1 | **Projeto do som dentro de 4 decodificadores**: música em 2 `IMedia` alternados (segmentos de ~10 s), efeitos misturados por software em 1 fluxo PCM ou no máximo 2 `IMedia` | Evita 7-21 MB de WAV em RAM; permite música além de 40 s; respeita o limite do aparelho | Alto | `AUDIO=1` já toca no zeebx (pico 1.000 em `--dump-audio`), mas decodifica 40 s (música) ou 120 s (filme, ~21 MB) em RAM de uma vez |
| O2 | **Texturas ATITC** (opacas 4 bpp = 4x menos que 16 bits; com alfa 8 bpp = 2x menos) | Memória de textura e banda; ajuda o orçamento de 30-32 MB, pouco nas texturas com alfa | Médio-alto: codificador offline, tirar o cabeçalho de 20 bytes, extensão GL, bit de modo (transparência) | Não iniciado. Falta confirmar se o zeebx aceita ATITC |
| O3 | **Menos float no caminho quente** (matrizes e vértices do `njModel`/`zlive`) | Só vale se o perfil (`vprof zlive`) mostrar a CPU ali | Alto | Medir antes. Na linha 68 o gargalo não é I/O, heap nem música; falta achar a tarefa filha |
| O4 | **Painel/log de RAM** | Medir o orçamento fase a fase | Baixo | Feito: `mem livre/minimo livre/maior bloco` a cada 2 s |
| O5 | **Purga na falha de `MALLOC`** | Só útil depois do áudio em stream | Baixo | Adiado |
| O6 | **Poços fixos em vez de `MALLOC` no caminho quente** | Evita fragmentação | Médio | Parcial: o jogo já usa áreas fixas; revisar `ztexture`/`zaudio` |
| O7 | **Cheats/tabela e menu de opções** | Recurso, não otimização | Médio | Fora do caminho crítico |
| O8 | **Auditoria de desalinhamento em hardware** | Evitar dado errado só no aparelho real | Baixo | O compilador já protege `packed`; falta testar no aparelho |

## 4. Fora do escopo (decidido)

Menu por `IMenuCtl`, fonte japonesa do sistema, endereços de debug do DC, 60 FPS,
rumble, REX/OKL4 (o guia não dá acesso), cheats por endereço de memória.

## 5. Lacunas desta análise

- Li por completo os turnos 10-20 (sobre o port, o hardware e o BREW) e só o resumo
  dos turnos 1-9 (sobre o jogo, mods, cheats do DC). Os dados do jogo original não
  mudam nenhum item acima.
- As citações do DeepSeek (links, números como "4 milhões de triângulos") não foram
  abertas uma a uma.
- O guia é a versão 0.97 (preliminar); alguns limites podem ter mudado.
- O áudio do zeebx só prova a lógica, não a qualidade nem o corte entre segmentos no aparelho.

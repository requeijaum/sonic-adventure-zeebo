# Menu de debug escondido

No `sa1n.mod` normal: segure **L + R + Cima por 3 segundos** (Z-Pad: ZL + ZR + cruz para cima)
em qualquer tela. O jogo reinicia (o mesmo `zgame_reset` do reset do console) e abre direto o menu
de debug do titulo (personagem e fase), sem o logo.

- Precisa do controle IHID: L e R so existem la. Sem HID (teclado) o atalho nao dispara.
- Menos de 3 s nao faz nada; soltar qualquer um dos tres zera a contagem.
- Implementacao: `zsys.c` (`zdebug_menu_watch`, `zdebug_menu_take`), `src/main.c` (caso BOOT) e
  `src/progress/title.c` (`title_setup`), os dois sob `ZEEBO_PORT`. `DEBUG_TITLE=N` continua
  valendo como antes (build de teste que abre o menu no lugar do titulo).
- O reset perde o progresso da fase em andamento; o jogo salvo (VMU) nao muda.
- O `$(PT)/.done` do `game.mk` agora depende dos fontes que o `gen_patch.py` remenda: antes, editar
  `src/main.c` nao refazia a copia remendada e a edicao ficava de fora do build.

# Controlar o Sonic com o Boomerang

O Boomerang é o controle de movimento do Zeebo (acelerômetro + cruz + 2 botões + HOME),
usado em Boomerang Sports e Crash Nitro Kart. Ele chega pelo mesmo IHID do Z-Pad, mas com
outro VID/PID (`0x15a2`/`0x0003`) e um formato próprio: o `AEEHIDPositionInfo` carrega o
**pacote bruto do receptor**, não eixos.

## Fonte e limite da evidência

O formato foi lido de `zeebx-v060/src/machine/hid.rs` (`boomerang_packet`), que o
reconstruiu a partir dos jogos. **Nenhum aparelho real foi medido.** Sinais e escala podem
diferir no hardware (`ZBOOM_STEER_SIGN` em `zboom.c`).

| Palavra do `AEEHIDPositionInfo` | Conteúdo |
| --- | --- |
| 1, 2, 3 (`nX`, `nY`, `nZ`) | acelerômetro, 1 byte por eixo, centro `0x80`, 60 por g; sensor girado 20° dentro do braço |
| 4 (`nRx`) | botões em lógica invertida (bit 0 = apertado): 0 botão 1, 1 botão 2, 2 HOME, 3 esq., 4 baixo, 5 dir., 6 cima |
| 5 (`nRy`) | bit 0 = qual dos dois Boomerangs; resto = tipo (0 = desconectado) |
| 8 (`nVY`) | contador de pacotes (8 bits) |

## Mapa para o SA1

| Boomerang | SA1 |
| --- | --- |
| Girar como volante (Boomerang deitado, de frente para o jogador) | analógico X (zona morta ~6°, fundo em ~35°) |
| Cruz | cruz + analógico a fundo |
| Botão 1 / 2 | A / B |
| HOME (toque) | Start |
| HOME + botão 1 / 2 | X / Y |
| HOME + esquerda / direita | gatilhos L / R (câmera) |
| HOME + baixo | recentra o volante |

Não há gatilhos nem segundo analógico no Boomerang: por isso a câmera e X/Y ficam atrás do HOME.

## Implementação

- `zboom.c/h`: C puro, sem BREW, sem float; testável no PC (`tests/boom_test.c`).
- `zhid.c`: detecta o Boomerang por VID/PID e usa `GetPositionState` + `zboom_update`.
  Também passou a tentar abrir o controle de novo a cada ~60 leituras (o controle é sem fio e pode ligar depois do jogo).

## Validação

- PC: `gcc -I.. -o /tmp/boom_test tests/boom_test.c ../zboom.c && /tmp/boom_test`.
- zeebx: o `zeebx run` sem janela não escolhe o Boomerang; só a interface gráfica
  (porta com dispositivo "Boomerang" + Wii Remote ou outro controle com sensor).
  Teste pendente do usuário.
- Aparelho real: não testado.

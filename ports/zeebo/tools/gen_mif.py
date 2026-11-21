#!/usr/bin/env python3
"""
gera o .mif de SA1HELLO — o manifesto que diz ao console qual applet existe.

POR QUE ISSO EXISTE
------------------
Um .mod sozinho nao e um jogo: e so codigo. O console precisa do .mif ao lado
para saber que classe de applet instanciar. Sem ele o modulo carrega, mas o
emulador responde "nenhum .mif encontrado ao lado do modulo" e o applet nunca
e criado. Foi o que aconteceu na primeira build.

O FORMATO
---------
Decodificado do helloworld.mif que acompanha os exemplos do BREW SDK 4.0.2, e
cruzeado com o parser em zeebx-emu/src/loader/miffile.rs:

    off 0x00  u16  magic = 0x0011
    off 0x10  u32  offset da tabela de limites
    off 0x14  u32  numero de secoes
    tabela    u32  (secoes + 1) limites, o ultimo fecha o arquivo
    secoes    ...  a secao do applet e um registro de 20 bytes:

        +0   u32  ClassID do applet
        +4   u32  zero
        +8   u32  um numero (no helloworld, 1000)
        +12  u32  zero
        +16  u32  campo que varia

    Os dois zeros em +4 e +12 sao o que separa um registro de applet de
    qualquer outra secao de 20 bytes que por acaso exista no arquivo.

O menor .mif valido tem entao um cabecalho, uma tabela de dois limites e um
unico registro: 60 bytes (0x20 cabecalho + 8 limites + 20 registro).
"""

import struct
import sys

CLSID = 0x0BE07002   # AEECLSID_SA1HELLO, em src/sa1hello.bid
APP_ID = 1000        # o "num" do registro; o helloworld usa 1000 tambem


def gera(clsid: int = CLSID, app_id: int = APP_ID) -> bytes:
    tabela = 0x20              # a tabela de limites comeca aqui
    dados = tabela + 8         # 1 secao => 2 limites => 8 bytes
    registro = dados           # o unico registro de applet, 20 bytes
    fim = registro + 20

    cab = bytearray(tabela)
    struct.pack_into('<H', cab, 0x00, 0x0011)      # magic
    struct.pack_into('<I', cab, 0x10, tabela)       # offset da tabela
    struct.pack_into('<I', cab, 0x14, 1)            # 1 secao

    limites = struct.pack('<II', dados, fim)        # [inicio, fim) da secao
    rec = struct.pack('<IIIII', clsid, 0, app_id, 0, 0)

    return bytes(cab) + limites + rec


def main() -> int:
    saida = sys.argv[1] if len(sys.argv) > 1 else 'sa1hello.mif'
    # ClassID opcional: cada applet tem o seu (sa1hello 0x0BE07002,
    # sa1z 0x0BE07003). Sem argumento, mantem o historico.
    esperado = int(sys.argv[2], 0) if len(sys.argv) > 2 else CLSID
    dados = gera(clsid=esperado)
    with open(saida, 'wb') as f:
        f.write(dados)

    # conferir o que o Zeebx vai ver, antes de qualquer coisa rodar
    tabela, = struct.unpack_from('<I', dados, 0x10)
    nsec, = struct.unpack_from('<I', dados, 0x14)
    ini, = struct.unpack_from('<I', dados, tabela)
    clsid, = struct.unpack_from('<I', dados, ini)
    print(f'  MIF  {saida}  ({len(dados)} bytes)')
    print(f'       ClassID={clsid:#010x}  secoes={nsec}  registro@{ini:#x}')

    if clsid != esperado:
        print('erro: o ClassID gerado nao bate com o .bid do applet',
              file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())

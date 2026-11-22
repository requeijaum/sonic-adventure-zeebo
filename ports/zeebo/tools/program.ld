/* ld -r de um programa (game.mk program_rules): junta os dados graváveis
 * entre marcas, para zprog_reset (zgame.c) restaurar o estado inicial a cada
 * leitura do programa, como o loader do DC faz ao ler o binário de novo */
SECTIONS
{
    .data : { @P@_data_start = .; *(.data .data.*) @P@_data_end = .; }
    .bss : { @P@_bss_start = .; *(.bss .bss.* COMMON) @P@_bss_end = .; }
}

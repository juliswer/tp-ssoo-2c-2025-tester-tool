#ifndef INSTRUCTION_CYCLE_H
#define INSTRUCTION_CYCLE_H

#include "globales.h"

typedef enum
{
    CREATE_FILE,
    TRUNCATE_FILE,
    WRITE_FILE,
    READ_FILE,
    TAG_FILE,
    COMMIT_FILE,
    FLUSH_FILE,
    DELETE_FILE,
    END_FILE
} TipoInstruccion;

typedef struct
{
    TipoInstruccion tipo_instruccion;
    char* archivo;
    char* tag;
    uint32_t tamanio;
    uint32_t dir_base;
    char* contenido;
    char* archivo_destino;
    char* tag_destino;
} Instruccion;

char* fetchear_proxima_instruccion(char* path, uint32_t pc);

Instruccion decode(char*);

void execute(Instruccion inst);

Instruccion inicializar_intruccion();

TipoInstruccion convert_string_to_tipoInstruccion(char* cadena);

char* convert_tipoInstruccion_to_string(TipoInstruccion tipoDeIntruccion);

void destruir_instruccion(Instruccion* inst);

#endif
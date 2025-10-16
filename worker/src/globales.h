#ifndef GLOBALES_H_
#define GLOBALES_H_

#include "../../utils/src/utils/sockets.h"

typedef struct 
{
    char* ip_master;
    char* puerto_master;
    char* ip_storage;
    char* puerto_storage;
    int tam_memoria;
    int retardo_memoria;
    char* algoritmo_reemplazo;
    char* path_scripts;
    t_log_level level;
} ConfigWorker;

extern t_log* logger;
extern t_config* config;
extern ConfigWorker config_worker;
extern int socket_master_escucha;
extern int socket_master_interrupt;
extern int socket_storage;
extern uint32_t tam_pagina;
_Atomic extern bool hay_que_ejecutar;
_Atomic extern bool hay_interrupcion;
_Atomic extern bool end;

char* format_path(char *path, char *file_name);

char** parsear_cadena(const char* cadena, const char* delimitador, int* cantidad_tokens);

void liberar_tokens(char** tokens);

#endif
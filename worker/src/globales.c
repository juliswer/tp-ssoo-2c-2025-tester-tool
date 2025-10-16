#include "globales.h"

t_log* logger;
t_config* config;
ConfigWorker config_worker;
int socket_master_escucha;
int socket_master_interrupt;
int socket_storage;
_Atomic bool hay_que_ejecutar = false;
_Atomic bool hay_interrupcion = false;
_Atomic bool end = false;
uint32_t tam_pagina;

char* format_path(char *path, char *file_name)
{
    if (!path || !file_name)
        return NULL;

    size_t len = strlen(path) + strlen(file_name) + 1;
    char *code_path = malloc(len);

    if (!code_path)
        return NULL;

    strcpy(code_path, path);
    strcat(code_path, file_name);

    return code_path;
}

char** parsear_cadena(const char* cadena, const char* delimitador, int* cantidad_tokens)
{
    char* copia = strdup(cadena);

    if (!copia) return NULL;

    int capacidad = 10;
    char** resultado = malloc(capacidad * sizeof(char*));

    if (!resultado) {
        free(copia);
        return NULL;
    }

    int count = 0;
    char* token = strtok(copia, delimitador);

    while (token)
    {
        if (count >= capacidad)
        {
            capacidad *= 2;
            char** temp = realloc(resultado, capacidad * sizeof(char*));

            if (!temp)
            {
                for (int i = 0; i < count; i++) free(resultado[i]);
                free(resultado);
                free(copia);
                return NULL;
            }

            resultado = temp;
        }

        resultado[count++] = strdup(token);
        token = strtok(NULL, delimitador);
    }

    resultado[count] = NULL;

    if (cantidad_tokens)
        *cantidad_tokens = count;

    free(copia);
    
    return resultado;
}

void liberar_tokens(char** tokens)
{
    if (!tokens)
        return;

    for (int i = 0; tokens[i] != NULL; i++)
        free(tokens[i]);
    
    free(tokens);
}
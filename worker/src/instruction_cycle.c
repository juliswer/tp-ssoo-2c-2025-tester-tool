#include "instruction_cycle.h"
#include "memory.h"

char* fetchear_proxima_instruccion(char* path, uint32_t pc)
{
    FILE* archivo_query = fopen(format_path(config_worker.path_scripts, path), "r");

    if (!archivo_query)
    {
        log_error(logger, "No se encontro el archivo de query");
        exit(EXIT_FAILURE);
    }

    char* buffer = NULL;
    size_t bf_size = 0;
    size_t length = 0;

    for (int i = 0; i <= pc; ++i)
        length = getline(&buffer, &bf_size, archivo_query);

    if (buffer[length] == '\n')
    {
        length = length - 1;
        buffer[length] = 0;
    }

    fclose(archivo_query);

    usleep(config_worker.retardo_memoria * 1000);

    return buffer;
}

Instruccion decode(char* linea)
{
    Instruccion inst = inicializar_intruccion();
    int cant_tokens;
    char** tokens = parsear_cadena(linea, " ", &cant_tokens);
    inst.tipo_instruccion = convert_string_to_tipoInstruccion(tokens[0]);
    char** file_tag = NULL;
    char** file_tag_destino = NULL;

    switch (inst.tipo_instruccion)
    {
        case CREATE_FILE:
            file_tag = parsear_cadena(tokens[1], ":", NULL);
            inst.archivo = strdup(file_tag[0]);
            inst.tag = strdup(file_tag[1]);
            break;

        case TRUNCATE_FILE:
            file_tag = parsear_cadena(tokens[1], ":", NULL);
            inst.archivo = strdup(file_tag[0]);
            inst.tag = strdup(file_tag[1]);
            inst.tamanio = (uint32_t)atoi(tokens[2]);
            break;

        case WRITE_FILE:
            file_tag = parsear_cadena(tokens[1], ":", NULL);
            inst.archivo = strdup(file_tag[0]);
            inst.tag = strdup(file_tag[1]);
            inst.dir_base = (uint32_t)atoi(tokens[2]);
            inst.contenido = strdup(tokens[3]);
            break;

        case READ_FILE:
            file_tag = parsear_cadena(tokens[1], ":", NULL);
            inst.archivo = strdup(file_tag[0]);
            inst.tag = strdup(file_tag[1]);
            inst.dir_base = (uint32_t)atoi(tokens[2]);
            inst.tamanio = (uint32_t)atoi(tokens[3]);
            break;

        case TAG_FILE:
            file_tag = parsear_cadena(tokens[1], ":", NULL);
            inst.archivo = strdup(file_tag[0]);
            inst.tag = strdup(file_tag[1]);
            file_tag_destino = parsear_cadena(tokens[2], ":", NULL);
            inst.archivo_destino = strdup(file_tag_destino[0]);
            inst.tag_destino = strdup(file_tag_destino[1]);
            break;

        case COMMIT_FILE:
            file_tag = parsear_cadena(tokens[1], ":", NULL);
            inst.archivo = strdup(file_tag[0]);
            inst.tag = strdup(file_tag[1]);
            break;

        case FLUSH_FILE:
            file_tag = parsear_cadena(tokens[1], ":", NULL);
            inst.archivo = strdup(file_tag[0]);
            inst.tag = strdup(file_tag[1]);
            break;

        case DELETE_FILE:
            file_tag = parsear_cadena(tokens[1], ":", NULL);
            inst.archivo = strdup(file_tag[0]);
            inst.tag = strdup(file_tag[1]);
            break;

        case END_FILE:
            break;

        default:
            log_info(logger, "Default");
            break; 
    }

    liberar_tokens(tokens);
    liberar_tokens(file_tag);
    liberar_tokens(file_tag_destino);

    return inst;
}

void execute(Instruccion inst)
{
    t_paquete* paquete = NULL;
    int res;
    int err;

    switch (inst.tipo_instruccion)
    {
        case CREATE_FILE:
            log_info(logger, "Create");
            paquete = crear_paquete();
            paquete->codigo_operacion = CREATE;
            agregar_a_paquete(paquete, inst.archivo, strlen(inst.archivo) + 1);
            agregar_a_paquete(paquete, inst.tag, strlen(inst.tag) + 1);

            res = enviar_paquete(paquete, socket_storage);
            eliminar_paquete(paquete);

            if (res < 0)
            {
                log_error(logger, "Se ha desconectado Storage... Terminando.");
                log_error(logger, "CREATE %s:%s", inst.archivo, inst.tag); 
                exit(EXIT_FAILURE);
            }

            err = crear_filetag(inst.archivo, inst.tag);

            if (err != SUCCESS)
            {
                log_error(logger, "Ocurrio un error a la hora de crear un archivo:tag");
                log_error(logger, "CREATE %s:%s", inst.archivo, inst.tag); 
                exit(EXIT_FAILURE);
            }

            log_info(logger, "FileTag creado: %s:%s", inst.archivo, inst.tag);

            break;

        case TRUNCATE_FILE:
            log_info(logger, "Truncate");
            paquete = crear_paquete();
            paquete->codigo_operacion = TRUNCATE;
            agregar_a_paquete(paquete, inst.archivo, strlen(inst.archivo) + 1);
            agregar_a_paquete(paquete, inst.tag, strlen(inst.tag) + 1);
            agregar_a_paquete(paquete, &(inst.tamanio), sizeof(uint32_t));

            if (inst.tamanio % tam_pagina != 0)
            {
                log_warning(logger, "Tamaño a truncar no multiplo del tamaño de página");
                eliminar_paquete(paquete);
                return;
            }

            res = enviar_paquete(paquete, socket_storage);
            eliminar_paquete(paquete);

            if (res < 0)
            {
                log_error(logger, "Se ha desconectado Storage... Terminando.");
                log_error(logger, "TRUNCATE %s:%s %u", inst.archivo, inst.tag, inst.tamanio); 
                exit(EXIT_FAILURE);
            }

            err = truncate_file(inst.archivo, inst.tag, inst.tamanio);

            if (err != SUCCESS)
            {
                log_error(logger, "Ocurrio un error a la hora de truncar un archivo"); 
                log_error(logger, "TRUNCATE %s:%s %u", inst.archivo, inst.tag, inst.tamanio); 
                exit(EXIT_FAILURE);
            }

            break;

        case WRITE_FILE:
            log_info(logger, "Write");
            
            err = write_file(inst.archivo, inst.tag, inst.dir_base, inst.contenido, strlen(inst.contenido));
            
            if (err != SUCCESS)
            {
                log_error(logger, "Ocurrio un error a la hora de escribir un archivo");
                log_error(logger, "WRITE %s:%s %d %s", inst.archivo, inst.tag, inst.dir_base, inst.contenido);
                exit(EXIT_FAILURE); 
            }

            break;

        case READ_FILE:
            log_info(logger, "Read");
            void* buffer = malloc(inst.tamanio);

            err = read_file(inst.archivo, inst.tag, inst.dir_base, buffer, inst.tamanio);
            if (err != SUCCESS)
            {
                free(buffer);
                log_error(logger, "Ocurrio un error a la hora de leer un archivo");
                log_error(logger, "READ %s:%s %d %d", inst.archivo, inst.tag, inst.dir_base, inst.tamanio);
                exit(EXIT_FAILURE); 
            }

            paquete = crear_paquete();
            paquete->codigo_operacion = LECTURA_QUERY; 
            agregar_a_paquete(paquete, buffer, inst.tamanio);

            res = enviar_paquete(paquete, socket_master_escucha); 
            if (res < 0)
            {
                log_error(logger, "Se desconecto master");
                log_error(logger, "READ %s:%s %d %d", inst.archivo, inst.tag, inst.dir_base, inst.tamanio);
                exit(EXIT_FAILURE); 
            }

            free(buffer); 
            break;

        case TAG_FILE:
            log_info(logger, "Tag");

            paquete = crear_paquete();
            paquete->codigo_operacion = TAG;

            // origen
            agregar_a_paquete(paquete, inst.archivo, strlen(inst.archivo) + 1);
            agregar_a_paquete(paquete, inst.tag, strlen(inst.tag) + 1);
            // destino
            agregar_a_paquete(paquete, inst.archivo_destino,strlen(inst.archivo_destino) + 1);
            agregar_a_paquete(paquete, inst.tag_destino,strlen(inst.tag_destino) + 1);

            res = enviar_paquete(paquete, socket_storage);
            eliminar_paquete(paquete);

            if (res < 0)
            {
                log_error(logger,
                    "Se ha desconectado Storage... Terminando. TAG %s:%s -> %s:%s",
                    inst.archivo, inst.tag,
                    inst.archivo_destino, inst.tag_destino);
                exit(EXIT_FAILURE);
            }
            break;


        case COMMIT_FILE:
            log_info(logger, "Commit");

            paquete = crear_paquete();
            paquete->codigo_operacion = COMMIT;
            agregar_a_paquete(paquete, inst.archivo, strlen(inst.archivo) + 1);
            agregar_a_paquete(paquete, inst.tag, strlen(inst.tag) + 1);

            res = enviar_paquete(paquete, socket_storage);
            eliminar_paquete(paquete);

            if (res < 0)
            {
                log_error(logger, "Se ha desconectado Storage... Terminando.");
                log_error(logger, "COMMIT %s:%s", inst.archivo, inst.tag);
                exit(EXIT_FAILURE);
            }

            break;

        case FLUSH_FILE:
            log_info(logger, "Flush");

            err = flush_file(inst.archivo, inst.tag);

            if (err != SUCCESS)
            {
                log_error(logger, "Error a la hora de flushear %s:%s",
                    inst.archivo, inst.tag);
                exit(EXIT_FAILURE);
            }

            break;

        case DELETE_FILE:
            log_info(logger, "Delete");

            paquete = crear_paquete();
            paquete->codigo_operacion = DELETE;
            agregar_a_paquete(paquete, inst.archivo, strlen(inst.archivo) + 1);
            agregar_a_paquete(paquete, inst.tag, strlen(inst.tag) + 1);

            res = enviar_paquete(paquete, socket_storage);
            eliminar_paquete(paquete);

            if (res < 0)
            {
                log_error(logger, "Se ha desconectado Storage... Terminando.");
                log_error(logger, "DELETE %s:%s", inst.archivo, inst.tag); 
                exit(EXIT_FAILURE);
            }
            break;

        case END_FILE:
            log_info(logger, "End");
            hay_que_ejecutar = false;
            break;

        default:
            log_info(logger, "Default");
            break; 
    }
}

Instruccion inicializar_intruccion()
{
    Instruccion inst;
    inst.archivo = NULL;
    inst.archivo_destino = NULL;
    inst.contenido =  NULL;
    inst.dir_base = 0;
    inst.tag = NULL;
    inst.tag_destino = NULL;
    inst.tamanio = 0;
    inst.tipo_instruccion = END_FILE;

    return inst;
}

TipoInstruccion convert_string_to_tipoInstruccion(char* cadena)
{
    if (strcmp(cadena, "CREATE") == 0)
        return CREATE_FILE;
    else if (strcmp(cadena, "TRUNCATE") == 0)
        return TRUNCATE_FILE;
    else if (strcmp(cadena, "WRITE") == 0)
        return WRITE_FILE;
    else if (strcmp(cadena, "READ") == 0)
        return READ_FILE;
    else if (strcmp(cadena, "TAG") == 0)
        return TAG_FILE;
    else if (strcmp(cadena, "COMMIT") == 0)
        return COMMIT_FILE;
    else if (strcmp(cadena, "FLUSH") == 0)
        return FLUSH_FILE;
    else if (strcmp(cadena, "DELETE") == 0)
        return DELETE_FILE;
    else
        return END_FILE;
}

char* convert_tipoInstruccion_to_string(TipoInstruccion tipoDeIntruccion)
{
    switch (tipoDeIntruccion)
    {
        case CREATE_FILE:
            return "CREATE";
        case TRUNCATE_FILE:
            return "TRUNCATE";
        case WRITE_FILE:
            return "WRITE";
        case READ_FILE:
            return "READ";
        case TAG_FILE:
            return "TAG";
        case COMMIT_FILE:
            return "COMMIT";
        case FLUSH_FILE:
            return "FLUSH";
        case DELETE_FILE:
            return "DELETE";
        case END_FILE:
            return "END";
        default:
            return "?";
    }
}

void destruir_instruccion(Instruccion *inst)
{
    if (inst->archivo) free(inst->archivo);
    if (inst->tag) free(inst->tag);
    if (inst->archivo_destino) free(inst->archivo_destino);
    if (inst->tag_destino) free(inst->tag_destino);
    if (inst->contenido) free(inst->contenido);

}

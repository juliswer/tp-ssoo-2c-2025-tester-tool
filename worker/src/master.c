#include "master.h"

void iniciar_interrupt()
{
    pthread_t hilo_interrupt;
    int res = pthread_create(&hilo_interrupt, NULL, registrar_interrupciones, NULL);

    if (res != 0)
    {
        log_error(logger, "Error al crear el hilo interrupt");
        exit(EXIT_FAILURE);
    }

    pthread_detach(hilo_interrupt);
}

void* registrar_interrupciones(void*)
{
    int cod_op;
    t_list* master_data;

    while (true)
    {
        log_info(logger, "Recibiendo operación de Master");
        cod_op = recibir_operacion(socket_master_interrupt);

        switch (cod_op)
        {
            case DESALOJO_QUERY:
                hay_interrupcion = true;
                master_data = recibir_paquete(socket_master_interrupt);
                uint32_t qid = *(uint32_t*)list_get(master_data, 0);
                log_info(logger, "## Query %u: Desalojada por pedido del Master", qid);
                list_destroy_and_destroy_elements(master_data, free);

                break;
                
            case -1:
                log_error(logger, "Master se desconectó... Terminando Worker.");
                exit(EXIT_FAILURE);

                break;

            default:
                log_warning(logger, "????");

                break;
        }
    }
}

void manejar_master()
{
    while (true)
    {
        int cod_op = recibir_operacion(socket_master_escucha);

        if (cod_op == EJECUCION_QUERY)
        {
            t_list* master_data = recibir_paquete(socket_master_escucha);

            char* path = strdup((char*)list_get(master_data, 0));
            uint32_t qid = *(uint32_t*)list_get(master_data, 1);
            uint32_t pc = *(uint32_t*)list_get(master_data, 2);

            list_destroy_and_destroy_elements(master_data, free);

            log_info(logger, "## Query %u: Se recibe la Query. El path de operaciones es: %s",
                    qid, path);
            
            hay_que_ejecutar = true;
            
            manejar_ejecucion(path, qid, pc);
        }
        else if (cod_op == -1)
        {
            log_error(logger, "## Se desconectó Master... Terminando.");
            exit(EXIT_FAILURE);
        }
        else
            log_error(logger, "? ? ? ? ? ? ?");
    }
}

void manejar_ejecucion(char* path, uint32_t qid, uint32_t pc)
{
    while (hay_que_ejecutar)
    {
        char* instruccion_cadena = fetchear_proxima_instruccion(path, pc);

        log_info(logger, "## Query %u: FETCH - Program Counter: %u - %s", qid, pc, instruccion_cadena);

        Instruccion instruccion = decode(instruccion_cadena);

        execute(instruccion);

        log_info(logger, "## Query %u: - Instrucción realizada: %s",
            qid, convert_tipoInstruccion_to_string(instruccion.tipo_instruccion));

        if (hay_interrupcion)
        {
            hay_que_ejecutar = false;
            hay_interrupcion = false;
        }

        if (instruccion.tipo_instruccion == END_FILE) end = true;
        
        destruir_instruccion(&instruccion);
        free(instruccion_cadena);

        pc++;
    }

    t_paquete* paquete = crear_paquete();

    if (end)
    {
        paquete->codigo_operacion = FINALIZACION_QUERY;
        agregar_a_paquete(paquete, path, strlen(path) + 1);
    }
    else
    {
        paquete->codigo_operacion = DESALOJO_QUERY;
        agregar_a_paquete(paquete, &pc, sizeof(uint32_t));
    }

    enviar_paquete(paquete, socket_master_escucha);
    eliminar_paquete(paquete);
    
    free(path);

    end = false;
}
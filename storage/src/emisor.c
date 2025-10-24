#include "emisor.h"
#include <commons/log.h>

t_paquete *handle_emisor(t_log *logger, int worker_id, int operaciones_count)
{
    t_paquete *paquete = crear_paquete();

    if (operaciones_count == 0)
    {
        paquete->codigo_operacion = HANDSHAKE_WORKER_STORAGE;
        agregar_a_paquete(paquete, &worker_id, sizeof(int));
        log_info(logger, "Enviando HANDSHAKE_WORKER_STORAGE");
    }
    else
    {
        log_info(logger, "Operación número %d", operaciones_count);
        log_info(logger, "Selecciona una operación:");
        log_warning(logger, "❌ 20 - HANDSHAKE_WORKER_STORAGE");
        log_warning(logger, "❌ 21 - TAM_BLOQUE");
        log_warning(logger, "✅ 22 - CREATE");
        log_warning(logger, "✅ 23 - TRUNCATE");
        log_warning(logger, "❌ 24 - FETCH_PAGE");
        log_warning(logger, "❌ 25 - PAGE_FOUND");
        log_warning(logger, "❌ 26 - PAGE_NOT_FOUND");
        log_warning(logger, "❌ 27 - PAGE_COPY");
        log_warning(logger, "✅ 28 - TAG");
        log_warning(logger, "✅ 29 - COMMIT");
        log_warning(logger, "✅ 30 - DELETE");
        log_warning(logger, "✅ 31 - READ");
        log_warning(logger, "✅ 32 - WRITE");

        int opcion;
        printf("Ingrese el número de operación: ");
        scanf("%d", &opcion);

        paquete->codigo_operacion = opcion;

        switch (opcion)
        {
        case CREATE:
        {
            char file[256], tag[256];
            printf("Ingrese el nombre del archivo: ");
            scanf("%s", file);
            printf("Ingrese el tag: ");
            scanf("%s", tag);

            agregar_a_paquete(paquete, file, strlen(file) + 1);
            agregar_a_paquete(paquete, tag, strlen(tag) + 1);

            log_info(logger, "CREATE - Archivo: %s, Tag: %s", file, tag);
            break;
        }
        case TRUNCATE:
        {
            char file[256], tag[256];
            uint32_t tamanio;
            printf("Ingrese el nombre del archivo: ");
            scanf("%s", file);
            printf("Ingrese el tag: ");
            scanf("%s", tag);
            printf("Ingrese el tamaño: ");
            scanf("%u", &tamanio);

            agregar_a_paquete(paquete, file, strlen(file) + 1);
            agregar_a_paquete(paquete, tag, strlen(tag) + 1);
            agregar_a_paquete(paquete, &tamanio, sizeof(uint32_t));

            log_info(logger, "TRUNCATE - Archivo: %s, Tag: %s, Tamaño: %u", file, tag, tamanio);
            break;
        }
        case READ:
        {
            char file[256], tag[256];
            int numero_bloque_logico;
            printf("Ingrese el nombre del archivo: ");
            scanf("%s", file);
            printf("Ingrese el tag: ");
            scanf("%s", tag);
            printf("Ingrese el número de bloque lógico: ");
            scanf("%d", &numero_bloque_logico);

            agregar_a_paquete(paquete, file, strlen(file) + 1);
            agregar_a_paquete(paquete, tag, strlen(tag) + 1);
            agregar_a_paquete(paquete, &numero_bloque_logico, sizeof(int));

            log_info(logger, "READ - Archivo: %s, Tag: %s, Bloque: %d", file, tag, numero_bloque_logico);

            break;
        }
        case TAG:
        {
            char file_origen[256], tag_origen[256], file_destino[256], tag_destino[256];
            printf("Ingrese el archivo origen: ");
            scanf("%s", file_origen);
            printf("Ingrese el tag origen: ");
            scanf("%s", tag_origen);
            printf("Ingrese el archivo destino: ");
            scanf("%s", file_destino);
            printf("Ingrese el tag destino: ");
            scanf("%s", tag_destino);

            agregar_a_paquete(paquete, file_origen, strlen(file_origen) + 1);
            agregar_a_paquete(paquete, tag_origen, strlen(tag_origen) + 1);
            agregar_a_paquete(paquete, file_destino, strlen(file_destino) + 1);
            agregar_a_paquete(paquete, tag_destino, strlen(tag_destino) + 1);
            log_info(logger, "TAG - Origen: %s:%s, Destino: %s:%s", file_origen, tag_origen, file_destino, tag_destino);
            break;
        }
        case COMMIT:
        {
            char file[256], tag[256];
            printf("Ingrese el nombre del archivo: ");
            scanf("%s", file);
            printf("Ingrese el tag: ");
            scanf("%s", tag);

            agregar_a_paquete(paquete, file, strlen(file) + 1);
            agregar_a_paquete(paquete, tag, strlen(tag) + 1);

            log_info(logger, "COMMIT - Archivo: %s, Tag: %s", file, tag);
            break;
        }
        case DELETE:
        {
            char file[256], tag[256];
            printf("Ingrese el nombre del archivo: ");
            scanf("%s", file);
            printf("Ingrese el tag: ");
            scanf("%s", tag);

            agregar_a_paquete(paquete, file, strlen(file) + 1);
            agregar_a_paquete(paquete, tag, strlen(tag) + 1);

            log_info(logger, "DELETE - Archivo: %s, Tag: %s", file, tag);
            break;
        }
        case ESCRIBIR_BLOQUE:
        {
            int block_size = 1024;
            int id_bloque_logico = 0;
            char file[256], tag[256], contenido[256];
            printf("Ingrese el nombre del archivo: ");
            scanf("%s", file);
            printf("Ingrese el tag: ");
            scanf("%s", tag);
            printf("Ingrese el ID de bloque lógico: ");
            scanf("%d", &id_bloque_logico);
            printf("Ingrese el contenido: ");
            scanf("%s", contenido);
            printf("Ingrese el tamanio de bloque a utilizar: ");
            scanf("%d", &block_size);

            if (strlen(contenido) >= block_size)
            {
                printf("Error: El contenido es más grande que el tamaño del bloque.\n");
                break;
            }

            void *buffer_bloque = malloc(block_size);
            memset(buffer_bloque, 0, block_size);
            memcpy(buffer_bloque, contenido, strlen(contenido));

            agregar_a_paquete(paquete, file, strlen(file) + 1);
            agregar_a_paquete(paquete, tag, strlen(tag) + 1);
            agregar_a_paquete(paquete, &id_bloque_logico, sizeof(int));
            agregar_a_paquete(paquete, buffer_bloque, block_size);

            log_info(logger, "WRITE - Archivo: %s, Tag: %s, Contenido: %s", file, tag, contenido);
            free(buffer_bloque);
            break;
        }
        default:
            log_error(logger, "Operación no soportada: %d", opcion);
            eliminar_paquete(paquete);
            return NULL;
        }
    }

    return paquete;
}

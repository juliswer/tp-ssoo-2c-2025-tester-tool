#include "main.h"
#include <string.h>
#include <stdint.h>

int main(int argc, char *argv[])
{
    t_log *logger = log_create("storage.log", "STORAGE", true, LOG_LEVEL_TRACE);
    char *ip_storage = "127.0.0.1";
    char *puerto_storage = "9002";

    int storage_fd = crear_conexion(ip_storage, puerto_storage);

    if (storage_fd == -1)
    {
        log_error(logger, "No se pudo conectar con el servidor");
        return EXIT_FAILURE;
    }
    int worker_id;
    printf("Ingrese el número de un worker: ");
    scanf("%d", &worker_id);

    int operaciones_count = 0;

    while (1)
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

                agregar_a_paquete(paquete, file, strlen(file) + 1);
                agregar_a_paquete(paquete, tag, strlen(tag) + 1);
                agregar_a_paquete(paquete, &id_bloque_logico, sizeof(int));
                agregar_a_paquete(paquete, contenido, strlen(contenido) + 1);

                log_info(logger, "WRITE - Archivo: %s, Tag: %s, Contenido: %s", file, tag, contenido);
                break;
            }
            default:
                log_error(logger, "Operación no soportada: %d", opcion);
                eliminar_paquete(paquete);
                continue;
            }
        }

        enviar_paquete(paquete, storage_fd);
        eliminar_paquete(paquete);
        operaciones_count++;

        if (operaciones_count > 0)
        {
            printf("¿Continuar? (1 para sí, 0 para no): ");
            int continuar;
            scanf("%d", &continuar);
            if (!continuar)
            {
                break;
            }
        }
    }

    return 0;
}
#include "main.h"

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
        t_paquete *paquete = handle_emisor(logger, worker_id, operaciones_count);

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
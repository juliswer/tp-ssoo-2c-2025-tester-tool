#ifndef EMISOR
#define EMISOR

#include <utils/sockets.h>
#include <string.h>
#include <stdint.h>

t_paquete *handle_emisor(t_log *logger, int worker_id, uint32_t query_id, int operaciones_count);

#endif

#ifndef SOCKETS_H_
#define SOCKETS_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <math.h>
#include <stdatomic.h>

#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/config.h>
#include <commons/temporal.h>
#include <commons/bitarray.h>
#include <commons/crypto.h>
#include <commons/string.h>

#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

typedef enum
{
	ERROR,
	MENSAJE,
	PAQUETE
} op_code;

typedef enum
{
	HANDSHAKE_MASTER_WORKER_ESCUCHA = 40,
	HANDSHAKE_MASTER_WORKER_INTERRUPT,
	LECTURA_QUERY,
	FINALIZACION_QUERY,
	EJECUCION_QUERY,
	DESALOJO_QUERY
} op_code_master_worker;

typedef enum
{
	HANDSHAKE_MASTER_QUERY_ESCUCHA = 30,
	HANDSHAKE_MASTER_QUERY_MONITOR,
	LECTURA_ARCHIVO,
	FINALIZACION,
	EJECUCION,
	DESALOJO
} op_code_master_query;

typedef enum
{
	HANDSHAKE_WORKER_STORAGE = 20,
	TAM_BLOQUE,
	CREATE,
	TRUNCATE,
	FETCH_PAGE,
	PAGE_FOUND,
	PAGE_NOT_FOUND,
	PAGE_COPY,
	TAG,
	COMMIT,
	DELETE,
	READ,
	ESCRIBIR_BLOQUE
} op_code_worker_storage;

typedef struct
{
	int socket;
	char *ip;
	char *puerto;
} c_modulo;

typedef struct
{
	int size;
	void *stream;
} t_buffer;

typedef struct
{
	int codigo_operacion;
	t_buffer *buffer;
} t_paquete;

// Receptor
void *recibir_buffer(int *, int);

int iniciar_servidor(char *puerto);

int esperar_cliente(int);

t_list *recibir_paquete(int);

int recibir_operacion(int);

// Emisor
int crear_conexion(char *ip, char *puerto);

t_paquete *crear_paquete(void);

void agregar_a_paquete(t_paquete *paquete, void *valor, int tamanio);

void *serializar_paquete(t_paquete *paquete, int bytes);

int enviar_paquete(t_paquete *paquete, int socket_cliente);

void liberar_conexion(int socket_cliente);

void eliminar_paquete(t_paquete *paquete);

void crear_buffer(t_paquete *paquete);

#endif /* SOCKETS_H_ */
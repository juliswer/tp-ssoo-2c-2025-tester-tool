#ifndef MASTER_H_
#define MASTER_H_

#include "globales.h"
#include "memory.h"
#include "instruction_cycle.h"

void iniciar_interrupt();

void* registrar_interrupciones(void*);

void manejar_master();

void manejar_ejecucion(char* path, uint32_t qid, uint32_t pc);

#endif
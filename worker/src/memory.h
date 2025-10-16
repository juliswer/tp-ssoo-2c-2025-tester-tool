#ifndef MEMORY_H
#define MEMORY_H

#include "globales.h"

#define PAGE_FAULT 0
#define PAGE_EMPTY 1
#define SUCCESS 0
#define OWNER_FOUND 0
#define OWNER_NOT_FOUND -1
#define FREE_FRAME_NOT_FOUND -1
#define FILE_NOT_FOUND -1
#define NOT_ENOUGH_MEMORY -2
#define MEMORY_INCONSISTENCY -3


typedef uint8_t byte; 

typedef struct 
{
    bool uso; 
    bool modificado; 
    bool presencia; 
    int frame_number;
    uint64_t last_used;
} Page;

typedef struct 
{
    char* file;
    char* tag;
    uint32_t size_bytes;
    uint32_t page_count;
    Page* paginas; 
} FileTag;


/* Inicialización / destrucción */
void iniciar_memoria_interna();

void destruir_memoria_interna();

/* Acceso físico */
byte* get_frame_ptr(int frame_num);

int search_free_frame();

int lru_select_victim_frame();

/* FileTag */
FileTag* get_filetag(const char* file, const char* tag);

int crear_filetag(const char* file, const char* tag);

int find_owner_of_frame(int frame, FileTag** out_owner, uint32_t* out_page_index);

/* Operaciones básicas */
int write_file(const char* file, const char* tag, uint32_t dir_base, const void* buffer, uint32_t buffer_size);

int read_file(const char* file, const char* tag, uint32_t dir_base, void* out_buffer, uint32_t buffer_size);

/* Helpers para integracion con Storage */
int fetch_page_from_storage(const char* file, const char* tag, uint32_t page_number, byte* dest_frame);

int flush_page_to_storage(FileTag* owner, uint32_t page_number);

int truncate_file(const char* file, const char* tag, uint32_t size_bytes);

void page_fault(FileTag* ft, Page* pg, uint32_t page_idx); 

int write_page(Page* page, const byte* buffer, const uint32_t size, const uint32_t offset_in_page); 

int read_page(Page* page, byte* dst, const uint32_t size, const uint32_t offset_in_page);

int flush_file(const char* file, const char* tag);

int delete_filetag(char* file, char* tag);

void remove_filetag(char* file, char* tag);

#endif
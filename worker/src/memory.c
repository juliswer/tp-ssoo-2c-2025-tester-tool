#include "memory.h"

byte* memory = NULL; 
uint32_t cant_paginas = 0;
uint32_t paginas_libres = 0;
t_list* file_tags = NULL;
bool* bitmap = NULL;
static uint64_t access_counter = 0;

int (*algoritmo_remplazo)();

void iniciar_memoria_interna(void)
{
    if (tam_pagina <= 0)
    {
        log_error(logger, "tam_pagina <= 0 al iniciar memoria");
        exit(EXIT_FAILURE);
    }

    memory = malloc(config_worker.tam_memoria);

    if (!memory)
    {
        log_error(logger, "No se pudo reservar memoria interna");
        exit(EXIT_FAILURE);
    }

    memset(memory, 0, config_worker.tam_memoria);

    cant_paginas = config_worker.tam_memoria / tam_pagina;
    paginas_libres = cant_paginas;

    file_tags = list_create();

    bitmap = calloc(cant_paginas, sizeof(bool));

    if (!bitmap)
    {
        log_error(logger, "No se pudo reservar bitmap");
        exit(EXIT_FAILURE);
    }

    log_info(logger, "Memoria interna inicializada: %u bytes, %u páginas de %u bytes",
             config_worker.tam_memoria, cant_paginas, tam_pagina);

    if (strcmp(config_worker.algoritmo_reemplazo, "LRU") == 0)
        algoritmo_remplazo = lru_select_victim_frame;
    else
    {
        log_error(logger, "De momento no implementado.");
        exit(EXIT_FAILURE);
    }
    
}

void destruir_memoria_interna()
{
    if (file_tags)
    {
        while (list_size(file_tags) > 0)
        {
            FileTag* ft = list_remove(file_tags, 0);
            if (ft)
            {
                if (ft->file) free(ft->file);
                if (ft->tag) free(ft->tag);
                if (ft->paginas) free(ft->paginas);
                free(ft);
            }
        }

        list_destroy(file_tags);
        file_tags = NULL;
    }

    if (bitmap) { free(bitmap); bitmap = NULL; }
    if (memory) { free(memory); memory = NULL; }

    log_info(logger, "Memoria interna liberada.");
}

byte* get_frame_ptr(int frame_num)
{
    if (frame_num < 0 || (uint32_t)frame_num >= cant_paginas) 
        return NULL;

    return memory + ((size_t)frame_num * tam_pagina);
}

int search_free_frame(void)
{
    for (uint32_t i = 0; i < cant_paginas; ++i)
    {
        if (!bitmap[i])
        {
            bitmap[i] = true;
            paginas_libres = (paginas_libres > 0) ? paginas_libres - 1 : 0;
            return (int)i;
        }
    }

    return FREE_FRAME_NOT_FOUND;
}

int find_owner_of_frame(int frame, FileTag** out_owner, uint32_t* out_page_index)
{
    for (int i = 0; i < list_size(file_tags); ++i)
    {
        FileTag* ft = (FileTag*)list_get(file_tags, i);
        for (uint32_t p = 0; p < ft->page_count; ++p)
        {
            if (ft->paginas[p].presencia && ft->paginas[p].frame_number == frame)
            {
                if (out_owner) *out_owner = ft;
                if (out_page_index) *out_page_index = p;
                return OWNER_FOUND;
            }
        }
    }

    return OWNER_NOT_FOUND;
}

int lru_select_victim_frame()
{
    int free_frame = search_free_frame();
    if (free_frame != FREE_FRAME_NOT_FOUND) return free_frame;

    uint64_t min_last = UINT64_MAX;
    FileTag* victim_owner = NULL;
    uint32_t victim_page = 0;
    bool found = false;

    for (size_t i = 0; i < list_size(file_tags); ++i)
    {
        FileTag* ft = (FileTag*)list_get(file_tags, i);
        for (uint32_t p = 0; p < ft->page_count; ++p)
        {
            Page* pg = &ft->paginas[p];
            if (pg->presencia)
            {
                if (pg->last_used < min_last)
                {
                    min_last = pg->last_used;
                    victim_owner = ft;
                    victim_page = p;
                    found = true;
                }
            }
        }
    }

    if (!found) 
        return FREE_FRAME_NOT_FOUND; // no hay páginas cargadas ??!
    

    int frame = victim_owner->paginas[victim_page].frame_number;

    if (victim_owner->paginas[victim_page].modificado)
    {
        log_info(logger, "Reemplazo: victim %s:%s page %u (frame %d) - writeback",
                 victim_owner->file, victim_owner->tag, victim_page, frame);

        int flush_res = flush_page_to_storage(victim_owner, victim_page);

        if (flush_res != 0)
        {
            log_error(logger, "flush_page_to_storage falló para %s:%s pag %u", victim_owner->file, victim_owner->tag, victim_page);
            return FREE_FRAME_NOT_FOUND;
        }
        
        victim_owner->paginas[victim_page].modificado = false;
    }
    else 
        log_info(logger, "Reemplazo: victim %s:%s page %u (frame %d) - no modificado",
                 victim_owner->file, victim_owner->tag, victim_page, frame);
    
    // liberar la página del owner
    victim_owner->paginas[victim_page].presencia = false;
    victim_owner->paginas[victim_page].frame_number = -1;
    victim_owner->paginas[victim_page].uso = false;
    victim_owner->paginas[victim_page].last_used = 0;

    if (frame >= 0 && (uint32_t)frame < cant_paginas)
        return frame;
    

    return FREE_FRAME_NOT_FOUND;
}

FileTag* get_filetag(const char* file, const char* tag)
{
    if (!file || !tag) return NULL;

    for (size_t i = 0; i < list_size(file_tags); ++i)
    {
        FileTag* ft = (FileTag*)list_get(file_tags, i);
        if (ft->file && ft->tag && strcmp(ft->file, file) == 0 && strcmp(ft->tag, tag) == 0)
            return ft;
    }
    
    return NULL;
}

int crear_filetag(const char* file, const char* tag)
{
    if (!file || !tag) return MEMORY_INCONSISTENCY;

    FileTag* ft = malloc(sizeof(FileTag));
    if (!ft) return MEMORY_INCONSISTENCY;
    memset(ft, 0, sizeof(FileTag));

    ft->file = strdup(file);
    ft->tag = strdup(tag);
    ft->size_bytes = 0;
    ft->page_count = 0;
    ft->paginas = NULL;

    list_add(file_tags, ft);

    return SUCCESS;
}

void page_fault(FileTag* ft, Page* pg, uint32_t page_idx)
{
    int frame = algoritmo_remplazo(); 
    
    if (frame == FREE_FRAME_NOT_FOUND)
    {
        log_error(logger, "No hay frames disponibles ni se pudo reemplazar");
        return;
    }

    byte* dst = get_frame_ptr(frame);

    int fetch_res = fetch_page_from_storage(ft->file, ft->tag, page_idx, dst);

    if (fetch_res == PAGE_EMPTY)
    {
        memset(dst, 0, tam_pagina);
        log_info(logger, "Page fault: %s:%s [%u] inicializada vacía (Storage no la tenía)",
            ft->file, ft->tag, page_idx);
    }
    else if (fetch_res == PAGE_FAULT)
        log_info(logger, "Page fault: %s:%s [%u] traída desde Storage",
            ft->file, ft->tag, page_idx);
    else
    {
        log_error(logger, "Error a la hora de conseguir la página...");
        exit(EXIT_FAILURE);
    }

    pg->frame_number = frame;
    pg->presencia = true;
    pg->modificado = false;
    pg->uso = true;
    pg->last_used = ++access_counter;
}

int write_page(Page* page, const byte* buffer, const uint32_t size, const uint32_t offset_in_page)
{
    byte* frame_ptr = get_frame_ptr(page->frame_number);

    if (!frame_ptr)
        return MEMORY_INCONSISTENCY;

    memcpy(frame_ptr + offset_in_page, buffer, size);
    page->modificado = true;
    page->uso = true;
    page->last_used = ++access_counter;

    return 0;
}

int read_page(Page* page, byte* dst, const uint32_t size, const uint32_t offset_in_page)
{
    byte* frame_ptr = get_frame_ptr(page->frame_number);
    
    if (!frame_ptr)
        return MEMORY_INCONSISTENCY;
    
    memcpy(dst, frame_ptr + offset_in_page, size);
    
    page->uso = true;
    page->last_used = ++access_counter;
    
    return 0;
}


int write_file(const char* file, const char* tag, uint32_t dir_base, const void* buffer, uint32_t buffer_size)
{
    if (!file || !tag || !buffer) return MEMORY_INCONSISTENCY;

    FileTag* ft = get_filetag(file, tag);
    if (!ft) return FILE_NOT_FOUND;

    uint64_t end = (uint64_t)dir_base + buffer_size;
    if (end > ft->size_bytes)
    {
        log_warning(logger, "write_file fuera de limite: dir_base %u + size %u > file size %u",
                    dir_base, buffer_size, ft->size_bytes);
        return MEMORY_INCONSISTENCY;
    }

    const byte* src = (const byte*)buffer;
    uint32_t remaining = buffer_size;
    uint32_t logical_offset = dir_base;

    while (remaining > 0) 
    {
        uint32_t page_idx = logical_offset / tam_pagina;
        uint32_t offset_in_page = logical_offset % tam_pagina;
        uint32_t can_write = (uint32_t)tam_pagina - offset_in_page;
        if (can_write > remaining) can_write = remaining;

        Page* pg = &ft->paginas[page_idx];

        // Si no está en memoria, cargar/asignar frame
        if (!pg->presencia)
            page_fault(ft, pg, page_idx);
        
        write_page(pg, src, can_write, offset_in_page); 

        src += can_write;
        remaining -= can_write;
        logical_offset += can_write;
    }

    log_info(logger, "Write a %s:%s bytes=%u base=%u", ft->file, ft->tag, buffer_size, dir_base);
    return SUCCESS;
}

int read_file(const char* file, const char* tag, uint32_t dir_base, void* out_buffer, uint32_t buffer_size)
{
    if (!file || !tag || !out_buffer) return MEMORY_INCONSISTENCY;

    FileTag* ft = get_filetag(file, tag);
    if (!ft) return FILE_NOT_FOUND;

    uint64_t end = (uint64_t)dir_base + buffer_size;
    if (end > ft->size_bytes)
    {
        log_warning(logger, "read_file fuera de limite: dir_base %u + size %u > file size %u",
                    dir_base, buffer_size, ft->size_bytes);
        return MEMORY_INCONSISTENCY;
    }

    byte* dst = (byte*)out_buffer;
    uint32_t remaining = buffer_size;
    uint32_t logical_offset = dir_base;

    while (remaining > 0)
    {
        uint32_t page_idx = logical_offset / tam_pagina;
        uint32_t offset_in_page = logical_offset % tam_pagina;
        uint32_t can_read = (uint32_t)tam_pagina - offset_in_page;
        if (can_read > remaining) can_read = remaining;

        Page* pg = &ft->paginas[page_idx];
        if (!pg->presencia)
            page_fault(ft, pg, page_idx);
        

        read_page(pg, dst, can_read, offset_in_page);

        dst += can_read;
        remaining -= can_read;
        logical_offset += can_read;
    }

    log_info(logger, "Read %u bytes from %s:%s base=%u", buffer_size, ft->file, ft->tag, dir_base);
    return SUCCESS;
}

// fetch: trae una página (page_number) desde Storage y la copia en dest_frame (tam_pagina bytes).
int fetch_page_from_storage(const char* file, const char* tag, uint32_t page_number, byte* dest_frame)
{
    if (!file || !tag || !dest_frame) return MEMORY_INCONSISTENCY;

    t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = FETCH_PAGE;
    agregar_a_paquete(paquete, (char*)file, strlen(file) + 1);
    agregar_a_paquete(paquete, (char*)tag, strlen(tag) + 1);
    agregar_a_paquete(paquete, &page_number, sizeof(uint32_t));

    int res = enviar_paquete(paquete, socket_storage);
    eliminar_paquete(paquete);

    if (res < 0)
    {
        log_error(logger, "fetch_page: Storage desconectado al enviar READ");
        exit(EXIT_FAILURE);
    }

    int cod = recibir_operacion(socket_storage);

    if (cod == -1)
    {
        log_error(logger, "fetch_page: Storage se desconectó");
        exit(EXIT_FAILURE);
    }

    if (cod != PAGE_FOUND && cod != PAGE_NOT_FOUND)
    {
        log_error(logger, "fetch_page: codigo inesperado de Storage: %d", cod);
        return -1;
    }

    if (cod == PAGE_NOT_FOUND) return PAGE_EMPTY;

    t_list* respuesta = recibir_paquete(socket_storage);
    if (list_size(respuesta) < 1)
    {
        list_destroy_and_destroy_elements(respuesta, free);
        log_error(logger, "fetch_page: paquete de Storage sin datos");
        return -1;
    }

    void* data = list_get(respuesta, 0);
    memcpy(dest_frame, data, tam_pagina);
    list_destroy_and_destroy_elements(respuesta, free);
    log_info(logger, "Memoria Miss - File: %s Tag: %s Pagina: %u (traida de Storage)", file, tag, page_number);
    return PAGE_FAULT;
}

// flush: escribe la página page_number del owner en Storage
int flush_page_to_storage(FileTag* owner, uint32_t page_number)
{
    if (!owner) return OWNER_NOT_FOUND;
    if (page_number >= owner->page_count) return MEMORY_INCONSISTENCY;
    int frame = owner->paginas[page_number].frame_number;
    if (frame < 0) return MEMORY_INCONSISTENCY;

    byte* src = get_frame_ptr(frame);
    if (!src) return MEMORY_INCONSISTENCY;

    t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = PAGE_COPY;
    agregar_a_paquete(paquete, owner->file, strlen(owner->file) + 1);
    agregar_a_paquete(paquete, owner->tag, strlen(owner->tag) + 1);
    agregar_a_paquete(paquete, &page_number, sizeof(uint32_t));
    agregar_a_paquete(paquete, src, tam_pagina);
    
    int res = enviar_paquete(paquete, socket_storage);
    eliminar_paquete(paquete);

    if (res < 0)
    {
        log_error(logger, "flush_page: Storage desconectado al enviar WRITE");
        exit(EXIT_FAILURE);
    }
/*
    int cod = recibir_operacion(socket_storage);

    if (cod == WRITE)
    {
        t_list* respuesta = recibir_paquete(socket_storage);
        list_destroy_and_destroy_elements(respuesta, free);
        log_info(logger, "Bloque fisico reservado/escrito (flush) - File: %s Tag: %s Pag: %u",
            owner->file, owner->tag, page_number);
        return 0;
    }
    else if (cod == -1)
    {
        log_error(logger, "flush_page: Storage se desconectó");
        exit(EXIT_FAILURE);
    }
    else
    {
        log_error(logger, "flush_page: codigo inesperado de Storage: %d", cod);
        return -1;
    }
*/
    log_info(logger, "Bloque fisico reservado/escrito (flush) - File: %s Tag: %s Pag: %u",
        owner->file, owner->tag, page_number);

    return SUCCESS;
}


int truncate_file(const char* file, const char* tag, uint32_t size_bytes)
{
    if (!file || !tag) return MEMORY_INCONSISTENCY;

    FileTag* ft = get_filetag(file, tag);
    if (!ft) return OWNER_NOT_FOUND;

    uint32_t new_pages = (size_bytes == 0) ? 0 : (size_bytes + tam_pagina - 1) / tam_pagina;

    /* Crecimiento */
    if (new_pages > ft->page_count)
    {
        ft->paginas = realloc(ft->paginas, new_pages * sizeof(Page));
        if (!ft->paginas) return MEMORY_INCONSISTENCY;

        for (uint32_t p = ft->page_count; p < new_pages; ++p)
        {
            ft->paginas[p].presencia = false;
            ft->paginas[p].modificado = false;
            ft->paginas[p].uso = false;
            ft->paginas[p].frame_number = -1;
            ft->paginas[p].last_used = 0;
        }

        log_info(logger, "truncate_file: %s:%s crece de %u -> %u páginas",
            file, tag, ft->page_count, new_pages);
    }
    /* Reducción */
    else if (new_pages < ft->page_count)
    {
        for (uint32_t p = new_pages; p < ft->page_count; ++p)
        {
            Page* pg = &ft->paginas[p];
            if (pg->presencia && pg->frame_number >= 0)
            {
                if (pg->modificado)
                    flush_page_to_storage(ft, p);

                bitmap[pg->frame_number] = false;
                paginas_libres++;
            }
        }

        ft->paginas = realloc(ft->paginas, new_pages * sizeof(Page));

        log_info(logger, "truncate_file: %s:%s se reduce de %u -> %u páginas",
            file, tag, ft->page_count, new_pages);
    }

    ft->page_count = new_pages;
    ft->size_bytes = size_bytes;

    return SUCCESS;
}

int flush_file(const char* file, const char* tag)
{
    if (!file || !tag) return MEMORY_INCONSISTENCY;

    FileTag* ft = get_filetag(file, tag);
    if (!ft) return OWNER_NOT_FOUND;

    t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = FLUSH;

    agregar_a_paquete(paquete, (void*)file, strlen(file) + 1);
    agregar_a_paquete(paquete, (void*)tag, strlen(tag) + 1);

    uint32_t modified_count = 0;
    for (uint32_t i = 0; i < ft->page_count; i++)
        if (ft->paginas[i].modificado) modified_count++;
    agregar_a_paquete(paquete, &modified_count, sizeof(uint32_t));

    for (uint32_t i = 0; i < ft->page_count; i++)
    {
        if (ft->paginas[i].modificado)
        {
            int frame = ft->paginas[i].frame_number;
            byte* src = get_frame_ptr(frame);
            if (!src)
            {
                eliminar_paquete(paquete);
                return MEMORY_INCONSISTENCY;
            }
            
            agregar_a_paquete(paquete, &i, sizeof(uint32_t));
            agregar_a_paquete(paquete, src, tam_pagina);

            ft->paginas[i].modificado = false;
        }
    }

    int res = enviar_paquete(paquete, socket_storage);
    eliminar_paquete(paquete);

    if (res < 0)
    {
        log_error(logger, "Se ha desconectado Storage... Terminando.");
        exit(EXIT_FAILURE);
    }

    log_info(logger, "FLUSH %s:%s -> %u páginas modificadas", file, tag, modified_count);

    return SUCCESS;
}

int delete_filetag(char* file, char* tag)
{
    if (!file || !tag) return MEMORY_INCONSISTENCY;

    FileTag* ft = get_filetag(file, tag);
    if (!ft) return OWNER_NOT_FOUND;

    for (uint32_t i = 0; i < ft->page_count; i++)
    {
        Page* pg = &ft->paginas[i];

        if (pg && pg->frame_number >= 0)
        {
            bitmap[pg->frame_number] = false;
            paginas_libres++;
        }
    }

    remove_filetag(file, tag);

    if (ft->paginas) free(ft->paginas);
    if (ft->file) free(ft->file);
    if(ft->tag) free(ft->tag);
    if (ft) free(ft);

    return SUCCESS;
}

void remove_filetag(char* file, char* tag)
{
    if (!file || !tag) return;

    for (size_t i = 0; i < list_size(file_tags); ++i)
    {
        FileTag* ft = (FileTag*)list_get(file_tags, i);
        if (ft->file && ft->tag && strcmp(ft->file, file) == 0 && strcmp(ft->tag, tag) == 0)
        {
            list_remove(file_tags, i);
            return;
        }       
    }
}
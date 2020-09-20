#ifndef ENGINE_UTILS_DYN_ARRAY_H
#define ENGINE_UTILS_DYN_ARRAY_H

typedef struct 
{
    u32   Size;
    u32   Cap;
    void *Ptr;
} dyn_array_header;

#define arr_init maple_arr_init
#define arr_free maple_arr_free
#define arr_put  maple_arr_put
#define arr_remove maple_arr_remove


#define maple_arr_init()
#define maple_arr_free()
#define maple_arr_put()
#define maple_arr_remove()


#endif //DYN_ARRAY_H

#if defined(USE_MAPLE_DYN_ARRAY_IMPLEMENTATION)



#endif
#ifndef ENGINE_PLATFORM_GLOBALS_H
#define ENGINE_PLATFORM_GLOBALS_H

typedef struct 
{
    u64 Size;
} memory_create_info;

typedef struct 
{
    memory_create_info Memory;
} globals_create_info;

typedef struct
{
    struct memory *Memory;
} globals;

extern globals *Core;

void globals_init(globals_create_info *CreateInfo);
void globals_free();

#endif //GLOBALS_H

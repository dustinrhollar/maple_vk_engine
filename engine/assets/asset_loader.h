#ifndef ENGINE_ASSETS_ASSET_LOADER_H
#define ENGINE_ASSETS_ASSET_LOADER_H

struct mesh_loader
{
    cgltf_data   *Data;
    mstring       Filename;
    mstring       BinaryFilename;
    
    //frame_params *FrameParams;    // is this necessary?
    
    file_t        Buffer;
    char         *BinaryData;
    asset_model  *Asset;
};

void LoadMesh(const char *Filename, free_allocator *Allocator);

#endif //ENGINE_ASSETS_ASSET_LOADER_H

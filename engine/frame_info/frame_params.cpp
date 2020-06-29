
void FrameParamsInit(frame_params      *FrameParams,
                     u32                Frame,
                     u64                FrameStartTime,
                     tagged_heap       *TaggedHeap,
                     renderer_t         Renderer,
                     resource_registry *ResourceRegistry,
                     asset_registry    *AssetRegistry)
{
    FrameParams->Frame                = Frame;
    FrameParams->FrameStartTime       = FrameStartTime;
    FrameParams->GameStageEndTime     = 0;
    FrameParams->RenderStageStartTime = 0;
    FrameParams->RenderStageEndTime   = 0;
    FrameParams->RenderCommandsCount  = 0;
    
    FrameParams->FrameHeapTag = { Frame, TAG_ID_GAME, 0 };
    FrameParams->FrameHeap = TaggedHeapRequestAllocation(TaggedHeap, FrameParams->FrameHeapTag);
    
    FrameParams->Renderer = Renderer;
    
    CopyAssets(&FrameParams->Assets, &FrameParams->AssetsCount, AssetRegistry, &FrameParams->FrameHeap);
    CopyResources(&FrameParams->Resources, &FrameParams->ResourcesCount, ResourceRegistry, &FrameParams->FrameHeap);
}

void FrameParamsFree(frame_params *FrameParams)
{
    FrameParams->Frame                = 0;
    FrameParams->FrameStartTime       = 0;
    FrameParams->GameStageEndTime     = 0;
    FrameParams->RenderStageStartTime = 0;
    FrameParams->RenderStageEndTime   = 0;
    FrameParams->RenderCommandsCount  = 0;
    FrameParams->AssetsCount          = 0;
    FrameParams->ResourcesCount       = 0;
    FrameParams->Assets               = NULL;
    FrameParams->Resources            = NULL;
    
    TaggedHeapReleaseAllocation(FrameParams->FrameHeap.TaggedHeap, FrameParams->FrameHeapTag);
    FrameParams->FrameHeap.TaggedHeap = NULL;
}

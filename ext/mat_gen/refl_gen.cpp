
// TODO(Dustin):...
// used for qsort - temp. solution
file_internal int InputBlockCompare(const void *a, const void *b)
{
    input_block *lhs = (input_block*)a;
    input_block *rhs = (input_block*)b;
    
    if (lhs->Set != rhs->Set) return lhs->Set > rhs->Set;
    return lhs->Binding > rhs->Binding;
}

file_internal jstring BaseTypeToString(spirv_cross::SPIRType::BaseType type)
{
    const char* names[] = {
		
        "Unknown",
		"Void",
		"Boolean",
		"SByte",
		"UByte",
		"Short",
		"UShort",
		"Int",
		"UInt",
		"Int64",
		"UInt64",
		"AtomicCounter",
		"Half",
		"Float",
		"Double",
		"Struct",
		"Image",
		"SampledImage",
		"Sampler",
		"AccelerationStructureNV",
    };
    
    jstring result = pstring(names[type]);
    return result;
}

file_internal void CreateUniformBlockForResource(input_block* outBlock, spirv_cross::Resource &res,
                                                 spirv_cross::CompilerGLSL& compiler)
{
    u32 id = res.id;
	spirv_cross::SmallVector<spirv_cross::BufferRange> ranges = compiler.get_active_buffer_ranges(id);
	const spirv_cross::SPIRType& ub_type = compiler.get_type(res.base_type_id);
    
    outBlock->Name = pstring(res.name.c_str());
	for (auto& range : ranges)
	{
        block_member mem;
        
        const char *str = compiler.get_member_name(res.base_type_id, range.index).c_str();
        mem.Name = pstring(str);
		mem.Size = range.range;
		mem.Offset = range.offset;
        
		auto type = compiler.get_type(res.type_id);
        
		outBlock->Members.PushBack(mem);
	}
    
	outBlock->Size = compiler.get_declared_struct_size(ub_type);
	outBlock->Binding = compiler.get_decoration(res.id, spv::DecorationBinding);
	outBlock->Set = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
	outBlock->IsTextureBlock = false;
}

file_internal void CreateTextureBlockForResource(input_block* outBlock, spirv_cross::Resource res,
                                                 spirv_cross::CompilerGLSL& compiler)
{
	uint32_t id = res.id;
	spirv_cross::SmallVector<spirv_cross::BufferRange> ranges = compiler.get_active_buffer_ranges(id);
    
    outBlock->Name = pstring(res.name.c_str());
    outBlock->Set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
    outBlock->Binding = compiler.get_decoration(res.id, spv::DecorationBinding);
    outBlock->IsTextureBlock = true;
}

file_internal void SerializeInputBlock(FileBuffer *Buffer, input_block *Block)
{
    UInt32ToBinaryBuffer(Buffer, &Block->Size, 1);
    UInt32ToBinaryBuffer(Buffer, &Block->Set, 1);
    UInt32ToBinaryBuffer(Buffer, &Block->Binding, 1);
    BoolToBinaryBuffer(Buffer, &Block->IsTextureBlock, 1);
    JStringToBinaryBuffer(Buffer, Block->Name);
    
    UInt32ToBinaryBuffer(Buffer, &Block->Members.size, 1);
    for (u32 MemberIdx = 0; MemberIdx < Block->Members.size; MemberIdx++)
    {
        UInt32ToBinaryBuffer(Buffer, &Block->Members[MemberIdx].Size, 1);
        UInt32ToBinaryBuffer(Buffer, &Block->Members[MemberIdx].Offset, 1);
        JStringToBinaryBuffer(Buffer, Block->Members[MemberIdx].Name);
    }
}

void GenerateReflectionInfo(DynamicArray<shader> &Shaders, jstring ReflFile)
{
    //~ Generate Reflection info
    DynamicArray<shader_data> ReflectionData = DynamicArray<shader_data>(Shaders.Size());
    
    for (u32 ShaderIdx = 0; ShaderIdx < Shaders.Size(); ++ShaderIdx)
    {
        shader_data ShaderData = {};
        ShaderData.Type = Shaders[ShaderIdx].Type;
        
        // NOTE(Dustin):
        // Right now not using Win32 helpers because the
        // CompilerGLSL requires the passed file as a u32*
        // rather than a char*. Haven't bothered to
        // implement a workaround yet.
        
        FILE* ShaderFile;
        fopen_s(&ShaderFile, Shaders[ShaderIdx].Filename.GetCStr(), "rb");
        assert(ShaderFile);
        
        fseek(ShaderFile, 0, SEEK_END);
        size_t filesize = ftell(ShaderFile);
        size_t wordSize = sizeof(uint32_t);
        size_t wordCount = filesize / wordSize;
        rewind(ShaderFile);
        
        uint32_t* ir = palloc<u32>(wordCount);
        
        fread(ir, filesize, 1, ShaderFile);
        fclose(ShaderFile);
        
        spirv_cross::CompilerGLSL glsl(ir, wordCount);
        
        spirv_cross::ShaderResources resources = glsl.get_shader_resources();
        
        for (spirv_cross::Resource res : resources.push_constant_buffers)
        {
            CreateUniformBlockForResource(&ShaderData.PushConstants, res, glsl);
        }
        
        ShaderData.DescriptorSets.Resize(resources.uniform_buffers.size() + resources.sampled_images.size());
        
        u32 idx = 0;
        for (spirv_cross::Resource res : resources.uniform_buffers)
        {
            // initialize the descriptor set at idx
            input_block Block = {};
            CreateUniformBlockForResource(&Block, res, glsl);
            
            ShaderData.DescriptorSets.PushBack(Block);
        }
        
        for (spirv_cross::Resource res : resources.sampled_images)
        {
            input_block Block = {};
            CreateTextureBlockForResource(&Block, res, glsl);
            
            ShaderData.DescriptorSets.PushBack(Block);
        }
        
        // NOTE(Dustin): let's not use qsort in the future - maybe replace it with something else?
        qsort(ShaderData.DescriptorSets.ptr, ShaderData.DescriptorSets.size, sizeof(input_block), &InputBlockCompare);
        
        // Organize into Global + Dynamic + Static descriptors
        for (uint32_t blockIdx = 0; blockIdx < ShaderData.DescriptorSets.size; ++blockIdx)
        {
            input_block& b = ShaderData.DescriptorSets[blockIdx];
            
            if (b.Set == GLOBAL_SET) ShaderData.GlobalSets.PushBack(b.Set);
            else if (b.Set == DYNAMIC_SET)
            {
                if (b.IsTextureBlock) ShaderData.NumDynamicTextures++;
                else ShaderData.NumDynamicUniforms++;
                
                // if this set is not already in dynamic sets, add it
                bool found_set = false;
                for (int i = 0; i < ShaderData.DynamicSets.size; ++i)
                {
                    if (ShaderData.DynamicSets[i] == b.Set)
                    {
                        found_set = true;
                        break;
                    }
                }
                
                if (found_set)
                {
                    ShaderData.DynamicSets.PushBack(b.Set);
                }
                
                ShaderData.DynamicSetSize += b.Size;
            }
            else
            {
                if (b.IsTextureBlock) ShaderData.NumStaticTextures++;
                else ShaderData.NumStaticUniforms++;
                
                // if this set is not already in static sets, add it
                bool found_set = false;
                for (int i = 0; i < ShaderData.StaticSets.size; ++i)
                {
                    if (ShaderData.StaticSets[i] == b.Set)
                    {
                        found_set = true;
                        break;
                    }
                }
                
                if (!found_set)
                {
                    ShaderData.StaticSets.PushBack(b.Set);
                }
                
                ShaderData.StaticSetSize += b.Size;
            }
        }
        
        ReflectionData.PushBack(ShaderData);
    }
    
    //~ Serialize the Reflection info
    mprinte("Reflection Count: %d\n", ReflectionData.Size());
    
    FileBuffer Buffer;
    CreateFileBuffer(&Buffer);
    
    UInt32ToBinaryBuffer(&Buffer, &ReflectionData.size, 1);
    for (u32 ReflectionIdx = 0; ReflectionIdx < ReflectionData.Size(); ++ReflectionIdx)
    {
        shader_data ShaderData = ReflectionData[ReflectionIdx];
        
        u32 ShaderDataCount[] = {
            static_cast<u32>(ShaderData.Type),
            ShaderData.DynamicSetSize,
            ShaderData.StaticSetSize,
            ShaderData.NumDynamicUniforms,
            ShaderData.NumDynamicTextures,
            ShaderData.NumStaticUniforms,
            ShaderData.NumStaticTextures
        };
        
        UInt32ToBinaryBuffer(&Buffer, ShaderDataCount,
                             sizeof(ShaderDataCount)/sizeof(ShaderDataCount[0]));
        
        SerializeInputBlock(&Buffer, &ShaderData.PushConstants);
        
        UInt32ToBinaryBuffer(&Buffer, &ShaderData.DescriptorSets.size, 1);
        for (u32 DescriptorIdx = 0; DescriptorIdx < ShaderData.DescriptorSets.Size(); DescriptorIdx++)
        {
            SerializeInputBlock(&Buffer, &ShaderData.DescriptorSets[DescriptorIdx]);
        }
        
        UInt32ToBinaryBuffer(&Buffer, &ShaderData.DynamicSets.size, 1);
        UInt32ToBinaryBuffer(&Buffer, ShaderData.DynamicSets.ptr, ShaderData.DynamicSets.size);
        
        UInt32ToBinaryBuffer(&Buffer, &ShaderData.GlobalSets.size, 1);
        UInt32ToBinaryBuffer(&Buffer, ShaderData.GlobalSets.ptr, ShaderData.GlobalSets.size);
        
        UInt32ToBinaryBuffer(&Buffer, &ShaderData.StaticSets.size, 1);
        UInt32ToBinaryBuffer(&Buffer, ShaderData.StaticSets.ptr, ShaderData.StaticSets.size);
    }
    
    Win32WriteBufferToFile(ReflFile, Buffer.start, Buffer.brkp - Buffer.start);
    DestroyFileBuffer(&Buffer);
}
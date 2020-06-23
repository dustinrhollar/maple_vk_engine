
file_internal void ProccessDataMember(data_member *Data, spirv_cross::TypeID &SpirvType);


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

file_internal void ProccessDataMember(data_member *Data, spirv_cross::TypeID &SpirvTypeId,
                                      spirv_cross::CompilerGLSL& compiler)
{
    auto SpirvType = compiler.get_type(SpirvTypeId);
    
    data_type Type = static_cast<data_type>(SpirvType.basetype);
    
    switch (Type)
    {
        
        case DataType_Boolean:
        {
            Data->Type  = DataType_Boolean;
            Data->Bool = false;
        } break;
        
        case DataType_SByte:
        {
            Data->Type  = DataType_SByte;
            Data->Int8 = 0;
        } break;
        
        case DataType_UByte:
        {
            Data->Type  = DataType_UByte;
            Data->UInt8 = 0;
        } break;
        
        case DataType_Short:
        {
            Data->Type  = DataType_Short;
            Data->Int16 = 0;
        } break;
        
        case DataType_UShort:
        {
            Data->Type  = DataType_UShort;
            Data->UInt16 = 0;
        } break;
        
        case DataType_Int:
        {
            Data->Type = DataType_Int;
            Data->Int32 = 0;
        } break;
        
        case DataType_UInt:
        {
            Data->Type  = DataType_UInt;
            Data->UInt32 = 0;
        } break;
        
        case DataType_Int64:
        {
            Data->Type  = DataType_Int64;
            Data->Int64 = 0;
        } break;
        
        case DataType_UInt64:
        {
            Data->Type  = DataType_UInt64;
            Data->UInt64 = 0;
        } break;
        
        case DataType_Float:
        {
            if (SpirvType.vecsize == 1 && SpirvType.columns == 1)
            {
                // it's actually a float
                Data->Type  = DataType_Float;
                Data->Float = 0.0f;
            }
            else if (SpirvType.vecsize == 2 && SpirvType.columns == 1)
            {
                // it's a vec2
                Data->Type = DataType_Vec2;
                Data->Vec2 = {0.0f,0.0f};
            }
            else if (SpirvType.vecsize == 3 && SpirvType.columns == 1)
            {
                // it's a vec3
                Data->Type = DataType_Vec3;
                Data->Vec3 = {0.0f,0.0f,0.0f};
            }
            else if (SpirvType.vecsize == 4 && SpirvType.columns == 1)
            {
                // it's a vec4
                Data->Type = DataType_Vec4;
                Data->Vec4 = {0.0f,0.0f,0.0f,0.0f};
            }
            else if (SpirvType.vecsize == 3 && SpirvType.columns == 3)
            {
                Data->Type = DataType_Mat3;
                Data->Mat3 = mat3(1.0f);
            }
            else if (SpirvType.vecsize == 4 && SpirvType.columns == 4)
            {
                Data->Type = DataType_Mat4;
                Data->Mat4 = mat4(1.0f);
            }
            else
            {
                mprinte("Unknown float size %d %d!\n", SpirvType.vecsize, SpirvType.columns);
            }
        } break;
        
        case DataType_Double:
        {
            Data->Type  = DataType_Double;
            Data->Double = 0;
        } break;
        
        case DataType_Struct:
        {
            Data->Type = DataType_Struct;
            Data->Struct.MembersCount = SpirvType.member_types.size();
            Data->Struct.Members = palloc<block_member>(Data->Struct.MembersCount);
            
            
            //spirv_cross::SmallVector<spirv_cross::BufferRange> ranges = compiler.get_active_buffer_ranges(SpirvTypeId);
            
            u32 Idx = 0;
            for (auto& InternalMemberId : SpirvType.member_types)
            {
                std::string MemberName = compiler.get_member_name(SpirvTypeId, Idx);
                
                block_member Block = {};
                Block.Name = pstring(MemberName.c_str());
                ProccessDataMember(&Block.Member, InternalMemberId, compiler);
                
                auto MemberType = compiler.get_type(InternalMemberId);
                Block.Size = (MemberType.vecsize * MemberType.columns * MemberType.width) / 8;
                
                Block.Offset = compiler.type_struct_member_offset(SpirvType, Idx);
                
                Data->Struct.Members[Idx++] = Block;
            }
            
        } break;
        
        case DataType_Void:
        case DataType_AtomicCount:
        case DataType_Half:
        case DataType_Image:
        case DataType_SampledImage:
        case DataType_Sampler:
        case DataType_AccelerationStructureNV:
        case DataType_Unknown:
        default: break;
    }
}

file_internal void CreateUniformBlockForResource(input_block* outBlock, spirv_cross::Resource &res,
                                                 spirv_cross::CompilerGLSL& compiler)
{
    u32 id = res.id;
	spirv_cross::SmallVector<spirv_cross::BufferRange> ranges = compiler.get_active_buffer_ranges(id);
	const spirv_cross::SPIRType& ub_type = compiler.get_type(res.base_type_id);
    outBlock->Name = pstring(res.name.c_str());
    
    // Create the data member blocks
    
    auto type = compiler.get_type(res.type_id);
    
    if (spv::StorageClass::StorageClassPushConstant == type.storage)
    {
        u32 Idx = 0;
        for (auto& range : type.member_types)
        {
            block_member mem = {};
            
            ProccessDataMember(&mem.Member, range, compiler);
            
            auto SpirvType = compiler.get_type(range);
            std::string mName = compiler.get_member_name(res.base_type_id, Idx);
            mem.Name = pstring(mName.c_str());
            
            outBlock->Members.PushBack(mem);
            
            Idx++;
        }
    }
    else
    {
        for (auto& range : ranges)
        {
            block_member mem = {};
            
            const char *str = compiler.get_member_name(res.base_type_id, range.index).c_str();
            mem.Name = pstring(str);
            mem.Size = range.range;
            mem.Offset = range.offset;
            
            jstring baseTypeString = BaseTypeToString(type.basetype);
            std::string mName = compiler.get_member_name(res.base_type_id, range.index);
            
            auto MemberId = type.member_types[range.index];
            ProccessDataMember(&mem.Member, MemberId, compiler);
            
            outBlock->Members.PushBack(mem);
        }
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

file_internal void SerializeDataMember(FileBuffer                 *Buffer,
                                       data_member                 Member,
                                       DynamicArray<block_member> &SerialMemberBlocks)
{
    WriteToFileBuffer(Buffer, "Type : u32 = %d\n", Member.Type);
    
    switch (Member.Type)
    {
        case DataType_Boolean:
        {
            WriteToFileBuffer(Buffer, "Value : u32 = %d\n", Member.Bool);
        } break;
        
        case DataType_SByte:
        {
            WriteToFileBuffer(Buffer, "Value : i8 = %d\n", Member.Int8);
        } break;
        
        case DataType_UByte:
        {
            WriteToFileBuffer(Buffer, "Value : u8 = %d\n", Member.UInt8);
        } break;
        
        case DataType_Short:
        {
            WriteToFileBuffer(Buffer, "Value : i16 = %d\n", Member.Int16);
        } break;
        
        case DataType_UShort:
        {
            WriteToFileBuffer(Buffer, "Value : u16 = %d\n", Member.UInt16);
        } break;
        
        case DataType_Int:
        {
            WriteToFileBuffer(Buffer, "Value : i32 = %d\n", Member.Int32);
        } break;
        
        case DataType_UInt:
        {
            WriteToFileBuffer(Buffer, "Value : u32 = %d\n", Member.UInt32);
        } break;
        
        case DataType_Int64:
        {
            WriteToFileBuffer(Buffer, "Value : i64 = %d\n", Member.Int64);
        } break;
        
        case DataType_UInt64:
        {
            WriteToFileBuffer(Buffer, "Value : u64 = %d\n", Member.UInt64);
        } break;
        
        case DataType_Float:
        {
            WriteToFileBuffer(Buffer, "Value : r32 = %lf\n", Member.Float);
        } break;
        
        case DataType_Double:
        {
            WriteToFileBuffer(Buffer, "Value : r64 = %llf\n", Member.Double);
        } break;
        
        case DataType_Struct:
        {
            WriteToFileBuffer(Buffer, "StructMemberCount : u32 = %d\n", Member.Struct.MembersCount);
            
            if (Member.Struct.MembersCount > 0)
            {
                WriteToFileBuffer(Buffer, "Value : str = [ \"%s\"", Member.Struct.Members[0].Name.GetCStr());
                SerialMemberBlocks.PushBack(Member.Struct.Members[0]);
                
                for (u32 Idx = 1; Idx < Member.Struct.MembersCount; ++Idx)
                {
                    WriteToFileBuffer(Buffer, ", \"%s\"", Member.Struct.Members[Idx].Name.GetCStr());
                    SerialMemberBlocks.PushBack(Member.Struct.Members[Idx]);
                }
                
                WriteToFileBuffer(Buffer, " ]\n\n");
            }
        } break;
        
        case DataType_Vec2:
        {
            WriteToFileBuffer(Buffer, "Value : vec2 = [%lf,%lf]\n",
                              Member.Vec2.x, Member.Vec2.y);
        } break;
        
        case DataType_Vec3:
        {
            WriteToFileBuffer(Buffer, "Value : vec3 = [%lf,%lf,%lf]\n",
                              Member.Vec3.x, Member.Vec3.y, Member.Vec3.z);
        } break;
        
        case DataType_Vec4:
        {
            WriteToFileBuffer(Buffer, "Value : vec4 = [%lf,%lf,%lf,%lf]\n",
                              Member.Vec4.x, Member.Vec4.y, Member.Vec4.z, Member.Vec4.w);
        } break;
        
        case DataType_Mat3:
        {
            WriteToFileBuffer(Buffer, "Value : mat3 = [ %lf", Member.Mat3.data[0]);
            for (u32 Idx = 1; Idx < 9; ++Idx)
                WriteToFileBuffer(Buffer, ", %lf", Member.Mat3.data[Idx]);
            WriteToFileBuffer(Buffer, " ]\n\n");
        } break;
        
        case DataType_Mat4:
        {
            WriteToFileBuffer(Buffer, "Value : mat4 = [ ");
            
            WriteToFileBuffer(Buffer, "%lf, %lf, %lf, %lf, ",
                              Member.Mat4.data[0][0],
                              Member.Mat4.data[0][1],
                              Member.Mat4.data[0][2],
                              Member.Mat4.data[0][3]);
            WriteToFileBuffer(Buffer, "%lf, %lf, %lf, %lf, ",
                              Member.Mat4.data[1][0],
                              Member.Mat4.data[1][1],
                              Member.Mat4.data[1][2],
                              Member.Mat4.data[1][3]);
            WriteToFileBuffer(Buffer, "%lf, %lf, %lf, %lf, ",
                              Member.Mat4.data[2][0],
                              Member.Mat4.data[2][1],
                              Member.Mat4.data[2][2],
                              Member.Mat4.data[2][3]);
            WriteToFileBuffer(Buffer, "%lf, %lf, %lf, %lf ]\n\n",
                              Member.Mat4.data[3][0],
                              Member.Mat4.data[3][1],
                              Member.Mat4.data[3][2],
                              Member.Mat4.data[3][3]);
        } break;
        
        case DataType_Unknown:
        case DataType_Void:
        case DataType_AtomicCount:
        case DataType_Half:
        case DataType_Image:
        case DataType_SampledImage:
        case DataType_Sampler:
        case DataType_AccelerationStructureNV:
        default:
        {
            mprinte("Data type %d not supported by config parser!\n", Member.Type);
        }
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
    
    //~ Serialze the Reflection data with the new file format
    DynamicArray<jstring> SerialGlobalNames   = DynamicArray<jstring>(2);
    DynamicArray<jstring> SerialMaterialNames = DynamicArray<jstring>(2);
    DynamicArray<jstring> SerialObjectNames   = DynamicArray<jstring>(2);
    
    DynamicArray<input_block> SerialPushBlocks    = DynamicArray<input_block>(1);
    DynamicArray<input_block> SerialInputBlocks   = DynamicArray<input_block>(5);
    DynamicArray<block_member> SerialMemberBlocks = DynamicArray<block_member>(5);
    
    FileBuffer Buffer;
    CreateFileBuffer(&Buffer);
    
    // Build the individual list that need to be serialized
    for (u32 ReflIdx = 0; ReflIdx < ReflectionData.size; ++ReflIdx)
    {
        shader_data ShaderData = ReflectionData[ReflIdx];;
        
        // queue push constant info
        if (ShaderData.PushConstants.Name.len > 0)
        { // not going to allow for no-name push constants
            SerialPushBlocks.PushBack(ShaderData.PushConstants);
        }
        
        for (u32 GlobalIdx = 0; GlobalIdx < ShaderData.GlobalSets.size; ++GlobalIdx)
        {
            u32 Idx = ShaderData.GlobalSets[GlobalIdx];
            SerialInputBlocks.PushBack(ShaderData.DescriptorSets[Idx]);
            SerialGlobalNames.PushBack(ShaderData.DescriptorSets[Idx].Name);
        }
        
        for (u32 GlobalIdx = 0; GlobalIdx < ShaderData.StaticSets.size; ++GlobalIdx)
        {
            u32 Idx = ShaderData.StaticSets[GlobalIdx];
            SerialInputBlocks.PushBack(ShaderData.DescriptorSets[Idx]);
            SerialObjectNames.PushBack(ShaderData.DescriptorSets[Idx].Name);
        }
        
        for (u32 GlobalIdx = 0; GlobalIdx < ShaderData.DynamicSets.size; ++GlobalIdx)
        {
            u32 Idx = ShaderData.DynamicSets[GlobalIdx];
            SerialInputBlocks.PushBack(ShaderData.DescriptorSets[Idx]);
            SerialMaterialNames.PushBack(ShaderData.DescriptorSets[Idx].Name);
        }
    }
    
    
    {
        // global data
        WriteToFileBuffer(&Buffer, "{Global Descriptors}\n");
        WriteToFileBuffer(&Buffer, "Count : u32 = %d\n", SerialGlobalNames.size);
        if (SerialGlobalNames.size > 0)
        {
            WriteToFileBuffer(&Buffer, "InputBlockNames : str = [ \"%s\"",
                              SerialGlobalNames[0].GetCStr());
            
            for (u32 GlobalIdx = 1; GlobalIdx < SerialGlobalNames.size; ++GlobalIdx)
            {
                WriteToFileBuffer(&Buffer, ", \"%s\"", SerialGlobalNames[GlobalIdx].GetCStr());
            }
            WriteToFileBuffer(&Buffer, " ]\n\n");
        }
        else
        {
            WriteToFileBuffer(&Buffer, "\n");
        }
    }
    
    {
        // Material Data
        WriteToFileBuffer(&Buffer, "{Material Descriptors}\n");
        WriteToFileBuffer(&Buffer, "Count : u32 = %d\n", SerialMaterialNames.size);
        if (SerialMaterialNames.size > 0)
        {
            WriteToFileBuffer(&Buffer, "InputBlockNames : str = [ \"%s\"",
                              SerialMaterialNames[0].GetCStr());
            
            for (u32 GlobalIdx = 1; GlobalIdx < SerialMaterialNames.size; ++GlobalIdx)
            {
                WriteToFileBuffer(&Buffer, ", \"%s\"", SerialMaterialNames[GlobalIdx].GetCStr());
            }
            WriteToFileBuffer(&Buffer, " ]\n\n");
        }
        else
        {
            WriteToFileBuffer(&Buffer, "\n");
        }
    }
    
    {
        // Object Data
        WriteToFileBuffer(&Buffer, "{Object Descriptors}\n");
        WriteToFileBuffer(&Buffer, "Count : u32 = %d\n", SerialObjectNames.size);
        if (SerialObjectNames.size > 0)
        {
            WriteToFileBuffer(&Buffer, "InputBlockNames : str = [ \"%s\"",
                              SerialObjectNames[0].GetCStr());
            
            for (u32 GlobalIdx = 1; GlobalIdx < SerialObjectNames.size; ++GlobalIdx)
            {
                WriteToFileBuffer(&Buffer, ", \"%s\"", SerialObjectNames[GlobalIdx].GetCStr());
            }
            WriteToFileBuffer(&Buffer, " ]\n\n");
        }
        else
        {
            WriteToFileBuffer(&Buffer, "\n");
        }
    }
    
    // Serialize Push Constant info
    if (SerialPushBlocks.size > 0)
    {
        WriteToFileBuffer(&Buffer, "{Push Constants}\n");
        WriteToFileBuffer(&Buffer, "Count : u32 = %d\n", SerialPushBlocks.size);
        WriteToFileBuffer(&Buffer, "PushConstantsNames : str = [ \"%s\"", SerialPushBlocks[0].Name.GetCStr());
        SerialInputBlocks.PushBack(SerialPushBlocks[0]);
        
        for (u32 PushIdx = 1; PushIdx < SerialPushBlocks.size; ++PushIdx)
        {
            input_block PushConstants = SerialPushBlocks[PushIdx];
            WriteToFileBuffer(&Buffer, ", \"%s\"", PushConstants.Name.GetCStr());
            
            SerialInputBlocks.PushBack(PushConstants);
        }
        WriteToFileBuffer(&Buffer, " ]\n\n");
    }
    
    // Write the Global/Object/Material Data
    for (u32 Idx = 0; Idx < SerialInputBlocks.size; ++Idx)
    {
        input_block Block = SerialInputBlocks[Idx];
        
        WriteToFileBuffer(&Buffer, "{%s}\n", Block.Name.GetCStr());
        WriteToFileBuffer(&Buffer, "Size : u32 = %d\n", Block.Size);
        WriteToFileBuffer(&Buffer, "Set : u32 = %d\n", Block.Set);
        WriteToFileBuffer(&Buffer, "Binding : u32 = %d\n", Block.Binding);
        WriteToFileBuffer(&Buffer, "IsTextureBlock : u32 = %d\n", Block.IsTextureBlock);
        
        WriteToFileBuffer(&Buffer, "MemberCount : u32 = %d\n", Block.Members.size);
        if (Block.Members.size > 0)
        {
            WriteToFileBuffer(&Buffer, "MemberNames : str = [ \"%s\"", Block.Members[0].Name.GetCStr());
            SerialMemberBlocks.PushBack(Block.Members[0]);
            
            for (u32 MemberIdx = 1; MemberIdx < Block.Members.size; MemberIdx++)
            {
                WriteToFileBuffer(&Buffer, ", \"%s\"", Block.Members[MemberIdx].Name.GetCStr());
                SerialMemberBlocks.PushBack(Block.Members[MemberIdx]);
            }
            WriteToFileBuffer(&Buffer, " ]\n\n");
        }
        
        WriteToFileBuffer(&Buffer, "\n");
    }
    
    // Write the block data
    for (u32 Idx = 0; Idx < SerialMemberBlocks.size; ++Idx)
    {
        block_member Member = SerialMemberBlocks[Idx];
        
        WriteToFileBuffer(&Buffer, "{%s}\n", Member.Name.GetCStr());
        WriteToFileBuffer(&Buffer, "Size : u32 = %d\n", Member.Size);
        WriteToFileBuffer(&Buffer, "Offset : u32 = %d\n", Member.Offset);
        
        SerializeDataMember(&Buffer, Member.Member, SerialMemberBlocks);
        
        WriteToFileBuffer(&Buffer, "\n");
    }
    
    Win32WriteBufferToFile(ReflFile, Buffer.start, Buffer.brkp - Buffer.start);
    DestroyFileBuffer(&Buffer);
}
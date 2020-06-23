#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

//--------------------------------------------------------------
// TODO(Dustin): Support these in Vector Math Library
struct ivec2
{
    i32 data[2];
};
struct ivec3
{
    i32 data[3];
};

struct ivec4
{
    i32 data[23];
};
//--------------------------------------------------------------


enum config_primitive_type
{
    Config_Bool,
    
    Config_I8,
    Config_I16,
    Config_I32,
    Config_I64,
    
    Config_U8,
    Config_U16,
    Config_U32,
    Config_U64,
    
    Config_R32,
    Config_R64,
    
    Config_IVec2,
    Config_IVec3,
    Config_IVec4,
    
    Config_Vec2,
    Config_Vec3,
    Config_Vec4,
    
    Config_Mat3,
    Config_Mat4,
    
    Config_Str,
    Config_Obj,
    Config_Array,
    
    Config_Unknown,
};

struct config_obj;
struct config_obj_table;
struct config_obj_table_entry;
struct config_member_table;
struct config_member_table_entry;

// stuff will go here
struct config_primitive_array
{
    config_primitive_type Type;
    u32 Len;
    void *Ptr;
};

struct config_member_table
{
    config_member_table_entry *Entries;
    u32 Count;
    u32 Cap;
    r32 LoadFactor;
};

struct config_obj
{
    jstring             Name;
    config_member_table ObjMembers;
};

struct config_var
{
    // does it need to store the name?
    // Name is stored as a key in the ObjMemberTable
    jstring Name;
    
    config_primitive_type Type;
    union
    {
        bool                   Bool;
        
        i8                     Int8;
        i16                    Int16;
        i32                    Int32;
        i64                    Int64;
        
        u8                     UInt8;
        u16                    UInt16;
        u32                    UInt32;
        u64                    UInt64;
        
        r32                    R32; // float
        r64                    R64; // double
        
        ivec2                  IVec2;
        ivec3                  IVec3;
        ivec4                  IVec4;
        
        vec2                   Vec2;
        vec3                   Vec3;
        vec4                   Vec4;
        
        mat3                   Mat3;
        mat4                   Mat4;
        
        jstring                Str;
        
        config_primitive_array Array;
        
        config_obj             Object;
    };
};

struct config_member_table_entry
{
    bool       IsEmpty;
    jstring    Key;
    config_var Value;
};


struct config_obj_table_entry
{
    bool       IsEmpty;
    jstring    Key;
    config_obj Value;
};

struct config_obj_table
{
    config_obj_table_entry *Entries;
    u32 Count;
    u32 Cap;
    r32 LoadFactor;
};

//~ Parser Structures

struct mscanner
{
    char  *Buffer;
    size_t BufferSize;
    u32    LineCount;
    char  *Current;
};

enum token_type
{
    Token_Invalid           = BIT(0),
    
    Token_OpenBracket       = BIT(1), // I am not entirely sure
    Token_ClosedBracket     = BIT(2), // what these two were for
    Token_Equals            = BIT(3),
    Token_OpenParenthesis   = BIT(4),
    Token_ClosedParenthesis = BIT(5),
    Token_OpenBrace         = BIT(6),
    Token_ClosedBrace       = BIT(7),
    Token_Comma             = BIT(8),
    Token_Quotation         = BIT(9),
    Token_Colon             = BIT(10),
    Token_BitwiseOr         = BIT(11),
    Token_Newline           = BIT(12),
    
    // Types
    Token_Object            = BIT(13),
    Token_Name              = BIT(14),
    Token_String            = BIT(15),
    Token_Int               = BIT(16),
    Token_Float             = BIT(17),
};

enum config_operator
{
    Operator_Or                = BIT(0),
    Operator_Equals            = BIT(1),
    Operator_Comma             = BIT(2),
    Operator_Colon             = BIT(3),
    Operator_OpenParenthesis   = BIT(4),
    Operator_OpenBrace         = BIT(5),
    Operator_ClosedParenthesis = BIT(6),
    Operator_ClosedBrace       = BIT(7),
    Operator_None              = BIT(8),
    Operator_Unset             = BIT(9),
};

struct token
{
    token_type Type;
    char      *Start; // memory in the file array
    u32        Len;
};

struct token_node
{
    token_node *Next;
    token       Token;
};

// Root is a Resource Type
struct token_list
{
    token_node *Head;
    token_node *Tail;
};

struct syntax_node
{
    token Token;
    u32 Op;
    bool IsArray; // hack-y way to determine if a token signals the start of an array
    
    syntax_node *Parent;
    syntax_node *Left;
    syntax_node *Right;
};

struct syntax_tree_list
{
    syntax_node **Head;
    
    u32 Size;
    u32 Cap;
};

//~ Functions

void InitConfigMemberTable(config_member_table *Table);
void FreeConfigMemberTable(config_member_table *Table);
void ConfigMemberTableInsert(config_member_table *Table, const char* Key, config_var Var);
config_var ConfigMemberTableGet(config_member_table *Table, const char* Key);
void ConfigMemberTableResize(config_member_table *Table, u32 Cap);

void InitConfigObjTable(config_obj_table *Table);
void FreeConfigObjTable(config_obj_table *Table);
void ConfigObjTableInsert(config_obj_table *Table, const char* Key, config_obj Var);
config_obj ConfigObjTableGet(config_obj_table *Table, const char* Key);
void ConfigObjTableResize(config_obj_table *Table, u32 Cap);

void InitConfigObj(config_obj *Obj, const char *Name, u32 NameLen);
void FreeConfigObj(config_obj *Obj);

bool GetConfigBool(config_obj *Obj, const char* VarName);
i8 GetConfigI8(config_obj *Obj, const char* VarName);
i16 GetConfigI16(config_obj *Obj, const char* VarName);
i32 GetConfigI32(config_obj *Obj, const char* VarName);
i64 GetConfigI64(config_obj *Obj, const char* VarName);
u8 GetConfigU8(config_obj *Obj, const char* VarName);
u16 GetConfigU16(config_obj *Obj, const char* VarName);
u32 GetConfigU32(config_obj *Obj, const char* VarName);
u64 GetConfigU64(config_obj *Obj, const char* VarName);
r32 GetConfigR32(config_obj *Obj, const char* VarName);
r64 GetConfigR64(config_obj *Obj, const char* VarName);
ivec2 GetConfigIVec2(config_obj *Obj, const char* VarName);
ivec3 GetConfigIVec3(config_obj *Obj, const char* VarName);
ivec4 GetConfigIVec4(config_obj *Obj, const char* VarName);
vec2 GetConfigVec2(config_obj *Obj, const char* VarName);
vec3 GetConfigVec3(config_obj *Obj, const char* VarName);
vec4 GetConfigVec4(config_obj *Obj, const char* VarName);
mat3 GetConfigMat3(config_obj *Obj, const char* VarName);
mat4 GetConfigMat4(config_obj *Obj, const char* VarName);
// Returns a COPY of the string
jstring GetConfigStr(config_obj *Obj, const char* VarName);
// The third and fourth arguments are the return values.
// Data is a pointer to the array
// Count is the length of the array
void GetConfigArray(config_obj *Obj, const char* VarName, void **Data, u32 *Count);
config_obj GetConfigObj(config_obj_table *Table, const char* ObjName);

// size is needed for when the Data Type is a string. Need to know the length
// of the string.
void InitConfigVar(config_var *Var, const char *Name, u32 NameLen,
                   config_primitive_type Type, void *Data, u32 size = 0,
                   config_primitive_type SecondaryType = Config_Unknown);
void FreeConfigVar(config_var *Var);

config_obj_table LoadConfigFile(jstring filename);
void SaveConfigFile();

#endif //CONFIG_PARSER_H

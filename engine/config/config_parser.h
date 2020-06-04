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


// stuff will go here

enum config_primitive_type
{
    Config_I32,
    Config_I64,
    Config_U32,
    Config_U64,
    Config_R32,
    Config_IVec2,
    Config_IVec3,
    Config_IVec4,
    Config_Vec2,
    Config_Vec3,
    Config_Vec4,
    Config_Str,
    Config_Obj,
    Config_Unknown,
};

/*

{Foo}
apple: i32 = 10
berry: r32 = 10.0

LoadConfig(file)
-- Generate Parse Trees

ConfigTable (hash table)
-- jstring
-- primitive type



---> Translation

struct Foo
{
 
i32 apple;
r32 berry;

}

config_obj foo = ObjTable.Get("Foo");
r32 berry = foo.GetR32("berry");

*/

struct config_obj;
struct config_obj_table;
struct config_obj_table_entry;
struct config_member_table;
struct config_member_table_entry;

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
        i32 Int32;
        i64 Int64;
        u32 UInt32;
        u64 UInt64;
        r32 R32;
        
        ivec2 IVec2;
        ivec3 IVec3;
        ivec4 IVec4;
        
        vec2 Vec2;
        vec3 Vec3;
        vec4 Vec4;
        
        jstring Str;
        
        config_obj Object;
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
    Token_Invalid = 0,
    
    Token_OpenBracket,
    Token_ClosedBracket,
    Token_Equals,
    Token_OpenParenthesis,
    Token_ClosedParenthesis,
    Token_OpenBrace,
    Token_ClosedBrace,
    Token_Comma,
    Token_Quotation,
    Token_Colon,
    Token_BitwiseOr,
    Token_Newline,
    
    // Types
    Token_Object,
    Token_Name,
    Token_String,
    Token_Int,
    Token_Float,
};

enum config_operator
{
    Operator_Or                = 1<<0,
    Operator_Equals            = 1<<1,
    Operator_Comma             = 1<<2,
    Operator_Colon             = 1<<3,
    Operator_OpenParenthesis   = 1<<4,
    Operator_OpenBrace         = 1<<5,
    Operator_ClosedParenthesis = 1<<6,
    Operator_ClosedBrace       = 1<<7,
    Operator_None              = 1<<8,
    Operator_Unset             = 1<<9,
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

i32 GetConfigI32(config_obj *Obj, const char* VarName);
i64 GetConfigI64(config_obj *Obj, const char* VarName);
u32 GetConfigU32(config_obj *Obj, const char* VarName);
u64 GetConfigU64(config_obj *Obj, const char* VarName);
r32 GetConfigR32(config_obj *Obj, const char* VarName);
ivec2 GetConfigIVec2(config_obj *Obj, const char* VarName);
ivec3 GetConfigIVec3(config_obj *Obj, const char* VarName);
ivec4 GetConfigIVec4(config_obj *Obj, const char* VarName);
vec2 GetConfigVec2(config_obj *Obj, const char* VarName);
vec3 GetConfigVec3(config_obj *Obj, const char* VarName);
vec4 GetConfigVec4(config_obj *Obj, const char* VarName);
// Returns a COPY of the string
jstring GetConfigStr(config_obj *Obj, const char* VarName);
config_obj GetConfigObj(config_obj_table *Table, const char* ObjName);

// size is needed for when the Data Type is a string. Need to know the length
// of the string.
void InitConfigVar(config_var *Var, const char *Name, u32 NameLen,
                   config_primitive_type Type, const char *Data, u32 size = 0);
void FreeConfigVar(config_var *Var);

config_obj_table LoadConfigFile(jstring filename);
void SaveConfigFile();

#endif //CONFIG_PARSER_H

#ifndef SPLICER_EXT_RESOURCE_MANAGER_PARSER_H
#define SPLICER_EXT_RESOURCE_MANAGER_PARSER_H

enum Operator
{
    OPERATOR_OR                 = 1<<0,
    OPERATOR_EQUALS             = 1<<1,
    OPERATOR_COMMA              = 1<<2,
    OPERATOR_COLON              = 1<<3,
    OPERATOR_OPEN_PARENTHESIS   = 1<<4,
    OPERATOR_OPEN_BRACE         = 1<<5,
    OPERATOR_CLOSED_PARENTHESIS = 1<<6,
    OPERATOR_CLOSED_BRACE       = 1<<7,
    OPERATOR_NONE               = 1<<8,
    OPERATOR_UNSET              = 1<<9,
};

struct Token
{
    TypeToken type;
    char *start; // memory in the file array
    char *end;
};

struct TokenNode
{
    TokenNode *next;
    Token     token;
};

// Root is a Resource Type
struct TokenList
{
    TokenNode *head;
    TokenNode *tail;
};

struct SyntaxNode
{
    Token token;
    u32 op;
    
    SyntaxNode *parent;
    SyntaxNode *left;
    SyntaxNode *right;
};

struct SyntaxTreeList
{
    SyntaxNode **head;
    
    u32 size;
    u32 cap;
};

//~ Functions
void InitializeParser();
void ShutdownParser();

jstring TokenTypeToName(TypeToken type);

void PrintTextMaterial(Material mat);
void LoadMaterialFromFile(Material *material, jstring &file);
void LoadMaterialInstanceFromFile(MaterialInstance *instance, jstring &file);
void LoadModelFromFile(Model *model, jstring &file);
void LoadSceneFromFile(Scene *scene, jstring &file);

#endif //SPLICER_EXT_RESOURCE_MANAGER_PARSER_H

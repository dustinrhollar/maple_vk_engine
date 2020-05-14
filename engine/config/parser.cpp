
enum ResourceType {
    RESOURCE_TYPE_MATERIAL = 0,
    RESOURCE_TYPE_INVALID,
};

enum PrimitiveType
{
    PRIMITIVE_TYPE_UNKNOWN,
    PRIMITIVE_TYPE_INT,
    PRIMITIVE_TYPE_FLOAT,
    PRIMITIVE_TYPE_IVEC3,
    PRIMITIVE_TYPE_IVEC4,
    PRIMITIVE_TYPE_VEC3,
    PRIMITIVE_TYPE_VEC4,
    PRIMITIVE_TYPE_STRING,
};

enum ReservedWords
{
    RESERVED_SCENE,
    RESERVED_MODEL,
    RESERVED_MATERIAL,
    RESERVED_MATERIAL_INSTANCE,
    
    RESERVED_NAME,
    RESERVED_MATERIAL_NAME,
    
    RESERVED_VERTEX,
    RESERVED_FRAGMENT,
    RESERVED_GEOMETRY,
    RESERVED_TESSELATION_CONTROL,
    RESERVED_TESSELATION_EVALUATION,
    
    RESERVED_REFLECTION,
    
    RESERVED_WORKFLOW,
    RESERVED_BASE_COLOR,
    RESERVED_METALLIC_ROUGHNESS,
    RESERVED_DIFFUSE,
    RESERVED_SPECULAR,
    RESERVED_CLEAR_COAT,
    RESERVED_CLEAR_COAT_ROUGHNESS,
    RESERVED_CLEAR_COAT_NORMAL,
    RESERVED_NORMAL,
    RESERVED_OCCLUSION,
    RESERVED_EMISSIVE,
    
    RESERVED_SET,
    RESERVED_BINDING,
};

file_global const int ReservedNameCount = 24;
const char* Names[] =
{
    // Resource types
    "scene",
    "model",
    "material",
    "material instance",
    
    // Name of resource (i.e. name of material)
    "name",
    "materialname",
    
    // Material Names: Shaders
    "vertex",
    "fragment",
    "geometry",
    "tesselationcontrol",
    "tesselationevaluation",
    
    // Reflection file
    "reflection",
    
    // Texture Names
    "workflow",
    "basecolor",
    "metallicroughness",
    "diffuse",
    "specular",
    "clearcoat",
    "clearcoatroughness",
    "clearcoatnormal",
    "normal",
    "occlusion",
    "emissive",
    
    // Uniform Blocks
    "set",
    "binding",
};

u128 HashedNames[ReservedNameCount];


//~ Print Helpers
file_internal jstring TokenTypeToName(TypeToken type)
{
    jstring result;
    
    switch (type)
    {
        case TOKEN_TYPE_INVALID: InitJString(&result, "Token Type Invalid"); break;
        
        // Operators
        case TOKEN_TYPE_OPEN_BRACKET: InitJString(&result, "Open Bracket"); break;
        case TOKEN_TYPE_CLOSED_BRACKET: InitJString(&result, "Closed Bracket"); break;
        case TOKEN_TYPE_EQUALS: InitJString(&result, "Equals"); break;
        case TOKEN_TYPE_OPEN_PARENTHESIS: InitJString(&result, "Open Parenthesis"); break;
        case TOKEN_TYPE_CLOSED_PARENTHESIS: InitJString(&result, "Closed Parenthesis"); break;
        case TOKEN_TYPE_OPEN_BRACE: InitJString(&result, "Open Brace"); break;
        case TOKEN_TYPE_CLOSED_BRACE: InitJString(&result, "Closed brace"); break;
        case TOKEN_TYPE_COMMA: InitJString(&result, "Comma"); break;
        case TOKEN_TYPE_QUOTATION: InitJString(&result, "Quotation"); break;
        case TOKEN_TYPE_BITWISE_OR: InitJString(&result, "Bitwise OR"); break;
        case TOKEN_TYPE_NEWLINE: InitJString(&result, "Newline"); break;
        case TOKEN_TYPE_COLON: InitJString(&result, "Colon"); break;
        
        // Types
        case TOKEN_TYPE_RESOURCE_NAME: InitJString(&result, "Resource Name"); break;
        case TOKEN_TYPE_NAME: InitJString(&result, "Name"); break;
        case TOKEN_TYPE_STRING: InitJString(&result, "String"); break;
        case TOKEN_TYPE_INT: InitJString(&result, "Int"); break;
        case TOKEN_TYPE_FLOAT: InitJString(&result, "Float"); break;
        
        default: InitJString(&result, "Unknown token type"); break;
    }
    
    return result;
}

file_internal int TokenToInt(Token token)
{
    // strtoint does not take into account negative
    // integers (in order to maintain compatibility
    // with unsigned types).
    int result = 0;
    if (token.start[0] == '-')
    {
        result = StrToInt(&token.start[1], token.end);
        result *= -1;
    }
    else
    {
        result = StrToInt(token.start, token.end);
    }
    
    return result;
}

file_internal i64 TokenToInt64(Token token)
{
    // strtoint does not take into account negative
    // integers (in order to maintain compatibility
    // with unsigned types).
    i64 result = 0;
    if (token.start[0] == '-')
    {
        result = StrToInt64(&token.start[1], token.end);
        result *= -1;
    }
    else
    {
        result = StrToInt64(token.start, token.end);
    }
    
    return result;
}


file_internal float TokenToFloat(Token token)
{
    // token strings are not null delimited
    return ReadFloatFromString(token.start);
}

file_internal void PrintSyntaxLevel(SyntaxNode *level_root, int *level)
{
    if (!level_root->left || !level_root->right) return;
    
    printf("\tLevel %d\n", *level);
    printf("\t\tLeft\n");
    
    SyntaxNode *left = level_root->left;
    SyntaxNode *right = level_root->right;
    
    jstring left_name = TokenTypeToName(left->token.type);
    jstring left_value;
    InitJString(&left_value, left->token.start,
                left->token.end - left->token.start);
    
    jstring right_name = TokenTypeToName(right->token.type);
    jstring right_value;
    InitJString(&right_value, right->token.start,
                right->token.end - right->token.start);
    
    printf("\t\t\tOperator: %s\n", left_name.GetCStr());
    printf("\t\t\tValue: %s\n", left_value.GetCStr());
    
    printf("\t\tRight\n");
    printf("\t\t\tOperator: %s\n", right_name.GetCStr());
    printf("\t\t\tValue: %s\n", right_value.GetCStr());
    
    int next_level = *level + 1;
    PrintSyntaxLevel(left, &next_level);
    PrintSyntaxLevel(right, &next_level);
}

file_internal void PrintSyntaxTree(SyntaxNode *root)
{
    int level = 1;
    
    printf("\tLevel 0\n");
    
    jstring name = TokenTypeToName(root->token.type);
    jstring value;
    InitJString(&value, root->token.start,
                root->token.end - root->token.start);
    
    printf("\t\t\tOperator: %s\n", name.GetCStr());
    printf("\t\t\tValue: %s\n", value.GetCStr());
    
    PrintSyntaxLevel(root, &level);
}

file_internal void PrintParseTrees(SyntaxTreeList *syntax_trees)
{
    for (u32 i = 0; i < syntax_trees->size; ++i)
    {
        printf("Printing Syntax Tree %d\n", i);
        
        PrintSyntaxTree(syntax_trees->head[i]);
    }
}

void PrintTextMaterial(Material mat)
{
    printf("Material %s\n", mat.Name.GetCStr());
    printf("Vertex Shader: %s\n", mat.Vertex.GetCStr());
    printf("Fragment Shader: %s\n", mat.Fragment.GetCStr());
    printf("Geometry Shader: %s\n", mat.Geometry.GetCStr());
    printf("Tesselation Control Shader: %s\n", mat.TesselationControl.GetCStr());
    printf("Tesselation Evaluation Shader: %s\n", mat.TesselationEvaluation.GetCStr());
    printf("Reflection File: %s\n", mat.ReflectionFile.GetCStr());
}

ResourceType ResourceNameToType(const char *name, size_t len)
{
    jstring jname;
    InitJString(&jname, name, len);
    u128 hashed_name = hash_char_array(name, len);
    
    ResourceType result;
    
    switch (name[0])
    {
        case 'M':
        {
            u128 material_hash = hash_char_array("Material",
                                                 8);
            
            if (compare_hash(material_hash, hashed_name))
            {
                result = RESOURCE_TYPE_MATERIAL;
            }
            else
            {
                printf("%s is an invalid resource name, did you mean \"Material\"?\n", name);
                result = RESOURCE_TYPE_INVALID;
            }
        } break;
        
        default:
        {
            printf("%s is an invalid resource name!\n", name);
            result = RESOURCE_TYPE_INVALID;
        } break;
    }
    
    return result;
}


jstring ResourceTypeToName(ResourceType type)
{
    jstring result;
    
    // TODO(Dustin): Very bad!
    if (type != RESOURCE_TYPE_INVALID)
    {
        InitJString(&result, "Material");
    }
    else
    {
        InitJString(&result);
    }
    
    return result;
}

PrimitiveType TokenToPrimitiveType(Token token)
{
    PrimitiveType result = PRIMITIVE_TYPE_UNKNOWN;
    jstring str_type;
    InitJString(&str_type, token.start, token.end - token.start);
    
    if (str_type == "i")
    {
        result = PRIMITIVE_TYPE_INT;
    }
    else if (str_type == "f")
    {
        result = PRIMITIVE_TYPE_FLOAT;
    }
    else if (str_type == "ivc3")
    {
        result = PRIMITIVE_TYPE_IVEC3;
    }
    else if (str_type == "ivc4")
    {
        result = PRIMITIVE_TYPE_IVEC4;
    }
    else if (str_type == "v3")
    {
        result = PRIMITIVE_TYPE_VEC3;
    }
    else if (str_type == "v4")
    {
        result = PRIMITIVE_TYPE_VEC4;
    }
    else if (str_type == "str")
    {
        result = PRIMITIVE_TYPE_STRING;
    }
    else
    {
        printf("Unknown primitive type: %s!\n", str_type.GetCStr());
    }
    
    return result;
}

//~ PARSER
/*
// TODO(Dustin): Things to add to the tokenizer

New Tokens:
- |
- \n

A way to distinguish between () and [] expressions

*/

//~
/*
Syntax Trees
*
*            2 + 2 * 3
*                 -
*                / \
*               2
*
*
*              4 * (2 + 3)
*
*                   *
*                  / \
*                 4   +
*                    / \
*                   2   3
*
*
*              (2 + 3) * 4
*
*
*
*                        *
*                       / \
*                    (+)   4
*                    / \
*                   2   3
*
*
*
*                 3 * 3 + 2
*
                *                        +
*                     *     2
*                    3 3
*
*
*
*
*
*
*
*
*                  +
*                 / \
*                2   *
*                   / \
*                  3   4
*
*
*
*             2 + 2 * 3 * 2 + 2 + 2
*
*                         +
*                        / \
*                       2   +
                   *                          / \
*                         +   2
                  *                        / \
*                       *   2
                *                      /\
*                     2  *
*                       / \
*                      3   2
*
*            3 * 3 * (2 + 2)
*
*                  *
*                 / \
*                3   *
*                   / \
*                  3   (
*                     / \
             *
*


     *         a = 3 * ( ( 3 + 2 + 2 + 2) * 3 + 2 + 3 + 4)
    *
*                      *
*                   3     +
*                      *     +
*                    +   3  2 3
 *                  +   2
                 *                +   2
*               3 2
*
*
* Rule: If token is ( or [
*  - Create a new node at left of current node, move to left child
* Rule: If token an operator
*  - Set the current to to the operator, create a right node and move to it
* Rule: If token is not an operator
*  - Set the node to that token, move to the parent
* Rule: If token is a ) or ]
*  - Go to the parent of the current node
         




*
* Assignment syntax
           *       name = type
*
            *            =
 *           / \
*          a  type
*
*
* Block Start Syntax
 *       (set = int | binding = int)
*
*
*    -  ->     c  ->     =    ->     =,c       -> (next line)
*  c       set       set   c     set     int
*
*
*          |                    |,c                      |
*    =          c  ->     =          binding ->    =          =    ->
* set int              set int                  set int   bdg   c
*
*          |                         |,c
*    =           =,c     ->    =          =
* set int    bdg   int      set int    bdg int
*
*
*
*


* Block Definition Syntax
*     int : type = [2, 2]
*
*   -  ->     c  ->     :    ->     :,c     ->  (next line)
* c       int       int   c     int   type
*
*         =                  =,c                  =
*    :        c  ->    :           [   ->    :        [
* int tp            int tp       -         int tp    2  c
*                              2
*
*
*
* Operators & Precendence
*
* |  = 0
* =  = 1
* ,  = 2
* :  = 3
* (  = 4
* )  = 4
* [  = 4
* ]  = 4
*
*
*
*
*
*
*/

file_internal bool IsSkippableChar(char *current)
{
    switch (current[0])
    {
        case ' ':
        case '\t':
        case '\r':
        {
            return true;
        }
        
        default: return false;
    }
}

file_internal char *consume_string(char *current, char end_char)
{
    while (current[0] != end_char) current++;
    
    char *end = current;
    return end;
}

file_internal ResourceType RetrieveResourceType(Scanner *scanner)
{
    ResourceType result;
    
    
    switch (scanner->current[0])
    {
        case '{':
        { // new material
            ++scanner->current;
            char *end_ptr = consume_string(scanner->current, '}');
            
            // TODO(Dustin): This is outdated!
            result = ResourceNameToType(scanner->current, end_ptr - scanner->current);
            
            if (result != RESOURCE_TYPE_MATERIAL)
            {
                printf("Attempting to parse a material file and the material type is not set correctly.\n");
            }
            
            jstring resource_name = ResourceTypeToName(result);
            printf("Resource type read from file %s\n", resource_name.GetCStr());
            
            scanner->current += 2;
        } break;
        
        default: result = RESOURCE_TYPE_INVALID;
    }
    
    return result;
}

inline bool IsChar(char ch)
{
    return isalpha(ch);
}

inline bool IsDigit(char ch)
{
    return isdigit(ch);
}

file_internal Token ConsumeNextToken(Scanner *scanner)
{
    Token token = {};
    token.type = TOKEN_TYPE_INVALID;
    
    while (IsSkippableChar(scanner->current)) scanner->current++;
    
    if (IsChar(scanner->current[0]) ||
        scanner->current[0] == '_')
    {
        // named token
        token.type  = TOKEN_TYPE_NAME;
        token.start = scanner->current;
        
        token.end   = scanner->current + 1;
        while (IsChar(token.end[0]) ||
               IsDigit(token.end[0]) ||
               token.end[0] == '_') token.end++;
        
        scanner->current = token.end;
    }
    else if (IsDigit(scanner->current[0]))
    {
        token.type  = TOKEN_TYPE_INT;
        token.start = scanner->current;
        
        token.end   = scanner->current + 1;
        while (IsDigit(token.end[0]) ||
               token.end[0] == '.')
        {
            if (token.end[0] == '.')
            {
                token.type  = TOKEN_TYPE_FLOAT;
            }
            
            token.end++;
        }
        
        scanner->current = token.end;
    }
    else {
        switch (scanner->current[0])
        {
            case '{':
            {
                token.type  = TOKEN_TYPE_RESOURCE_NAME;
                token.start = scanner->current + 1;
                
                if (token.start[0] == '}')
                {
                    token.end = token.start;
                }
                else
                {
                    token.end = token.start + 1;
                    while (token.end[0] != '}') ++token.end;
                    
                    scanner->current = token.end + 1;
                }
            } break;
            
            case '|':
            {
                token.type  = TOKEN_TYPE_BITWISE_OR;
                token.start = scanner->current;
                token.end   = ++scanner->current;
            } break;
            
            case '\n':
            {
                scanner->line_count++;
                token.type  = TOKEN_TYPE_NEWLINE;
                token.start = scanner->current;
                token.end   = ++scanner->current;
            } break;
            
            case '=':
            {
                token.type  = TOKEN_TYPE_EQUALS;
                token.start = scanner->current;
                token.end   = ++scanner->current;
            } break;
            
            case '(':
            {
                token.type  = TOKEN_TYPE_OPEN_PARENTHESIS;
                token.start = scanner->current;
                token.end   = ++scanner->current;
            } break;
            
            case ')':
            {
                token.type  = TOKEN_TYPE_CLOSED_PARENTHESIS;
                token.start = scanner->current;
                token.end   = ++scanner->current;
            } break;
            
            case '[':
            {
                token.type  = TOKEN_TYPE_OPEN_BRACE;
                token.start = scanner->current;
                token.end   = ++scanner->current;
            } break;
            
            case ']':
            {
                token.type  = TOKEN_TYPE_CLOSED_BRACE;
                token.start = scanner->current;
                token.end   = ++scanner->current;
            } break;
            
            case ',':
            {
                token.type  = TOKEN_TYPE_COMMA;
                token.start = scanner->current;
                token.end   = ++scanner->current;
            } break;
            
            case ':':
            {
                token.type  = TOKEN_TYPE_COLON;
                token.start = scanner->current;
                token.end   = ++scanner->current;
            } break;
            
            case '\"':
            { // string token
                token.type  = TOKEN_TYPE_STRING;
                token.start = ++scanner->current;
                
                if (token.start[0] == '\"')
                { // ""
                    token.end = token.start;
                }
                else {
                    token.end   = scanner->current + 1;
                    while (token.end[0] != '\"')
                    {
                        ++token.end;
                    }
                }
                
                scanner->current = token.end + 1;
            } break;
            
            default: break;
        }
    }
    
    return token;
}

file_internal TokenList Tokenizer(Scanner *scanner)
{
    TokenList tokens = {};
    tokens.head = nullptr;
    tokens.tail = tokens.head;
    
    Token token = ConsumeNextToken(scanner);
    
    while (token.type != TOKEN_TYPE_INVALID)
    {
        TokenNode *node = palloc<TokenNode>(1);
        node->token = token;
        node->next  = nullptr;
        
        if (!tokens.head)
        {
            tokens.head = node;
            tokens.tail = tokens.head;
        }
        else
        {
            tokens.tail->next = node;
            tokens.tail = node;
        }
        
        token = ConsumeNextToken(scanner);
    }
    
    TokenNode *iter = tokens.head;
    while (iter)
    {
        jstring value;
        InitJString(&value, iter->token.start, iter->token.end - iter->token.start);
        
        iter = iter->next;
    }
    
    return tokens;
}

file_internal void CreateSyntaxNode(SyntaxNode *node, Token token)
{
    node->token  = token;
    node->left   = nullptr;
    node->right  = nullptr;
    node->parent = nullptr;
    node->op     = OPERATOR_UNSET;
}

file_internal Operator TokenTypeToOperator(TypeToken type)
{
    Operator result = OPERATOR_UNSET;
    
    switch (type)
    {
        case TOKEN_TYPE_EQUALS:
        {
            result = OPERATOR_EQUALS;
        } break;
        
        case TOKEN_TYPE_OPEN_PARENTHESIS:
        {
            result = OPERATOR_OPEN_PARENTHESIS;
        } break;
        
        case TOKEN_TYPE_CLOSED_PARENTHESIS:
        {
            result = OPERATOR_CLOSED_PARENTHESIS;
        } break;
        
        case TOKEN_TYPE_OPEN_BRACE:
        {
            result = OPERATOR_OPEN_BRACE;
        } break;
        
        case TOKEN_TYPE_CLOSED_BRACE:
        {
            result = OPERATOR_CLOSED_BRACE;
        } break;
        
        case TOKEN_TYPE_COMMA:
        {
            result = OPERATOR_COMMA;
        } break;
        case TOKEN_TYPE_COLON:
        {
            result = OPERATOR_COLON;
        } break;
        
        case TOKEN_TYPE_BITWISE_OR:
        {
            result = OPERATOR_OR;
        } break;
        
        // Types
        case TOKEN_TYPE_RESOURCE_NAME:
        case TOKEN_TYPE_NAME:
        case TOKEN_TYPE_STRING:
        case TOKEN_TYPE_INT:
        case TOKEN_TYPE_FLOAT:
        {
            result = OPERATOR_NONE;
        } break;
    }
    
    return result;
}

file_internal void BuildSyntaxTree(TokenNode **iter, SyntaxNode **root)
{
    SyntaxNode **cursor = root;
    while (*iter && (*iter)->token.type != TOKEN_TYPE_NEWLINE)
    {
        Operator node_op = TokenTypeToOperator((*iter)->token.type);
        
        if ((*iter)->token.type == TOKEN_TYPE_OPEN_BRACE ||
            (*iter)->token.type == TOKEN_TYPE_OPEN_PARENTHESIS)
        {
            if (*cursor == nullptr)
            {
                (*cursor) = palloc<SyntaxNode>(1);
                CreateSyntaxNode((*cursor), (*iter)->token);
            }
            
            (*cursor)->left = palloc<SyntaxNode>(1);
            CreateSyntaxNode((*cursor)->left, (*iter)->token);
            (*cursor)->left->parent = (*cursor);
            
            (*cursor) = (*cursor)->left;
        }
        else if (node_op < OPERATOR_OPEN_PARENTHESIS)
        { // operator case
            // Current node is not set
            if (*cursor == nullptr)
            {
                // ERROR!
            }
            else
            {
                // by default NONE
                if ((*cursor)->op == OPERATOR_UNSET)
                {
                }
                else if ((*cursor)->op != OPERATOR_UNSET && (*cursor)->op >= node_op)
                {
                    // Lower precedence operator currently set
                    SyntaxNode *parent_node = palloc<SyntaxNode>(1);
                    CreateSyntaxNode(parent_node, (*iter)->token);
                    parent_node->parent = (*cursor)->parent;
                    
                    if ((*cursor)->parent)
                    {
                        if ((*cursor)->parent->left == (*cursor))
                        { // left child
                            (*cursor)->parent->left = parent_node;
                        }
                        else
                        { // right child
                            (*cursor)->parent->right = parent_node;
                        }
                    }
                    
                    (*cursor)->parent = parent_node;
                    
                    parent_node->left = (*cursor);
                    (*cursor) = parent_node;
                }
                else if ((*cursor)->op != OPERATOR_UNSET)
                { // Higher precedence currently set
                    SyntaxNode *right_node = palloc<SyntaxNode>(1);
                    CreateSyntaxNode(right_node, (*iter)->token);
                    right_node->parent = (*cursor);
                    
                    if ((*cursor)->right)
                    {
                        right_node->left = (*cursor)->right;
                        (*cursor)->right->parent = right_node;
                    }
                    
                    (*cursor)->right = right_node;
                    (*cursor) = right_node;
                }
                
                (*cursor)->op    = node_op;
                (*cursor)->token = (*iter)->token;
                (*cursor)->right = palloc<SyntaxNode>(1);
                CreateSyntaxNode((*cursor)->right, (*iter)->token);
                
                (*cursor)->right->parent = (*cursor);
                (*cursor) = (*cursor)->right;
            }
        }
        else if ((*iter)->token.type == TOKEN_TYPE_CLOSED_BRACE ||
                 (*iter)->token.type == TOKEN_TYPE_CLOSED_PARENTHESIS)
        {
            (*cursor) = (*cursor)->parent;
        }
        else
        { // not an operator
            if (*cursor == nullptr)
            {
                // Creates the root
                (*cursor) = palloc<SyntaxNode>(1);
                CreateSyntaxNode((*cursor), (*iter)->token);
                
                // Creates the left and moves to it
                (*cursor)->left = palloc<SyntaxNode>(1);
                CreateSyntaxNode((*cursor)->left, (*iter)->token);
                (*cursor)->left->parent = (*cursor);
                
                (*cursor) = (*cursor)->left;
            }
            
            // Sets the info at the left
            (*cursor)->token = (*iter)->token;
            (*cursor)->op    = node_op;
            
            // Create the right node and move to it
            (*cursor) = (*cursor)->parent;
        }
        
        *iter = (*iter)->next;
    }
}

file_internal void BuildParseTrees(TokenList *tokens, SyntaxTreeList *syntax_trees)
{
    u32 idx = 0;
    TokenNode *iter = tokens->head;
    
    while (iter)
    {
        while (iter && iter->token.type == TOKEN_TYPE_NEWLINE)
        {
            iter = iter->next;
        }
        
        if (iter)
        {
            SyntaxNode *current_tree = syntax_trees->head[idx];
            
            BuildSyntaxTree(&iter, &syntax_trees->head[idx]);
            
            ++idx;
            ++syntax_trees->size;
        }
    }
}

file_internal void ValidateParseTrees(SyntaxTreeList *syntax_trees)
{
}

inline size_t Align(size_t size)
{
    // Align a size request
    size_t shader_alignment = 16;
    return ((size + shader_alignment - 1) & ~(shader_alignment - 1));
}

file_internal void RecurseArray(SyntaxNode *root, PrimitiveType type, char *buffer, int *idx)
{
    if (!root->left && !root->right)
    {
        if (type == PRIMITIVE_TYPE_VEC3 || type == PRIMITIVE_TYPE_VEC4)
        {
            ((r32*)buffer)[*idx] = TokenToFloat(root->token);
        }
        else
        {
            ((int*)buffer)[*idx] = TokenToInt(root->token);
        }
        
        *idx += 1;
    }
    
    if (root->left)
    { // reached the left node
        RecurseArray(root->left, type, buffer, idx);
    }
    
    if (root->right)
    {
        RecurseArray(root->right, type, buffer, idx);
    }
}

// called when the root is assign and the child nodes contain a set or binding
file_internal void ParseUniformBlock(SyntaxTreeList *syntax_trees, int *tree_idx,
                                     MaterialInstance *instance)
{
    UniformBlock block = {};
    
    //~ First tree should be the set binding tree
    SyntaxNode *mroot = syntax_trees->head[*tree_idx];
    SyntaxNode *left_rt = mroot->left;
    SyntaxNode *right_rt = mroot->right;
    
    if (left_rt->op != OPERATOR_EQUALS || right_rt->op != OPERATOR_EQUALS)
    {
        printf("Uniform set or binding is not assigned to a number!\n");
        return;
    }
    
    // parse the left root
    SyntaxNode *left = left_rt->left;
    SyntaxNode *right = left_rt->right;
    
    jstring value_name;
    InitJString(&value_name, left->token.start, left->token.end - left->token.start);
    ToLowerCase(value_name);
    
    if (value_name == Names[RESERVED_SET])
    {
        block.Set = TokenToInt(right->token);
    }
    else if (value_name == Names[RESERVED_BINDING])
    {
        block.Binding = TokenToInt(right->token);
    }
    value_name.Clear();
    
    // parse the right root
    left  = right_rt->left;
    right = right_rt->right;
    
    value_name.Clear();
    InitJString(&value_name, left->token.start, left->token.end - left->token.start);
    ToLowerCase(value_name);
    
    if (value_name == Names[RESERVED_SET])
    {
        block.Set = TokenToInt(right->token);
    }
    else if (value_name == Names[RESERVED_BINDING])
    {
        block.Binding = TokenToInt(right->token);
    }
    value_name.Clear();
    
    
    //~ Determine the number of data segments and their types by traversing the list until
    // another OR operator is found
    ++(*tree_idx);
    
    int start_idx = *tree_idx;
    int stop_idx = -1;
    
    for (; *tree_idx < syntax_trees->size; ++(*tree_idx))
    {
        SyntaxNode *root = syntax_trees->head[*tree_idx];
        
        if (root->op == OPERATOR_OR)
        {
            stop_idx = *tree_idx;
            
            --(*tree_idx);
            break;
        }
    }
    
    if (*tree_idx >= syntax_trees->size)
    {
        stop_idx = *tree_idx;
    }
    
    //~ Create the data block and copy in the data using appropriate alignment
    struct UniDef
    {
        PrimitiveType type;
        size_t size;
        
        SyntaxNode *root; // root node for the data
    };
    
    u32 block_count = stop_idx - start_idx;
    UniDef *sizes = palloc<UniDef>(block_count);
    for (size_t i = 0; i < block_count; ++i)
    {
        sizes[i].type = PRIMITIVE_TYPE_UNKNOWN;
        sizes[i].size = 0;
    }
    
    // get the necessary size of the block definitions
    for (int idx = 0; idx < block_count; ++idx)
    {
        SyntaxNode *root = syntax_trees->head[idx + start_idx];
        left  = root->left;
        
        i32 uniform_offset_idx = TokenToInt(left->left->token);
        
        jstring type;
        InitJString(&type, left->right->token.start, left->right->token.end - left->right->token.start);
        ToLowerCase(type);
        
        if (type == "i")
        {
            sizes[uniform_offset_idx].type = PRIMITIVE_TYPE_INT;
            sizes[uniform_offset_idx].size = Align(sizeof(i32));
        }
        else if (type == "f")
        {
            sizes[uniform_offset_idx].type = PRIMITIVE_TYPE_FLOAT;
            sizes[uniform_offset_idx].size = Align(sizeof(r32));
        }
        else if (type == "iv3")
        {
            sizes[uniform_offset_idx].type = PRIMITIVE_TYPE_IVEC3;
            sizes[uniform_offset_idx].size = Align(3 * sizeof(i32));
        }
        else if (type == "v3")
        {
            sizes[uniform_offset_idx].type = PRIMITIVE_TYPE_VEC3;
            sizes[uniform_offset_idx].size = Align(3 * sizeof(r32));
        }
        else if (type == "iv4")
        {
            sizes[uniform_offset_idx].type = PRIMITIVE_TYPE_IVEC4;
            sizes[uniform_offset_idx].size = Align(4 * sizeof(i32));
        }
        else if (type == "v4")
        {
            sizes[uniform_offset_idx].type = PRIMITIVE_TYPE_VEC4;
            sizes[uniform_offset_idx].size = Align(4 * sizeof(r32));
        }
        
        else
        {
            printf("Invalid type provided for uniform block definition!\n");
            return;
        }
        
        sizes[uniform_offset_idx].root = root->right;
    }
    
    // make sure all indices are provided
    size_t total_sz = 0;
    for (int idx = 0; idx < block_count; ++idx)
    {
        if (sizes[idx].size == 0)
        {
            printf("A uniform block definition is missing: index %d\n", idx);
            return;
        }
        
        total_sz += sizes[idx].size;
    }
    
    
    block.Size   = total_sz;
    block.Memory = (char*)palloc(block.Size);
    
    size_t offset = 0;
    for (int idx = 0; idx < block_count; ++idx)
    {
        SyntaxNode *root = sizes[idx].root;
        
        offset += (idx > 0) ? sizes[idx - 1].size : 0;
        char *start_blk = block.Memory + offset;
        
        if (root->op == OPERATOR_COMMA)
        { // array type
            if (sizes[idx].type != PRIMITIVE_TYPE_VEC4 &&
                sizes[idx].type != PRIMITIVE_TYPE_VEC3 &&
                sizes[idx].type != PRIMITIVE_TYPE_IVEC4 &&
                sizes[idx].type != PRIMITIVE_TYPE_IVEC3)
            {
                printf("Primitive type does not match primitive values! Array type specified, but not given.\n");
                return;
            }
            
            RecurseArray(root, sizes[idx].type, start_blk, 0);
        }
        else
        { // simple type
            if (sizes[idx].type != PRIMITIVE_TYPE_INT &&
                sizes[idx].type != PRIMITIVE_TYPE_FLOAT)
            {
                printf("Primitive type does not match primitive values! Int or float type specified, but not given.\n");
                return;
            }
            
            if (sizes[idx].type == PRIMITIVE_TYPE_INT)
            {
                *start_blk = TokenToInt(root->token);
            }
            else
            {
                *start_blk = TokenToFloat(root->token);
            }
        }
    }
    
    pfree(sizes);
    
    instance->Blocks.PushBack(block);
}


file_internal void ParseModelFile(Model *model, SyntaxTreeList *syntax_trees)
{
    for (int i = 1; i < syntax_trees->size; ++i)
    {
        SyntaxNode *root = syntax_trees->head[i];
        
        if (root->op == OPERATOR_EQUALS)
        {
            SyntaxNode *left_rt  = root->left;
            SyntaxNode *right_rt = root->right;
            
            if (!left_rt)
            {
                printf("L-Value for assignment operator does not exist!\n");
            }
            else if (!right_rt)
            {
                printf("R-Value for assignment operator does not exist!\n");
            }
            else if (left_rt->op != OPERATOR_COLON)
            {
                printf("Assignment operator found, but the lvalue was not a variable type pair!\n");
            }
            else
            {
                jstring var_name;
                InitJString(&var_name, left_rt->left->token.start, left_rt->left->token.end - left_rt->left->token.start);
                ToLowerCase(var_name);
                
                PrimitiveType var_type = TokenToPrimitiveType(left_rt->right->token);
                
                if (var_type != PRIMITIVE_TYPE_UNKNOWN)
                {
                    if (var_type == PRIMITIVE_TYPE_INT || var_type == PRIMITIVE_TYPE_FLOAT)
                    {
                        printf("No model member that takes a variable of type int or float!\n");
                    }
                    else if (var_type == PRIMITIVE_TYPE_STRING)
                    {
                        jstring str;
                        InitJString(&str, right_rt->token.start,
                                    right_rt->token.end - right_rt->token.start);
                        if (var_name == Names[RESERVED_NAME])
                        {
                            model->Name = str;
                        }
                        else if (var_name == "mesh")
                        {
                            model->MeshFile = str;
                        }
                        else if (var_name == Names[RESERVED_MATERIAL])
                        {
                            model->Material = str;
                        }
                        else if (var_name == "materialinstance")
                        {
                            model->MaterialInstance = str;
                        }
                        else
                        {
                            printf("Model does not have the member \"%s\"!\n", str.GetCStr());
                        }
                    }
                    else
                    {
                        int idx = 0;
                        if (var_name == "rotation")
                        {
                            RecurseArray(right_rt, var_type, (char*)model->Rotation.data, &idx);
                        }
                        else if (var_name == "translation")
                        {
                            RecurseArray(right_rt, var_type, (char*)model->Translation.data, &idx);
                        }
                        else if (var_name == "scale")
                        {
                            RecurseArray(right_rt, var_type, (char*)model->Scale.data, &idx);
                        }
                        else
                        {
                            printf("Model does not have the member \"%s\"!\n", var_name.GetCStr());
                        }
                    }
                }
            }
        }
        else
        {
            printf("Unsupported root operator in Model Config File!\n");
        }
    }
}

file_internal void ParseSceneFile(Scene *scene, SyntaxTreeList *syntax_trees)
{
    for (int i = 1; i < syntax_trees->size; ++i)
    {
        SyntaxNode *root = syntax_trees->head[i];
        
        if (root->op == OPERATOR_EQUALS)
        {
            SyntaxNode *left_rt  = root->left;
            SyntaxNode *right_rt = root->right;
            
            if (!left_rt)
            {
                printf("L-Value for assignment operator does not exist!\n");
            }
            else if (!right_rt)
            {
                printf("R-Value for assignment operator does not exist!\n");
            }
            else if (left_rt->op != OPERATOR_COLON)
            {
                printf("Assignment operator found, but the lvalue was not a variable type pair!\n");
            }
            else
            {
                jstring var_name;
                InitJString(&var_name, left_rt->left->token.start, left_rt->left->token.end - left_rt->left->token.start);
                ToLowerCase(var_name);
                
                PrimitiveType var_type = TokenToPrimitiveType(left_rt->right->token);
                
                if (var_type != PRIMITIVE_TYPE_INT && var_type != PRIMITIVE_TYPE_STRING)
                {
                    printf("Scene member variables are Ints or Str!\n");
                }
                else if (var_type == PRIMITIVE_TYPE_STRING)
                {
                    jstring str;
                    InitJString(&str, right_rt->token.start,
                                right_rt->token.end - right_rt->token.start);
                    if (var_name == Names[RESERVED_NAME])
                    {
                        scene->Name = str;
                    }
                    else
                    {
                        printf("Scene does not have a string member \"%s\"!\n", var_name.GetCStr());
                    }
                }
                else
                { // var_type == PRIMITIVE_TYPE_INT
                    if (var_name == "modelcount")
                    {
                        scene->ModelCount = TokenToInt(right_rt->token);
                    }
                    else if (var_name == "materialcount")
                    {
                        scene->MaterialCount = TokenToInt(right_rt->token);
                    }
                    else if (var_name == "materialinstancecount")
                    {
                        scene->MaterialInstanceCount = TokenToInt(right_rt->token);
                    }
                    else
                    {
                        printf("Scene does not have integer member \"%s\"!\n", var_name.GetCStr());
                    }
                }
            }
        }
        else if (root->token.type == TOKEN_TYPE_RESOURCE_NAME)
        {
            Token resource_token  = root->token;
            jstring resource_name;
            InitJString(&resource_name, resource_token.start, resource_token.end - resource_token.start);
            ToLowerCase(resource_name);
            
            if (resource_name == Names[RESERVED_MODEL])
            {
                if (scene->ModelCount > 0)
                {
                    scene->Models.Resize(scene->ModelCount);
                    
                    ++i;
                    while (i < syntax_trees->size)
                    {
                        root = syntax_trees->head[i];
                        
                        if (root->token.type == TOKEN_TYPE_RESOURCE_NAME)
                        {
                            --i;
                            break;
                        }
                        
                        u64 result = StrToUInt64(root->token.start, root->token.end);
                        scene->Models.PushBack(result);
                        ++i;
                    }
                }
            }
            else if (resource_name == Names[RESERVED_MATERIAL])
            {
                if (scene->MaterialCount > 0)
                {
                    scene->Materials.Resize(scene->MaterialCount);
                    
                    ++i;
                    while (i < syntax_trees->size)
                    {
                        root = syntax_trees->head[i];
                        
                        if (root->token.type == TOKEN_TYPE_RESOURCE_NAME)
                        {
                            --i;
                            break;
                        }
                        
                        u64 result = StrToUInt64(root->token.start, root->token.end);
                        scene->Materials.PushBack(result);
                        ++i;
                    }
                }
            }
            else if (resource_name == Names[RESERVED_MATERIAL_INSTANCE])
            {
                if (scene->MaterialInstanceCount > 0)
                {
                    scene->MaterialInstances.Resize(scene->MaterialInstanceCount);
                    
                    ++i;
                    while (i < syntax_trees->size)
                    {
                        root = syntax_trees->head[i];
                        
                        if (root->token.type == TOKEN_TYPE_RESOURCE_NAME)
                        {
                            --i;
                            break;
                        }
                        
                        u64 result = StrToUInt64(root->token.start, root->token.end);
                        scene->MaterialInstances.PushBack(result);
                        ++i;
                    }
                }
            }
            else
            {
                printf("Unsupported Resource Name in the Scene Config File: \"%s\"!\n", resource_name.GetCStr());
            }
        }
        else
        {
            printf("Unsupported root operator in Scene Config File!\n");
        }
    }
}


file_internal void ParseMaterialInstanceFile(MaterialInstance *instance, SyntaxTreeList *syntax_trees)
{
    instance->ShaderType = -1;
    
    for (int i = 1; i < syntax_trees->size; ++i)
    {
        SyntaxNode *root = syntax_trees->head[i];
        
        if (root->op == OPERATOR_OR)
        {
            SyntaxNode *left_root = root->left;
            SyntaxNode *right_root = root->right;
            
            if (!root->left || !root->right)
            {
                printf("Set/Binding Start block found, but missing one of the names!\n");
                continue;
            }
            
            ParseUniformBlock(syntax_trees, &i, instance);
        }
        else if (root->op == OPERATOR_EQUALS)
        {
            SyntaxNode *left = root->left;
            SyntaxNode *right = root->right;
            
            if (!left)
            {
                printf("L-Value for assignment operator does not exist!\n");
            }
            else if (!right)
            {
                printf("R-Value for assignment operator does not exist!\n");
            }
            else if (left->token.type == TOKEN_TYPE_NAME)
            {
                //ParseUniformBlock(&root, &instance);
                Token token = root->left->token;
                Token rtoken = root->right->token;
                
                jstring lvalue_name;
                InitJString(&lvalue_name, token.start, token.end - token.start);
                ToLowerCase(lvalue_name);
                
                switch (lvalue_name[0])
                {
                    
                    case 'c':
                    { // clear coat textures: texture, rough, normal
                        if(lvalue_name == Names[RESERVED_CLEAR_COAT])
                        { // clear coat texture
                            InitJString(&instance->ClearCoatTexture, rtoken.start, rtoken.end - rtoken.start);
                        }
                        else if(lvalue_name == Names[RESERVED_CLEAR_COAT_ROUGHNESS])
                        { // clear coat metallic roughness texture
                            InitJString(&instance->ClearCoatRoughnessTexture, rtoken.start,
                                        rtoken.end - rtoken.start);
                        }
                        else if(lvalue_name == Names[RESERVED_CLEAR_COAT_NORMAL])
                        { // clear coat normal texture
                            InitJString(&instance->ClearCoatNormalTexture, rtoken.start,
                                        rtoken.end - rtoken.start);
                        }
                        else
                        {
                            printf("Unknown name type for material instance: %s\n", lvalue_name.GetCStr());
                            continue;
                        }
                    } break;
                    
                    case 'b':
                    { // base color, binding
                        if (lvalue_name == Names[RESERVED_BASE_COLOR])
                        { // base color texture
                            InitJString(&instance->BaseColorTexture, rtoken.start, rtoken.end - rtoken.start);
                        }
                        else
                        {
                            printf("Unknown name type for material instance: %s\n", lvalue_name.GetCStr());
                            continue;
                        }
                    } break;
                    
                    case 'd':
                    { // diffuse texture
                        if(lvalue_name == Names[RESERVED_DIFFUSE])
                        { // diffuse texture
                            InitJString(&instance->DiffuseTexture, rtoken.start, rtoken.end - rtoken.start);
                        }
                        else
                        {
                            printf("Unknown name type for material instance: %s\n", lvalue_name.GetCStr());
                            continue;
                        }
                    } break;
                    
                    case 'e':
                    { // emissive texture
                        if(lvalue_name == Names[RESERVED_EMISSIVE])
                        { // metallic roughness texture
                            InitJString(&instance->EmissiveTexture, rtoken.start, rtoken.end - rtoken.start);
                        }
                        else
                        {
                            printf("Unknown name type for material instance: %s\n", lvalue_name.GetCStr());
                            continue;
                        }
                    } break;
                    
                    case 'm':
                    { // metallic rough
                        if(lvalue_name == Names[RESERVED_METALLIC_ROUGHNESS])
                        { // metallic roughness texture
                            InitJString(&instance->MetallicRoughnessTexture, rtoken.start, rtoken.end - rtoken.start);
                        }
                        else if (lvalue_name == Names[RESERVED_MATERIAL_NAME])
                        {
                            InitJString(&instance->MaterialName, root->right->token.start,
                                        root->right->token.end - root->right->token.start);
                        }
                        else
                        {
                            printf("Unknown name type for material instance: %s\n", lvalue_name.GetCStr());
                            continue;
                        }
                        
                    } break;
                    
                    case 'n':
                    { // normal texture
                        if(lvalue_name == Names[RESERVED_NORMAL])
                        { // normal texture
                            InitJString(&instance->NormalTexture, rtoken.start, rtoken.end - rtoken.start);
                        }
                        else if(lvalue_name == Names[RESERVED_NAME])
                        { // name of material the instance is attached to
                            InitJString(&instance->Name, rtoken.start, rtoken.end - rtoken.start);
                        }
                        else
                        {
                            printf("Unknown name type for material instance: %s\n", lvalue_name.GetCStr());
                            continue;
                        }
                    } break;
                    
                    case 'o':
                    { // occlusion texture
                        if(lvalue_name == Names[RESERVED_OCCLUSION])
                        { // occlusion texture
                            InitJString(&instance->OcclusionTexture, rtoken.start, rtoken.end - rtoken.start);
                        }
                        else
                        {
                            printf("Unknown name type for material instance: %s\n", lvalue_name.GetCStr());
                            continue;
                        }
                    } break;
                    
                    case 's':
                    { // shader type, specular, set
                        switch (lvalue_name[1])
                        {
                            case 'p':
                            { // sepcular texture
                                if (lvalue_name == Names[RESERVED_SPECULAR])
                                {
                                    InitJString(&instance->SpecularTexture, rtoken.start, rtoken.end - rtoken.start);
                                }
                            } break;
                            
                            default:
                            {
                                printf("Unknown name type for material instance: %s\n", lvalue_name.GetCStr());
                                continue;
                            } break;
                        }
                    } break;
                    
                    case 'w':
                    {
                        if (lvalue_name == Names[RESERVED_WORKFLOW])
                        {
                            instance->ShaderType = TokenToInt(rtoken);
                        }
                    } break;
                    
                    default:
                    {
                        printf("Unknown name type for material instance: %s\n", lvalue_name.GetCStr());
                        continue;
                    } break;
                }
            }
            // name
            else
            {
                printf("Invalid token left of an assignement!\n");
            }
        }
        else
        {
            printf("Unsupported root operator in Material Instance file!\n");
        }
    }
}

file_internal void ParseMaterialFile(Material *material, SyntaxTreeList *syntax_trees)
{
    for (int i = 1; i < syntax_trees->size; ++i)
    {
        SyntaxNode *root = syntax_trees->head[i];
        
        if (root->op != OPERATOR_EQUALS)
        {
            printf("Invalid expression in Material File\n");
            
            jstring root_name = TokenTypeToName(root->token.type);
            jstring root_value;
            InitJString(&root_value, root->token.start,
                        root->token.end - root->token.start);
            
            printf("\tOperator: %s\n", root_name.GetCStr());
            printf("\tValue: %s\n",    root_value.GetCStr());
        }
        else
        {
            
            if (!root->left)
            {
                printf("L-Value for assignment operator does not exist!\n");
            }
            else if (!root->right)
            {
                printf("R-Value for assignment operator does not exist!\n");
            }
            else
            {
                Token token = root->left->token;
                
                if (token.type != TOKEN_TYPE_NAME)
                {
                    printf("Left of assignment operator is not a name!\n");
                    
                    jstring root_name = TokenTypeToName(root->token.type);
                    printf("\tOperator: %s\n", root_name.GetCStr());
                }
                else
                {
                    jstring lvalue_name;
                    InitJString(&lvalue_name, token.start, token.end - token.start);
                    ToLowerCase(lvalue_name);
                    
                    switch (lvalue_name[0])
                    {
                        case 'f':
                        {
                            if (lvalue_name == Names[RESERVED_FRAGMENT])
                            {
                                InitJString(&material->Fragment, root->right->token.start,
                                            root->right->token.end - root->right->token.start);
                            }
                            else
                            {
                                printf("Unknown rvalue name: %s\n", lvalue_name.GetCStr());
                            }
                        } break;
                        
                        case 'g':
                        {
                            if (lvalue_name == Names[RESERVED_GEOMETRY])
                            {
                                InitJString(&material->Geometry, root->right->token.start,
                                            root->right->token.end - root->right->token.start);
                            }
                            else
                            {
                                printf("Unknown rvalue name: %s\n", lvalue_name.GetCStr());
                            }
                        } break;
                        
                        case 'n':
                        {
                            if (lvalue_name == Names[RESERVED_NAME])
                            {
                                InitJString(&material->Name, root->right->token.start,
                                            root->right->token.end - root->right->token.start);
                            }
                            else
                            {
                                printf("Unknown rvalue name: %s\n", lvalue_name.GetCStr());
                            }
                        } break;
                        
                        case 'r':
                        {
                            if (lvalue_name == Names[RESERVED_REFLECTION])
                            {
                                InitJString(&material->ReflectionFile, root->right->token.start,
                                            root->right->token.end - root->right->token.start);
                            }
                            else
                            {
                                printf("Unknown rvalue name: %s\n", lvalue_name.GetCStr());
                            }
                        } break;
                        
                        case 't':
                        { // tess ctrl or tess eval
                            if (lvalue_name == Names[RESERVED_TESSELATION_CONTROL])
                            {
                                InitJString(&material->TesselationControl, root->right->token.start,
                                            root->right->token.end - root->right->token.start);
                            }
                            else if (lvalue_name == Names[RESERVED_TESSELATION_EVALUATION])
                            {
                                InitJString(&material->TesselationEvaluation, root->right->token.start,
                                            root->right->token.end - root->right->token.start);
                            }
                            else
                            {
                                printf("Unknown rvalue name: %s\n", lvalue_name.GetCStr());
                            }
                        } break;
                        
                        case 'v':
                        {
                            if (lvalue_name == Names[RESERVED_VERTEX])
                            {
                                InitJString(&material->Vertex, root->right->token.start,
                                            root->right->token.end - root->right->token.start);
                            }
                            else
                            {
                                printf("Unknown rvalue name: %s\n", lvalue_name.GetCStr());
                            }
                        } break;
                    }
                }
            }
        }
    }
}

/*

Resource Type: { NAME }
- Start Resource Type

Named Token: name = string
- Name : string identifier


Block Token ( name = int, name = int )

SubBlock Token [ int : type = values,values, ... ]

RT -> Named Tokes -> Block Token -> Subblock Token

Types of Resources
- Material
- Material Instance

TODO(Dustin):
- Scene
- Models

*/

void InitializeParser()
{ // TODO(Dustin): Pre-compute these values
    for (int i = 0; i < ReservedNameCount; ++i)
    {
        HashedNames[i] = hash_char_array(Names[i], strlen(Names[i]));
    }
}

void ShutdownParser()
{
    /*
for (int i = 0; i < ReservedNameCount; ++i)
    {
        Names[i].Clear();
    }
*/
}

file_internal SyntaxTreeList ParseResourceFile(jstring &file)
{
    Scanner scanner = {};
    scanner.buffer      = (file.heap) ? file.hptr : file.sptr;
    scanner.buffer_size = file.len;
    scanner.line_count  = 0;
    scanner.current     = scanner.buffer;
    
    
    // Pass 1 Tokenize the resource File
    TokenList tokens = Tokenizer(&scanner);
    
    // Pass 2.1 Build parse tree from the token
    SyntaxTreeList syntax_trees = {};
    syntax_trees.cap  = scanner.line_count + 1;
    syntax_trees.head = palloc<SyntaxNode*>(syntax_trees.cap);
    syntax_trees.size = 0;
    
    SyntaxNode *iter = nullptr;
    for (u32 i = 0; i < syntax_trees.cap; ++i)
    {
        syntax_trees.head[i] = nullptr;
    }
    
    BuildParseTrees(&tokens, &syntax_trees);
    
    // 4.1 Determine the type of resource
    return syntax_trees;
}

void LoadMaterialFromFile(Material *material, jstring &file)
{
    SyntaxTreeList syntax_trees = ParseResourceFile(file);
    
    
    if (syntax_trees.head[0]->token.type == TOKEN_TYPE_RESOURCE_NAME)
    {
        Token resource_token  = syntax_trees.head[0]->token;
        jstring resource_name;
        InitJString(&resource_name, resource_token.start, resource_token.end - resource_token.start);
        ToLowerCase(resource_name);
        
        switch(resource_name[0])
        {
            case 'm':
            {
                // material or instance
                if (resource_name == Names[RESERVED_MATERIAL])
                {
                    ParseMaterialFile(material, &syntax_trees);
                }
                else if (resource_name == Names[RESERVED_MATERIAL_INSTANCE])
                {
                    
                }
                else
                {
                    printf("Unknown Resource Name: %s\n", resource_name.GetCStr());
                }
            } break;
            
            default: printf("Unknown Resource Name: %s\n", resource_name.GetCStr());
        }
    }
    else
    {
        printf("First line in the file was not a resource type!\n");
        printf("Acceptable types are:\n");
        printf("\tScene\n");
        printf("\tModel\n");
        printf("\tMaterial\n");
        printf("\tMaterial Instance\n");
    }
}


void LoadMaterialInstanceFromFile(MaterialInstance *instance, jstring &file)
{
    SyntaxTreeList syntax_trees = ParseResourceFile(file);
    
    if (syntax_trees.head[0]->token.type == TOKEN_TYPE_RESOURCE_NAME)
    {
        Token resource_token  = syntax_trees.head[0]->token;
        jstring resource_name;
        InitJString(&resource_name, resource_token.start, resource_token.end - resource_token.start);
        ToLowerCase(resource_name);
        
        switch(resource_name[0])
        {
            case 'm':
            {
                // material or instance
                if (resource_name == Names[RESERVED_MATERIAL_INSTANCE])
                {
                    ParseMaterialInstanceFile(instance, &syntax_trees);
                }
                else
                {
                    printf("Not a valid material instance file: %s\n", resource_name.GetCStr());
                }
            } break;
            
            default: printf("Unknown Resource Name: %s\n", resource_name.GetCStr());
        }
    }
    else
    {
        printf("First line in the file was not a resource type!\n");
        printf("Acceptable types are:\n");
        printf("\tScene\n");
        printf("\tModel\n");
        printf("\tMaterial\n");
        printf("\tMaterial Instance\n");
    }
}

void LoadModelFromFile(Model *model, jstring &file)
{
    SyntaxTreeList syntax_trees = ParseResourceFile(file);
    
    if (syntax_trees.head[0]->token.type == TOKEN_TYPE_RESOURCE_NAME)
    {
        Token resource_token  = syntax_trees.head[0]->token;
        jstring resource_name;
        InitJString(&resource_name, resource_token.start, resource_token.end - resource_token.start);
        ToLowerCase(resource_name);
        
        switch(resource_name[0])
        {
            case 'm':
            {
                // material or instance
                if (resource_name == Names[RESERVED_MODEL])
                {
                    ParseModelFile(model, &syntax_trees);
                }
                else
                {
                    printf("Not a valid model file: %s\n", resource_name.GetCStr());
                }
            } break;
            
            default: printf("Unknown Resource Name: %s\n", resource_name.GetCStr());
        }
    }
    else
    {
        printf("First line in the file was not a model resource type!\n");
        printf("Acceptable types are:\n");
        printf("\tScene\n");
        printf("\tModel\n");
        printf("\tMaterial\n");
        printf("\tMaterial Instance\n");
    }
    
}

void LoadSceneFromFile(Scene *scene, jstring &file)
{
    SyntaxTreeList syntax_trees = ParseResourceFile(file);
    
    if (syntax_trees.head[0]->token.type == TOKEN_TYPE_RESOURCE_NAME)
    {
        Token resource_token  = syntax_trees.head[0]->token;
        jstring resource_name;
        InitJString(&resource_name, resource_token.start, resource_token.end - resource_token.start);
        ToLowerCase(resource_name);
        
        switch(resource_name[0])
        {
            case 's':
            {
                // material or instance
                if (resource_name == Names[RESERVED_SCENE])
                {
                    ParseSceneFile(scene, &syntax_trees);
                }
                else
                {
                    printf("Not a valid scene file: %s\n", resource_name.GetCStr());
                }
            } break;
            
            default: printf("Unknown Resource Name: %s\n", resource_name.GetCStr());
        }
    }
    else
    {
        printf("First line in the file was not a resource type!\n");
        printf("Acceptable types are:\n");
        printf("\tScene\n");
        printf("\tModel\n");
        printf("\tMaterial\n");
        printf("\tMaterial Instance\n");
    }
}

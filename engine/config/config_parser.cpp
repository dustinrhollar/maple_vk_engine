
//~ Config Member HashTable
void InitConfigMemberTable(config_member_table *Table)
{
    Table->LoadFactor = 0.70;
    Table->Count = 0;
    Table->Cap = 5;
    
    Table->Entries = palloc<config_member_table_entry>(Table->Cap);
    for (u32 entry = 0; entry < Table->Cap; ++entry)
        Table->Entries[entry].IsEmpty = true;
}

void FreeConfigMemberTable(config_member_table *Table)
{
    Table->LoadFactor = 0.0f;
    Table->Count      = 0;
    Table->Cap        = 0;
    
    pfree(Table->Entries);
    Table->Entries    = nullptr;
}

file_internal config_member_table_entry* ConfigMemberTableFindEntry(config_member_table *Table, u128 HashedKey)
{
    // use lower 64 bits as the index into the entry array
    u64 Index = (abs(HashedKey.lower)) % Table->Cap;
    config_member_table_entry *Entry = nullptr;
    for (;;)
    {
        Entry = &Table->Entries[Index];
        u128 EntryHash = Hash<jstring>(Entry->Key);
        
        
        if (Entry->IsEmpty || // index is not occupies
            (EntryHash.upper == HashedKey.upper && // index is occupied, but
             EntryHash.lower == HashedKey.lower))  // elements contain same hash
        {
            return Entry;
        }
        
        Index = (Index + 1) % Table->Cap;
    }
}

void ConfigMemberTableInsert(config_member_table *Table, jstring Key, config_var Var)
{
    // Do we need to resize?
    u32 max_allowed_entry = Table->Cap * Table->LoadFactor;
    if (Table->Count + 1 > max_allowed_entry)
    {
        ConfigMemberTableResize(Table, Table->Cap * 2);
    }
    
    u128 HashedKey = Hash<jstring>(Key);
    config_member_table_entry *Entry = ConfigMemberTableFindEntry(Table, HashedKey);
    
    if (Entry->IsEmpty)
    {
        Entry->IsEmpty   = false;
        Entry->Key       = Key;
        Entry->Value     = Var;
        
        Table->Count++;
    }
    else
    {
        mprinte("Key \"%s\" is already in the entry table!\n", Key.GetCStr());
    }
}

config_var ConfigMemberTableGet(config_member_table *Table, jstring Key)
{
    config_var result = {};
    result.Type = Config_Unknown;
    
    u128 HashedKey = Hash<jstring>(Key);
    config_member_table_entry *Entry = ConfigMemberTableFindEntry(Table, HashedKey);
    
    if (!Entry->IsEmpty)
    {
        return Entry->Value;
    }
    else
    {
        mprinte("Key \"%s\" does not exist in the table!\n", Key.GetCStr());
    }
    
    return result;
}
void ConfigMemberTableResize(config_member_table *Table, u32 Cap)
{
    u32 OldCap = Table->Cap;
    config_member_table_entry *OldEntries = Table->Entries;
    
    // allocate new entry storage and zero the mem
    Table->Entries = palloc<config_member_table_entry>(Cap);
    for (u32 entry = 0; entry < Table->Cap; ++entry)
        Table->Entries[entry].IsEmpty = true;
    
    for (u32 Entry = 0; Entry < OldCap; ++Entry)
    {
        ConfigMemberTableInsert(Table, OldEntries[Entry].Key, OldEntries[Entry].Value);
    }
    
    pfree(OldEntries);
}


//~ Config Obj HashTable

void InitConfigObjTable(config_obj_table *Table)
{
    Table->LoadFactor = 0.70;
    Table->Count = 0;
    Table->Cap = 5;
    
    Table->Entries = palloc<config_obj_table_entry>(Table->Cap);
    for (u32 entry = 0; entry < Table->Cap; ++entry)
        Table->Entries[entry].IsEmpty = true;
}

void FreeConfigObjTable(config_obj_table *Table)
{
    Table->LoadFactor = 0.0f;
    Table->Count      = 0;
    Table->Cap        = 0;
    
    pfree(Table->Entries);
    Table->Entries    = nullptr;
}

file_internal config_obj_table_entry* ConfigObjTableFindEntry(config_obj_table *Table, u128 HashedKey)
{
    // use lower 64 bits as the index into the entry array
    u64 Index = (abs(HashedKey.lower)) % Table->Cap;
    config_obj_table_entry *Entry = nullptr;
    for (;;)
    {
        Entry = &Table->Entries[Index];
        u128 EntryHash = Hash<jstring>(Entry->Key);
        
        
        if (Entry->IsEmpty || // index is not occupies
            (EntryHash.upper == HashedKey.upper && // index is occupied, but
             EntryHash.lower == HashedKey.lower))  // elements contain same hash
        {
            return Entry;
        }
        
        Index = (Index + 1) % Table->Cap;
    }
}

void ConfigObjTableInsert(config_obj_table *Table, jstring Key, config_obj Var)
{
    // Do we need to resize?
    u32 max_allowed_entry = Table->Cap * Table->LoadFactor;
    if (Table->Count + 1 > max_allowed_entry)
    {
        ConfigObjTableResize(Table, Table->Cap * 2);
    }
    
    u128 HashedKey = Hash<jstring>(Key);
    config_obj_table_entry *Entry = ConfigObjTableFindEntry(Table, HashedKey);
    
    if (Entry->IsEmpty)
    {
        Entry->IsEmpty   = false;
        Entry->Key       = Key;
        Entry->Value     = Var;
        
        Table->Count++;
    }
    else
    {
        mprinte("Key \"%s\" is already in the entry table!\n", Key.GetCStr());
    }
}

config_obj ConfigObjTableGet(config_obj_table *Table, jstring Key)
{
    config_obj result = {};
    
    u128 HashedKey = Hash<jstring>(Key);
    config_obj_table_entry *Entry = ConfigObjTableFindEntry(Table, HashedKey);
    
    if (!Entry->IsEmpty)
    {
        return Entry->Value;
    }
    else
    {
        mprinte("Key \"%s\" does not exist in the table!\n", Key.GetCStr());
    }
    
    return result;
}
void ConfigObjTableResize(config_obj_table *Table, u32 Cap)
{
    u32 OldCap = Table->Cap;
    config_obj_table_entry *OldEntries = Table->Entries;
    
    // allocate new entry storage and zero the mem
    Table->Entries = palloc<config_obj_table_entry>(Cap);
    for (u32 entry = 0; entry < Table->Cap; ++entry)
        Table->Entries[entry].IsEmpty = true;
    
    for (u32 Entry = 0; Entry < OldCap; ++Entry)
    {
        ConfigObjTableInsert(Table, OldEntries[Entry].Key, OldEntries[Entry].Value);
    }
    
    pfree(OldEntries);
}


//~ Config Obj Implementation

void InitConfigObj(config_obj *Obj, const char *Name, u32 NameLen)
{
    Obj->Name = InitJString(Name, NameLen);
    InitConfigMemberTable(&Obj->ObjMembers);
}

void FreeConfigObj(config_obj *Obj)
{
    Obj->Name.Clear();
    FreeConfigMemberTable(&Obj->ObjMembers);
}


//~ Config Variable Implementation

void InitConfigVar(config_var *Var, const char *Name, u32 NameLen,
                   config_primitive_type Type, const char *Data)
{
    Var->Name = InitJString(Name, NameLen);
    Var->Type = Type;
    // TODO(Dustin): Fill out the data portion
    switch (Var->Type)
    {
        case Config_I32:
        case Config_I64:
        case Config_U32:
        case Config_U64:
        case Config_R32:
        case Config_IVec2:
        case Config_IVec3:
        case Config_IVec4:
        case Config_Vec2:
        case Config_Vec3:
        case Config_Vec4:
        case Config_Str:
        
        // NOTE(Dustin): These will require some special handling...
        case Config_Obj:
        case Config_Unknown:
        default:
        {
            mprinte("Unknown config primitive type!\n");
        } break;
    }
}

void FreeConfigVar(config_var *Var)
{
    Var->Name.Clear();
    Var->Type = Config_Unknown;
    Var->Object = {0};
}

//~ Tokenizer

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

inline bool IsChar(char ch)
{
    return isalpha(ch);
}

inline bool IsDigit(char ch)
{
    return isdigit(ch);
}

file_internal token ConsumeNextToken(mscanner *Scanner)
{
    token Token = {};
    Token.Type  = Token_Invalid;
    Token.Len   = 0;
    
    while (IsSkippableChar(Scanner->Current)) Scanner->Current++;
    
    if (IsChar(Scanner->Current[0]) ||
        Scanner->Current[0] == '_')
    {
        // named Token
        Token.Type  = Token_Name;
        Token.Start = Scanner->Current;
        
        Token.Len   = Token.Len + 1;
        while (IsChar(Token.Start[Token.Len-1]) ||
               IsDigit(Token.Start[Token.Len-1]) ||
               Token.Start[Token.Len-1] == '_') Token.Len++;
        
        Scanner->Current = Token.Start + Token.Len;
    }
    else if (IsDigit(Scanner->Current[0]))
    {
        Token.Type  = Token_Int;
        Token.Start = Scanner->Current;
        
        Token.Len   = 1;
        while (IsDigit(Token.Start[Token.Len-1]) ||
               Token.Start[Token.Len-1] == '.')
        {
            if (Token.Start[Token.Len-1] == '.')
            {
                Token.Type  = Token_Float;
            }
            
            Token.Len++;
        }
        Token.Len--;
        
        Scanner->Current = Token.Start + Token.Len;
    }
    else {
        switch (Scanner->Current[0])
        {
            case '{':
            {
                Token.Type  = Token_Object;
                Token.Start = Scanner->Current + 1;
                
                if (Token.Start[0] == '}')
                {
                    Token.Len = 0;
                }
                else
                {
                    Token.Len = 1;
                    while (Token.Start[Token.Len] != '}') ++Token.Len;
                    
                    Scanner->Current = Token.Start + Token.Len + 1;
                }
            } break;
            
            case '|':
            {
                Token.Type  = Token_BitwiseOr;
                Token.Start = Scanner->Current;
                Token.Len   = 1;
                
                ++Scanner->Current;
            } break;
            
            case '\n':
            {
                Token.Type  = Token_Newline;
                Token.Start = Scanner->Current;
                Token.Len   = 1;
                
                Scanner->LineCount++;
                ++Scanner->Current;
            } break;
            
            case '=':
            {
                Token.Type  = Token_Equals;
                Token.Start = Scanner->Current;
                Token.Len   = 1;
                
                ++Scanner->Current;
            } break;
            
            case '(':
            {
                Token.Type  = Token_OpenParenthesis;
                Token.Start = Scanner->Current;
                Token.Len   = 1;
                
                ++Scanner->Current;
            } break;
            
            case ')':
            {
                Token.Type  = Token_ClosedParenthesis;
                Token.Start = Scanner->Current;
                Token.Len   = 1;
                
                ++Scanner->Current;
            } break;
            
            case '[':
            {
                Token.Type  = Token_OpenBrace;
                Token.Start = Scanner->Current;
                Token.Len   = 1;
                
                ++Scanner->Current;
            } break;
            
            case ']':
            {
                Token.Type  = Token_ClosedBrace;
                Token.Start = Scanner->Current;
                Token.Len   = 1;
                
                ++Scanner->Current;
            } break;
            
            case ',':
            {
                Token.Type  = Token_Comma;
                Token.Start = Scanner->Current;
                Token.Len   = 1;
                
                ++Scanner->Current;
            } break;
            
            case ':':
            {
                Token.Type  = Token_Colon;
                Token.Start = Scanner->Current;
                Token.Len   = 1;
                
                ++Scanner->Current;
            } break;
            
            case '\"':
            { // string Token
                Token.Type  = Token_String;
                Token.Start = ++Scanner->Current;
                
                if (Token.Start[0] == '\"')
                { // ""
                    Token.Len = 0;
                }
                else {
                    Token.Len   = 1;
                    while (Token.Start[Token.Len-1] != '\"')
                    {
                        ++Token.Len;
                    }
                    
                    --Token.Len;
                }
                
                Scanner->Current = Token.Start + Token.Len + 1;
            } break;
            
            default: break;
        }
    }
    
    return Token;
}

file_internal token_list Tokenizer(mscanner *Scanner)
{
    token_list Tokens = {};
    Tokens.Head = nullptr;
    Tokens.Tail = Tokens.Head;
    
    token Token = ConsumeNextToken(Scanner);
    while (Token.Type != Token_Invalid)
    {
        token_node *Node = palloc<token_node>(1);
        Node->Token = Token;
        Node->Next  = nullptr;
        
        if (!Tokens.Head)
        {
            Tokens.Head = Node;
            Tokens.Tail = Tokens.Head;
        }
        else
        {
            Tokens.Tail->Next = Node;
            Tokens.Tail = Node;
        }
        
        Token = ConsumeNextToken(Scanner);
    }
    
    return Tokens;
}

//~ Parse Tree Gen

file_internal config_operator TokenTypeToOperator(token_type type)
{
    config_operator result = Operator_Unset;
    
    switch (type)
    {
        case Token_Equals:
        {
            result = Operator_Equals;
        } break;
        
        case Token_OpenParenthesis:
        {
            result = Operator_OpenParenthesis;
        } break;
        
        case Token_ClosedParenthesis:
        {
            result = Operator_ClosedParenthesis;
        } break;
        
        case Token_OpenBrace:
        {
            result = Operator_OpenBrace;
        } break;
        
        case Token_ClosedBrace:
        {
            result = Operator_ClosedBrace;
        } break;
        
        case Token_Comma:
        {
            result = Operator_Comma;
        } break;
        case Token_Colon:
        {
            result = Operator_Colon;
        } break;
        
        case Token_BitwiseOr:
        {
            result = Operator_Or;
        } break;
        
        // Types
        case Token_Object:
        case Token_Name:
        case Token_String:
        case Token_Int:
        case Token_Float:
        {
            result = Operator_None;
        } break;
    }
    
    return result;
}


file_internal void CreateSyntaxNode(syntax_node *Node, token Token)
{
    Node->Token  = Token;
    Node->Left   = nullptr;
    Node->Right  = nullptr;
    Node->Parent = nullptr;
    Node->Op     = Operator_Unset;
}

file_internal void BuildSyntaxTree(token_node **Iter, syntax_node **Root)
{
    syntax_node **Cursor = Root;
    while (*Iter && (*Iter)->Token.Type != Token_Newline)
    {
        config_operator NodeOp = TokenTypeToOperator((*Iter)->Token.Type);
        
        if ((*Iter)->Token.Type == Token_OpenBrace ||
            (*Iter)->Token.Type == Token_OpenParenthesis)
        {
            if (*Cursor == nullptr)
            {
                (*Cursor) = palloc<syntax_node>(1);
                CreateSyntaxNode((*Cursor), (*Iter)->Token);
            }
            
            (*Cursor)->Left = palloc<syntax_node>(1);
            CreateSyntaxNode((*Cursor)->Left, (*Iter)->Token);
            (*Cursor)->Left->Parent = (*Cursor);
            
            (*Cursor) = (*Cursor)->Left;
        }
        else if ((*Iter)->Token.Type == Token_Object)
        { // special case: token object
            syntax_node *ObjNode = palloc<syntax_node>(1);
            CreateSyntaxNode(ObjNode, (*Iter)->Token);
            *Cursor = ObjNode;
            
            *Iter = (*Iter)->Next;
            break;
        }
        else if (NodeOp < Operator_OpenParenthesis)
        { // operator case
            // Current node is not set
            if (*Cursor == nullptr)
            {
                // ERROR!
                mprinte("Building a syntax tree and Cursor is not set!\n");
            }
            else
            {
                // by default NONE
                if ((*Cursor)->Op == Operator_Unset)
                {
                }
                else if ((*Cursor)->Op != Operator_Unset && (*Cursor)->Op >= NodeOp)
                {
                    // Lower precedence operator currently set
                    syntax_node *ParentNode = palloc<syntax_node>(1);
                    CreateSyntaxNode(ParentNode, (*Iter)->Token);
                    ParentNode->Parent = (*Cursor)->Parent;
                    
                    if ((*Cursor)->Parent)
                    {
                        if ((*Cursor)->Parent->Left == (*Cursor))
                        { // Left child
                            (*Cursor)->Parent->Left = ParentNode;
                        }
                        else
                        { // Right child
                            (*Cursor)->Parent->Right = ParentNode;
                        }
                    }
                    
                    (*Cursor)->Parent = ParentNode;
                    
                    ParentNode->Left = (*Cursor);
                    (*Cursor) = ParentNode;
                }
                else if ((*Cursor)->Op != Operator_Unset)
                { // Higher precedence currently set
                    syntax_node *RightNode = palloc<syntax_node>(1);
                    CreateSyntaxNode(RightNode, (*Iter)->Token);
                    RightNode->Parent = (*Cursor);
                    
                    if ((*Cursor)->Right)
                    {
                        RightNode->Left = (*Cursor)->Right;
                        (*Cursor)->Right->Parent = RightNode;
                    }
                    
                    (*Cursor)->Right = RightNode;
                    (*Cursor) = RightNode;
                }
                
                (*Cursor)->Op    = NodeOp;
                (*Cursor)->Token = (*Iter)->Token;
                (*Cursor)->Right = palloc<syntax_node>(1);
                CreateSyntaxNode((*Cursor)->Right, (*Iter)->Token);
                
                (*Cursor)->Right->Parent = (*Cursor);
                (*Cursor) = (*Cursor)->Right;
            }
        }
        else if ((*Iter)->Token.Type == Token_ClosedBrace ||
                 (*Iter)->Token.Type == Token_ClosedParenthesis)
        {
            (*Cursor) = (*Cursor)->Parent;
        }
        else
        { // not an operator
            if (*Cursor == nullptr)
            {
                // Creates the Root
                (*Cursor) = palloc<syntax_node>(1);
                CreateSyntaxNode((*Cursor), (*Iter)->Token);
                
                // Creates the Left and moves to it
                (*Cursor)->Left = palloc<syntax_node>(1);
                CreateSyntaxNode((*Cursor)->Left, (*Iter)->Token);
                (*Cursor)->Left->Parent = (*Cursor);
                
                (*Cursor) = (*Cursor)->Left;
            }
            
            // Sets the info at the Left
            (*Cursor)->Token = (*Iter)->Token;
            (*Cursor)->Op    = NodeOp;
            
            // Create the Right node and move to it
            (*Cursor) = (*Cursor)->Parent;
        }
        
        *Iter = (*Iter)->Next;
    }
}

file_internal void BuildParseTrees(token_list *Tokens, syntax_tree_list *SyntaxTrees)
{
    u32 idx = 0;
    token_node *Iter = Tokens->Head;
    
    while (Iter)
    {
        while (Iter && Iter->Token.Type == Token_Newline)
        {
            Iter = Iter->Next;
        }
        
        if (Iter)
        {
            BuildSyntaxTree(&Iter, &SyntaxTrees->Head[idx]);
            
            ++idx;
            ++SyntaxTrees->Size;
        }
    }
}

config_primitive_type TokenToPrimitiveType(token Token)
{
    config_primitive_type Result;
    jstring Type = InitJString(Token.Start, Token.Len);
    
    if (Type == "i32")
    {
        Result = Config_I32;
    }
    else if (Type == "i64")
    {
        Result = Config_I64;
    }
    else if (Type == "u32")
    {
        Result = Config_U32;
    }
    else if (Type == "u64")
    {
        Result = Config_U64;
    }
    else if (Type == "r32")
    {
        Result = Config_R32;
    }
    else if (Type == "ivec2")
    {
        Result = Config_IVec2;
    }
    else if (Type == "ivec3")
    {
        Result = Config_IVec3;
    }
    else if (Type == "ivec4")
    {
        Result = Config_IVec4;
    }
    else if (Type == "vec2")
    {
        Result = Config_Vec2;
    }
    else if (Type == "vec3")
    {
        Result = Config_Vec3;
    }
    else if (Type == "vec4")
    {
        Result = Config_Vec4;
    }
    else if (Type == "str")
    {
        Result = Config_Str;
    }
    else
    {
        Result = Config_Unknown;
    }
    
    Type.Clear();
    return Result;
}

file_internal void RecurseIVecArray(syntax_node *Root, config_primitive_type Type,
                                    char *buffer, int *idx)
{
    if (!Root->Left && !Root->Right)
    {
        ((int*)buffer)[*idx] = StrToInt(Root->Token.Start, Root->Token.Start + Root->Token.Len);
        
        *idx += 1;
    }
    
    if (Root->Left)
    { // reached the Left node
        RecurseIVecArray(Root->Left, Type, buffer, idx);
    }
    
    if (Root->Right)
    {
        RecurseIVecArray(Root->Right, Type, buffer, idx);
    }
}

file_internal void RecurseVecArray(syntax_node *Root, config_primitive_type Type,
                                   char *buffer, int *idx)
{
    if (!Root->Left && !Root->Right)
    {
        ((r32*)buffer)[*idx] = StrToR32(Root->Token.Start);
        
        *idx += 1;
    }
    
    if (Root->Left)
    { // reached the Left node
        RecurseVecArray(Root->Left, Type, buffer, idx);
    }
    
    if (Root->Right)
    {
        RecurseVecArray(Root->Right, Type, buffer, idx);
    }
}

file_internal void BuildObjMemberTable(config_obj *Obj, syntax_tree_list *SyntaxTrees, u32 *TreeIdx)
{
    while (*TreeIdx < SyntaxTrees->Size &&
           SyntaxTrees->Head[*TreeIdx]->Token.Type != Token_Object)
    {
        syntax_node *Root = SyntaxTrees->Head[*TreeIdx];
        
        if (Root->Op != Operator_Equals)
        {
            mprinte("Invalid expression. Variable declarion does not follow the syntax: \"name : type = def\"!\n");
            break;
        }
        
        syntax_node *LeftRootNode  = Root->Left;
        syntax_node *RightRootNode = Root->Right;
        
        if (!LeftRootNode || LeftRootNode->Op != Operator_Colon)
        {
            mprinte("Invalid expression. Variable declarion does not follow the syntax: \"name : type = def\"!\n");
            break;
        }
        
        if (!LeftRootNode->Left || !LeftRootNode->Right)
        {
            mprinte("Invalid expression. Variable declarion does not follow the syntax: \"name : type = def\"!\n");
            break;
        }
        
        syntax_node *VarNameNode = LeftRootNode->Left;
        syntax_node *VarTypeNode = LeftRootNode->Right;
        
        jstring MemberName = InitJString(VarNameNode->Token.Start, VarNameNode->Token.Len);
        config_primitive_type VarType = TokenToPrimitiveType(VarTypeNode->Token);
        
        if (VarType == Config_Unknown)
        {
            mprinte("Invalid expression. Variable is not a valid type!\n");
            break;
        }
        
        config_var Var = {};
        if (VarType == Config_I32)
        {
            i32 Data = StrToInt(Root->Token.Start, Root->Token.Start + Root->Token.Len);
            
            InitConfigVar(&Var, VarNameNode->Token.Start, VarNameNode->Token.Len,
                          VarType, (char*)&Data);
        }
        else if (VarType == Config_I64)
        {
            i64 Data = StrToInt64(Root->Token.Start, Root->Token.Start + Root->Token.Len);
            InitConfigVar(&Var, VarNameNode->Token.Start, VarNameNode->Token.Len,
                          VarType, (char*)&Data);
        }
        else if (VarType == Config_U32)
        {
            u32 Data = StrToUInt(Root->Token.Start, Root->Token.Start + Root->Token.Len);
            InitConfigVar(&Var, VarNameNode->Token.Start, VarNameNode->Token.Len,
                          VarType, (char*)&Data);
        }
        else if (VarType == Config_U64)
        {
            u64 Data = StrToUInt64(Root->Token.Start, Root->Token.Start + Root->Token.Len);
            InitConfigVar(&Var, VarNameNode->Token.Start, VarNameNode->Token.Len,
                          VarType, (char*)&Data);
        }
        else if (VarType == Config_R32)
        {
            u64 Data = StrToR32(Root->Token.Start);
            InitConfigVar(&Var, VarNameNode->Token.Start, VarNameNode->Token.Len,
                          VarType, (char*)&Data);
        }
        else if (VarType > Config_R32 && VarType < Config_Str)
        { // type is an array type
            int ElementCount = 0;
            
            if (VarType > Config_IVec4 && VarType < Config_Str)
            {
                // max needed space is a vec4
                r32 Data[4] = {0};
                RecurseVecArray(RightRootNode, VarType,
                                (char*)Data, &ElementCount);
                
                if (VarType == Config_Vec2 && ElementCount != 2)
                {
                    mprinte("Variable type declared as vec2 but does not contain 2 elements!\n");
                }
                else if (VarType == Config_Vec3 && ElementCount != 3)
                {
                    mprinte("Variable type declared as vec3 but does not contain 2 elements!\n");
                }
                else
                {
                    mprinte("Variable type declared as vec4 but does not contain 2 elements!\n");
                }
                
                InitConfigVar(&Var, VarNameNode->Token.Start, VarNameNode->Token.Len,
                              VarType, (char*)Data);
            }
            else
            {
                // max needed space is a ivec4
                i32 Data[4] = {0};
                RecurseIVecArray(RightRootNode, VarType,
                                 (char*)Data, &ElementCount);
                
                if (VarType == Config_IVec2 && ElementCount != 2)
                {
                    mprinte("Variable type declared as ivec2 but does not contain 2 elements!\n");
                }
                else if (VarType == Config_IVec3 && ElementCount != 3)
                {
                    mprinte("Variable type declared as ivec3 but does not contain 2 elements!\n");
                }
                else
                {
                    mprinte("Variable type declared as ivec4 but does not contain 2 elements!\n");
                }
                
                InitConfigVar(&Var, VarNameNode->Token.Start, VarNameNode->Token.Len,
                              VarType, (char*)Data);
            }
            
        }
        
        ConfigMemberTableInsert(&Obj->ObjMembers, MemberName, Var);
    }
}

file_internal void BuildObjTable(config_obj_table *ObjTable, syntax_tree_list *SyntaxTrees)
{
    for (u32 TreeIdx = 0; TreeIdx < SyntaxTrees->Size; ++TreeIdx)
    {
        if (SyntaxTrees->Head[TreeIdx]->Token.Type != Token_Object)
        {
            jstring token_error = InitJString(SyntaxTrees->Head[TreeIdx]->Token.Start,
                                              SyntaxTrees->Head[TreeIdx]->Token.Len);
            mprinte("A variable is located outside of an object!\n", token_error.GetCStr());
            token_error.Clear();
            break;
        }
        
        token Token = SyntaxTrees->Head[TreeIdx]->Token;
        
        // Create the Object
        config_obj Obj = {};
        InitConfigObj(&Obj, Token.Start, Token.Len);
        
        // Build the Member Table
        ++TreeIdx;
        BuildObjMemberTable(&Obj, SyntaxTrees, &TreeIdx);
    }
}

//~ User Calls

void LoadConfigFile(jstring filename)
{
    jstring FileData = PlatformLoadFile(filename);
    
    mscanner Scanner = {};
    Scanner.Buffer     = (FileData.heap) ? FileData.hptr : FileData.sptr;
    Scanner.BufferSize = FileData.len;
    Scanner.LineCount  = 0;
    Scanner.Current    = Scanner.Buffer;
    
    // Pass 1 Tokenize the resource File
    token_list Tokens = Tokenizer(&Scanner);
    
#if 1
    // print the tokens
    token_node *Iter = Tokens.Head;
    while (Iter)
    {
        jstring TokenData = InitJString(Iter->Token.Start, Iter->Token.Len);
        PlatformPrintMessage(EConsoleColor::Yellow, EConsoleColor::DarkGrey, "Token Data: %s\n", TokenData.GetCStr());
        
        TokenData.Clear();
        
        Iter = Iter->Next;
    }
#endif
    
    // Pass 2 Build parse tree from the token
    syntax_tree_list SyntaxTrees = {};
    SyntaxTrees.Cap  = Scanner.LineCount + 1;
    SyntaxTrees.Head = palloc<syntax_node*>(SyntaxTrees.Cap);
    SyntaxTrees.Size = 0;
    
    for (u32 i = 0; i < SyntaxTrees.Cap; ++i)
    {
        SyntaxTrees.Head[i] = nullptr;
    }
    
    BuildParseTrees(&Tokens, &SyntaxTrees);
    
    // Pass 3 Build Table Data Struct to rturn to the user
    config_obj_table ObjTable;
    InitConfigObjTable(&ObjTable);
    
    BuildObjTable(&ObjTable, &SyntaxTrees);
}

void SaveConfigFile()
{
}

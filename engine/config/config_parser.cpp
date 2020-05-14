
//~ Config Member HashTable
void InitConfigMemberTable(config_member_table *Table)
{
    Table->LoadFactor = 0.70;
    Table->Count      = 0;
    Table->Cap        = 5;
    
    Table->Entries = palloc<config_member_table_entry>(Table->Cap);
    for (u32 entry = 0; entry < Table->Cap; ++entry)
        Table->Entries[entry].IsEmpty = true;
}

void FreeConfigMemberTable(config_member_table *Table)
{
    for (u32 EntryIdx = 0; EntryIdx < Table->Cap; EntryIdx++)
    {
        if (!Table->Entries[EntryIdx].IsEmpty)
        {
            FreeConfigVar(&Table->Entries[EntryIdx].Value);
        }
    }
    
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
        
        if (Entry->IsEmpty) // index is not occupies
        {
            return Entry;
        }
        else
        {
            u128 EntryHash = Hash<jstring>(Entry->Key);
            
            if (EntryHash.upper == HashedKey.upper && // index is occupied, but
                EntryHash.lower == HashedKey.lower)  // elements contain same hash
            {
                return Entry;
            }
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
    for (u32 EntryIdx = 0; EntryIdx < Table->Cap; EntryIdx++)
    {
        if (!Table->Entries[EntryIdx].IsEmpty)
        {
            FreeConfigObj(&Table->Entries[EntryIdx].Value);
        }
    }
    
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
        
        if (Entry->IsEmpty) // index is not occupies
        {
            return Entry;
        }
        else
        {
            u128 EntryHash = Hash<jstring>(Entry->Key);
            
            if (EntryHash.upper == HashedKey.upper && // index is occupied, but
                EntryHash.lower == HashedKey.lower)  // elements contain same hash
            {
                return Entry;
            }
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
        result.Name = InitJString(); // return an empty name
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
                   config_primitive_type Type, const char *Data, u32 Size)
{
    Var->Name = InitJString(Name, NameLen);
    Var->Type = Type;
    switch (Var->Type)
    {
        case Config_I32:   Var->Int32  = *((i32*)Data);           break;
        case Config_I64:   Var->Int64  = *((i64*)Data);           break;
        case Config_U32:   Var->UInt32 = *((u32*)Data);           break;
        case Config_U64:   Var->UInt64 = *((u64*)Data);           break;
        case Config_R32:   Var->R32    = *((r32*)Data);           break;
        case Config_IVec2: Var->IVec2  = *((ivec2*)Data);         break;
        case Config_IVec3: Var->IVec3  = *((ivec3*)Data);         break;
        case Config_IVec4: Var->IVec4  = *((ivec4*)Data);         break;
        case Config_Vec2:  Var->Vec2   = *((vec2*)Data);          break;
        case Config_Vec3:  Var->Vec3   = *((vec3*)Data);          break;
        case Config_Vec4:  Var->Vec4   = *((vec4*)Data);          break;
        case Config_Str:   Var->Str    = InitJString(Data, Size); break;
        
        // NOTE(Dustin): These will require some special handling...
        case Config_Obj:
        case Config_Unknown:
        default:
        { // by special handling, I mean send an error log
            mprinte("Unknown config primitive type!\n");
        } break;
    }
}

void FreeConfigVar(config_var *Var)
{
    if (Var->Type == Config_Str)
        Var->Str.Clear();
    
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
        Token.Len--;
        
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

file_internal void FreeTokenList(token_list *List)
{
    token_node *Iter = List->Head;
    while (Iter)
    {
        token_node *tmp = Iter->Next;
        pfree(Iter);
        Iter = tmp;
    }
    
    List->Head = nullptr;
    List->Tail = nullptr;
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

file_internal void FreeSyntaxNode(syntax_node *Node)
{
    if (Node->Left)
    {
        FreeSyntaxNode(Node->Left);
    }
    
    if (Node->Right)
    {
        FreeSyntaxNode(Node->Right);
    }
    
    pfree(Node);
}

file_internal void FreeParseTree(syntax_tree_list *SyntaxTrees)
{
    for (u32 RootIdx = 0; RootIdx < SyntaxTrees->Size; ++RootIdx)
    {
        FreeSyntaxNode(SyntaxTrees->Head[RootIdx]);
    }
    
    pfree(SyntaxTrees->Head);
    SyntaxTrees->Head = nullptr;
    SyntaxTrees->Size = 0;
    SyntaxTrees->Cap  = 0;
}

file_internal config_primitive_type TokenToPrimitiveType(token Token)
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
        
        //jstring MemberName = InitJString(VarNameNode->Token.Start, VarNameNode->Token.Len);
        config_primitive_type VarType = TokenToPrimitiveType(VarTypeNode->Token);
        
        if (VarType == Config_Unknown)
        {
            mprinte("Invalid expression. Variable is not a valid type!\n");
            break;
        }
        
        config_var Var = {};
        if (VarType == Config_I32)
        {
            i32 Data = StrToInt(RightRootNode->Token.Start,
                                RightRootNode->Token.Start + RightRootNode->Token.Len);
            
            InitConfigVar(&Var, VarNameNode->Token.Start, VarNameNode->Token.Len,
                          VarType, (char*)&Data);
        }
        else if (VarType == Config_I64)
        {
            i64 Data = StrToInt64(RightRootNode->Token.Start,
                                  RightRootNode->Token.Start + RightRootNode->Token.Len);
            InitConfigVar(&Var, VarNameNode->Token.Start, VarNameNode->Token.Len,
                          VarType, (char*)&Data);
        }
        else if (VarType == Config_U32)
        {
            u32 Data = StrToUInt(RightRootNode->Token.Start,
                                 RightRootNode->Token.Start + RightRootNode->Token.Len);
            InitConfigVar(&Var, VarNameNode->Token.Start, VarNameNode->Token.Len,
                          VarType, (char*)&Data);
        }
        else if (VarType == Config_U64)
        {
            u64 Data = StrToUInt64(RightRootNode->Token.Start,
                                   RightRootNode->Token.Start + RightRootNode->Token.Len);
            InitConfigVar(&Var, VarNameNode->Token.Start, VarNameNode->Token.Len,
                          VarType, (char*)&Data);
        }
        else if (VarType == Config_R32)
        {
            u64 Data = StrToR32(RightRootNode->Token.Start);
            InitConfigVar(&Var, VarNameNode->Token.Start, VarNameNode->Token.Len,
                          VarType, (char*)&Data);
        }
        else if (VarType == Config_Str)
        {
            char *Data = RightRootNode->Token.Start;
            InitConfigVar(&Var, VarNameNode->Token.Start, VarNameNode->Token.Len,
                          VarType, Data, RightRootNode->Token.Len);
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
        
        (*TreeIdx)++;
        ConfigMemberTableInsert(&Obj->ObjMembers, Var.Name, Var);
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
        
        ConfigObjTableInsert(ObjTable, Obj.Name, Obj);
        
        // if we are still within the syntax tree list, and if the current
        // tree is an object node, then need to decrement the indx because
        // the above build increments one too mant times.
        if (TreeIdx < SyntaxTrees->Size && SyntaxTrees->Head[TreeIdx]->Token.Type == Token_Object)
        {
            TreeIdx--;
        }
    }
}

//~ User Calls

config_obj_table LoadConfigFile(jstring filename)
{
    jstring FileData = PlatformLoadFile(filename);
    
    mscanner Scanner = {};
    Scanner.Buffer     = (FileData.heap) ? FileData.hptr : FileData.sptr;
    Scanner.BufferSize = FileData.len;
    Scanner.LineCount  = 0;
    Scanner.Current    = Scanner.Buffer;
    
    // Pass 1 Tokenize the resource File
    token_list Tokens = Tokenizer(&Scanner);
    
#if 0
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
    
    config_obj_table ObjTable;
    InitConfigObjTable(&ObjTable);
    
    // Pass 3 Build Table Data Struct to rturn to the user
    BuildObjTable(&ObjTable, &SyntaxTrees);
    
    // Pass 4 Free Resources
    FreeParseTree(&SyntaxTrees);
    FreeTokenList(&Tokens);
    FileData.Clear();
    
    return ObjTable;
}

void SaveConfigFile()
{
}

//~ Getters for a config obj

i32 GetConfigI32(config_obj *Obj, jstring VarName)
{
    i32 Result = {};
    
    config_var Var = ConfigMemberTableGet(&Obj->ObjMembers, VarName);
    if (Var.Type != Config_I32)
    {
        mprinte("Requested i32 variable \"%s\" from object \"%s\", but it does not exist!\n",
                Obj->Name.GetCStr(), VarName.GetCStr());
    }
    else
    {
        Result = Var.Int32;
    }
    
    return Result;
}

i64 GetConfigI64(config_obj *Obj, jstring VarName)
{
    i64 Result = {};
    
    config_var Var = ConfigMemberTableGet(&Obj->ObjMembers, VarName);
    if (Var.Type != Config_I64)
    {
        mprinte("Requested i64 variable \"%s\" from object \"%s\", but it does not exist!\n",
                Obj->Name.GetCStr(), VarName.GetCStr());
    }
    else
    {
        Result = Var.Int64;
    }
    
    return Result;
}

u32 GetConfigU32(config_obj *Obj, jstring VarName)
{
    u32 Result = {};
    
    config_var Var = ConfigMemberTableGet(&Obj->ObjMembers, VarName);
    if (Var.Type != Config_U32)
    {
        mprinte("Requested u32 variable \"%s\" from object \"%s\", but it does not exist!\n",
                Obj->Name.GetCStr(), VarName.GetCStr());
    }
    else
    {
        Result = Var.UInt64;
    }
    
    return Result;
}

u64 GetConfigU64(config_obj *Obj, jstring VarName)
{
    u64 Result = {};
    
    config_var Var = ConfigMemberTableGet(&Obj->ObjMembers, VarName);
    if (Var.Type != Config_U64)
    {
        mprinte("Requested u64 variable \"%s\" from object \"%s\", but it does not exist!\n",
                Obj->Name.GetCStr(), VarName.GetCStr());
    }
    else
    {
        Result = Var.UInt64;
    }
    
    return Result;
}

r32 GetConfigR32(config_obj *Obj, jstring VarName)
{
    r32 Result = {};
    
    config_var Var = ConfigMemberTableGet(&Obj->ObjMembers, VarName);
    if (Var.Type != Config_R32)
    {
        mprinte("Requested r32 variable \"%s\" from object \"%s\", but it does not exist!\n",
                Obj->Name.GetCStr(), VarName.GetCStr());
    }
    else
    {
        Result = Var.R32;
    }
    
    return Result;
}

ivec2 GetConfigIVec2(config_obj *Obj, jstring VarName)
{
    ivec2 Result = {};
    
    config_var Var = ConfigMemberTableGet(&Obj->ObjMembers, VarName);
    if (Var.Type != Config_IVec2)
    {
        mprinte("Requested ivec2 variable \"%s\" from object \"%s\", but it does not exist!\n",
                Obj->Name.GetCStr(), VarName.GetCStr());
    }
    else
    {
        Result = Var.IVec2;
    }
    
    return Result;
}

ivec3 GetConfigIVec3(config_obj *Obj, jstring VarName)
{
    ivec3 Result = {};
    
    config_var Var = ConfigMemberTableGet(&Obj->ObjMembers, VarName);
    if (Var.Type != Config_IVec3)
    {
        mprinte("Requested ivec3 variable \"%s\" from object \"%s\", but it does not exist!\n",
                Obj->Name.GetCStr(), VarName.GetCStr());
    }
    else
    {
        Result = Var.IVec3;
    }
    
    return Result;
}

ivec4 GetConfigIVec4(config_obj *Obj, jstring VarName)
{
    ivec4 Result = {};
    
    config_var Var = ConfigMemberTableGet(&Obj->ObjMembers, VarName);
    if (Var.Type != Config_IVec4)
    {
        mprinte("Requested ivec4 variable \"%s\" from object \"%s\", but it does not exist!\n",
                Obj->Name.GetCStr(), VarName.GetCStr());
    }
    else
    {
        Result = Var.IVec4;
    }
    
    return Result;
}

vec2 GetConfigVec2(config_obj *Obj, jstring VarName)
{
    vec2 Result = {};
    
    config_var Var = ConfigMemberTableGet(&Obj->ObjMembers, VarName);
    if (Var.Type != Config_Vec2)
    {
        mprinte("Requested vec2 variable \"%s\" from object \"%s\", but it does not exist!\n",
                Obj->Name.GetCStr(), VarName.GetCStr());
    }
    else
    {
        Result = Var.Vec2;
    }
    
    return Result;
}

vec3 GetConfigVec3(config_obj *Obj, jstring VarName)
{
    vec3 Result = {};
    
    config_var Var = ConfigMemberTableGet(&Obj->ObjMembers, VarName);
    if (Var.Type != Config_Vec3)
    {
        mprinte("Requested vec3 variable \"%s\" from object \"%s\", but it does not exist!\n",
                Obj->Name.GetCStr(), VarName.GetCStr());
    }
    else
    {
        Result = Var.Vec3;
    }
    
    return Result;
}

vec4 GetConfigVec4(config_obj *Obj, jstring VarName)
{
    vec4 Result = {};
    
    config_var Var = ConfigMemberTableGet(&Obj->ObjMembers, VarName);
    if (Var.Type != Config_Vec4)
    {
        mprinte("Requested vec4 variable \"%s\" from object \"%s\", but it does not exist!\n",
                Obj->Name.GetCStr(), VarName.GetCStr());
    }
    else
    {
        Result = Var.Vec4;
    }
    
    return Result;
}

// Returns a COPY of the string
jstring GetConfigStr(config_obj *Obj, jstring VarName)
{
    jstring Result;
    
    config_var Var = ConfigMemberTableGet(&Obj->ObjMembers, VarName);
    if (Var.Type != Config_Str)
    {
        mprinte("Requested string variable \"%s\" from object \"%s\", but it does not exist!\n",
                Obj->Name.GetCStr(), VarName.GetCStr());
    }
    else
    {
        Result = CopyJString(Var.Str);
    }
    
    return Result;
}

config_obj GetConfigObj(config_obj_table *Table, jstring ObjName)
{
    config_obj Result = ConfigObjTableGet(Table, ObjName);
    
    if (Result.Name.len == 0)
    {
        mprinte("Could not find an Object with the name \"%s\"\n", ObjName.GetCStr());
    }
    
    return Result;
}


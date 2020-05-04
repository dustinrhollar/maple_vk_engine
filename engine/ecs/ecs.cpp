
namespace ecs {
    
    void InitializeECS()
    {
        IntializeEntityRegistry();
        InitializeComponentRegistry();
        InitializeSystemRegistry();
    }
    
    void ShutdownECS()
    {
        ShutdownSystemRegistry();
        ShutdownComponentRegistry();
        ShutdownEntityRegistry();
    }
    
} // ecs

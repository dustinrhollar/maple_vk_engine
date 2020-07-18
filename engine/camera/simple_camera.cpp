void InitCamera(camera *Camera,
                camera_input_callback *Callback,
                vec3 Position,
                vec3 Forward,
                vec3 Up,
                r32 MovementSpeed,
                r32 MouseSensitivity,
                r32 Zoom)
{
    Camera->Position = Position;
    Camera->WorldUp  = Up;
    
    Camera->MovementSpeed    = MovementSpeed;
    Camera->MouseSensitivity = MouseSensitivity;
    Camera->Zoom             = Zoom;
    
    Camera->MouseXPos = 0.0f;
    Camera->MouseYPos = 0.0f;
    
    Camera->Front = norm(Forward);
    Camera->Up    = norm(Up);
    Camera->Right = cross(Up, Forward);
    
    Camera->InputCallback = Callback;
    
    Camera->IsActive = false;
}

void FreeCamera(camera *Camera)
{
    Camera->IsActive = false;
    Camera->InputCallback = nullptr;
}

mat4 GetViewMatrix(camera *Camera)
{
    return LookAt(Camera->Position, Camera->Position + Camera->Front, Camera->Up);
}

void RotateCameraAboutX(camera *Camera, r32 angle)
{
    vec3 haxis = norm(cross(Camera->WorldUp, Camera->Front));
    
    Quaternion result = CreateQuaternion(haxis, angle);
    Quaternion conj   = conjugate(result);
    
    Quaternion fq = {};
    fq.x = Camera->Front.x;
    fq.y = Camera->Front.y;
    fq.z = Camera->Front.z;
    fq.w = 0;
    
    result = QuaternionMul(QuaternionMul(result, fq), conj);
    
    Camera->Front = norm(result.xyz);}

void RotateCameraAboutY(camera *Camera, r32 angle)
{
    Quaternion result = CreateQuaternion(Camera->WorldUp, angle);
    Quaternion conj   = conjugate(result);
    
    Quaternion fq = {};
    fq.x = Camera->Front.x;
    fq.y = Camera->Front.y;
    fq.z = Camera->Front.z;
    fq.w = 0;
    
    result = QuaternionMul(QuaternionMul(result, fq), conj);
    
    Camera->Front = norm(result.xyz);
}

void UpdateCameraVectors(camera *Camera)
{
    Camera->Right = norm(cross(Camera->Front, Camera->WorldUp));
    Camera->Up    = norm(cross(Camera->Right, Camera->Front));
}

CAMERA_INPUT_CALLBACK(PlayerCameraCallback)
{
    camera *PlayerCamera = static_cast<camera*>(instance);
    if (!PlayerCamera->IsActive) return;
    
    // HACK(Dustin): hardcoded time-step. need a better solution
    r32 time = 0.016667;
    
    r32 delta_x = 0.0f;
    r32 delta_y = 0.0f;
    
    if (Movement == Camera_LookUp)
    {
        delta_y -= time;
    }
    
    if (Movement == Camera_LookDown)
    {
        delta_y += time;
    }
    
    if (Movement == Camera_LookLeft)
    {
        delta_x -= time;
    }
    
    if (Movement == Camera_LookRight)
    {
        delta_x += time;
    }
    
    if (delta_x != 0.0f || delta_y != 0.0f)
    {
        r32 xoffset = (delta_x - PlayerCamera->MouseXPos);
        r32 yoffset = (delta_y - PlayerCamera->MouseYPos);
        
        xoffset *= PlayerCamera->MouseSensitivity;
        yoffset *= PlayerCamera->MouseSensitivity;
        
        vec2 mouse_rotation = {xoffset, yoffset};
        
        RotateCameraAboutX(PlayerCamera, mouse_rotation.y);
        RotateCameraAboutY(PlayerCamera, -mouse_rotation.x);
    }
    
    r32 velocity = PlayerCamera->MovementSpeed * time;
    if (Movement == Camera_MoveForward)
    {
        PlayerCamera->Position += PlayerCamera->Front * velocity;
    }
    
    if (Movement == Camera_MoveBackward)
    {
        PlayerCamera->Position -= PlayerCamera->Front * velocity;
    }
    
    if (Movement == Camera_MoveLeft)
    {
        PlayerCamera->Position -= PlayerCamera->Right * velocity;
    }
    
    if (Movement == Camera_MoveRight)
    {
        PlayerCamera->Position += PlayerCamera->Right * velocity;
    }
    
    UpdateCameraVectors(PlayerCamera);
}

#if 0
CAMERA_INPUT_CALLBACK(DevCameraCallback)
{
    camera *DevCamera = static_cast<camera*>(instance);
    if (!DevCamera->IsActive) return;
    
    EventKey ki = event.Key;
    
    // TODO(Dustin): Implement an actual dev camera...
    
    // HACK(Dustin): hardcoded time-step. need a better solution
    r32 time = 0.016667;
    
    r32 delta_x = 0.0f;
    r32 delta_y = 0.0f;
    
    if (ki == KEY_Up)
    {
        delta_y -= time;
    }
    
    if (ki == KEY_Down)
    {
        delta_y += time;
    }
    
    if (ki == KEY_Left)
    {
        delta_x -= time;
    }
    
    if (ki == KEY_Right)
    {
        delta_x += time;
    }
    
    if (delta_x != 0.0f || delta_y != 0.0f)
    {
        r32 xoffset = (delta_x - DevCamera->MouseXPos);
        r32 yoffset = (delta_y - DevCamera->MouseYPos);
        
        xoffset *= DevCamera->MouseSensitivity;
        yoffset *= DevCamera->MouseSensitivity;
        
        vec2 mouse_rotation = {xoffset, yoffset};
        
        RotateCameraAboutX(DevCamera, mouse_rotation.y);
        RotateCameraAboutY(DevCamera, -mouse_rotation.x);
    }
    
    r32 velocity = DevCamera->MovementSpeed * time;
    if (ki == KEY_w)
    {
        DevCamera->Position += DevCamera->Front * velocity;
    }
    
    if (ki == KEY_s)
    {
        DevCamera->Position -= DevCamera->Front * velocity;
    }
    
    if (ki == KEY_a)
    {
        DevCamera->Position -= DevCamera->Right * velocity;
    }
    
    if (ki == KEY_d)
    {
        DevCamera->Position += DevCamera->Right * velocity;
    }
    
    UpdateCameraVectors(DevCamera);
    
}
#endif
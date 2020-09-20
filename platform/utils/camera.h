#ifndef PLATFORM_UTILS_CAMERA_H
#define PLATFORM_UTILS_CAMERA_H

#define CAMERA_DEFAULT_POSITION   {0,0,0}
#define CAMERA_DEFAULT_YAW        -90.0f
#define CAMERA_DEFAULT_PITCH       0.0f
#define CAMERA_DEFAULT_SPEED       500.5f
#define CAMERA_DEFAULT_SENSITIVITY 300.f
#define CAMERA_DEFAULT_ZOOM        45.0f

typedef struct camera
{
    // Positional vectors
    vec3 Position;
    vec3 Front;
    vec3 Up;
    vec3 Right;
    vec3 WorldUp;
    
    // Mouse Position
    r32 MouseXPos;
    r32 MouseYPos;
    
    // Camera options
    r32 MovementSpeed;
    r32 MouseSensitivity;
    r32 Zoom;
} camera;

void camera_init(camera *Camera,
                 vec3 Position,
                 vec3 Forward,
                 vec3 Up,
                 r32 MovementSpeed,
                 r32 MouseSensitivity,
                 r32 Zoom);
void camera_default_init(camera *Camera, vec3 Position);


#endif //PLATFORM_UTILS_CAMERA_H


#ifdef MAPLE_CAMERA_IMPLEMENTATION

void camera_init(camera *Camera,
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
    
    Camera->Front = vec3_norm(Forward);
    Camera->Up    = vec3_norm(Up);
    Camera->Right = vec3_cross(Up, Forward);
}

void camera_default_init(camera *Camera,
                         vec3 Position)
{
    vec3 Forward = {0.0f,0.0f,1.0f};
    vec3 Up = {0.0f,1.0f,0.0f};
    
    camera_init(Camera,
                Position,
                Forward,
                Up,
                CAMERA_DEFAULT_SPEED,
                CAMERA_DEFAULT_SENSITIVITY,
                CAMERA_DEFAULT_ZOOM);
}

#endif

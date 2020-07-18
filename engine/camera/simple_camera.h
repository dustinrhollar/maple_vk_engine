#ifndef ENGINE_CAMERA_SIMPLE_CAMERA_H
#define ENGINE_CAMERA_SIMPLE_CAMERA_H

enum camera_movement
{
    Camera_MoveForward,
    Camera_MoveBackward,
    Camera_MoveRight,
    Camera_MoveLeft,
    
    Camera_LookLeft,
    Camera_LookRight,
    Camera_LookUp,
    Camera_LookDown,
};

struct camera;
#define CAMERA_INPUT_CALLBACK(fn) void fn(camera *instance, u64 FrameDelta, camera_movement Movement)
typedef CAMERA_INPUT_CALLBACK(camera_input_callback);

const r32 YAW         = -90.0f;
const r32 PITCH       = 0.0f;
const r32 SPEED       = 500.5f;
const r32 SENSITIVITY = 300.f;
const r32 ZOOM        = 45.0f;

struct camera
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
    
    // Callback function used for camera input
    camera_input_callback *InputCallback;
    
    // Determines if the camera is an active camera
    bool IsActive;
};

void InitCamera(camera *Camera,
                camera_input_callback *Callback,
                vec3 Position     = {0.0f,0.0f,0.0f},
                vec3 Forward      = {0.0f,0.0f,1.0f},
                vec3 Up           = {0.0f,1.0f,0.0f},
                r32 MovementSpeed    = SPEED,
                r32 MouseSensitivity = SENSITIVITY,
                r32 Zoom              = ZOOM);


mat4 GetViewMatrix(camera *Camera);
void UpdateCameraVectors(camera *Camera);

void RotateCameraAboutX(camera *Camera, r32 angle);
void RotateCameraAboutY(camera *Camera, r32 angle);

CAMERA_INPUT_CALLBACK(PlayerCameraCallback);
CAMERA_INPUT_CALLBACK(DevCameraCallback);

#endif //ENGINE_CAMERA_SIMPLE_CAMERA_H

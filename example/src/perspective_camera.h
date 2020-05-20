#ifndef PERSPECTIVE_CAMERA_H
#define PERSPECTIVE_CAMERA_H

enum CameraMovement
{
    CAMERA_FORWARD,
    CAMERA_BACKWARD,
    CAMERA_LEFT,
    CAMERA_RIGHT,
    CAMERA_ARROW_UP,
    CAMERA_ARROW_DOWN,
    CAMERA_ARROW_RIGHT,
    CAMERA_ARROW_LEFT,
};

const r32 YAW         = -90.0f;
const r32 PITCH       = 0.0f;
const r32 SPEED       = 500.5f;
const r32 SENSITIVITY = 300.f;
const r32 ZOOM        = 45.0f;

struct PerspectiveCamera
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
};

void InitPerspectiveCamera(PerspectiveCamera *camera,
                           vec3 position     = {0.0f,0.0f,0.0f},
                           vec3 forward      = {0.0f,0.0f,1.0f},
                           vec3 up           = {0.0f,1.0f,0.0f},
                           r32 movement_speed    = SPEED,
                           r32 mouse_sensitivity = SENSITIVITY,
                           r32 zoom              = ZOOM);

mat4 GetViewMatrix(PerspectiveCamera *camera);
void UpdatePerspectiveCameraVectors(PerspectiveCamera *camera);

void RotateCameraAboutX(PerspectiveCamera *camera, r32 angle);
void RotateCameraAboutY(PerspectiveCamera *camera, r32 angle);


#endif //PERSPECTIVE_CAMERA_H


void InitPerspectiveCamera(PerspectiveCamera *camera,
                           vec3 position,
                           vec3 forward,
                           vec3 up,
                           r32 movement_speed,
                           r32 mouse_sensitivity,
                           r32 zoom)
{
    camera->Position = position;
    camera->WorldUp  = up;
    
    camera->MovementSpeed    = movement_speed;
    camera->MouseSensitivity = mouse_sensitivity;
    camera->Zoom             = zoom;
    
    camera->MouseXPos = 0.0f;
    camera->MouseYPos = 0.0f;
    
    camera->Front = norm(forward);
    camera->Up    = norm(up);
    camera->Right = cross(up, forward);
}

mat4 GetViewMatrix(PerspectiveCamera *camera)
{
    return LookAt(camera->Position, camera->Position + camera->Front, camera->Up);
}

void RotateCameraAboutX(PerspectiveCamera *camera, r32 angle)
{
    vec3 haxis = norm(cross(camera->WorldUp, camera->Front));
    
    Quaternion result = CreateQuaternion(haxis, angle);
    Quaternion conj   = conjugate(result);
    
    Quaternion fq = {};
    fq.x = camera->Front.x;
    fq.y = camera->Front.y;
    fq.z = camera->Front.z;
    fq.w = 0;
    
    result = QuaternionMul(QuaternionMul(result, fq), conj);
    
    camera->Front = norm(result.xyz);}

void RotateCameraAboutY(PerspectiveCamera *camera, r32 angle)
{
    Quaternion result = CreateQuaternion(camera->WorldUp, angle);
    Quaternion conj   = conjugate(result);
    
    Quaternion fq = {};
    fq.x = camera->Front.x;
    fq.y = camera->Front.y;
    fq.z = camera->Front.z;
    fq.w = 0;
    
    result = QuaternionMul(QuaternionMul(result, fq), conj);
    
    camera->Front = norm(result.xyz);
}

void UpdatePerspectiveCameraVectors(PerspectiveCamera *camera)
{
    camera->Right = norm(cross(camera->Front, camera->WorldUp));
    camera->Up    = norm(cross(camera->Right, camera->Front));
}

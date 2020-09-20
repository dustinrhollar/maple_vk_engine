
 graphics_api *Graphics;
platform     *Platform;

file_internal void rotate_camera_about_x(camera *Camera, r32 angle)
{
    vec3 haxis = vec3_norm(vec3_cross(Camera->WorldUp, Camera->Front));
    
    quaternion result = quaternion_init(haxis, angle);
    quaternion conj   = quaternion_conjugate(result);
    
    quaternion fq = {};
    fq.x = Camera->Front.x;
    fq.y = Camera->Front.y;
    fq.z = Camera->Front.z;
    fq.w = 0;
    
    result = quaternion_mul(quaternion_mul(result, fq), conj);
    
    Camera->Front = vec3_norm(result.xyz);}

file_internal void rotate_camera_about_y(camera *Camera, r32 angle)
{
    quaternion result = quaternion_init(Camera->WorldUp, angle);
    quaternion conj   = quaternion_conjugate(result);
    
    quaternion fq = {};
    fq.x = Camera->Front.x;
    fq.y = Camera->Front.y;
    fq.z = Camera->Front.z;
    fq.w = 0;
    
    result = quaternion_mul(quaternion_mul(result, fq), conj);
    
    Camera->Front = vec3_norm(result.xyz);
}

file_internal void process_user_input(input *Input, camera *Camera)
{
    
    // HACK(Dustin): hardcoded time-step. need a better solution
    r32 time = 0.016667 / 4;
    
    r32 delta_x = 0.0f;
    r32 delta_y = 0.0f;
    
    if (Input->KeyPress & Key_Up)
    {
        delta_y -= time;
    }
    
    if (Input->KeyPress & Key_Down)
    {
        delta_y += time;
    }
    
    if (Input->KeyPress & Key_Left)
    {
        delta_x -= time;
    }
    
    if (Input->KeyPress & Key_Right)
    {
        delta_x += time;
    }
    
    if (delta_x != 0.0f || delta_y != 0.0f)
    {
        r32 xoffset = (delta_x - Camera->MouseXPos);
        r32 yoffset = (delta_y - Camera->MouseYPos);
        
        xoffset *= Camera->MouseSensitivity;
        yoffset *= Camera->MouseSensitivity;
        
        vec2 mouse_rotation = {xoffset, yoffset};
        
        rotate_camera_about_x(Camera, mouse_rotation.y);
        rotate_camera_about_y(Camera, -mouse_rotation.x);
    }
    
    r32 velocity = Camera->MovementSpeed * time;
    if (Input->KeyPress & Key_w)
    {
        Camera->Position = vec3_add(Camera->Position,
                                    vec3_mulf(Camera->Front, velocity));
    }
    
    if (Input->KeyPress & Key_s)
    {
        Camera->Position = vec3_sub(Camera->Position,
                                    vec3_mulf(Camera->Front, velocity));
    }
    
    if (Input->KeyPress & Key_a)
    {
        Camera->Position = vec3_sub(Camera->Position,
                                    vec3_mulf(Camera->Right, velocity));
    }
    
    if (Input->KeyPress & Key_d)
    {
        Camera->Position = vec3_add(Camera->Position,
                                    vec3_mulf(Camera->Right, velocity));
    }
    
    Camera->Right = vec3_norm(vec3_cross(Camera->Front, Camera->WorldUp));
    Camera->Up    = vec3_norm(vec3_cross(Camera->Right, Camera->Front));
}

GAME_ENTRY(game_entry)
{
    // This is probably an expensive copy, but for now let it happen
    Graphics  = FrameInfo->Graphics;
    Platform  = FrameInfo->Platform;
    
    process_user_input(&FrameInfo->Input, FrameInfo->Camera);
    
    mat4 LookAt = look_at(FrameInfo->Camera->Position, 
                          vec3_add(FrameInfo->Camera->Position, FrameInfo->Camera->Front), 
                          FrameInfo->Camera->Up);
    
    mat4 Projection = perspective_projection(90.0f, (r32)1920/(r32)1080, 0.1f, 1000.f);
    Projection.data[1][1] *= -1;
    
    //Graphics->cmd_set_camera(World->CommandList, Projection, LookAt);
}

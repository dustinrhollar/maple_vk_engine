#ifndef ENGINE_UTILS_VECTOR_MATH_H
#define ENGINE_UTILS_VECTOR_MATH_H

/*

User API:
// TODO(Dustin):

*/


#define JPI 3.1415926535
#define degrees_to_radians(theta) (r32)((theta) * (JPI / 180.0f))

typedef union vec2
{
    struct { r32 x, y; };
    struct { r32 r, g; };
    
    r32 data[2];
} vec2;

typedef union vec3
{
    struct { r32 x, y, z;     };
    struct { vec2 xy; r32 p0; };
    struct { r32 p1; vec2 yz; };
    
    struct { r32 r, g, b;     };
    struct { vec2 rg; r32 p2; };
    struct { r32 p3; vec2 gb; };
    
    r32 data[3];
} vec3;

typedef union vec4
{
    struct { r32 x, y, z, w;          };
    struct { vec2 xy, zw;             };
    struct { r32 p0; vec2 yz; r32 p1; };
    struct { vec3 xyz; r32 p2;        };
    struct { r32 p3; vec3 yzw;        };
    
    struct { r32 r, g, b, a;          };
    struct { vec2 rg, ba;             };
    struct { vec3 rgb; r32 p4;        };
    struct { r32 p5; vec2 gb; r32 p6; };
    struct { r32 p7; vec3 gba;        };
    
    r32 data[4];
} vec4;

// Column major
typedef union mat3
{
    r32 data[9];
    
    struct { vec3 col0, col1, col2; };
} mat3;

typedef union mat4
{
    r32 data[4][4];
    
    struct { vec4 col0, col1, col2, col3; };
} mat4;

typedef union quaternion
{
    struct { r32 x, y, z, w;  };
    struct { vec3 xyz; r32 p0; };
} quaternion;


//----------------------------------------------------------------------------------------//
// Pre-declarations
//----------------------------------------------------------------------------------------//

//~ Vec3 Functions

file_internal vec3  vec3_add(vec3 Left, vec3 Right);
file_internal vec3  vec3_sub(vec3 Left, vec3 Right);
file_internal vec3  vec3_mul(vec3 Left, vec3 Right);
file_internal vec3  vec3_mulf(vec3 In, r32 Scalar);
file_internal vec3  vec3_divf(vec3 In, r32 Denom);
file_internal r32   vec3_dot(vec3 left, vec3 right);
file_internal vec3  vec3_cross(vec3 left, vec3 right);
file_internal vec3  vec3_norm(vec3 left);
file_internal r32   vec3_mag(vec3 v);
file_internal r32   vec3_mag_sq(vec3 v);

//~ Vec4 Functions

file_internal r32 vec4_dot(vec4 left, vec4 right);
file_internal r32 vec4_mag(vec4 v);

//~ Quaternion Functions

file_internal quaternion quaternion_init(vec3 axis, r32 theta);
file_internal quaternion quaternion_mul(quaternion l, quaternion r);
file_internal quaternion quaternion_conjugate(quaternion q);
file_internal quaternion quaternion_norm(quaternion q);
file_internal vec3       quaternion_transform(vec3 v, quaternion q);
file_internal mat4       quaternion_get_rotation_matrix(quaternion In);

//~ Mat4 Functions

file_internal mat4 mat4_diag(r32 Val);
file_internal mat4 mat4_mul(mat4 left, mat4 r);

file_internal mat4 scale(r32 sx, r32 sy, r32 sz);
file_internal mat4 translate(vec3 trans);
file_internal mat4 look_at(vec3 eye, vec3 center, vec3 up);
file_internal mat4 perspective_projection(r32 fov, r32 aspect_ratio, r32 near, r32 far);

//~ Interpolation

file_internal r32 clamp(r32 min, r32 max, r32 val);

file_internal r32 lerp(r32 v0, r32 v1, r32 t);
file_internal r32 inv_lerp(r32 v0, r32 v1, r32 v);
file_internal r32 remap(r32 i0, r32 i1, r32 o0, r32 o1, r32 v);

file_internal r32 smoothstep(r32 v0, r32 v1, r32 t);
file_internal r32 smootherstep(r32 v0, r32 v1, r32 t);

#if 0

// Calculates the cross product of two vectors
// cross pruduct of a vect2 is the same as calculating the
// determinate of 2x2 matrix
inline r32 cross(vec2 const& left, vec2 const& right);
inline vec3 cross(vec3 const& left, vec3 const& right);
// NOTE(Dustin), there really isn't an actual representation
// of a vec4 cross product. Therefore, the cross product of the
// xyz components are computed instead, where the w component is
// set to 1.
inline vec4 cross(vec4 const& left, vec4 const& right);

// Finds the magnitude of a vector
inline r32 mag(vec2 const& vector);
inline r32 mag(vec3 const& vector);
inline r32 mag(vec4 const& vector);

// Finds the squared magnitude of a vector
inline r32 mag_sq(vec2 const& vector);
inline r32 mag_sq(vec3 const& vector);
inline r32 mag_sq(vec4 const& vector);

// Normalizs a vector so that its magnitude == 1
inline vec2 norm(vec2 vector);
inline vec3 norm(vec3 vector);
inline vec4 norm(vec4 vector);

// MATRICES

inline mat3 MakeMat3(r32 *ptr);
mat4 MakeMat4(r32 *ptr);

inline mat3 Mul(mat3 const& left, mat3 const& right);
mat4 Mul(mat4 const& left, mat4 const& right);
inline mat3 RotateX(r32 theta);
inline mat3 RotateY(r32 theta);
inline mat3 RotateZ(r32 theta);
inline mat3 Rotate(r32 theta, vec3 axis);
inline mat3 Reflect(vec3 vec3);
mat4 Scale(r32 sx, r32 sy, r32 sz);
inline mat4 Scale(r32 s, vec3 scale);
// skew is the vector representing the direction along which the skew ocurs
// perp is the vector perpendicular to skew along which vectors are measured to
// to determine how to skew.
inline mat3 Skew(r32 theta, vec3 skew, vec3 perp);

mat4 Translate(vec3 trans);

// QUATERNIONS

Quaternion MakeQuaternion(r32 x, r32 y, r32 z, r32 w);
Quaternion CreateQuaternion(vec3 axis, r32 theta);
vec3 GetQuaternionVector(Quaternion const& quat);
mat3 GetQuaternionRotationMatrix(Quaternion const& q);
// Creates a quaternion from a rotation matrix
Quaternion CreateQuaternionFromRotationMatrix(const mat3& m);
Quaternion QuaternionMul(Quaternion const& left, Quaternion const& right);
// rotates the vector around the quaternion using the sandwich product
vec3 Transform(const vec3& v, const Quaternion& q);
Quaternion norm(const Quaternion& q);

Quaternion conjugate(const Quaternion& q);

#endif // Turning off pre-decs

#endif // JENGINE_UTILS_VECTOR_MATH_H

#if defined(MAPLE_VECTOR_MATH_IMPLEMENTATION)


//----------------------------------------------------------------------------------------//

file_internal r32 vec3_mag(vec3 v)
{
    return (r32)sqrt((v.x * v.x) + (v.y * v.y) + (v.z * v.z));
}

file_internal r32 vec3_mag_sq(vec3 v)
{
    return (r32)(v.x * v.x) + (v.y * v.y) + (v.z * v.z);
}

file_internal vec3 vec3_add(vec3 Left, vec3 Right)
{
    vec3 Result;
    
    Result.x = Left.x + Right.x;
    Result.y = Left.y + Right.y;
    Result.z = Left.z + Right.z;
    
    return Result;
}

file_internal vec3 vec3_sub(vec3 Left, vec3 Right)
{
    vec3 Result;
    
    Result.x = Left.x - Right.x;
    Result.y = Left.y - Right.y;
    Result.z = Left.z - Right.z;
    
    return Result;
}

file_internal vec3 vec3_divf(vec3 In, r32 Denom)
{
    vec3 Result = {0};
    
    Result.x = In.x / Denom;
    Result.y = In.y / Denom;
    Result.z = In.z / Denom;
    
    return Result;
}

// Normalizs a vector so that its magnitude == 1
file_internal vec3 vec3_norm(vec3 v)
{
    r32 m = vec3_mag(v);
    
    if (m == 0.0f)
    {
        int a = 0;
    }
    
    v = vec3_divf(v, m);
    return v;
}

file_internal vec3 vec3_cross(vec3 left, vec3 right)
{
    vec3 result = {};
    
    result.x = (left.y * right.z) - (left.z * right.y);
    result.y = (left.z * right.x) - (left.x * right.z);
    result.z = (left.x * right.y) - (left.y * right.x);
    
    return result;
}

file_internal vec3 vec3_mul(vec3 Left, vec3 Right)
{
    vec3 Result = {0};
    
    Result.x = Left.x * Right.x;
    Result.y = Left.y * Right.y;
    Result.z = Left.z * Right.z;
    
    return Result;
}

file_internal vec3 vec3_mulf(vec3 In, r32 Scalar)
{
    vec3 Result = {0};
    
    Result.x = In.x * Scalar;
    Result.y = In.y * Scalar;
    Result.z = In.z * Scalar;
    
    return Result;
}

// Calculates the dot product of two vectors
file_internal r32 vec3_dot(vec3 left, vec3 right)
{
    return (left.x * right.x)
        +  (left.y * right.y)
        +  (left.z * right.z);
}

// Calculates the dot product of two vectors
file_internal r32 vec4_dot(vec4 left, vec4 right)
{
    return (left.x * right.x)
        +  (left.y * right.y)
        +  (left.z * right.z)
        +  (left.w * right.w);
}

file_internal r32 vec4_mag(vec4 v)
{
    return (r32)sqrt((v.x * v.x) + (v.y * v.y) + (v.z * v.z) + (v.w * v.w));
}

// Creates a quaternion for rotation from an axis and angle (degrees)
file_internal quaternion quaternion_init(vec3 axis, r32 theta)
{
    r32 r = degrees_to_radians(theta);
    r32 half_angle = r * 0.5f;
    
    r32 x = axis.x * sinf(half_angle);
    r32 y = axis.y * sinf(half_angle);
    r32 z = axis.z * sinf(half_angle);
    r32 w = cosf(half_angle);
    
    quaternion result = {0};
    
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;
    
    return result;
}

file_internal quaternion quaternion_mul(quaternion l, quaternion r)
{
    quaternion result = {};
    
    result.x = l.w * r.x + l.x * r.w + l.y * r.z - l.z * r.y;
    result.y = l.w * r.y - l.x * r.z + l.y * r.w + l.z * r.x;
    result.z = l.w * r.z + l.x * r.y - l.y * r.x + l.z * r.w;
    result.w = l.w * r.w - l.x * r.x - l.y * r.y - l.z * r.z;
    
    return result;
}


file_internal quaternion quaternion_conjugate(quaternion q)
{
    quaternion result = {};
    
    result.x = -q.x;
    result.y = -q.y;
    result.z = -q.z;
    result.w = q.w;
    
    return result;
}

file_internal quaternion quaternion_norm(quaternion q)
{
    r32 d = 1/(r32)sqrt(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w);
    
    quaternion result = {};
    
    result.x = q.x*d;
    result.y = q.y*d;
    result.z = q.z*d;
    result.w = q.w*d;
    
    return result;
}

file_internal vec3 quaternion_transform(vec3 v, quaternion q)
{
    vec3 b = q.xyz;
    r32 b2 = b.x * b.x + b.y * b.y + b.z * b.z;
    
#if 0
    
    return v * (q.w * q.w - b2) +
        b * (dot(v, b) * 2.0f) +
        cross(b, v) * (q.w * 2.0f);
#else
    
    // v * (q.w * q.w - b2) + b * (dot(v, b) * 2.0f) + cross(b, v) * (q.w * 2.0f)
    return vec3_add(vec3_mulf(v, q.w * q.w - b2), 
                    vec3_add(vec3_mulf(b, vec3_dot(v, b) * 2.0f),
                             vec3_mulf(vec3_cross(b, v), q.w * 2.0f)));
#endif
}

file_internal mat4 quaternion_get_rotation_matrix(quaternion In)
{
    mat4 Result = mat4_diag(1.0f);
    
    quaternion q = quaternion_norm(In);
    
    r32 x2 = q.x * q.x;
    r32 y2 = q.y * q.y;
    r32 z2 = q.z * q.z;
    r32 xy = q.x * q.y;
    r32 xz = q.x * q.z;
    r32 yz = q.y * q.z;
    r32 wx = q.w * q.x;
    r32 wy = q.w * q.y;
    r32 wz = q.w * q.z;
    
    Result.data[0][0] = 1.0f - 2.0f * (y2 + z2);
    Result.data[0][1] = 2.0f * (xy + wz);
    Result.data[0][2] = 2.0f * (xz - wy);
    Result.data[0][3] = 0.0f;
    
    Result.data[1][0] = 2.0f * (xy - wz);
    Result.data[1][1] = 1.0f - 2.0f * (x2 + z2);
    Result.data[1][2] = 2.0f * (yz + wx);
    Result.data[1][3] = 0.0f;
    
    Result.data[2][0] = 2.0f * (xz + wy);
    Result.data[2][1] = 2.0f * (yz - wx);
    Result.data[2][2] = 1.0f - 2.0f * (x2 + y2);
    Result.data[2][3] = 0.0f;
    
    Result.data[3][0] = 0.0f;
    Result.data[3][1] = 0.0f;
    Result.data[3][2] = 0.0f;
    Result.data[3][3] = 1.0f;
    
    return Result;
}

#if 0
// TODO(Dustin): The if-else might need to swap the matrix indexes
Quaternion CreateQuaternionFromRotationMatrix(const mat3& m)
{
    r32 x, y, z, w;
    
    r32 m00 = m[0][0];
    r32 m11 = m[1][1];
    r32 m22 = m[2][2];
    r32 sum = m00 + m11 + m22;
    
    if (sum > 0.0f)
    {
        w = (r32)sqrt(sum + 1.0f) * 0.5f;
        r32 f = 0.25f / w;
        x = (m[2][1] - m[1][2]) * f;
        y = (m[0][2] - m[2][0]) * f;
        z = (m[1][0] - m[0][1]) * f;
    }
    else if ((m00 > m11) && (m00 > m22))
    {
        x = (r32)sqrt(m00 - m11 - m22 + 1.0f) * 0.5f;
        r32 f = 0.25f / x;
        
        y = (m[1][0] + m[0][1]) * f;
        z = (m[0][2] + m[2][0]) * f;
        w = (m[2][1] - m[1][2]) * f;
    }
    else if (m11 > m22)
    {
        y = (r32)sqrt(m11 - m00 - m22 + 1.0f) * 0.5f;
        r32 f = 0.25f / y;
        
        x = (m[1][0] + m[0][1]) * f;
        z = (m[2][1] + m[1][2]) * f;
        w = (m[0][2] - m[2][1]) * f;
    }
    else
    {
        z = (r32)sqrt(m22 - m00 - m11 + 1.0f) * 0.5f;
        r32 f = 0.25f / z;
        
        x = (m[0][2] + m[2][0]) * f;
        y = (m[2][1] + m[1][2]) * f;
        w = (m[1][0] - m[0][1]) * f;
    }
    
    
    Quaternion result = {};
    
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;
    
    return result;
}
#endif

file_internal mat4 mat4_diag(r32 diagonal)
{
    mat4 Result = {0};
    
    Result.data[0][0] = diagonal;
    Result.data[0][1] = 0.0f;
    Result.data[0][2] = 0.0f;
    Result.data[0][3] = 0.0f;
    
    Result.data[1][0] = 0.0f;
    Result.data[1][1] = diagonal;
    Result.data[1][2] = 0.0f;
    Result.data[1][3] = 0.0f;
    
    Result.data[2][0] = 0.0f;
    Result.data[2][1] = 0.0f;
    Result.data[2][2] = diagonal;
    Result.data[2][3] = 0.0f;
    
    Result.data[3][0] = 0.0f;
    Result.data[3][1] = 0.0f;
    Result.data[3][2] = 0.0f;
    Result.data[3][3] = diagonal;
    
    return Result;
}

file_internal mat4 mat4_mul(mat4 left, mat4 r)
{
    mat4 Result = mat4_diag(1.0f);
    
    vec4 lr0 = { left.data[0][0], left.data[1][0], left.data[2][0], left.data[3][0] };
    vec4 lr1 = { left.data[0][1], left.data[1][1], left.data[2][1], left.data[3][1] };
    vec4 lr2 = { left.data[0][2], left.data[1][2], left.data[2][2], left.data[3][2] };
    vec4 lr3 = { left.data[0][3], left.data[1][3], left.data[2][3], left.data[3][3] };
    
    Result.data[0][0] = vec4_dot(lr0, r.col0);
    Result.data[0][1] = vec4_dot(lr1, r.col0);
    Result.data[0][2] = vec4_dot(lr2, r.col0);
    Result.data[0][3] = vec4_dot(lr3, r.col0);
    
    Result.data[1][0] = vec4_dot(lr0, r.col1);
    Result.data[1][1] = vec4_dot(lr1, r.col1);
    Result.data[1][2] = vec4_dot(lr2, r.col1);
    Result.data[1][3] = vec4_dot(lr3, r.col1);
    
    Result.data[2][0] = vec4_dot(lr0, r.col2);
    Result.data[2][1] = vec4_dot(lr1, r.col2);
    Result.data[2][2] = vec4_dot(lr2, r.col2);
    Result.data[2][3] = vec4_dot(lr3, r.col2);
    
    Result.data[3][0] = vec4_dot(lr0, r.col3);
    Result.data[3][1] = vec4_dot(lr1, r.col3);
    Result.data[3][2] = vec4_dot(lr2, r.col3);
    Result.data[3][3] = vec4_dot(lr3, r.col3);
    
    return Result;
}


file_internal mat4 scale(r32 sx, r32 sy, r32 sz)
{
    mat4 result = mat4_diag(1.0f);
    
    result.data[0][0] = sx;
    result.data[1][1] = sy;
    result.data[2][2] = sz;
    
    return result;
}

file_internal mat4 translate(vec3 trans)
{
    mat4 result = mat4_diag(1.0f);
    
    result.data[3][0] = trans.x;
    result.data[3][1] = trans.y;
    result.data[3][2] = trans.z;
    
    return result;
}

file_internal mat4 look_at(vec3 eye, vec3 center, vec3 up)
{
    mat4 result = mat4_diag(1.0f);
    
    vec3 f = vec3_norm(vec3_sub(center, eye));
    vec3 s = vec3_norm(vec3_cross(f, up));
    vec3 u = vec3_cross(s, f);
    
    result.data[0][0] = s.x;
    result.data[0][1] = u.x;
    result.data[0][2] = -f.x;
    result.data[0][3] = 0.0f;
    
    result.data[1][0] = s.y;
    result.data[1][1] = u.y;
    result.data[1][2] = -f.y;
    result.data[1][3] = 0.0f;
    
    result.data[2][0] = s.z;
    result.data[2][1] = u.z;
    result.data[2][2] = -f.z;
    result.data[2][3] = 0.0f;
    
    result.data[3][0] = -vec3_dot(s, eye);
    result.data[3][1] = -vec3_dot(u, eye);
    result.data[3][2] = vec3_dot(f, eye);
    result.data[3][3] = 1.0f;
    
    return result;
}

file_internal mat4 perspective_projection(r32 fov, r32 aspect_ratio, r32 near_plane, r32 far_plane)
{
    mat4 result = mat4_diag(1.0f);
    
    r32 cotangent = 1.0f / tanf(fov * ((r32)JPI / 360.0f));
    
#if 1
    result.data[0][0] = cotangent / aspect_ratio;
    result.data[1][1] = cotangent;
    result.data[2][3] = -1.0f;
    result.data[2][2] = (near_plane + far_plane) / (near_plane - far_plane);
    result.data[3][2] = (2.0f * near_plane * far_plane) / (near_plane - far_plane);
    result.data[3][3] = 0.0f;
#else
    result.data[0][0] = cotangent / aspect_ratio;
    result.data[1][1] = cotangent;
    result.data[3][2] = -1.0f;
    result.data[2][2] = (near_plane + far_plane) / (near_plane - far_plane);
    result.data[2][3] = (2.0f * near_plane * far_plane) / (near_plane - far_plane);
    result.data[3][3] = 0.0f;
#endif
    
    return result;
}


file_internal r32 lerp(r32 v0, r32 v1, r32 t)
{
    return v0 + t * (v1 - v0);
}

file_internal r32 inv_lerp(r32 v0, r32 v1, r32 v)
{
    r32 Result = 0.0f;
    
    Result = (v - v0) / (v1 - v0);
    Result = (Result < 0.0f) ? 0.0f : (Result > 1.0f) ? 1.0f : Result;
    
    return Result;
}

file_internal r32 remap(r32 i0, r32 i1, r32 o0, r32 o1, r32 v)
{
    r32 t = inv_lerp(i0, i1, v);
    return lerp(o0, o1, t);
}

file_internal r32 clamp(r32 min, r32 max, r32 val)
{
    r32 Result = val;
    
    if (val < min) Result = min;
    else if (val > max) Result = max;
    
    return Result;
}

file_internal r32 smoothstep(r32 v0, r32 v1, r32 t)
{
    t = clamp(0.0f, 1.0f, (t - v0) / (v1 - v0));
    return t * t * (3 - 2 * t);
}

file_internal r32 smootherstep(r32 v0, r32 v1, r32 t)
{
    t = clamp(0.0f, 1.0f, (t - v0) / (v1 - v0));
    return t * t * t * (t * (t * 6 - 15) + 10);
}


#if 0

//----------------------------------------------------------------------------------------//
// Vector Implementations
//----------------------------------------------------------------------------------------//

// Vec2 operator overloads
inline vec2 operator+(vec2 left, vec2 const& right)
{
    left += right;
    return left;
}

inline vec2 operator-(vec2 left, vec2 const& right)
{
    left -= right;
    return left;
}

inline vec2 operator*(vec2 left, vec2 const& right)
{
    left -= right;
    return left;
}

inline vec2 operator/(vec2 left, vec2 const& right)
{
    left += right;
    return left;
}

inline vec2 operator/(vec2 left, r32 const& denominator)
{
    left /= denominator;
    return left;
}

inline vec2 operator*(vec2 left, r32 const& scalar)
{
    left *= scalar;
    return left;
}

// vec3 operator overloads
inline vec3 operator+(vec3 left, vec3 const& right)
{
    left += right;
    return left;
}

inline vec3 operator-(vec3 left, vec3 const& right)
{
    left -= right;
    return left;
}

inline vec3 operator*(vec3 left, vec3 const& right)
{
    left -= right;
    return left;
}

inline vec3 operator/(vec3 left, vec3 const& right)
{
    left += right;
    return left;
}

inline vec3 operator/(vec3 left, r32 const& denominator)
{
    left /= denominator;
    return left;
}

inline vec3 operator*(vec3 left, r32 const& scalar)
{
    left *= scalar;
    return left;
}


inline vec2 operator*(r32 const& scalar, vec2 left)
{
    left *= scalar;
    return left;
}

inline vec3 operator*(r32 const& scalar, vec3 left)
{
    left *= scalar;
    return left;
}

inline vec4 operator*(r32 const& scalar, vec4 left)
{
    left *= scalar;
    return left;
}

// vec4 operator overloads
inline vec4 operator+(vec4 left, vec4 const& right)
{
    left += right;
    return left;
}

inline vec4 operator-(vec4 left, vec4 const& right)
{
    left -= right;
    return left;
}

inline vec4 operator*(vec4 left, vec4 const& right)
{
    left -= right;
    return left;
}

inline vec4 operator/(vec4 left, vec4 const& right)
{
    left += right;
    return left;
}

inline vec4 operator/(vec4 left, r32 const& denominator)
{
    left /= denominator;
    return left;
}

inline vec4 operator*(vec4 left, r32 const& scalar)
{
    left *= scalar;
    return left;
}


vec2 MakeVec2(r32 *ptr)
{
    vec2 result;
    
    result.x = ptr[0];
    result.y = ptr[1];
    
    return result;
}

vec3 MakeVec3(r32 *ptr)
{
    vec3 result;
    
    result.x = ptr[0];
    result.y = ptr[1];
    result.z = ptr[2];
    
    return result;
}

vec4 MakeVec4(r32 *ptr)
{
    vec4 result;
    
    result.x = ptr[0];
    result.y = ptr[1];
    result.z = ptr[2];
    result.w = ptr[3];
    
    return result;
}


// Calculates the dot product of two vectors
inline r32 dot(vec2 const& left, vec2 const& right)
{
    return (left.x * right.x)
        + (left.y * right.y);
}

inline r32 dot(vec3 const& left, vec3 const& right)
{
    return (left.x * right.x)
        + (left.y * right.y)
        + (left.z * right.z);
}

inline r32 dot(vec4 const& left, vec4 const& right)
{
    return (left.x * right.x)
        + (left.y * right.y)
        + (left.z * right.z)
        + (left.w * right.w);
}

// Calculates the cross product of two vectors
inline r32 cross(vec2 const& left, vec2 const& right)
{
    return (left.x * right.y) - (left.y * right.x);
}

inline vec3 cross(vec3 const& left, vec3 const& right)
{
    vec3 result = {};
    
    result.x = (left.y * right.z) - (left.z * right.y);
    result.y = (left.z * right.x) - (left.x * right.z);
    result.z = (left.x * right.y) - (left.y * right.x);
    
    return result;
}

inline vec4 cross(vec4 const& left, vec4 const& right)
{
    vec4 result = {};
    
    result.xyz = cross(left.xyz, right.xyz);
    result.w   = 1;
    
    return result;
}

// Finds the squared magnitude of a vector
inline r32 mag_sq(vec2 const& v)
{
    return (v.x * v.x) + (v.y * v.y);
}

inline r32 mag_sq(vec3 const& v)
{
    return (v.x * v.x) + (v.y * v.y) + (v.z * v.z);
}

inline r32 mag_sq(vec4 const& v)
{
    return (v.x * v.x) + (v.y * v.y) + (v.z * v.z) + (v.w * v.w);
}

// Finds the magnitude of a vector
inline r32 mag(vec2 const& v)
{
    return (r32)sqrt((v.x * v.x) + (v.y * v.y));
}

inline r32 mag(vec3 const& v)
{
    return (r32)sqrt((v.x * v.x) + (v.y * v.y) + (v.z * v.z));
}

inline r32 mag(vec4 const& v)
{
    return (r32)sqrt((v.x * v.x) + (v.y * v.y) + (v.z * v.z) + (v.w * v.w));
}

// Normalizs a vector so that its magnitude == 1
inline vec2 norm(vec2 v)
{
    r32 m = mag(v);
    v /= m;
    return v;
}

inline vec3 norm(vec3 v)
{
    r32 m = mag(v);
    v /= m;
    return v;
}

inline vec4 norm(vec4 v)
{
    r32 m = mag(v);
    v /= m;
    return v;
}

//----------------------------------------------------------------------------------------//


//----------------------------------------------------------------------------------------//
// Matrix  Implementations
//----------------------------------------------------------------------------------------//

inline mat3 MakeMat3(r32 *ptr)
{
    mat3 result = mat3();
    
    result.col0 = MakeVec3(ptr);
    result.col1 = MakeVec3(ptr + 3);
    result.col2 = MakeVec3(ptr + 6);
    
    return result;
}

mat4 MakeMat4(r32 *ptr)
{
    mat4 result = mat4();
    
    result.col0 = MakeVec4(ptr);
    result.col1 = MakeVec4(ptr + 4);
    result.col2 = MakeVec4(ptr + 8);
    result.col3 = MakeVec4(ptr + 12);
    
    return result;
}

inline mat3 operator*(mat3 const& left, mat3 const& r)
{
    vec3 lr0 = { left[0][0], left[1][0], left[2][0] };
    vec3 lr1 = { left[0][1], left[1][1], left[2][1] };
    vec3 lr2 = { left[0][2], left[1][2], left[2][2] };
    
    return mat3(dot(lr0, r.col0), dot(lr0, r.col1), dot(lr0, r.col2),
                dot(lr1, r.col0), dot(lr1, r.col1), dot(lr1, r.col2),
                dot(lr2, r.col0), dot(lr2, r.col1), dot(lr2, r.col2));
}

inline mat4 operator*(mat4 const& left, mat4 const& r)
{
    vec4 lr0 = { left[0][0], left[1][0], left[2][0], left[3][0] };
    vec4 lr1 = { left[0][1], left[1][1], left[2][1], left[3][1] };
    vec4 lr2 = { left[0][2], left[1][2], left[2][2], left[3][2] };
    vec4 lr3 = { left[0][3], left[1][3], left[2][3], left[3][3] };
    
    return mat4(dot(lr0, r.col0), dot(lr0, r.col1), dot(lr0, r.col2), dot(lr0, r.col3),
                dot(lr1, r.col0), dot(lr1, r.col1), dot(lr1, r.col2), dot(lr1, r.col3),
                dot(lr2, r.col0), dot(lr2, r.col1), dot(lr2, r.col2), dot(lr2, r.col3),
                dot(lr3, r.col0), dot(lr3, r.col1), dot(lr3, r.col2), dot(lr3, r.col3));
}

inline mat3 Mul(mat3 const& left, mat3 const& right)
{
    return left * right;
}

mat4 Mul(mat4 const& left, mat4 const& right)
{
    return left * right;
}


inline mat3 RotateX(r32 theta)
{
    r32 rad = DegreesToRadians(theta);
    
    r32 c = (r32)cos(rad);
    r32 s = (r32)sin(rad);
    
    return mat3(1.0f, 0.0f, 0.0f,
                0.0f, c, -s,
                0.0f, s, c);
}

inline mat3 RotateY(r32 theta)
{
    r32 rad = DegreesToRadians(theta);
    
    r32 c = (r32)cos(rad);
    r32 s = (r32)sin(rad);
    
    return mat3(c, 0.0f, s,
                0.0f, 1.0f, 0.0f,
                -s, 0.0f, c);
}

inline mat3 RotateZ(r32 theta)
{
    r32 rad = DegreesToRadians(theta);
    
    r32 c = (r32)cos(rad);
    r32 s = (r32)sin(rad);
    
    return mat3(c, -s, 0.0f,
                s, c, 0.0f,
                0.0f, 0.0f, 1.0f);
}

inline mat3 Rotate(r32 theta, vec3 axis)
{
    r32 rad = DegreesToRadians(theta);
    axis = norm(axis);
    
    r32 c = cosf(rad);
    r32 s = sinf(rad);
    r32 d = 1.0f - c;
    
    r32 x = axis.x * d;
    r32 y = axis.y * d;
    r32 z = axis.z * d;
    r32 axay = x * axis.y;
    r32 axaz = x * axis.z;
    r32 ayaz = y * axis.z;
    
    return mat3(   c + x * axis.x, axay - s * axis.z, axaz + s * axis.y,
                axay + s * axis.z,    c + y * axis.y, ayaz - s * axis.x,
                axaz - s * axis.y, ayaz + s * axis.x, c + z * axis.z);
}

inline mat3 Reflect(vec3 axis)
{
    axis = norm(axis);
    
    r32 x = axis.x * -2.0f;
    r32 y = axis.y * -2.0f;
    r32 z = axis.z * -2.0f;
    
    r32 axay = x * axis.y;
    r32 axaz = x * axis.z;
    r32 ayaz = y * axis.z;
    
    return mat3(x * axis.x + 1.0f, axay, axaz,
                axay, y * axis.y + 1.0f, ayaz,
                axaz, ayaz, z * axis.z + 1.0f);
}

mat4 Scale(r32 sx, r32 sy, r32 sz)
{
    mat4 result = mat4(1.0f);
    
    result[0][0] = sx;
    result[1][1] = sy;
    result[2][2] = sz;
    
    return result;
}

inline mat4 Scale(r32 s, vec3 scale)
{
    scale = norm(scale);
    
    s -= 1.0f;
    r32 x = scale.x * s;
    r32 y = scale.y * s;
    r32 z = scale.z * s;
    r32 axay = x * scale.y;
    r32 axaz = x * scale.z;
    r32 ayaz = y * scale.z;
    
    mat4 result = mat4(1.0f);
    
    result[0][0] = x * scale.x + 1.0f;
    result[1][0] = axay;
    result[2][0] = axaz;
    result[3][0] = 0.0f;
    
    result[0][1] = axay;
    result[1][1] = y * scale.y + 1.0f;
    result[2][1] = ayaz;
    result[3][1] = 0.0f;
    
    result[0][2] = axaz;
    result[1][2] = ayaz;
    result[2][2] = z * scale.z + 1.0f;
    result[3][2] = 0.0f;
    
    result[0][3] = 0.0f;
    result[0][3] = 0.0f;
    result[0][3] = 0.0f;
    result[0][3] = 1.0f;
    
    return result;
    
}

// skew is the vector representing the direction along which the skew ocurs
// perp is the vector perpendicular to skew along which vectors are measured to
// to determine how to skew.
inline mat3 Skew(r32 theta, vec3 skew, vec3 perp)
{
    r32 t = DegreesToRadians(theta);
    vec3 a = norm(skew);
    vec3 b = norm(perp);
    
    r32 r = DegreesToRadians(theta);
    r = (r32)tan(r);
    r32 x = a.x * t;
    r32 y = a.y * t;
    r32 z = a.z * t;
    
    return mat3(x * b.x + 1.0f, x * b.y, x * b.z,
                y * b.x, y * b.y + 1.0f, y * b.z,
                z * b.x, z * b.y, z * b.z + 1.0f);
}

mat4 Translate(vec3 trans)
{
    mat4 result = mat4(1.0f);
    
    result[3][0] = trans.x;
    result[3][1] = trans.y;
    result[3][2] = trans.z;
    
    return result;
}

//----------------------------------------------------------------------------------------//


//----------------------------------------------------------------------------------------//
// Quaternion Implementations
//----------------------------------------------------------------------------------------//

#endif // turning off original implementation

#endif //MAPLE_VECTOR_MATH_IMPLEMENTATION

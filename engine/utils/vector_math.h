#ifndef ENGINE_UTILS_VECTOR_MATH_H
#define ENGINE_UTILS_VECTOR_MATH_H

/*

User API:

// VECTORS


inline Vec2 MakeVec2(float *ptr);
inline vec3 MakeVec3(float *ptr);
inline Vec4 MakeVec4(float *ptr);


// Calculates the dot product of two vectors
inline float dot(Vec2 const& left, Vec2 const& right);
inline float dot(vec3 const& left, vec3 const& right);
inline float dot(Vec4 const& left, Vec4 const& right);

// Calculates the cross product of two vectors
// cross pruduct of a vect2 is the same as calculating the
// determinate of 2x2 matrix
inline float cross(Vec2 const& left, Vec2 const& right);
inline vec3 cross(vec3 const& left, vec3 const& right);
// NOTE(Dustin), there really isn't an actual representation
// of a vec4 cross product. Therefore, the cross product of the
// xyz components are computed instead, where the w component is
// set to 1.
inline Vec4 cross(Vec4 const& left, Vec4 const& right);

// Finds the magnitude of a vector
inline float mag(Vec2 const& vector);
inline float mag(vec3 const& vector);
inline float mag(Vec4 const& vector);

// Finds the squared magnitude of a vector
inline float mag_sq(Vec2 const& vector);
inline float mag_sq(vec3 const& vector);
inline float mag_sq(Vec4 const& vector);

// Normalizs a vector so that its magnitude == 1
inline Vec2 norm(Vec2 vector);
inline vec3 norm(vec3 vector);
inline Vec4 norm(Vec4 vector);

// MATRICES

inline Mat3 Mul(mat3 const& left, mat3 const& right);
inline Mat4 Mul(Mat4 const& left, Mat4 const& right);
inline float DegreesToRadians(float theta);
inline Mat3 RotateX(float theta);
inline mat3 RotateY(float theta);
inline mat3 RotateZ(float theta);
inline mat3 Rotate(float theta, vec3 axis);
inline mat3 Reflect(vec3 vec3);
inline mat3 Scale(float sx, float sy, float sz);
inline mat3 Scale(float s, vec3 scale);
// skew is the vector representing the direction along which the skew ocurs
// perp is the vector perpendicular to skew along which vectors are measured to
// to determine how to skew.
inline mat3 Skew(float theta, vec3 skew, vec3 perp);
inline Mat4 Translate(vec3 trans);

// QUATERNIONS

vec3 GetQuaternionVector(Quaternion const& quat);
mat3 GetQuaternionRotationMatrix(Quaternion const& q);
// Creates a quaternion from a rotation matrix
Quaternion CreateQuaternionFromRotationMatrix(const mat3& m);
Quaternion QuaternionMul(Quaternion const& left, Quaternion const& right);
// rotates the vector around the quaternion using the sandwich product
vec3 Transform(const vec3& v, const Quaternion& q);
Quaternion norm(const Quaternion& q);

// OTHER USEFUL GRAPHICS FUNCTIONS
Mat4 LookAt(vec3 right, vec3 up, vec3 forward, vec3 position)
Mat4 PerspectiveProjection(float fov, float aspect_ratio, float near, float far);

*/

inline float DegreesToRadians(float theta);

struct vec2
{
    union
    {
        struct { float x, y; };
        struct { float r, g; };
        
        float data[2];
    };
    
    inline float operator[](int i)
    {
        return data[i];
    }
    
    vec2& operator+=(vec2 const& other)
    {
        x += other.x;
        y += other.y;
        
        return *this;
    }
    
    vec2& operator-=(vec2 const& other)
    {
        x -= other.x;
        y -= other.y;
        
        return *this;
    }
    
    vec2& operator*=(vec2 const& other)
    {
        x *= other.x;
        y *= other.y;
        
        return *this;
    }
    
    vec2& operator/=(vec2 const& other)
    {
        x = (other.x == 0) ? 0 : x / other.x;
        y = (other.y == 0) ? 0 : y / other.y;
        
        return *this;
    }
    
    vec2& operator/=(float const& other)
    {
        if (other != 0)
        {
            x /= other;
            y /= other;
        }
        
        return *this;
    }
    
    vec2& operator*=(float const& other)
    {
        x *= other;
        y *= other;
        
        return *this;
    }
};

struct vec3
{
    union
    {
        struct { float x, y, z;     };
        struct { vec2 xy; float p0; };
        struct { float p1; vec2 yz; };
        
        struct { float r, g, b;     };
        struct { vec2 rg; float p2; };
        struct { float p3; vec2 gb; };
        
        float data[3];
    };
    
    inline float &operator[](const int &i)
    {
        return data[i];
    }
    
    vec3& operator+=(vec3 const& other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        
        return *this;
    }
    
    vec3& operator-=(vec3 const& other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        
        return *this;
    }
    
    vec3& operator*=(vec3 const& other)
    {
        x *= other.x;
        y *= other.y;
        z *= other.z;
        
        return *this;
    }
    
    vec3& operator/=(vec3 const& other)
    {
        x = (other.x == 0) ? 0 : x / other.x;
        y = (other.y == 0) ? 0 : y / other.y;
        z = (other.z == 0) ? 0 : z / other.z;
        
        return *this;
    }
    
    vec3& operator/=(float const& other)
    {
        if (other != 0)
        {
            x /= other;
            y /= other;
            z /= other;
        }
        
        return *this;
    }
    
    vec3& operator*=(float const& other)
    {
        x *= other;
        y *= other;
        z *= other;
        
        return *this;
    }
};

struct vec4
{
    union
    {
        struct { float x, y, z, w;            };
        struct { vec2 xy, zw;                 };
        struct { float p0; vec2 yz; float p1; };
        struct { vec3 xyz; float p2;          };
        struct { float p3; vec3 yzw;          };
        
        struct { float r, g, b, a;            };
        struct { vec2 rg, ba;                 };
        struct { vec3 rgb; float p4;          };
        struct { float p5; vec2 gb; float p6; };
        struct { float p7; vec3 gba;          };
        
        float data[4];
    };
    
    inline float &operator[](const int &i)
    {
        return data[i];
    }
    
    vec4& operator+=(vec4 const& other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        w += other.w;
        
        return *this;
    }
    
    vec4& operator-=(vec4 const& other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        w -= other.w;
        
        return *this;
    }
    
    vec4& operator*=(vec4 const& other)
    {
        x *= other.x;
        y *= other.y;
        z *= other.z;
        w *= other.w;
        
        return *this;
    }
    
    vec4& operator/=(vec4 const& other)
    {
        x = (other.x == 0) ? 0 : x / other.x;
        y = (other.y == 0) ? 0 : y / other.y;
        z = (other.z == 0) ? 0 : z / other.z;
        w = (other.w == 0) ? 0 : w / other.w;
        
        return *this;
    }
    
    vec4& operator/=(float const& other)
    {
        if (other != 0)
        {
            x /= other;
            y /= other;
            z /= other;
            w /= other;
        }
        
        return *this;
    }
    
    vec4& operator*=(float const& other)
    {
        x *= other;
        y *= other;
        z *= other;
        w *= other;
        
        return *this;
    }
};

// Column major
struct mat3
{
    union
    {
        float data[9];
        
        struct
        {
            vec3 col0;
            vec3 col1;
            vec3 col2;
        };
    };
    
    float *operator[](int idx)
    {
        return &data[3*idx];
    }
    
    const float* operator[](int idx) const
    {
        return &data[3*idx];
    }
    
    // Expects the data to be passed in row order
    // and is converted to column order
    mat3(float a, float b, float c,
         float e, float f, float g,
         float i, float j, float k)
    {
        data[3*0 + 0] = a;
        data[3*0 + 1] = e;
        data[3*0 + 2] = i;
        
        data[3*1 + 0] = b;
        data[3*1 + 1] = f;
        data[3*1 + 2] = j;
        
        data[3*2 + 0] = c;
        data[3*2 + 1] = g;
        data[3*2 + 2] = k;
    }
    
    mat3(float ptr[3][3])
    {
        data[3*0 + 0] = ptr[0][0];
        data[3*0 + 1] = ptr[0][1];
        data[3*0 + 2] = ptr[0][2];
        
        data[3*1 + 0] = ptr[1][0];
        data[3*1 + 1] = ptr[1][1];
        data[3*1 + 2] = ptr[1][2];
        
        data[3*2 + 0] = ptr[2][0];
        data[3*2 + 1] = ptr[2][1];
        data[3*2 + 2] = ptr[2][2];
    }
    
    mat3(float Diagonal)
    {
        data[3*0 + 0] = Diagonal;
        data[3*0 + 1] = 0;
        data[3*0 + 2] = 0;
        
        data[3*1 + 0] = 0;
        data[3*1 + 1] = Diagonal;
        data[3*1 + 2] = 0;
        
        data[3*2 + 0] = 0;
        data[3*2 + 1] = 0;
        data[3*2 + 2] = Diagonal;
    }
    
    mat3()
    {
        data[3*0 + 0] = 1;
        data[3*0 + 1] = 0;
        data[3*0 + 2] = 0;
        
        data[3*1 + 0] = 0;
        data[3*1 + 1] = 1;
        data[3*1 + 2] = 0;
        
        data[3*2 + 0] = 0;
        data[3*2 + 1] = 0;
        data[3*2 + 2] = 1;
    }
};

struct mat4
{
    union
    {
        float data[4][4];
        
        struct
        { // useful for matrix math?
            vec4 col0;
            vec4 col1;
            vec4 col2;
            vec4 col3;
        };
    };
    
    float* operator[](int idx)
    {
        return &data[idx][0];
    }
    
    const float* operator[](int idx) const
    {
        return &data[idx][0];
    }
    
    mat4(float diagonal = 0.0f)
    {
        data[0][0] = diagonal;
        data[0][1] = 0.0f;
        data[0][2] = 0.0f;
        data[0][3] = 0.0f;
        
        data[1][0] = 0.0f;
        data[1][1] = diagonal;
        data[1][2] = 0.0f;
        data[1][3] = 0.0f;
        
        data[2][0] = 0.0f;
        data[2][1] = 0.0f;
        data[2][2] = diagonal;
        data[2][3] = 0.0f;
        
        data[3][0] = 0.0f;
        data[3][1] = 0.0f;
        data[3][2] = 0.0f;
        data[3][3] = diagonal;
    }
    
    // Expects the data to be passed in row order
    // and is converted to column order
    mat4(float a, float b, float c, float d,
         float e, float f, float g, float h,
         float i, float j, float k, float l,
         float m, float n, float o, float p)
    {
        data[0][0] = a;
        data[0][1] = e;
        data[0][2] = i;
        data[0][3] = m;
        
        data[1][0] = b;
        data[1][1] = f;
        data[1][2] = j;
        data[1][3] = n;
        
        data[2][0] = c;
        data[2][1] = g;
        data[2][2] = k;
        data[2][3] = o;
        
        data[3][0] = d;
        data[3][1] = h;
        data[3][2] = l;
        data[3][3] = p;
    }
    
    mat4(mat3 mat)
    {
        
        data[0][0] = mat[0][0];
        data[0][1] = mat[0][1];
        data[0][2] = mat[0][2];
        data[0][3] = 0.0f;
        
        data[1][0] = mat[1][0];
        data[1][1] = mat[1][1];
        data[1][2] = mat[1][2];
        data[1][3] = 0.0f;
        
        data[2][0] = mat[2][0];
        data[2][1] = mat[2][1];
        data[2][2] = mat[2][2];
        data[2][3] = 0.0f;
        
        data[3][0] = 0.0f;
        data[3][1] = 0.0f;
        data[3][2] = 0.0f;
        data[3][3] = 1.0f;
    }
};

struct Quaternion
{
    union
    {
        struct { float x, y, z, w;  };
        struct { vec3 xyz; float p0; };
    };
    
    /*
    Quaternion(float _x, float _y, float _z, float _w)
    {
        x = _x;
        y = _y;
        z = _z;
        w = _w;
    }
*/
};


//----------------------------------------------------------------------------------------//
// Pre-declarations
//----------------------------------------------------------------------------------------//

// Operator overloads
inline vec2 operator+(vec2 left, vec2 const& right);
inline vec3 operator+(vec3 left, vec3 const& right);
inline vec4 operator+(vec4 left, vec4 const& right);

inline vec2 operator-(vec2 left, vec2 const& right);
inline vec3 operator-(vec3 left, vec3 const& right);
inline vec4 operator-(vec4 left, vec4 const& right);

inline vec2 operator*(vec2 left, vec2 const& right);
inline vec3 operator*(vec3 left, vec3 const& right);
inline vec4 operator*(vec4 left, vec4 const& right);

inline vec2 operator/(vec2 left, vec2 const& right);
inline vec3 operator/(vec3 left, vec3 const& right);
inline vec4 operator/(vec4 left, vec4 const& right);

inline vec2 operator/(vec2 left, float const& denominator);
inline vec3 operator/(vec3 left, float const& denominator);
inline vec4 operator/(vec4 left, float const& denominator);

inline vec2 operator*(vec2 left, float const& scalar);
inline vec3 operator*(vec3 left, float const& scalar);
inline vec4 operator*(vec4 left, float const& scalar);

inline vec2 operator*(float const& scalar, vec2 left);
inline vec3 operator*(float const& scalar, vec3 left);
inline vec4 operator*(float const& scalar, vec4 left);

inline mat3 operator*(mat3 const& left, mat3 const& r);
inline mat4 operator*(mat4 const& left, mat4 const& r);

Quaternion operator*(const Quaternion& l, const Quaternion& r);

// VECTORS

vec2 MakeVec2(float *ptr);
vec3 MakeVec3(float *ptr);
vec4 MakeVec4(float *ptr);

// Calculates the dot product of two vectors
inline float dot(vec2 const& left, vec2 const& right);
inline float dot(vec3 const& left, vec3 const& right);
inline float dot(vec4 const& left, vec4 const& right);

// Calculates the cross product of two vectors
// cross pruduct of a vect2 is the same as calculating the
// determinate of 2x2 matrix
inline float cross(vec2 const& left, vec2 const& right);
inline vec3 cross(vec3 const& left, vec3 const& right);
// NOTE(Dustin), there really isn't an actual representation
// of a vec4 cross product. Therefore, the cross product of the
// xyz components are computed instead, where the w component is
// set to 1.
inline vec4 cross(vec4 const& left, vec4 const& right);

// Finds the magnitude of a vector
inline float mag(vec2 const& vector);
inline float mag(vec3 const& vector);
inline float mag(vec4 const& vector);

// Finds the squared magnitude of a vector
inline float mag_sq(vec2 const& vector);
inline float mag_sq(vec3 const& vector);
inline float mag_sq(vec4 const& vector);

// Normalizs a vector so that its magnitude == 1
inline vec2 norm(vec2 vector);
inline vec3 norm(vec3 vector);
inline vec4 norm(vec4 vector);

// MATRICES

inline mat3 MakeMat3(float *ptr);
mat4 MakeMat4(float *ptr);

inline mat3 Mul(mat3 const& left, mat3 const& right);
mat4 Mul(mat4 const& left, mat4 const& right);
inline mat3 RotateX(float theta);
inline mat3 RotateY(float theta);
inline mat3 RotateZ(float theta);
inline mat3 Rotate(float theta, vec3 axis);
inline mat3 Reflect(vec3 vec3);
mat4 Scale(float sx, float sy, float sz);
inline mat4 Scale(float s, vec3 scale);
// skew is the vector representing the direction along which the skew ocurs
// perp is the vector perpendicular to skew along which vectors are measured to
// to determine how to skew.
inline mat3 Skew(float theta, vec3 skew, vec3 perp);

mat4 Translate(vec3 trans);

// QUATERNIONS

Quaternion MakeQuaternion(float x, float y, float z, float w);
Quaternion CreateQuaternion(vec3 axis, float theta);
vec3 GetQuaternionVector(Quaternion const& quat);
mat3 GetQuaternionRotationMatrix(Quaternion const& q);
// Creates a quaternion from a rotation matrix
Quaternion CreateQuaternionFromRotationMatrix(const mat3& m);
Quaternion QuaternionMul(Quaternion const& left, Quaternion const& right);
// rotates the vector around the quaternion using the sandwich product
vec3 Transform(const vec3& v, const Quaternion& q);
Quaternion norm(const Quaternion& q);

Quaternion conjugate(const Quaternion& q);

// OTHER USEFUL GRAPHICS FUNCTIONS
mat4 LookAt(vec3 eye, vec3 center, vec3 up);
mat4 PerspectiveProjection(float fov, float aspect_ratio, float near, float far);


#endif // JENGINE_UTILS_VECTOR_MATH_H

#if defined(MAPLE_VECTOR_MATH_IMPLEMENTATION)


//----------------------------------------------------------------------------------------//

#define JPI 3.1415926535

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

inline vec2 operator/(vec2 left, float const& denominator)
{
    left /= denominator;
    return left;
}

inline vec2 operator*(vec2 left, float const& scalar)
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

inline vec3 operator/(vec3 left, float const& denominator)
{
    left /= denominator;
    return left;
}

inline vec3 operator*(vec3 left, float const& scalar)
{
    left *= scalar;
    return left;
}


inline vec2 operator*(float const& scalar, vec2 left)
{
    left *= scalar;
    return left;
}

inline vec3 operator*(float const& scalar, vec3 left)
{
    left *= scalar;
    return left;
}

inline vec4 operator*(float const& scalar, vec4 left)
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

inline vec4 operator/(vec4 left, float const& denominator)
{
    left /= denominator;
    return left;
}

inline vec4 operator*(vec4 left, float const& scalar)
{
    left *= scalar;
    return left;
}


vec2 MakeVec2(float *ptr)
{
    vec2 result;
    
    result.x = ptr[0];
    result.y = ptr[1];
    
    return result;
}

vec3 MakeVec3(float *ptr)
{
    vec3 result;
    
    result.x = ptr[0];
    result.y = ptr[1];
    result.z = ptr[2];
    
    return result;
}

vec4 MakeVec4(float *ptr)
{
    vec4 result;
    
    result.x = ptr[0];
    result.y = ptr[1];
    result.z = ptr[2];
    result.w = ptr[3];
    
    return result;
}


// Calculates the dot product of two vectors
inline float dot(vec2 const& left, vec2 const& right)
{
    return (left.x * right.x)
        + (left.y * right.y);
}

inline float dot(vec3 const& left, vec3 const& right)
{
    return (left.x * right.x)
        + (left.y * right.y)
        + (left.z * right.z);
}

inline float dot(vec4 const& left, vec4 const& right)
{
    return (left.x * right.x)
        + (left.y * right.y)
        + (left.z * right.z)
        + (left.w * right.w);
}

// Calculates the cross product of two vectors
inline float cross(vec2 const& left, vec2 const& right)
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
inline float mag_sq(vec2 const& v)
{
    return (v.x * v.x) + (v.y * v.y);
}

inline float mag_sq(vec3 const& v)
{
    return (v.x * v.x) + (v.y * v.y) + (v.z * v.z);
}

inline float mag_sq(vec4 const& v)
{
    return (v.x * v.x) + (v.y * v.y) + (v.z * v.z) + (v.w * v.w);
}

// Finds the magnitude of a vector
inline float mag(vec2 const& v)
{
    return (float)sqrt((v.x * v.x) + (v.y * v.y));
}

inline float mag(vec3 const& v)
{
    return (float)sqrt((v.x * v.x) + (v.y * v.y) + (v.z * v.z));
}

inline float mag(vec4 const& v)
{
    return (float)sqrt((v.x * v.x) + (v.y * v.y) + (v.z * v.z) + (v.w * v.w));
}

// Normalizs a vector so that its magnitude == 1
inline vec2 norm(vec2 v)
{
    float m = mag(v);
    v /= m;
    return v;
}

inline vec3 norm(vec3 v)
{
    float m = mag(v);
    v /= m;
    return v;
}

inline vec4 norm(vec4 v)
{
    float m = mag(v);
    v /= m;
    return v;
}

//----------------------------------------------------------------------------------------//


//----------------------------------------------------------------------------------------//
// Matrix  Implementations
//----------------------------------------------------------------------------------------//

inline mat3 MakeMat3(float *ptr)
{
    mat3 result = mat3();
    
    result.col0 = MakeVec3(ptr);
    result.col1 = MakeVec3(ptr + 3);
    result.col2 = MakeVec3(ptr + 6);
    
    return result;
}

mat4 MakeMat4(float *ptr)
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


inline float DegreesToRadians(float theta)
{
    return (float)(theta * (JPI / 180.0f));
}

inline mat3 RotateX(float theta)
{
    float rad = DegreesToRadians(theta);
    
    float c = (float)cos(rad);
    float s = (float)sin(rad);
    
    return mat3(1.0f, 0.0f, 0.0f,
                0.0f, c, -s,
                0.0f, s, c);
}

inline mat3 RotateY(float theta)
{
    float rad = DegreesToRadians(theta);
    
    float c = (float)cos(rad);
    float s = (float)sin(rad);
    
    return mat3(c, 0.0f, s,
                0.0f, 1.0f, 0.0f,
                -s, 0.0f, c);
}

inline mat3 RotateZ(float theta)
{
    float rad = DegreesToRadians(theta);
    
    float c = (float)cos(rad);
    float s = (float)sin(rad);
    
    return mat3(c, -s, 0.0f,
                s, c, 0.0f,
                0.0f, 0.0f, 1.0f);
}

inline mat3 Rotate(float theta, vec3 axis)
{
    float rad = DegreesToRadians(theta);
    axis = norm(axis);
    
    float c = cosf(rad);
    float s = sinf(rad);
    float d = 1.0f - c;
    
    float x = axis.x * d;
    float y = axis.y * d;
    float z = axis.z * d;
    float axay = x * axis.y;
    float axaz = x * axis.z;
    float ayaz = y * axis.z;
    
    return mat3(   c + x * axis.x, axay - s * axis.z, axaz + s * axis.y,
                axay + s * axis.z,    c + y * axis.y, ayaz - s * axis.x,
                axaz - s * axis.y, ayaz + s * axis.x, c + z * axis.z);
}

inline mat3 Reflect(vec3 axis)
{
    axis = norm(axis);
    
    float x = axis.x * -2.0f;
    float y = axis.y * -2.0f;
    float z = axis.z * -2.0f;
    
    float axay = x * axis.y;
    float axaz = x * axis.z;
    float ayaz = y * axis.z;
    
    return mat3(x * axis.x + 1.0f, axay, axaz,
                axay, y * axis.y + 1.0f, ayaz,
                axaz, ayaz, z * axis.z + 1.0f);
}

mat4 Scale(float sx, float sy, float sz)
{
    mat4 result = mat4(1.0f);
    
    result[0][0] = sx;
    result[1][1] = sy;
    result[2][2] = sz;
    
    return result;
}

inline mat4 Scale(float s, vec3 scale)
{
    scale = norm(scale);
    
    s -= 1.0f;
    float x = scale.x * s;
    float y = scale.y * s;
    float z = scale.z * s;
    float axay = x * scale.y;
    float axaz = x * scale.z;
    float ayaz = y * scale.z;
    
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
inline mat3 Skew(float theta, vec3 skew, vec3 perp)
{
    float t = DegreesToRadians(theta);
    vec3 a = norm(skew);
    vec3 b = norm(perp);
    
    float r = DegreesToRadians(theta);
    r = (float)tan(r);
    float x = a.x * t;
    float y = a.y * t;
    float z = a.z * t;
    
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

Quaternion MakeQuaternion(float x, float y, float z, float w)
{
    Quaternion result = {};
    
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;
    
    return result;
}


// Creates a quaternion for rotation from an axis and angle (degrees)
Quaternion CreateQuaternion(vec3 axis, r32 theta)
{
    r32 r = DegreesToRadians(theta);
    r32 half_angle = r * 0.5f;
    
    r32 x = axis.x * sinf(half_angle);
    r32 y = axis.y * sinf(half_angle);
    r32 z = axis.z * sinf(half_angle);
    r32 w = cosf(half_angle);
    
    Quaternion result = {};
    
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;
    
    return result;
}

Quaternion operator*(const Quaternion& l, const Quaternion& r)
{
    Quaternion result = {};
    
    result.x = l.w * r.x + l.x * r.w + l.y * r.z - l.z * r.y;
    result.y = l.w * r.y - l.x * r.z + l.y * r.w + l.z * r.x;
    result.z = l.w * r.z + l.x * r.y - l.y * r.x + l.z * r.w;
    result.w = l.w * r.w - l.x * r.x - l.y * r.y - l.z * r.z;
    
    return result;
}

Quaternion QuaternionMul(Quaternion const& left, Quaternion const& right)
{
    return left * right;
}


vec3 GetQuaternionVector(Quaternion const& quat)
{
    return quat.xyz;
}


Quaternion conjugate(const Quaternion& q)
{
    Quaternion result = {};
    
    result.x = -q.x;
    result.y = -q.y;
    result.z = -q.z;
    result.w = q.w;
    
    return result;
}

Quaternion norm(const Quaternion& q)
{
    float d = 1/(float)sqrt(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w);
    
    Quaternion result = {};
    
    result.x = q.x*d;
    result.y = q.y*d;
    result.z = q.z*d;
    result.w = q.w*d;
    
    return result;
}

vec3 Transform(const vec3& v, const Quaternion& q)
{
    const vec3& b = q.xyz;
    float b2 = b.x * b.x + b.y * b.y + b.z * b.z;
    return v * (q.w * q.w - b2) +
        b * (dot(v, b) * 2.0f) +
        cross(b, v) * (q.w * 2.0f);
}

mat3 GetQuaternionRotationMatrix(Quaternion const& quat)
{
    Quaternion q = norm(quat);
    
    float x2 = q.x * q.x;
    float y2 = q.y * q.y;
    float z2 = q.z * q.z;
    float xy = q.x * q.y;
    float xz = q.x * q.z;
    float yz = q.y * q.z;
    float wx = q.w * q.x;
    float wy = q.w * q.y;
    float wz = q.w * q.z;
    
    return mat3(1.0f - 2.0f * (y2 + z2), 2.0f * (xy - wz), 2.0f * (xz + wy),
                2.0f * (xy + wz), 1.0f - 2.0f * (x2 + z2), 2.0f * (yz - wx),
                2.0f * (xz - wy), 2.0f * (yz + wx), 1.0f - 2.0f * (x2 + y2));
}

// TODO(Dustin): The if-else might need to swap the matrix indexes
Quaternion CreateQuaternionFromRotationMatrix(const mat3& m)
{
    float x, y, z, w;
    
    float m00 = m[0][0];
    float m11 = m[1][1];
    float m22 = m[2][2];
    float sum = m00 + m11 + m22;
    
    if (sum > 0.0f)
    {
        w = (float)sqrt(sum + 1.0f) * 0.5f;
        float f = 0.25f / w;
        x = (m[2][1] - m[1][2]) * f;
        y = (m[0][2] - m[2][0]) * f;
        z = (m[1][0] - m[0][1]) * f;
    }
    else if ((m00 > m11) && (m00 > m22))
    {
        x = (float)sqrt(m00 - m11 - m22 + 1.0f) * 0.5f;
        float f = 0.25f / x;
        
        y = (m[1][0] + m[0][1]) * f;
        z = (m[0][2] + m[2][0]) * f;
        w = (m[2][1] - m[1][2]) * f;
    }
    else if (m11 > m22)
    {
        y = (float)sqrt(m11 - m00 - m22 + 1.0f) * 0.5f;
        float f = 0.25f / y;
        
        x = (m[1][0] + m[0][1]) * f;
        z = (m[2][1] + m[1][2]) * f;
        w = (m[0][2] - m[2][1]) * f;
    }
    else
    {
        z = (float)sqrt(m22 - m00 - m11 + 1.0f) * 0.5f;
        float f = 0.25f / z;
        
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

//----------------------------------------------------------------------------------------//

mat4 LookAt(vec3 eye, vec3 center, vec3 up)
{
    mat4 result = mat4(1.0f);
    
    vec3 f = norm(center - eye);
    vec3 s = norm(cross(f, up));
    vec3 u = cross(s, f);
    
    result[0][0] = s.x;
    result[0][1] = u.x;
    result[0][2] = -f.x;
    result[0][3] = 0.0f;
    
    result[1][0] = s.y;
    result[1][1] = u.y;
    result[1][2] = -f.y;
    result[1][3] = 0.0f;
    
    result[2][0] = s.z;
    result[2][1] = u.z;
    result[2][2] = -f.z;
    result[2][3] = 0.0f;
    
    result[3][0] = -dot(s, eye);
    result[3][1] = -dot(u, eye);
    result[3][2] = dot(f, eye);
    result[3][3] = 1.0f;
    
    return result;
}

mat4 PerspectiveProjection(float fov, float aspect_ratio, float near_plane, float far_plane)
{
    mat4 result = mat4(1.0f);
    
    float cotangent = 1.0f / tanf(fov * ((float)JPI / 360.0f));
    
    result[0][0] = cotangent / aspect_ratio;
    result[1][1] = cotangent;
    result[2][3] = -1.0f;
    result[2][2] = (near_plane + far_plane) / (near_plane - far_plane);
    result[3][2] = (2.0f * near_plane * far_plane) / (near_plane - far_plane);
    result[3][3] = 0.0f;
    
    return result;
}

#endif //MAPLE_VECTOR_MATH_IMPLEMENTATION

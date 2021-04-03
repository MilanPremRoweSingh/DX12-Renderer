#include "Camera.h"


Camera::Camera(
    Vector3& vecEye,
    Vector3& vecTarget,
    Vector3& vecUp,
    float flNear,
    float flFar,
    float flFOVInDeg,
    float flAspectRatio)
{
    sCalcLookAtMatrix(vecEye, vecTarget, vecUp, m_matView);
    sCalcProjMatrix(flNear, flFar, flFOVInDeg, flAspectRatio, m_matProj);
}

Camera::Camera(
    float yaw,
    float pitch,
    float roll,
    Vector3& eye,
    float flNear,
    float flFar,
    float flFOVInDeg,
    float flAspectRatio)
{
    sCalcTransformationMatrix(yaw, pitch, roll, eye, m_matView);
    sCalcProjMatrix(flNear, flFar, flFOVInDeg, flAspectRatio, m_matProj);
}

void Camera::SetViewMatrix(
    const Matrix4x4& matView)
{
    m_matView = matView;
}

void Camera::SetProjMatrix(
    const Matrix4x4& matProj)
{
    m_matProj = matProj;
}

void Camera::GetViewMatrix(
    Matrix4x4& matViewOut) const
{
    matViewOut = m_matView;
}

void Camera::GetProjMatrix(
    Matrix4x4& matProjOut) const
{
    matProjOut = m_matProj;
}

void Camera::GetViewProjMatrix(
    Matrix4x4& vpMatOut) const
{
    vpMatOut = m_matProj * m_matView;
}

void Camera::sCalcProjMatrix(
    float n, 
    float f, 
    float fovInDeg,
    float aspectRatio,
    Matrix4x4& projMatOut)
{
    float s = 1.0f / tanf(fovInDeg * F_DEG_TO_RAD / 2.0f);

    projMatOut = Matrix4x4(
        s / aspectRatio, 0.0f, 0.0f, 0.0f,
        0.0f, s , 0.0f, 0.0f,
        0.0f, 0.0f, f / (f - n), -n*f / (f - n),
        0.0f, 0.0f, 1.0f, 0.0f
    );
}

void Camera::sCalcLookAtMatrix(
    const Vector3& eye, 
    const Vector3& target, 
    const Vector3& up,
    Matrix4x4& matLookAtOut)
{
    assert(abs(up.Length() - 1.0f) <= FLT_EPSILON);

    Vector3 look = target - eye;
    look.Normalize();
    
    Vector3 right;
    up.Cross(look, right);
    right.Normalize();

    sCalcTransformationMatrix(right, up, look, eye, matLookAtOut);
}

// RrRpRy
void Camera::sCalcTransformationMatrix(
    float yaw,
    float pitch,
    float roll,
    Vector3& pos,
    Matrix4x4& matTransformOut)
{
    float gamma = yaw;
    float alpha = pitch;
    float beta = roll;

    /*
    Vector3 right = {
        cosf(beta)*cosf(gamma) - sinf(beta)*sinf(alpha)*sinf(gamma),
        -cosf(alpha)*sinf(beta),
        cosf(beta)*sinf(gamma) + cosf(gamma)*sinf(beta)*sinf(alpha)
    };
        
    Vector3 up = {
        cosf(gamma)*sinf(beta) + cosf(beta)*sinf(alpha)*sinf(gamma),
        cosf(alpha)*sinf(beta) + cosf(beta)*sinf(alpha),
        sinf(beta)*sinf(gamma) - cosf(beta)*cosf(gamma)*sinf(alpha)
    };
    
    Vector3 look = {
          -cosf(alpha)*sinf(gamma),
          sinf(alpha),
          cosf(alpha)*cosf(gamma)
    };
    */
    
    Vector3 right = {
        cosf(beta)*cosf(gamma) - sinf(beta)*sinf(alpha)*sinf(gamma),
        cosf(gamma)*sinf(beta) + cosf(beta)*sinf(alpha)*sinf(gamma),
        -cosf(alpha)*sinf(gamma),
    };
        
    Vector3 up = {
        -cosf(alpha)*sinf(beta),
        cosf(alpha)*cosf(beta),
        sinf(alpha),
    };
    
    Vector3 look = {
        cosf(beta)*sinf(gamma) + cosf(gamma)*sinf(beta)*sinf(alpha),
        sinf(beta)*sinf(gamma) - cosf(beta)*cosf(gamma)*sinf(alpha),
        cosf(alpha)* cosf(gamma)
    };

    sCalcTransformationMatrix(right, up, look, pos, matTransformOut);
}

void Camera::sCalcTransformationMatrix(
    const Vector3& right,
    const Vector3& up,
    const Vector3& forward,
    const Vector3& pos,
    Matrix4x4& matTransformOut)
{
    matTransformOut._11 = right.x;
    matTransformOut._12 = right.y;
    matTransformOut._13 = right.z;
    matTransformOut._14 = -right.Dot(pos);

    matTransformOut._21 = up.x;
    matTransformOut._22 = up.y;
    matTransformOut._23 = up.z;
    matTransformOut._24 = -up.Dot(pos);

    matTransformOut._31 = forward.x;
    matTransformOut._32 = forward.y;
    matTransformOut._33 = forward.z;
    matTransformOut._34 = -forward.Dot(pos);

    matTransformOut._41 = 0.0f;
    matTransformOut._42 = 0.0f;
    matTransformOut._43 = 0.0f;
    matTransformOut._44 = 1.0f;
}
#include "Camera.h"


Camera::Camera(
    Vector3& vecEye,
    Vector3& vecTarget,
    Vector3& vecUp,
    float flNear,
    float flFar,
    float flFOVInDeg)
{
    sCalcLookAtMatrix(vecEye, vecTarget, vecUp, m_matView);
    sCalcProjMatrix(flNear, flFar, flFOVInDeg, m_matProj);
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
    float flNear, 
    float flFar, 
    float flFOVInDeg,
    Matrix4x4& projMatOut)
{
    float s = 1.0f / tanf(flFOVInDeg * F_DEG_TO_RAD / 2.0f);

    projMatOut = Matrix4x4(
        s, 0.0f, 0.0f, 0.0f,
        0.0f, s, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f / (flFar - flNear), 2 * flNear / (flFar - flNear),
        0.0f, 0.0f, 1.0f, 0.0f
    );
}

void Camera::sCalcLookAtMatrix(
    const Vector3& vecEye, 
    const Vector3& vecTarget, 
    const Vector3& vecUp,
    Matrix4x4& matLookAtOut)
{
    assert(abs(vecUp.Length() - 1.0f) <= FLT_EPSILON);

    Vector3 vecLook = vecTarget - vecEye;
    vecLook.Normalize();
    
    Vector3 vecRight;
    vecUp.Cross(vecLook, vecRight);
    vecRight.Normalize();

    matLookAtOut._11 = vecRight.x;
    matLookAtOut._12 = vecRight.y;
    matLookAtOut._13 = vecRight.z;
    matLookAtOut._14 = -vecRight.Dot(vecEye);

    matLookAtOut._21 = vecUp.x;
    matLookAtOut._22 = vecUp.y;
    matLookAtOut._23 = vecUp.z;
    matLookAtOut._24 = -vecUp.Dot(vecEye);

    matLookAtOut._31 = vecLook.x;
    matLookAtOut._32 = vecLook.y;
    matLookAtOut._33 = vecLook.z;
    matLookAtOut._34 = -vecLook.Dot(vecEye);

    matLookAtOut._41 = 0.0f;
    matLookAtOut._42 = 0.0f;
    matLookAtOut._43 = 0.0f;
    matLookAtOut._44 = 1.0f;
}
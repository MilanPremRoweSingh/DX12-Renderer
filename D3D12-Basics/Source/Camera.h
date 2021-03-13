#pragma once

// For now, a camera is just a view matrix + a projection matrix. 
// +Z is look direction, and we use RH coords so X is Left Y is Up
// Not efficient at all
class Camera
{
public:
    Camera(
        Vector3& vecEye,
        Vector3& vecTarget,
        Vector3& vecUp,
        float flNear,
        float flFar,
        float flFOVInDeg);

    void SetViewMatrix(
        const Matrix4x4& matView);

    void SetProjMatrix(
        const Matrix4x4& matProj);

    void GetViewMatrix(
        Matrix4x4& matView) const;

    void GetProjMatrix(
        Matrix4x4& matProj) const;

    void GetViewProjMatrix(
        Matrix4x4& matVP) const;

    static void sCalcProjMatrix(
        float flNeat, 
        float flFar, 
        float flFOVInDeg,
        Matrix4x4& projMatOut);

    static void sCalcLookAtMatrix(
        const Vector3& vecEye,
        const Vector3& vecTarget,
        const Vector3& vecUp,
        Matrix4x4& matLookAtOut);

private:
    Matrix4x4 m_matView;
    Matrix4x4 m_matProj;
};


#pragma once
#include <DirectXMath.h>


class Camera
{
protected:
	DirectX::XMVECTOR pos;
	DirectX::XMVECTOR up;
	DirectX::XMVECTOR lookAt;
	DirectX::XMMATRIX projMat;

public:
	Camera();
	Camera(DirectX::XMVECTOR init_pos, DirectX::XMVECTOR init_up, DirectX::XMVECTOR init_lookAt);
	~Camera();

	// Accessor methods
	void setProjMatrix(DirectX::XMMATRIX setProjMat);
	void setLookAt(DirectX::XMVECTOR init_lookAt);
	void LookAt(DirectX::FXMVECTOR pos, DirectX::FXMVECTOR target, DirectX::FXMVECTOR worldUp);
	void setPos(DirectX::XMVECTOR init_pos);
	void setUp(DirectX::XMVECTOR init_up);
	DirectX::XMMATRIX getViewMatrix();
	DirectX::XMMATRIX getProjMatrix();
	DirectX::XMVECTOR getPos();
	DirectX::XMVECTOR getLookAt();
	DirectX::XMVECTOR getUp();
	void SetLens(float fovY, float aspect, float zn, float zf);
	void UpdateViewMatrix();

	// Cache frustum properties.
	float mNearZ;
	float mFarZ;
	float mAspect;
	float mFovY;
	float mNearWindowHeight;
	float mFarWindowHeight;

	DirectX::XMFLOAT4X4 mView;
	DirectX::XMFLOAT4X4 mProj;

	// Camera coordinate system with coordinates relative to world space.
	DirectX::XMFLOAT3 mPosition;
	DirectX::XMFLOAT3 mRight;
	DirectX::XMFLOAT3 mUp;
	DirectX::XMFLOAT3 mLook;
};


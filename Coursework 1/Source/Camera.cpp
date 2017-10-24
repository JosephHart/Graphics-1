#include "stdafx.h"
#include "Camera.h"


Camera::Camera() {
	pos = DirectX::XMVectorSet(0, 0, -10, 1.0f);
	up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	lookAt = DirectX::XMVectorZero();
}
Camera::Camera(DirectX::XMVECTOR init_pos, DirectX::XMVECTOR init_up, DirectX::XMVECTOR init_lookAt) {
	pos = init_pos;
	up = init_up;
	lookAt = init_lookAt;
}
Camera::~Camera()
{
}

//
// Accessor methods
//
void Camera::setProjMatrix(DirectX::XMMATRIX setProjMat) {
	projMat = setProjMat;
}
void Camera::setLookAt(DirectX::XMVECTOR init_lookAt) {
	lookAt = init_lookAt;
}
void Camera::LookAt(DirectX::FXMVECTOR pos, DirectX::FXMVECTOR target, DirectX::FXMVECTOR worldUp)
{
	DirectX::XMVECTOR L = XMVector3Normalize(XMVectorSubtract(target, pos));
	DirectX::XMVECTOR R = XMVector3Normalize(XMVector3Cross(worldUp, L));
	DirectX::XMVECTOR U = XMVector3Cross(L, R);

	Camera::pos = pos;
	lookAt = L;
	up = U;
}
void Camera::setPos(DirectX::XMVECTOR init_pos) {
	pos = init_pos;
}
void Camera::setUp(DirectX::XMVECTOR init_up) {
	up = init_up;
}
DirectX::XMMATRIX Camera::getViewMatrix() {
	return 		DirectX::XMMatrixLookAtLH(pos, lookAt, up);
}
DirectX::XMMATRIX Camera::getProjMatrix() {
	return 		projMat;
}
DirectX::XMVECTOR Camera::getPos() {
	return pos;
}
DirectX::XMVECTOR Camera::getLookAt() {
	return lookAt;
}
DirectX::XMVECTOR Camera::getUp() {
	return up;
}
void Camera::SetLens(float fovY, float aspect, float zn, float zf)
{
	// cache properties
	mFovY = fovY;
	mAspect = aspect;
	mNearZ = zn;
	mFarZ = zf;

	mNearWindowHeight = 2.0f * mNearZ * tanf(0.5f*mFovY);
	mFarWindowHeight = 2.0f * mFarZ * tanf(0.5f*mFovY);

	DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(mFovY, mAspect, mNearZ, mFarZ);
	XMStoreFloat4x4(&mProj, P);
}

void Camera::UpdateViewMatrix()
{
	DirectX::XMVECTOR R = DirectX::XMLoadFloat3(&mRight);
	DirectX::XMVECTOR U = DirectX::XMLoadFloat3(&mUp);
	DirectX::XMVECTOR L = DirectX::XMLoadFloat3(&mLook);
	DirectX::XMVECTOR P = DirectX::XMLoadFloat3(&mPosition);

	// Keep camera's axes orthogonal to each other and of unit length.
	L = XMVector3Normalize(L);
	U = XMVector3Normalize(XMVector3Cross(L, R));

	// U, L already ortho-normal, so no need to normalize cross product.
	R = XMVector3Cross(U, L);

	// Fill in the view matrix entries.
	float x = -XMVectorGetX(XMVector3Dot(P, R));
	float y = -XMVectorGetX(XMVector3Dot(P, U));
	float z = -XMVectorGetX(XMVector3Dot(P, L));

	XMStoreFloat3(&mRight, R);
	XMStoreFloat3(&mUp, U);
	XMStoreFloat3(&mLook, L);

	mView(0, 0) = mRight.x;
	mView(1, 0) = mRight.y;
	mView(2, 0) = mRight.z;
	mView(3, 0) = x;

	mView(0, 1) = mUp.x;
	mView(1, 1) = mUp.y;
	mView(2, 1) = mUp.z;
	mView(3, 1) = y;

	mView(0, 2) = mLook.x;
	mView(1, 2) = mLook.y;
	mView(2, 2) = mLook.z;
	mView(3, 2) = z;

	mView(0, 3) = 0.0f;
	mView(1, 3) = 0.0f;
	mView(2, 3) = 0.0f;
	mView(3, 3) = 1.0f;
}

//
// Scene.cpp
//

#include <stdafx.h>
#include <string.h>
#include <d3d11shader.h>
#include <d3dcompiler.h>
#include <Scene.h>
#include <DirectXMath.h>
#include <DXSystem.h>
#include <DirectXTK\DDSTextureLoader.h>
#include <DirectXTK\WICTextureLoader.h>
#include <CGDClock.h>
#include <Model.h>
#include <LookAtCamera.h>
#include <FirstPersonCamera.h>
#include <Material.h>
#include <Effect.h>
#include <Texture.h>
#include <VertexStructures.h>

using namespace std;
using namespace DirectX;
using namespace DirectX::PackedVector;

// Load the Compiled Shader Object (CSO) file 'filename' and return the bytecode in the blob object **bytecode.  This is used to create shader interfaces that require class linkage interfaces.
// Taken from DXShaderFactory by Paul Angel. This function has been included here for clarity.
uint32_t DXLoadCSO(const char *filename, char **bytecode)
{

	ifstream	*fp = nullptr;
	//char		*memBlock = nullptr;
	//bytecode = nullptr;
	uint32_t shaderBytes = -1;
	cout << "loading shader" << endl;

	try
	{
		// Validate parameters
		if (!filename )
			throw exception("loadCSO: Invalid parameters");

		// Open file
		fp = new ifstream(filename, ios::in | ios::binary);

		if (!fp->is_open())
			throw exception("loadCSO: Cannot open file");

		// Get file size
		fp->seekg(0, ios::end);
		shaderBytes = (uint32_t)fp->tellg();

		// Create blob object to store bytecode (exceptions propagate up if any occur)
		//memBlock = new DXBlob(size);
		cout << "allocating shader memory bytes = " << shaderBytes << endl;
		*bytecode = (char*)malloc(shaderBytes);
		// Read binary data into blob object
		fp->seekg(0, ios::beg);
		fp->read(*bytecode, shaderBytes);


		// Close file and release local resources
		fp->close();
		delete fp;

		// Return DXBlob - ownership implicity passed to caller
		//*bytecode = memBlock;
		cout << "Done: shader memory bytes = " << shaderBytes << endl;
	}
	catch (exception& e)
	{
		cout << e.what() << endl;

		// Cleanup local resources
		if (fp) {

			if (fp->is_open())
				fp->close();

			delete fp;
		}

		if (bytecode)
			delete bytecode;

		// Re-throw exception
		throw;
	}
	return shaderBytes;
}

//
// Private interface implementation
//

// Private constructor
Scene::Scene(const LONG _width, const LONG _height, const wchar_t* wndClassName, const wchar_t* wndTitle, int nCmdShow, HINSTANCE hInstance, WNDPROC WndProc) {

	try
	{
		// 1. Register window class for main DirectX window
		WNDCLASSEX wcex;

		wcex.cbSize = sizeof(WNDCLASSEX);

		wcex.style = CS_DBLCLKS | CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = hInstance;
		wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wcex.hCursor = LoadCursor(NULL, IDC_CROSS);
		wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = wndClassName;
		wcex.hIconSm = NULL;

		if (!RegisterClassEx(&wcex))
			throw exception("Cannot register window class for Scene HWND");

		
		// 2. Store instance handle in our global variable
		hInst = hInstance;


		// 3. Setup window rect and resize according to set styles
		RECT		windowRect;

		windowRect.left = 0;
		windowRect.right = _width;
		windowRect.top = 0;
		windowRect.bottom = _height;

		DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		DWORD dwStyle = WS_OVERLAPPEDWINDOW;

		AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);

		// 4. Create and validate the main window handle
		wndHandle = CreateWindowEx(dwExStyle, wndClassName, wndTitle, dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 500, 500, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, NULL, NULL, hInst, this);

		if (!wndHandle)
			throw exception("Cannot create main window handle");

		ShowWindow(wndHandle, nCmdShow);
		UpdateWindow(wndHandle);
		SetFocus(wndHandle);


		// 5. Initialise render pipeline model (simply sets up an internal std::vector of pipeline objects)
	

		// 6. Create DirectX host environment (associated with main application wnd)
		dx = DXSystem::CreateDirectXSystem(wndHandle);

		if (!dx)
			throw exception("Cannot create Direct3D device and context model");

		// 7. Setup application-specific objects
		HRESULT hr = initialiseSceneResources();

		if (!SUCCEEDED(hr))
			throw exception("Cannot initalise scene resources");


		// 8. Create main clock / FPS timer (do this last with deferred start of 3 seconds so min FPS / SPF are not skewed by start-up events firing and taking CPU cycles).
		mainClock = CGDClock::CreateClock(string("mainClock"), 3.0f);

		if (!mainClock)
			throw exception("Cannot create main clock / timer");

	}
	catch (exception &e)
	{
		cout << e.what() << endl;

		// Re-throw exception
		throw;
	}
	
}


// Return TRUE if the window is in a minimised state, FALSE otherwise
BOOL Scene::isMinimised() {

	WINDOWPLACEMENT				wp;

	ZeroMemory(&wp, sizeof(WINDOWPLACEMENT));
	wp.length = sizeof(WINDOWPLACEMENT);

	return (GetWindowPlacement(wndHandle, &wp) != 0 && wp.showCmd == SW_SHOWMINIMIZED);
}



//
// Public interface implementation
//

// Factory method to create the main Scene instance (singleton)
Scene* Scene::CreateScene(const LONG _width, const LONG _height, const wchar_t* wndClassName, const wchar_t* wndTitle, int nCmdShow, HINSTANCE hInstance, WNDPROC WndProc) {

	static bool _scene_created = false;

	Scene *dxScene = nullptr;

	if (!_scene_created) {

		dxScene = new Scene(_width, _height, wndClassName, wndTitle, nCmdShow, hInstance, WndProc);

		if (dxScene)
			_scene_created = true;
	}

	return dxScene;
}


// Destructor
Scene::~Scene() {

	
	//free local resources

	if (cBufferExtSrc)
		_aligned_free(cBufferExtSrc);

	if (mainCamera)
		delete(mainCamera);


	if (brickTexture)
		delete(brickTexture);
	
	if (perPixelLightingEffect)
		delete(perPixelLightingEffect);

	//Clean Up- release local interfaces

	if (mainClock)
		mainClock->release();

	if (cBufferBridge)
		cBufferBridge->Release();

	if (bridge)
		bridge->release();

	if (cube)
		cube->release();

	if (dx) {

		dx->release();
		dx = nullptr;
	}

	if (wndHandle)
		DestroyWindow(wndHandle);
}


// Decouple the encapsulated HWND and call DestoryWindow on the HWND
void Scene::destoryWindow() {

	if (wndHandle != NULL) {

		HWND hWnd = wndHandle;

		wndHandle = NULL;
		DestroyWindow(hWnd);
	}
}


// Resize swap chain buffers and update pipeline viewport configurations in response to a window resize event
HRESULT Scene::resizeResources() {

	if (dx && !isMinimised()) {

		// Only process resize if the DXSystem *dx exists (on initial resize window creation this will not be the case so this branch is ignored)
		HRESULT hr = dx->resizeSwapChainBuffers(wndHandle);
		rebuildViewport(mainCamera);
		RECT clientRect;
		GetClientRect(wndHandle, &clientRect);

			renderScene();
	}

	return S_OK;
}


// Helper function to call updateScene followed by renderScene
HRESULT Scene::updateAndRenderScene() {
	ID3D11DeviceContext *context = dx->getDeviceContext();
	HRESULT hr = updateScene(context, mainCamera);

	if (SUCCEEDED(hr))
		hr = renderScene();

	return hr;
}


// Clock handling methods

void Scene::startClock() {

	mainClock->start();
}

void Scene::stopClock() {

	mainClock->stop();
}

void Scene::reportTimingData() {

	cout << "Actual time elapsed = " << mainClock->actualTimeElapsed() << endl;
	cout << "Game time elapsed = " << mainClock->gameTimeElapsed() << endl << endl;
	mainClock->reportTimingData();
}



//
// Event handling methods
//
// Process mouse move with the left button held down
void Scene::handleMouseLDrag(const POINT &disp) {
	//LookAtCamera
	mainCamera->rotateElevation((float)-disp.y * 0.01f);
	mainCamera->rotateOnYAxis((float)-disp.x * 0.01f);

	//FirstPersonCamera
	//mainCamera->elevate((float)-disp.y * 0.01f);
	//mainCamera->turn((float)-disp.x * 0.01f);
}

// Process mouse wheel movement
void Scene::handleMouseWheel(const short zDelta) {

	//LookAtCamera
	if (zDelta<0)
		mainCamera->zoomCamera(1.2f);
	else if (zDelta>0)
		mainCamera->zoomCamera(0.9f);
	//FirstPersonCamera
	//mainCamera->move(zDelta*0.01);
}


// Process key down event.  keyCode indicates the key pressed while extKeyFlags indicates the extended key status at the time of the key down event (see http://msdn.microsoft.com/en-gb/library/windows/desktop/ms646280%28v=vs.85%29.aspx).
void Scene::handleKeyDown(const WPARAM keyCode, const LPARAM extKeyFlags) {

	// Add key down handler here...
}


// Process key up event.  keyCode indicates the key released while extKeyFlags indicates the extended key status at the time of the key up event (see http://msdn.microsoft.com/en-us/library/windows/desktop/ms646281%28v=vs.85%29.aspx).
void Scene::handleKeyUp(const WPARAM keyCode, const LPARAM extKeyFlags) {

	// Add key up handler here...
}



//
// Methods to handle initialisation, update and rendering of the scene
//

HRESULT Scene::rebuildViewport(Camera *camera){
	// Binds the render target view and depth/stencil view to the pipeline.
	// Sets up viewport for the main window (wndHandle) 
	// Called at initialisation or in response to window resize

	ID3D11DeviceContext *context = dx->getDeviceContext();

	if ( !context)
		return E_FAIL;

	// Bind the render target view and depth/stencil view to the pipeline.
	ID3D11RenderTargetView* renderTargetView = dx->getBackBufferRTV();
	context->OMSetRenderTargets(1, &renderTargetView, dx->getDepthStencil());
	// Setup viewport for the main window (wndHandle)
	RECT clientRect;
	GetClientRect(wndHandle, &clientRect);

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<FLOAT>(clientRect.right - clientRect.left);
	viewport.Height = static_cast<FLOAT>(clientRect.bottom - clientRect.top);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	//Set Viewport
	context->RSSetViewports(1, &viewport);
	
	// Compute the projection matrix.
	
	camera->setProjMatrix(XMMatrixPerspectiveFovLH(0.25f*3.14, viewport.Width / viewport.Height, 1.0f, 1000.0f));
	return S_OK;
}





HRESULT Scene::LoadShader(ID3D11Device *device, const char *filename, char **PSBytecode, ID3D11PixelShader **pixelShader){

	char *PSBytecodeLocal;

	//Load the compiled shader byte code.
	uint32_t shaderBytes = DXLoadCSO(filename, &PSBytecodeLocal);

	PSBytecode = &PSBytecodeLocal;
	cout << "Done: PShader memory bytes = " << shaderBytes << endl;
	// Create shader object
	HRESULT hr = device->CreatePixelShader(PSBytecodeLocal, shaderBytes, NULL, pixelShader);
	
	if (!SUCCEEDED(hr))
		throw std::exception("Cannot create PixelShader interface");
	return hr;
}


uint32_t Scene::LoadShader(ID3D11Device *device, const char *filename, char **VSBytecode, ID3D11VertexShader **vertexShader){

	char *VSBytecodeLocal;

	//Load the compiled shader byte code.
	uint32_t shaderBytes = DXLoadCSO(filename, &VSBytecodeLocal);

	cout << "Done: VShader memory bytes = " << shaderBytes << endl;

	*VSBytecode = VSBytecodeLocal;
	cout << "Done: VShader writting = " << shaderBytes << endl;
	HRESULT hr = device->CreateVertexShader(VSBytecodeLocal, shaderBytes, NULL, vertexShader);
	cout << "Done: VShader return = " << hr << endl;
	if (!SUCCEEDED(hr))
		throw std::exception("Cannot create VertexShader interface");
	return shaderBytes;
}


// Main resource setup for the application.  These are setup around a given Direct3D device.
HRESULT Scene::initialiseSceneResources() {
	//ID3D11DeviceContext *context = dx->getDeviceContext();
	ID3D11Device *device = dx->getDevice();
	if (!device)
		return E_FAIL;

	//
	// Setup main pipeline objects
	//

	// Setup objects for fixed function pipeline stages
	// Rasterizer Stage
	// Bind the render target view and depth/stencil view to the pipeline
	// and sets up viewport for the main window (wndHandle) 


	// Create main camera
	//
	mainCamera = new LookAtCamera();
	mainCamera->setPos(XMVectorSet(25, 2, -14.5, 1));

	rebuildViewport(mainCamera);

	ID3D11DeviceContext *context = dx->getDeviceContext();
	if (!context)
		return E_FAIL;

	//Dynamic cube mapping setup
	//Constants
	static const int						 CubeMapSize = 256;

	D3D11_TEXTURE2D_DESC cubeTexDesc;
	cubeTexDesc.Width = CubeMapSize;
	cubeTexDesc.Height = CubeMapSize;
	cubeTexDesc.MipLevels = 0;
	cubeTexDesc.ArraySize = 6;
	cubeTexDesc.SampleDesc.Count = 1;
	cubeTexDesc.SampleDesc.Quality = 0;
	cubeTexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	cubeTexDesc.Usage = D3D11_USAGE_DEFAULT;
	cubeTexDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	cubeTexDesc.CPUAccessFlags = 0;
	cubeTexDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS |
		D3D11_RESOURCE_MISC_TEXTURECUBE;
	ID3D11Texture2D* cubeTex = 0;
	HRESULT hr(device->CreateTexture2D(&cubeTexDesc, 0, &cubeTex));

	//
	// Create a render target view to each cube map face
	// (i.e., each element in the texture array).
	//
	D3D11_RENDER_TARGET_VIEW_DESC cubeRtvDesc;
	cubeRtvDesc.Format = cubeTexDesc.Format;
	cubeRtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
	cubeRtvDesc.Texture2DArray.MipSlice = 0;
	// Only create a view to one array element.
	cubeRtvDesc.Texture2DArray.ArraySize = 1;
	for (int i = 0; i < 6; ++i)
	{
		// Create a render target view to the ith element.
		cubeRtvDesc.Texture2DArray.FirstArraySlice = i;
		hr = (device->CreateRenderTargetView(cubeTex, &cubeRtvDesc, &mDynamicCubeMapRTV[i]));
	}

	//
	// Create a shader resource view to the cube map.
	//
	D3D11_SHADER_RESOURCE_VIEW_DESC cubeSrvDesc;
	cubeSrvDesc.Format = cubeTexDesc.Format;
	cubeSrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	cubeSrvDesc.TextureCube.MostDetailedMip = 0;
	cubeSrvDesc.TextureCube.MipLevels = -1;
	hr = (device->CreateShaderResourceView(
		cubeTex, &cubeSrvDesc, &mDynamicCubeMapSRV));


	D3D11_TEXTURE2D_DESC depthTexDesc;
	depthTexDesc.Width = CubeMapSize;
	depthTexDesc.Height = CubeMapSize;
	depthTexDesc.MipLevels = 1;
	depthTexDesc.ArraySize = 1;
	depthTexDesc.SampleDesc.Count = 1;
	depthTexDesc.SampleDesc.Quality = 0;
	depthTexDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthTexDesc.Usage = D3D11_USAGE_DEFAULT;
	depthTexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthTexDesc.CPUAccessFlags = 0;
	depthTexDesc.MiscFlags = 0;

	ID3D11Texture2D* depthTex = 0;
	hr = (device->CreateTexture2D(&depthTexDesc, 0, &depthTex));
	// Create the depth stencil view for the entire buffer.
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Format = depthTexDesc.Format;
	dsvDesc.Flags = 0;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;
	hr = (device->CreateDepthStencilView(depthTex, &dsvDesc, &mDynamicCubeMapDSV));

	//Setup viewport
	mCubeMapViewport.TopLeftX = 0.0f;
	mCubeMapViewport.TopLeftY = 0.0f;
	mCubeMapViewport.Width = (float)CubeMapSize;
	mCubeMapViewport.Height = (float)CubeMapSize;
	mCubeMapViewport.MinDepth = 0.0f;
	mCubeMapViewport.MaxDepth = 1.0f;
	


	//////////////////////////////////////////////////////// Tutorial Begin
	//Tutorial 04 task 1 create render target texture
	
	D3D11_TEXTURE2D_DESC texDesc;
	// fill out texture descrition
	texDesc.Width = viewport.Width;
	texDesc.Height = viewport.Height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize =1;

	//texDesc.SampleDesc.Count = 8; // Multi-sample properties much match the above DXGI_SWAP_CHAIN_DESC structure

	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;


	//Create Texture
	ID3D11Texture2D *renderTargetTexture = nullptr;
	hr = device->CreateTexture2D(&texDesc, 0, &renderTargetTexture);

	//Create render target view
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
	rtvDesc.Format = texDesc.Format;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
	rtvDesc.Texture2DArray.MipSlice = 0;
	rtvDesc.Texture2DArray.ArraySize = 1;
	rtvDesc.Texture2DArray.FirstArraySlice = 0;

	hr = device->CreateRenderTargetView(renderTargetTexture, &rtvDesc, &renderTargetRTV);


	//Create shader resource view
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	hr = device->CreateShaderResourceView(renderTargetTexture, &srvDesc, &renderTargetSRV);

	// View saves reference.
	renderTargetTexture->Release();


	// Setup objects for the programmable (shader) stages of the pipeline

	perPixelLightingEffect = new Effect(device, "Shaders\\cso\\per_pixel_lighting_vs.cso", "Shaders\\cso\\per_pixel_lighting_ps.cso", extVertexDesc, ARRAYSIZE(extVertexDesc));

	skyBoxEffect = new Effect(device, "Shaders\\cso\\sky_box_vs.cso", "Shaders\\cso\\sky_box_ps.cso", extVertexDesc, ARRAYSIZE(extVertexDesc));
	//basicEffect = new Effect(device, "Shaders\\cso\\basic_colour_vs.cso", "Shaders\\cso\\basic_colour_ps.cso", "Shaders\\cso\\basic_colour_gs.cso", basicVertexDesc, ARRAYSIZE(basicVertexDesc));
	basicEffect = new Effect(device, "Shaders\\cso\\basic_texture_vs.cso", "Shaders\\cso\\basic_texture_ps.cso", basicVertexDesc, ARRAYSIZE(basicVertexDesc));
	refMapEffect = new Effect(device, "Shaders\\cso\\reflection_map_vs.cso", "Shaders\\cso\\reflection_map_ps.cso", extVertexDesc, ARRAYSIZE(extVertexDesc));

	// Setup CBuffer
	cBufferExtSrc = (CBufferExt*)_aligned_malloc(sizeof(CBufferExt), 16);
	// Initialise CBuffer
	cBufferExtSrc->worldMatrix = XMMatrixIdentity();
	cBufferExtSrc->worldITMatrix = XMMatrixIdentity();
	//cBufferExtSrc->WVPMatrix = mainCamera->getViewMatrix()*projMatrix->projMatrix;
	cBufferExtSrc->WVPMatrix = mainCamera->getViewMatrix()*mainCamera->getProjMatrix();
	cBufferExtSrc->lightVec = XMFLOAT4(-250.0, 130.0, 145.0, 1.0); // Positional light
	cBufferExtSrc->lightAmbient = XMFLOAT4(0.3, 0.3, 0.3, 1.0);
	cBufferExtSrc->lightDiffuse = XMFLOAT4(0.8, 0.8, 0.8, 1.0);
	cBufferExtSrc->lightSpecular = XMFLOAT4(1.0, 1.0, 1.0, 1.0);

	XMStoreFloat4(&cBufferExtSrc->eyePos, mainCamera->getPos());// camera->pos;


	D3D11_BUFFER_DESC cbufferDesc;
	D3D11_SUBRESOURCE_DATA cbufferInitData;
	ZeroMemory(&cbufferDesc, sizeof(D3D11_BUFFER_DESC));
	ZeroMemory(&cbufferInitData, sizeof(D3D11_SUBRESOURCE_DATA));
	cbufferDesc.ByteWidth = sizeof(CBufferExt);
	cbufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbufferInitData.pSysMem = cBufferExtSrc;
	

	//Create bridge CBuffer
	hr = device->CreateBuffer(&cbufferDesc, &cbufferInitData, &cBufferBridge);
	hr = device->CreateBuffer(&cbufferDesc, &cbufferInitData, &cBufferSkyBox);
	hr = device->CreateBuffer(&cbufferDesc, &cbufferInitData, &cBufferSphere);


	// Setup example objects
	//


	mattWhite.setSpecular(XMCOLOR(0, 0, 0, 0));
	glossWhite.setSpecular(XMCOLOR(1, 1, 1, 1));


	brickTexture = new Texture(device, L"Resources\\Textures\\brick_DIFFUSE.jpg");
	envMapTexture = new Texture(device, L"Resources\\Textures\\grassenvmap1024.dds");
	rustDiffTexture = new Texture(device, L"Resources\\Textures\\rustDiff.jpg");
	rustSpecTexture = new Texture(device, L"Resources\\Textures\\rustSpec.jpg");

	ID3D11ShaderResourceView *sphereTextureArray[] = { rustDiffTexture->SRV, mDynamicCubeMapSRV, rustSpecTexture->SRV };

	//load bridge
	bridge = new Model(device, perPixelLightingEffect, wstring(L"Resources\\Models\\bridge.3ds"), brickTexture->SRV, &mattWhite);
	sphere = new Model(device, refMapEffect, wstring(L"Resources\\Models\\sphere.3ds"), sphereTextureArray, 3, &glossWhite);

	box = new Box(device, skyBoxEffect, envMapTexture->SRV);
	triangle = new Quad(device, basicEffect->getVSInputLayout());

	BuildCubeFaceCamera(0, 0, 0);
	return S_OK;
}

void Scene::BuildCubeFaceCamera(float x, float y, float z)
{
	// Generate the cube map about the given position.
	XMFLOAT3 center(x, y, z);
	XMFLOAT3 worldUp(0.0f, 1.0f, 0.0f);
	// Look along each coordinate axis.
	XMFLOAT3 targets[6] =
	{
		XMFLOAT3(x + 1.0f, y, z), // +X
		XMFLOAT3(x - 1.0f, y, z), // -X
		XMFLOAT3(x, y + 1.0f, z), // +Y
		XMFLOAT3(x, y - 1.0f, z), // -Y
		XMFLOAT3(x, y, z + 1.0f), // +Z
		XMFLOAT3(x, y, z - 1.0f) // -Z
	};
	// Use world up vector (0,1,0) for all directions except +Y/-Y.
	// In these cases, we are looking down +Y or -Y, so we need a
	// different "up" vector.
	XMFLOAT3 ups[6] =
	{
		XMFLOAT3(0.0f, 1.0f, 0.0f), // +X
		XMFLOAT3(0.0f, 1.0f, 0.0f), // -X
		XMFLOAT3(0.0f, 0.0f, -1.0f), // +Y
		XMFLOAT3(0.0f, 0.0f, +1.0f), // -Y
		XMFLOAT3(0.0f, 1.0f, 0.0f), // +Z
		XMFLOAT3(0.0f, 1.0f, 0.0f) // -Z
	};
	for (int i = 0; i < 6; ++i)
	{
		//Conversions
		const XMFLOAT3 *pSource = nullptr;
		pSource = &center;
		XMVECTOR tempCenter = XMLoadFloat3(pSource);
		pSource = &targets[i];
		XMVECTOR tempTarget = XMLoadFloat3(pSource);
		pSource = &ups[i];
		XMVECTOR tempUps = XMLoadFloat3(pSource);

		mCubeMapCamera[i].LookAt(tempCenter, tempTarget, tempUps);
		mCubeMapCamera[i].SetLens(0.5f*XM_PI, 1.0f, 0.1f, 1000.0f);
		mCubeMapCamera[i].UpdateViewMatrix();
	}
}


// Update scene state (perform animations etc)
HRESULT Scene::updateScene(ID3D11DeviceContext *context, Camera *camera) {


	mainClock->tick();
	gu_seconds tDelta = mainClock->gameTimeElapsed();

	cBufferExtSrc->Timer = (FLOAT)tDelta;
	XMStoreFloat4(&cBufferExtSrc->eyePos, camera->getPos());


	
	// Update bridge cBuffer
	// Scale and translate bridge world matrix

	cBufferExtSrc->worldMatrix = XMMatrixScaling(0.05, 0.05, 0.05)*XMMatrixTranslation(4.5, -1.2, 4);
	cBufferExtSrc->worldITMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, cBufferExtSrc->worldMatrix));	
	cBufferExtSrc->WVPMatrix = cBufferExtSrc->worldMatrix*camera->getViewMatrix()*camera->getProjMatrix();
	mapCbuffer(cBufferExtSrc, cBufferBridge);

	cBufferExtSrc->worldMatrix = XMMatrixScaling(100.0,100, 100)*XMMatrixTranslation(0, 0, 0);
	cBufferExtSrc->worldITMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, cBufferExtSrc->worldMatrix));
	cBufferExtSrc->WVPMatrix = cBufferExtSrc->worldMatrix*camera->getViewMatrix()*camera->getProjMatrix();
	mapCbuffer(cBufferExtSrc, cBufferSkyBox);

	cBufferExtSrc->worldMatrix = XMMatrixScaling(1.0, 1, 1)*XMMatrixTranslation(0, 0, 0)*XMMatrixRotationX(tDelta);
	cBufferExtSrc->worldITMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, cBufferExtSrc->worldMatrix));
	cBufferExtSrc->WVPMatrix = cBufferExtSrc->worldMatrix*camera->getViewMatrix()*camera->getProjMatrix();
	mapCbuffer(cBufferExtSrc, cBufferSphere);



	return S_OK;
}

// Helper function to copy cbuffer data from cpu to gpu
HRESULT Scene::mapCbuffer(void *cBufferExtSrcL, ID3D11Buffer *cBufferExtL)
{
	ID3D11DeviceContext *context = dx->getDeviceContext();
	// Map cBuffer
	D3D11_MAPPED_SUBRESOURCE res;
	HRESULT hr = context->Map(cBufferExtL, 0, D3D11_MAP_WRITE_DISCARD, 0, &res);

	if (SUCCEEDED(hr)) {
		memcpy(res.pData, cBufferExtSrcL, sizeof(CBufferExt));
		context->Unmap(cBufferExtL, 0);
	}
	return hr;
}


// Render scene
HRESULT Scene::renderScene()
{

	ID3D11DeviceContext *context = dx->getDeviceContext();
	// Validate window and D3D context
	if (isMinimised() || !context)
		return E_FAIL;

	// Clear the screen
	static const FLOAT clearColor[4] = {1.0f, 0.0f, 0.0f, 1.0f };

	// Save current render targets
	ID3D11RenderTargetView* defaultRenderTargetView;
	ID3D11DepthStencilView* defaultDepthStencilView;



	context->OMGetRenderTargets(1, &defaultRenderTargetView, &defaultDepthStencilView);

	ID3D11RenderTargetView* renderTargets[1];
	// Generate the cube map by rendering to each cube map face.
	context->RSSetViewports(1, &mCubeMapViewport);
	for (int i = 0; i < 6; ++i)
	{
		updateScene(context, &mCubeMapCamera[i]);
		rebuildViewport(&mCubeMapCamera[i]);
		// Clear cube map face and depth buffer.
		context->ClearRenderTargetView(mDynamicCubeMapRTV[i], clearColor);
		context->ClearDepthStencilView(mDynamicCubeMapDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		//mDynamicCubeMapDSV = dx->getDepthStencil();
		// Bind cube map face as render target.
		renderTargets[0] = mDynamicCubeMapRTV[i];
		context->OMSetRenderTargets(1, renderTargets, mDynamicCubeMapDSV);
		// Draw the scene with the exception of the
		// center sphere, to this cube map face.
		if (bridge) {

			// Setup pipeline for effect
			// Apply the bridge cBuffer.
			context->VSSetConstantBuffers(0, 1, &cBufferBridge);
			context->PSSetConstantBuffers(0, 1, &cBufferBridge);
			// Render
			bridge->render(context);
		}

		if (box) {
			// Apply the box cBuffer.
			context->VSSetConstantBuffers(0, 1, &cBufferSkyBox);
			context->PSSetConstantBuffers(0, 1, &cBufferSkyBox);
			// Render
			box->render(context);
		}
	}
	// Restore old viewport and render targets.
	context->RSSetViewports(1, &viewport);
	renderTargets[0] = renderTargetRTV;
	context->OMSetRenderTargets(1, &defaultRenderTargetView, defaultDepthStencilView);
	// Have hardware generate lower mipmap levels of cube map.
	context->GenerateMips(mDynamicCubeMapSRV);

	//update scene for main camera
	rebuildViewport(mainCamera);
	updateScene(context, mainCamera);

	//testing
	//rebuildViewport(&mCubeMapCamera[2]);
	//updateScene(context, &mCubeMapCamera[2]);

	// Now draw the scene as normal, but with the center sphere.
	context->ClearRenderTargetView(defaultRenderTargetView, clearColor);
	context->ClearDepthStencilView(defaultDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);



	// Tutorial 04

	if (bridge) {

		// Setup pipeline for effect
		// Apply the bridge cBuffer.
		context->VSSetConstantBuffers(0, 1, &cBufferBridge);
		context->PSSetConstantBuffers(0, 1, &cBufferBridge);
		// Render
		bridge->render(context);
	}

	if (box) {
		// Apply the box cBuffer.
		context->VSSetConstantBuffers(0, 1, &cBufferSkyBox);
		context->PSSetConstantBuffers(0, 1, &cBufferSkyBox);
		// Render
		box->render(context);
	}

	if (sphere) {
		
		
		// Apply the sphere cBuffer.
		context->VSSetConstantBuffers(0, 1, &cBufferSphere);
		context->PSSetConstantBuffers(0, 1, &cBufferSphere);
		// Render
		sphere->render(context);
	}

	// Present current frame to the screen
	HRESULT hr = dx->presentBackBuffer();

	return S_OK;
}
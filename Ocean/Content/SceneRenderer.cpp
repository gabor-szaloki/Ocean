﻿#include "pch.h"
#include "Windows.h"
#include "SceneRenderer.h"

#include "..\Common\DirectXHelper.h"

using namespace Ocean;

using namespace DirectX;
using namespace Windows::Foundation;

// Loads vertex and pixel shaders from files and instantiates the cube geometry.
SceneRenderer::SceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	loadingComplete(false),
	deviceResources(deviceResources)
{
	InitializeScene();
	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}

// Initialize scene objects
void SceneRenderer::InitializeScene()
{
	water = std::shared_ptr<Water>(new Water());
	XMStoreFloat4x4(&water->vsConstantBufferData.model, XMMatrixIdentity());
	water->vsConstantBufferData.uvWaveSpeed = XMFLOAT4(.4f, -.5f, -.7f, .3f);
	water->psConstantBufferData.lightDir = XMFLOAT4(-.9f, -.34f, -.25f, 1.f);
	water->psConstantBufferData.lightColor = XMFLOAT4(1.f, 1.f, 1.f, 1.f);
	
	skybox = std::shared_ptr<SkyBox>(new SkyBox());

	camera = std::shared_ptr<Camera>(new Camera(
		XMFLOAT4(-10.0f, 7.f, 5.f, 0.0f),
		XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f),
		XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f),
		deviceResources));

	//water->GeneratePolarGridMesh(deviceResources, 1000, 100, 500);
	water->LoadMeshes(deviceResources, camera);
}

// Initializes view parameters when the window size changes.
void SceneRenderer::CreateWindowSizeDependentResources()
{
	auto outputSize = deviceResources->GetOutputSize();
	camera->aspectRatio = outputSize.Width / outputSize.Height;

	XMStoreFloat4x4(&water->vsConstantBufferData.projection, camera->getProjection());
	XMStoreFloat4x4(&skybox->vsConstantBufferData.projection, camera->getProjection());

	water->UpdateMeshes(deviceResources, camera);
}

// Called once per frame, rotates the cube and calculates the model and view matrices.
void SceneRenderer::Update(DX::StepTimer const& timer)
{
	ProcessInput();

	XMVECTOR oldEye = camera->getEye();
	camera->Update(timer, deviceResources);

	water->UpdateMeshes(deviceResources, camera);

	//XMStoreFloat4x4(&water->vsConstantBufferData.model, XMMatrixTranspose(XMMatrixTranslation(XMVectorGetX(camera->getEye()), 0, XMVectorGetZ(camera->getEye()))));
	XMStoreFloat4x4(&water->vsConstantBufferData.view, camera->getView());
	XMStoreFloat4(&water->vsConstantBufferData.cameraPos, camera->getEye());
	float totalTime = (float)timer.GetTotalSeconds();
	water->vsConstantBufferData.totalTime = XMFLOAT4(totalTime, totalTime, totalTime, totalTime);
	
	XMStoreFloat4x4(&skybox->vsConstantBufferData.model, XMMatrixTranspose(XMMatrixScaling(500.f, 500.f, 500.f) * XMMatrixTranslationFromVector(camera->getEye())));
	XMStoreFloat4x4(&skybox->vsConstantBufferData.view, camera->getView());
}

// Processes user input
void SceneRenderer::ProcessInput()
{
	using namespace Windows::UI::Core;
	using namespace Windows::System;

	auto window = deviceResources->GetWindow();

	// Keyboard handling
	if (window->GetAsyncKeyState(VirtualKey::F) == CoreVirtualKeyStates::Down)
	{
		wireframe = !wireframe;
	}
}

// Renders one frame using the vertex and pixel shaders.
void SceneRenderer::Render()
{
	// Loading is asynchronous. Only draw geometry after it's loaded.
	if (!loadingComplete)
	{
		return;
	}
	
	auto context = deviceResources->GetD3DDeviceContext();

	// Set render targets to the screen.
	ID3D11RenderTargetView *const targets[1] = { deviceResources->GetBackBufferRenderTargetView() };
	context->OMSetRenderTargets(1, targets, deviceResources->GetDepthStencilView());

	context->RSSetState(states->CullClockwise());
	skybox->Draw(deviceResources);
	
	if (wireframe)
		context->RSSetState(states->Wireframe());
	else
		context->RSSetState(states->CullCounterClockwise());

	context->OMSetBlendState(states->AlphaBlend(), nullptr, 0xFFFFFFFF);
	context->OMSetDepthStencilState(states->DepthRead(), 0);
	water->Draw(deviceResources);
}

void SceneRenderer::CreateDeviceDependentResources()
{
	states = std::shared_ptr<CommonStates>(new CommonStates(deviceResources->GetD3DDevice()));

	auto loadWaterVSTask = DX::ReadDataAsync(L"WaterVertexShader.cso");
	auto loadWaterPSTask = DX::ReadDataAsync(L"WaterPixelShader.cso");
	auto loadSkyboxVSTask = DX::ReadDataAsync(L"SkyboxVertexShader.cso");
	auto loadSkyboxPSTask = DX::ReadDataAsync(L"SkyboxPixelShader.cso");

	auto createWaterVSTask = loadWaterVSTask.then([this](const std::vector<byte>& fileData) {
		water->LoadVertexShader(deviceResources, fileData);
	});

	auto createWaterPSTask = loadWaterPSTask.then([this](const std::vector<byte>& fileData) {
		water->LoadPixelShader(deviceResources, fileData);
		water->CreateConstantBuffers(deviceResources);
	});

	auto loadWaterTexturesTask = (createWaterVSTask && createWaterPSTask).then([this] () {
		water->LoadTextures(deviceResources, L"assets/textures/water_normal.dds", L"assets/textures/skybox.dds");
	});


	auto createSkyboxVSTask = loadSkyboxVSTask.then([this](const std::vector<byte>& fileData) {
		skybox->LoadVertexShader(deviceResources, fileData);
		skybox->CreateConstantBuffers(deviceResources);
	});

	auto createSkyboxPSTask = loadSkyboxPSTask.then([this](const std::vector<byte>& fileData) {
		skybox->LoadPixelShader(deviceResources, fileData);
	});

	auto createSkyboxMeshTask = (createSkyboxVSTask && createSkyboxPSTask).then([this]() {
		skybox->LoadMesh(deviceResources);
		skybox->LoadTextures(deviceResources, L"assets/textures/skybox.dds");
	});

	// Once the everything is loaded, the scene is ready to be rendered.
	(loadWaterTexturesTask && createSkyboxMeshTask).then([this] () {
		loadingComplete = true;
	});
}

void SceneRenderer::ReleaseDeviceDependentResources()
{
	loadingComplete = false;
	states.reset();
	camera.reset();
	water.reset();
	skybox.reset();
}
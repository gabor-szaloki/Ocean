﻿#include "pch.h"
#include "SceneRenderer.h"

#include "..\Common\DirectXHelper.h"

using namespace Ocean;

using namespace DirectX;
using namespace Windows::Foundation;

// Loads vertex and pixel shaders from files and instantiates the cube geometry.
SceneRenderer::SceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_loadingComplete(false),
	m_deviceResources(deviceResources)
{
	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}

// Initializes view parameters when the window size changes.
void SceneRenderer::CreateWindowSizeDependentResources()
{
	m_water = std::shared_ptr<Water>(new Water());
	m_skybox = std::shared_ptr<SkyBox>(new SkyBox());

	m_camera = std::shared_ptr<Camera>(new Camera(
		XMFLOAT4(1.0f, 5.f, 5.f, 0.0f),
		XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f),
		XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f),
		m_deviceResources));

	XMStoreFloat4x4(&m_water->vsConstantBufferData.model, XMMatrixIdentity());
	XMStoreFloat4x4(&m_water->vsConstantBufferData.projection, m_camera->getProjection());
	m_water->vsConstantBufferData.lightPos = XMFLOAT4(1000.f, 500.f, 0.f, 1.f);
	m_water->vsConstantBufferData.lightColor = XMFLOAT4(1.f, 1.f, 1.f, 1.f);
	
	XMStoreFloat4x4(&m_skybox->vsConstantBufferData.projection, m_camera->getProjection());
}

// Called once per frame, rotates the cube and calculates the model and view matrices.
void SceneRenderer::Update(DX::StepTimer const& timer)
{
	m_camera->Update(timer, m_deviceResources);

	XMStoreFloat4x4(&m_water->vsConstantBufferData.view, m_camera->getView());
	XMStoreFloat4(&m_water->vsConstantBufferData.cameraPos, m_camera->getEye());
	
	XMStoreFloat4x4(&m_skybox->vsConstantBufferData.model, XMMatrixTranspose(XMMatrixScaling(500.f, 500.f, 500.f) * XMMatrixTranslationFromVector(m_camera->getEye())));
	XMStoreFloat4x4(&m_skybox->vsConstantBufferData.view, m_camera->getView());
}

// Renders one frame using the vertex and pixel shaders.
void SceneRenderer::Render()
{
	// Loading is asynchronous. Only draw geometry after it's loaded.
	if (!m_loadingComplete)
	{
		return;
	}
	
	auto context = m_deviceResources->GetD3DDeviceContext();

	// Set render targets to the screen.
	ID3D11RenderTargetView *const targets[1] = { m_deviceResources->GetBackBufferRenderTargetView() };
	context->OMSetRenderTargets(1, targets, m_deviceResources->GetDepthStencilView());

	context->RSSetState(m_states->CullClockwise());
	m_skybox->Draw(m_deviceResources);
	
	context->RSSetState(m_states->Wireframe());
	//context->RSSetState(m_states->CullCounterClockwise());
	context->OMSetBlendState(m_states->AlphaBlend(), nullptr, 0xFFFFFFFF);
	context->OMSetDepthStencilState(m_states->DepthRead(), 0);
	m_water->Draw(m_deviceResources);
}

void SceneRenderer::CreateDeviceDependentResources()
{
	m_states = std::shared_ptr<CommonStates>(new CommonStates(m_deviceResources->GetD3DDevice()));

	auto loadWaterVSTask = DX::ReadDataAsync(L"WaterVertexShader.cso");
	auto loadWaterPSTask = DX::ReadDataAsync(L"WaterPixelShader.cso");
	auto loadSkyboxVSTask = DX::ReadDataAsync(L"SkyboxVertexShader.cso");
	auto loadSkyboxPSTask = DX::ReadDataAsync(L"SkyboxPixelShader.cso");

	auto createWaterVSTask = loadWaterVSTask.then([this](const std::vector<byte>& fileData) {
		m_water->LoadVertexShader(m_deviceResources, fileData);
	});

	auto createWaterPSTask = loadWaterPSTask.then([this](const std::vector<byte>& fileData) {
		m_water->LoadPixelShader(m_deviceResources, fileData);
		m_water->CreateConstantBuffers(m_deviceResources);
	});

	auto createWaterMeshTask = (createWaterVSTask && createWaterPSTask).then([this] () {
		//m_water->GenerateSimpleGridMesh(m_deviceResources, 100, 100, .2f);
		m_water->GenerateProjectedGridMesh(m_deviceResources, 10, 10, m_camera);
		m_water->LoadTextures(m_deviceResources, L"assets/textures/water_normal.dds", L"assets/textures/skybox.dds");
	});


	auto createSkyboxVSTask = loadSkyboxVSTask.then([this](const std::vector<byte>& fileData) {
		m_skybox->LoadVertexShader(m_deviceResources, fileData);
		m_skybox->CreateConstantBuffers(m_deviceResources);
	});

	auto createSkyboxPSTask = loadSkyboxPSTask.then([this](const std::vector<byte>& fileData) {
		m_skybox->LoadPixelShader(m_deviceResources, fileData);
	});

	auto createSkyboxMeshTask = (createSkyboxVSTask && createSkyboxPSTask).then([this]() {
		m_skybox->GenerateSphereMesh(m_deviceResources, 20, 20, .5f);
		m_skybox->LoadTextures(m_deviceResources, L"assets/textures/skybox.dds");
	});

	// Once the everything is loaded, the scene is ready to be rendered.
	(createWaterMeshTask && createSkyboxMeshTask).then([this] () {
		m_loadingComplete = true;
	});
}

void SceneRenderer::ReleaseDeviceDependentResources()
{
	m_loadingComplete = false;
	m_states.reset();
	m_camera.reset();
	m_water.reset();
	m_skybox.reset();
}
#include "pch.h"
#include "Water.h"
#include "Camera.h"
#include "DDSTextureLoader.h"

using namespace Windows::Foundation;
using namespace Ocean;

Water::Water() 
{
	polarMesh = std::shared_ptr<GeneratedMesh>(new GeneratedMesh());
	projectedMesh = std::shared_ptr<GeneratedMesh>(new GeneratedMesh());

	currentMesh = polarMesh;
}

void Water::LoadTextures(
	std::shared_ptr<DX::DeviceResources> deviceResources,
	const wchar_t* normalTextureFile1,
	const wchar_t* normalTextureFile2,
	const wchar_t* environmentTextureFile,
	const wchar_t* foamTextureFile)
{
	auto device = deviceResources->GetD3DDevice();

	// Load textures
	DX::ThrowIfFailed(DirectX::CreateDDSTextureFromFile(device, environmentTextureFile, nullptr, environmentTexture.ReleaseAndGetAddressOf()));
	DX::ThrowIfFailed(DirectX::CreateDDSTextureFromFile(device, normalTextureFile1, nullptr, normalTexture1.ReleaseAndGetAddressOf()));
	DX::ThrowIfFailed(DirectX::CreateDDSTextureFromFile(device, normalTextureFile2, nullptr, normalTexture2.ReleaseAndGetAddressOf()));
	DX::ThrowIfFailed(DirectX::CreateDDSTextureFromFile(device, foamTextureFile, nullptr, foamTexture.ReleaseAndGetAddressOf()));
	
	// Create samplers
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	device->CreateSamplerState(&sampDesc, &linearSampler);
}

void Water::LoadVertexShader(
	std::shared_ptr<DX::DeviceResources> deviceResources,
	const std::vector<byte>& vsFileData)
{
	// Vertex Shader
	DX::ThrowIfFailed(
		deviceResources->GetD3DDevice()->CreateVertexShader(
			&vsFileData[0],
			vsFileData.size(),
			nullptr,
			&vertexShader
			)
		);

	// Input Layout
	static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{ "SV_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	DX::ThrowIfFailed(
		deviceResources->GetD3DDevice()->CreateInputLayout(
			vertexDesc,
			ARRAYSIZE(vertexDesc),
			&vsFileData[0],
			vsFileData.size(),
			&inputLayout
			)
		);
}

void Water::LoadPixelShader(
	std::shared_ptr<DX::DeviceResources> deviceResources,
	const std::vector<byte>& psFileData)
{
	DX::ThrowIfFailed(
		deviceResources->GetD3DDevice()->CreatePixelShader(
			&psFileData[0],
			psFileData.size(),
			nullptr,
			&pixelShader
			)
		);
}

void Water::LoadWireFramePixelShader(
	std::shared_ptr<DX::DeviceResources> deviceResources,
	const std::vector<byte>& wfpsFileData)
{
	DX::ThrowIfFailed(
		deviceResources->GetD3DDevice()->CreatePixelShader(
			&wfpsFileData[0],
			wfpsFileData.size(),
			nullptr,
			&wireFramePixelShader
			)
		);
}

void Water::CreateConstantBuffers(
	std::shared_ptr<DX::DeviceResources> deviceResources)
{
	CD3D11_BUFFER_DESC vsConstantBufferDesc(sizeof(WaterVSConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
	DX::ThrowIfFailed(
		deviceResources->GetD3DDevice()->CreateBuffer(
			&vsConstantBufferDesc,
			nullptr,
			&vsConstantBuffer
			)
		);

	CD3D11_BUFFER_DESC psConstantBufferDesc(sizeof(WaterPSConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
	DX::ThrowIfFailed(
		deviceResources->GetD3DDevice()->CreateBuffer(
			&psConstantBufferDesc,
			nullptr,
			&psConstantBuffer
			)
		);
}

void Water::LoadMeshes(
	std::shared_ptr<DX::DeviceResources> deviceResources,
	std::shared_ptr<Camera> camera)
{
	polarMesh->GeneratePolarGridMesh(deviceResources, 500, 100, 500);
	projectedMesh->GenerateProjectedGridMesh(deviceResources, (int)((float)projectedGridHeight * camera->aspectRatio), projectedGridHeight, 7.0f, camera);
}

void Water::UpdateMeshes(
	std::shared_ptr<DX::DeviceResources> deviceResources,
	std::shared_ptr<Camera> camera)
{
	if (camera->getPitch() < -XM_PIDIV4)
	{
		currentMesh = projectedMesh;
	}
	else
	{
		currentMesh = polarMesh;
	}

	if (currentMesh == polarMesh)
	{
		XMStoreFloat4x4(&vsConstantBufferData.model, XMMatrixTranspose(XMMatrixTranslation(XMVectorGetX(camera->getEye()), 0, XMVectorGetZ(camera->getEye()))));
	}
	else if (currentMesh == projectedMesh)
	{
		XMStoreFloat4x4(&vsConstantBufferData.model, XMMatrixTranspose(XMMatrixIdentity()));
		projectedMesh->GenerateProjectedGridMesh(deviceResources, (int)((float)projectedGridHeight * camera->aspectRatio), projectedGridHeight, 7.0f, camera);
	}
}

void Water::Draw(std::shared_ptr<DX::DeviceResources> deviceResources)
{
	auto device = deviceResources->GetD3DDevice();
	auto context = deviceResources->GetD3DDeviceContext();

	context->UpdateSubresource(
		vsConstantBuffer.Get(),
		0,
		NULL,
		&vsConstantBufferData,
		0,
		0);

	context->UpdateSubresource(
		psConstantBuffer.Get(),
		0,
		NULL,
		&psConstantBufferData,
		0,
		0);

	UINT stride = sizeof(VertexPositionNormal);
	UINT offset = 0;
	context->IASetVertexBuffers(
		0,
		1,
		currentMesh->vertexBuffer.GetAddressOf(),
		&stride,
		&offset
		);

	context->IASetIndexBuffer(
		currentMesh->indexBuffer.Get(),
		DXGI_FORMAT_R32_UINT,
		0
		);

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->IASetInputLayout(inputLayout.Get());

	// Attach our vertex shader.
	context->VSSetShader(
		vertexShader.Get(),
		nullptr,
		0
		);

	// Send the constant buffer to the graphics device.
	context->VSSetConstantBuffers(
		0,
		1,
		vsConstantBuffer.GetAddressOf()
		);

	if (wireframe)
	{
		// Attach our pixel shader.
		context->PSSetShader(
			wireFramePixelShader.Get(),
			nullptr,
			0
			);
	}
	else
	{
		// Attach our pixel shader.
		context->PSSetShader(
			pixelShader.Get(),
			nullptr,
			0
			);

		// Send the constant buffer to the graphics device.
		context->PSSetConstantBuffers(
			0,
			1,
			psConstantBuffer.GetAddressOf()
			);

		context->PSSetShaderResources(0, 1, normalTexture1.GetAddressOf());
		context->PSSetShaderResources(1, 1, normalTexture1.GetAddressOf());
		context->PSSetShaderResources(2, 1, environmentTexture.GetAddressOf());
		context->PSSetShaderResources(3, 1, foamTexture.GetAddressOf());
		context->PSSetSamplers(0, 1, linearSampler.GetAddressOf());
	}

	// Draw the objects.
	context->DrawIndexed(
		currentMesh->indexCount,
		0,
		0
		);
}

Water::~Water()
{
	vertexShader.Reset();
	pixelShader.Reset();
	vsConstantBuffer.Reset();
	psConstantBuffer.Reset();
	inputLayout.Reset();
	environmentTexture.Reset();
	normalTexture1.Reset();
	normalTexture2.Reset();
	foamTexture.Reset();
	linearSampler.Reset();
}
#pragma once
#include "Camera.h"
#include "Content\ShaderStructures.h"

using namespace Ocean;

class GeneratedMesh
{
public:
	GeneratedMesh();
	void GenerateSphereMesh(std::shared_ptr<DX::DeviceResources> deviceResources, int latitudeBands, int longitudeBands, float radius);
	void GenerateSimpleGridMesh(std::shared_ptr<DX::DeviceResources> deviceResources, int width, int height, float stride);
	void GenerateProjectedGridMesh(std::shared_ptr<DX::DeviceResources> deviceResources, int width, int height, std::shared_ptr<Camera> camera);
	~GeneratedMesh();

protected:
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	int indexCount;
};

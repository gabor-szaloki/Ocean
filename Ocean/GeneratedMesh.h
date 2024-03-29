#pragma once
#include "Camera.h"
#include "Content\ShaderStructures.h"

namespace Ocean
{
	class GeneratedMesh
	{
	public:
		GeneratedMesh();
		void GenerateSphereMesh(std::shared_ptr<DX::DeviceResources> deviceResources, int latitudeBands, int longitudeBands, float radius);
		void GenerateSimpleGridMesh(std::shared_ptr<DX::DeviceResources> deviceResources, int width, int height, float stride);
		void GeneratePolarGridMesh(std::shared_ptr<DX::DeviceResources> deviceResources, int rads, int angs, float radius);
		void GenerateProjectedGridMesh(std::shared_ptr<DX::DeviceResources> deviceResources, int width, int height, float bias, std::shared_ptr<Camera> camera);
		~GeneratedMesh();

		Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
		int indexCount;
	};
}
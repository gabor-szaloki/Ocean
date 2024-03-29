#pragma once
#include "GeneratedMesh.h"

namespace Ocean
{
	class Skybox
	{
	public:
		Skybox();

		void LoadTextures(
			std::shared_ptr<DX::DeviceResources> deviceResources,
			const wchar_t* diffuseTextureFile);
		void LoadVertexShader(
			std::shared_ptr<DX::DeviceResources> deviceResources,
			const std::vector<byte>& vsFileData);
		void LoadPixelShader(
			std::shared_ptr<DX::DeviceResources> deviceResources,
			const std::vector<byte>& psFileData);
		void CreateConstantBuffers(
			std::shared_ptr<DX::DeviceResources> deviceResources);
		void LoadMesh(
			std::shared_ptr<DX::DeviceResources> deviceResources);
		void Draw(std::shared_ptr<DX::DeviceResources> deviceResources);

		~Skybox();

		ModelViewProjectionConstantBuffer vsConstantBufferData;

	protected:

		std::shared_ptr<GeneratedMesh> mesh;

		Microsoft::WRL::ComPtr<ID3D11VertexShader>         vertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>          pixelShader;
		Microsoft::WRL::ComPtr<ID3D11Buffer>               vsConstantBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>               psConstantBuffer;
		Microsoft::WRL::ComPtr<ID3D11InputLayout>          inputLayout;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>   diffuseTexture;
		Microsoft::WRL::ComPtr<ID3D11SamplerState>         linearSampler;
	};
}
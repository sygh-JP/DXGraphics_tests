#pragma once


class MyComException final
{
public:
	HRESULT Code;
	CString Message;
public:
	MyComException(HRESULT hr, LPCTSTR message)
		: Code(hr)
		, Message(message)
	{}
};

inline void ThrowIfFailed(HRESULT hr, LPCTSTR message)
{
	if (FAILED(hr))
	{
		throw MyComException(hr, message);
	}
}


using Microsoft::WRL::ComPtr;

using MyVector3F = DirectX::XMFLOAT3;
using MyVector4F = DirectX::XMFLOAT4;
using MyMatrix4x4F = DirectX::XMFLOAT4X4;

inline const MyMatrix4x4F& GetIdentityMatrix4x4F()
{
	static const MyMatrix4x4F identity(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
	return identity;
}

class MyVertex final
{
public:
	MyVector3F Position;
	MyVector4F Color;
public:
	MyVertex()
		: Position(DirectX::g_XMZero)
		, Color(DirectX::g_XMZero)
	{}
	MyVertex(const MyVector3F& pos, const MyVector4F& color)
		: Position(pos)
		, Color(color)
	{}
};

const D3D12_INPUT_ELEMENT_DESC InputLayoutDescMyVertex[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

struct MyConstBuffer
{
	MyMatrix4x4F UniWorld[2];
	MyMatrix4x4F UniProjection;
};

// いずれストア アプリ（Windows アプリ）への Direct3D 12 移植・相互運用も試したいので、今回は MFC の RAII クラス（CEvent など）は使わない。

class MyD3DManager
{
private:
	ComPtr<ID3D12Device> m_d3dDevice;
	//ComPtr<IDXGISwapChain> m_dxgiSwapChain;
	ComPtr<IDXGISwapChain1> m_dxgiSwapChain;
	ComPtr<ID3D12PipelineState> m_simpleGraphicsPSO1;
	ComPtr<ID3D12PipelineState> m_simpleGraphicsPSO2;
	ComPtr<ID3D12DescriptorHeap> m_descHeapRtv;
	ComPtr<ID3D12DescriptorHeap> m_descHeapCbvSrvUav;
	ComPtr<ID3D12CommandAllocator> m_cmdAllocator;
	ComPtr<ID3D12CommandQueue> m_cmdQueue;
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12GraphicsCommandList> m_graphicsCmdList;
	ComPtr<ID3D12Resource> m_resRenderTarget;
	ComPtr<ID3D12Resource> m_resVertexBuffer;
	ComPtr<ID3D12Resource> m_resConstBuffer;
	ComPtr<ID3D12Fence> m_d3dFence;
	HANDLE m_handleEvent = nullptr;
	UINT64 m_currentFence = 0;
	D3D12_VIEWPORT m_viewport = D3D12_VIEWPORT();
	D3D12_RECT m_rectScissor = D3D12_RECT();
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView = D3D12_VERTEX_BUFFER_VIEW();
	void* m_constBufferHostPtr = nullptr;
private:
	int m_indexLastSwapBuf = 0;
private:
	bool m_isInitialized = false;
	MyVector3F m_vRotAngle = MyVector3F(DirectX::g_XMZero);
	bool m_drawsBlendingObjects = true;
private:
	static const int NumSwapBufs = 2;
	static const UINT MyConstBufferSizeInBytes = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT * 1;
public:
	MyD3DManager()
	{}
	virtual ~MyD3DManager()
	{
		this->OnDestroy();
	}
public:
	void Init(HWND hWnd, UINT width, UINT height)
	{
		this->LoadPipeline(hWnd, width, height);
		this->LoadAssets(width, height);
	}
	void Render();
public:
	MyVector3F GetRotAngle() const { return m_vRotAngle; }
	void SetRotAngle(const MyVector3F& inVal) { m_vRotAngle = inVal; }
	bool GetDrawsBlendingObjects() const { return m_drawsBlendingObjects; }
	void SetDrawsBlendingObjects(bool inVal) { m_drawsBlendingObjects = inVal; }
private:
	void LoadPipeline(HWND hWnd, UINT width, UINT height);
	void LoadAssets(UINT width, UINT height);
	void WaitForGPU();
	void PopulateCommandLists();
	static void SetResourceBarrier(ID3D12GraphicsCommandList* commandList, ID3D12Resource* resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter);
	void OnDestroy();
};

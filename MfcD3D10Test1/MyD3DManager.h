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

class MyD3DManager
{
private:
	ComPtr<ID3D10Device1> m_pD3DDevice;
	ComPtr<IDXGISwapChain> m_pSwapChain;
	ComPtr<ID3D10Texture2D> m_pBackBufferTex;
	ComPtr<ID3D10RenderTargetView> m_pBackBufferRTV;
private:
	bool m_isInitialized = false;

public:
	void Init(HWND hWnd, UINT width, UINT height)
	{
		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		swapChainDesc.BufferCount = 1;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		swapChainDesc.BufferDesc.Width = width;
		swapChainDesc.BufferDesc.Height = height;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapChainDesc.Windowed = true;
		swapChainDesc.OutputWindow = hWnd;

		const UINT deviceCreationFlags =
#if defined(DEBUG) || defined(_DEBUG)
			D3D10_CREATE_DEVICE_BGRA_SUPPORT | D3D10_CREATE_DEVICE_DEBUG;
#else
			D3D10_CREATE_DEVICE_BGRA_SUPPORT;
#endif

		// Direct3D 10.1 自体は Vista SP1 で実装され、
		// 初期化時に D3D10_FEATURE_LEVEL_10_0 と D3D10_FEATURE_LEVEL_10_1 が選択できるようになったが、
		// ダウンレベルである D3D10_FEATURE_LEVEL_9_1, D3D10_FEATURE_LEVEL_9_2, D3D10_FEATURE_LEVEL_9_3 が使えるのは
		// Direct3D 11 ランタイムがインストールされている環境であることに注意。
		// Windows 7 だと無印で利用可能だが、Vista の場合は SP2 + Platform Update が必要。
		// https://msdn.microsoft.com/en-us/library/windows/desktop/bb694529.aspx
		// https://msdn.microsoft.com/ja-jp/library/ee415631.aspx
		ThrowIfFailed(D3D10CreateDeviceAndSwapChain1(
			nullptr,
			D3D10_DRIVER_TYPE_HARDWARE,
			nullptr,
			deviceCreationFlags,
			D3D10_FEATURE_LEVEL_10_1,
			D3D10_1_SDK_VERSION,
			&swapChainDesc,
			&m_pSwapChain,
			&m_pD3DDevice), _T("D3D10CreateDeviceAndSwapChain1"));

		ThrowIfFailed(m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&m_pBackBufferTex)), _T("IDXGISwapChain::GetBuffer"));

		ThrowIfFailed(m_pD3DDevice->CreateRenderTargetView(m_pBackBufferTex.Get(), nullptr, &m_pBackBufferRTV), _T("ID3D10Device::CreateRenderTargetView"));

		m_isInitialized = true;
	}

	void Render()
	{
		if (!m_isInitialized)
		{
			return;
		}

		const float clearColor[4] = { 0, 0.5f, 0.5f, 1 };
		m_pD3DDevice->ClearRenderTargetView(m_pBackBufferRTV.Get(), clearColor);

		ThrowIfFailed(m_pSwapChain->Present(1, 0), _T("IDXGISwapChain::Present"));
	}
};

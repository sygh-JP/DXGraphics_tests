#include "stdafx.h"
#include "MfcD3D11ParticleTest1.h"
#include "MfcD3D11ParticleTest1Dlg.h"
//#include "afxdialogex.h"
#include "Shaders/CommonDefs.hlsli"


// BAB を使う方法も、SB を使う方法も、トータルの速度的にはあまり大差ない？
//#define USE_SB_PARTICLE


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#pragma region // アプリケーションのバージョン情報に使われる CAboutDlg ダイアログ

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// ダイアログ データ
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

	// 実装
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

#pragma endregion


// CMfcD3D11ParticleTest1Dlg ダイアログ



CMfcD3D11ParticleTest1Dlg::CMfcD3D11ParticleTest1Dlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMfcD3D11ParticleTest1Dlg::IDD, pParent)
	, m_elapsedTime()
	, m_cbCommonHost()
	, m_vbParticleHost(MaxParticleCount)
	, m_renderingTimerID()
	, m_frameCounter()
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMfcD3D11ParticleTest1Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PICTURE1, m_ddxcPicture1);
	DDX_Control(pDX, IDC_PICTURE2, m_ddxcPicture2);
	DDX_Control(pDX, IDC_STATIC_FPS_VAL, m_ddxcStaticFpsVal);
	DDX_Control(pDX, IDC_RADIO_CPU_BASED, m_ddxcRadioCpuBased);
	DDX_Control(pDX, IDC_RADIO_GPU_BASED, m_ddxcRadioGpuBased);
}

BEGIN_MESSAGE_MAP(CMfcD3D11ParticleTest1Dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_WM_DESTROY()
END_MESSAGE_MAP()


namespace MyUtils
{
	template<typename T> void LoadBinaryFromFileImpl(LPCWSTR pFilePath, std::vector<T>& outBuffer)
	{
		outBuffer.clear();

		struct _stat64 fileStats = {};
		const auto getFileStatFunc = _wstat64;

		if (getFileStatFunc(pFilePath, &fileStats) != 0 || fileStats.st_size < 0)
		{
			throw std::exception("Cannot get the file stats for the file!!");
		}

		if (fileStats.st_size % sizeof(T) != 0)
		{
			throw std::exception("The file size is not a multiple of the expected size of element!!");
		}

		const auto fileSizeInBytes = static_cast<uint64_t>(fileStats.st_size);

		if (sizeof(size_t) < 8 && (std::numeric_limits<size_t>::max)() < fileSizeInBytes)
		{
			throw std::exception("The file size is over the capacity on this platform!!");
		}

		if (fileStats.st_size == 0)
		{
			return;
		}

		const auto numElementsInFile = static_cast<size_t>(fileStats.st_size / sizeof(T));

		outBuffer.resize(numElementsInFile);

		FILE* pFile = nullptr;
		const auto retCode = _wfopen_s(&pFile, pFilePath, L"rb");
		if (retCode != 0 || pFile == nullptr)
		{
			throw std::exception("Cannot open the file!!");
		}
		fread_s(&outBuffer[0], numElementsInFile * sizeof(T), sizeof(T), numElementsInFile, pFile);
		fclose(pFile);
		pFile = nullptr;
	}

	template<typename T> bool LoadBinaryFromFileImpl2(LPCWSTR pFilePath, std::vector<T>& outBuffer)
	{
		try
		{
			LoadBinaryFromFileImpl(pFilePath, outBuffer);
			return true;
		}
		catch (const std::exception& ex)
		{
			const CStringW strMsg(ex.what());
			ATLTRACE(L"%s <%s>\n", strMsg.GetString(), pFilePath);
			return false;
		}
	}

	bool LoadBinaryFromFile(LPCWSTR pFilePath, std::vector<char>& outBuffer)
	{
		return LoadBinaryFromFileImpl2(pFilePath, outBuffer);
	}
}

namespace
{
	const D3D11_INPUT_ELEMENT_DESC VertexInputLayoutDescArrayPT[] =
	{
		// LPCSTR SemanticName, UINT SemanticIndex, DXGI_FORMAT Format, UINT InputSlot, UINT AlignedByteOffset, D3D11_INPUT_CLASSIFICATION InputSlotClass, UINT InstanceDataStepRate.

		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	const D3D11_INPUT_ELEMENT_DESC VertexInputLayoutDescArrayPCT[] =
	{
		// LPCSTR SemanticName, UINT SemanticIndex, DXGI_FORMAT Format, UINT InputSlot, UINT AlignedByteOffset, D3D11_INPUT_CLASSIFICATION InputSlotClass, UINT InstanceDataStepRate.

		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	const D3D11_INPUT_ELEMENT_DESC VertexInputLayoutDescArrayParticle1[] =
	{
		// LPCSTR SemanticName, UINT SemanticIndex, DXGI_FORMAT Format, UINT InputSlot, UINT AlignedByteOffset, D3D11_INPUT_CLASSIFICATION InputSlotClass, UINT InstanceDataStepRate.

		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "ANGLE",    0, DXGI_FORMAT_R32_FLOAT,          0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "PSIZE",    0, DXGI_FORMAT_R32_FLOAT,          0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	const D3D11_INPUT_ELEMENT_DESC VertexInputLayoutDescArrayParticle2[] =
	{
		// LPCSTR SemanticName, UINT SemanticIndex, DXGI_FORMAT Format, UINT InputSlot, UINT AlignedByteOffset, D3D11_INPUT_CLASSIFICATION InputSlotClass, UINT InstanceDataStepRate.

		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "VELOCITY", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "ANGLE",    0, DXGI_FORMAT_R32_FLOAT,          0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "DANGLE",   0, DXGI_FORMAT_R32_FLOAT,          0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "PSIZE",    0, DXGI_FORMAT_R32_FLOAT,          0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		//{ "LIFE",     0, DXGI_FORMAT_R32_UINT,           0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "RANDOM",   0, DXGI_FORMAT_R32G32B32A32_UINT,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};


	template<typename T> void UpdateCBuffer(ID3D11DeviceContext* pDeviceContext, ID3D11Buffer* pDstCBuffer, const T* pSrc)
	{
		pDeviceContext->UpdateSubresource(pDstCBuffer, 0, nullptr, pSrc, 0, 0);
	}


	inline float GetNextStdRandomUF()
	{
		return float(std::rand()) / float(RAND_MAX);
	}

	inline float GetNextStdRandomSF()
	{
		return 2.0f * GetNextStdRandomUF() - 1.0f;
	}


	inline XMFLOAT4 CreateColor4F(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
	{
		return XMFLOAT4(
			float(r) / float(255),
			float(g) / float(255),
			float(b) / float(255),
			float(a) / float(255));
	}


	void InvalidateStatic(CWnd* pParentWnd, CStatic& targetCtrl)
	{
		CRect pictWinRect;
		targetCtrl.GetWindowRect(pictWinRect);
		pParentWnd->ScreenToClient(pictWinRect);
		pParentWnd->InvalidateRect(pictWinRect, false);
	}
}


bool CMfcD3D11ParticleTest1Dlg::InitDirect3D(HWND hWnd)
{
	HRESULT hr = S_OK;

	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferDesc.Width = ScreenSizeX;
	swapChainDesc.BufferDesc.Height = ScreenSizeY;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_CENTERED;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 1;
	swapChainDesc.OutputWindow = hWnd;
	swapChainDesc.Windowed = true;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	auto featureLevel = D3D_FEATURE_LEVEL();

	hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
#ifdef _DEBUG
		D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
#else
		D3D11_CREATE_DEVICE_BGRA_SUPPORT,
#endif
		nullptr, 0,
		D3D11_SDK_VERSION,
		&swapChainDesc,
		&m_pSwapChain,
		&m_pD3DDevice,
		&featureLevel,
		&m_pD3DDeviceContext);
	if (FAILED(hr))
	{
		AfxMessageBox(_T("Failed to create D3D11 device!!"));
		return false;
	}
	if (featureLevel < D3D_FEATURE_LEVEL_11_0)
	{
		AfxMessageBox(_T("H/W does not support Direct3D 11.0!!"));
		return false;
	}

	hr = m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(m_pBackBufferTex.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		AfxMessageBox(_T("Failed to get DXGI back buffer!!"));
		return false;
	}

	hr = m_pD3DDevice->CreateRenderTargetView(m_pBackBufferTex.Get(), nullptr, &m_pBackBufferRTV);
	if (FAILED(hr))
	{
		AfxMessageBox(_T("Failed to create back buffer RTV!!"));
		return false;
	}

	// パーティクル スプライト用アルファマップ テクスチャ。
	{
		D3D11_TEXTURE2D_DESC texDesc = {};
		texDesc.Width = ParticleSpriteTexSize;
		texDesc.Height = ParticleSpriteTexSize;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.Format = DXGI_FORMAT_A8_UNORM;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		texDesc.SampleDesc.Count = 1;

		_ASSERTE(m_spriteTexSrcBmp.GetSafeHandle()); // ソースとなる GDI ビットマップが作成済みであること。
		_ASSERTE(ParticleSpriteTexSize % 4 == 0); // 幅が4の倍数であれば、8bpp でもパディングを考慮する必要はない。
		BITMAP bmpInfo = {};
		m_spriteTexSrcBmp.GetBitmap(&bmpInfo);
		_ASSERTE(bmpInfo.bmBitsPixel == 32);
		_ASSERTE(bmpInfo.bmWidth == ParticleSpriteTexSize);
		_ASSERTE(bmpInfo.bmHeight == ParticleSpriteTexSize);
		std::vector<RGBQUAD> tempBuffer32(ParticleSpriteTexSize * ParticleSpriteTexSize);
		const DWORD buffer32SizeInBytes = sizeof(RGBQUAD) * ParticleSpriteTexSize * ParticleSpriteTexSize;
		VERIFY(m_spriteTexSrcBmp.GetBitmapBits(buffer32SizeInBytes, &tempBuffer32[0]) == buffer32SizeInBytes);
		std::vector<BYTE> tempBuffer8(ParticleSpriteTexSize * ParticleSpriteTexSize);
		for (uint32_t i = 0; i < ParticleSpriteTexSize * ParticleSpriteTexSize; ++i)
		{
			tempBuffer8[i] = tempBuffer32[i].rgbRed;
		}
		D3D11_SUBRESOURCE_DATA subData = {};
		subData.pSysMem = &tempBuffer8[0];
		subData.SysMemPitch = ParticleSpriteTexSize;

		hr = m_pD3DDevice->CreateTexture2D(&texDesc, &subData, &m_pParticleSpriteTex);
		if (FAILED(hr))
		{
			AfxMessageBox(_T("Failed to create texture!!"));
			return false;
		}
		hr = m_pD3DDevice->CreateShaderResourceView(m_pParticleSpriteTex.Get(), nullptr, &m_pParticleSpriteSRV);
		if (FAILED(hr))
		{
			AfxMessageBox(_T("Failed to create texture SRV!!"));
			return false;
		}
	}

	// 定数バッファの作成。
	hr = m_pD3DDevice->CreateBuffer(&CD3D11_BUFFER_DESC(sizeof(MyCBufferCommon), D3D11_BIND_CONSTANT_BUFFER), nullptr, &m_pConstBufferCommon);
	if (FAILED(hr))
	{
		AfxMessageBox(_T("Failed to create const buffer!!"));
		return false;
	}

	m_cbCommonHost.UniGravity = 0.2f;
	m_cbCommonHost.UniNear = 1.0f;
	m_cbCommonHost.UniFar = 1000.0f;
	m_cbCommonHost.UniSpawnTargetParticleIndex = 0;
	m_cbCommonHost.UniMaxParticleVelocity = XMFLOAT3(2.0f, 5.0f, 2.0f);
	m_cbCommonHost.UniMaxAngularVelocity = XMConvertToRadians(2.0f);
	m_cbCommonHost.UniMaxParticleCount = MaxParticleCount;
	m_cbCommonHost.UniParticleInitSize = 3.0f;

	XMStoreFloat4x4(&m_cbCommonHost.UniMatrixView, XMMatrixTranspose(
		XMMatrixLookAtRH(XMVectorSet(0, 0, 100, 0), g_XMZero, XMVectorSet(0, 1, 0, 0))));
	XMStoreFloat4x4(&m_cbCommonHost.UniMatrixProj, XMMatrixTranspose(
		XMMatrixPerspectiveFovRH(XMConvertToRadians(60), float(ScreenSizeX) / float(ScreenSizeY),
		m_cbCommonHost.UniNear, m_cbCommonHost.UniFar)));

	{
		// CPU で書き換える頂点バッファ。
		hr = m_pD3DDevice->CreateBuffer(&CD3D11_BUFFER_DESC(sizeof(MyVertexParticle1) * MaxParticleCount,
			D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE),
			nullptr, &m_pVertexBufferParticles1);
		if (FAILED(hr))
		{
			AfxMessageBox(_T("Failed to create vertex buffer!!"));
			return false;
		}

		// CPU パーティクル用のホスト側コピーを初期化。GPU パーティクル バッファの初期化にも使う。
		// 全パーティクルの乱数シードだけセットしておく。
		auto& tempBuffer = m_vbParticleHost;
		ATLVERIFY(m_vbParticleHost.size() == MaxParticleCount);
		std::for_each(tempBuffer.begin(), tempBuffer.end(),
			[](MyVertexParticle2& x)
			{
				x.Position = XMFLOAT3(0,0,0);
				x.Velocity = XMFLOAT3(0,0,0);
				x.Angle = 0;
				x.DeltaAngle = 0;
				x.Size = 0;
				x.RandomSeed = Xorshift128Random::CreateInitialNumber(std::rand());
			});
		D3D11_SUBRESOURCE_DATA subData = {};
		subData.pSysMem = &tempBuffer[0];

		// GPU で書き換える頂点バッファ。
		hr = m_pD3DDevice->CreateBuffer(&CD3D11_BUFFER_DESC(sizeof(MyVertexParticle2) * MaxParticleCount,
			D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS, D3D11_USAGE_DEFAULT, 0, D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS),
			&subData, &m_pVertexBufferParticles2);
		if (FAILED(hr))
		{
			AfxMessageBox(_T("Failed to create vertex buffer!!"));
			return false;
		}

		// GPU で書き換える構造化バッファ。
		hr = m_pD3DDevice->CreateBuffer(&CD3D11_BUFFER_DESC(sizeof(MyVertexParticle2) * MaxParticleCount,
			D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS, D3D11_USAGE_DEFAULT, 0, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, sizeof(MyVertexParticle2)),
			&subData, &m_pStructBufferParticles2);
		if (FAILED(hr))
		{
			AfxMessageBox(_T("Failed to create structured buffer!!"));
			return false;
		}
	}

	// 頂点バッファの UAV (RWByteAddressBuffer) を作成する。
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = sizeof(MyVertexParticle2) * MaxParticleCount / sizeof(float);
		// MSDN の API ヘルプには構造化バッファの場合の説明はあるが、BAB の場合の説明がない。
		// ただ、DX SDK 付属の BasicCompute11 サンプルでは BAB でも NumElements が指定されている。
		// BAB の場合、4バイト単位での要素数を指定すればよいらしい。
		uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;

		hr = m_pD3DDevice->CreateUnorderedAccessView(m_pVertexBufferParticles2.Get(), &uavDesc, &m_pVertexBufferParticles2UAV);
		if (FAILED(hr))
		{
			AfxMessageBox(_T("Failed to create vertex buffer UAV!!"));
			return false;
		}
	}

	// 構造化バッファの SRV (StructuredBuffer) と UAV (RWStructuredBuffer) を作成する。
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX; // D3D11_SRV_DIMENSION_BUFFER ではダメらしい？
		srvDesc.BufferEx.FirstElement = 0;
		srvDesc.BufferEx.NumElements = MaxParticleCount;

		hr = m_pD3DDevice->CreateShaderResourceView(m_pStructBufferParticles2.Get(), &srvDesc, &m_pStructBufferParticles2SRV);
		if (FAILED(hr))
		{
			AfxMessageBox(_T("Failed to create structured buffer SRV!!"));
			return false;
		}

		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = MaxParticleCount;

		hr = m_pD3DDevice->CreateUnorderedAccessView(m_pStructBufferParticles2.Get(), &uavDesc, &m_pStructBufferParticles2UAV);
		if (FAILED(hr))
		{
			AfxMessageBox(_T("Failed to create structured buffer UAV!!"));
			return false;
		}
	}

	{
		auto samplerDesc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT());
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		hr = m_pD3DDevice->CreateSamplerState(&samplerDesc, &m_pSamplerLinearWrap);
		if (FAILED(hr))
		{
			AfxMessageBox(_T("Failed to create sampler state!!"));
			return false;
		}
	}

	{
		D3D11_DEPTH_STENCIL_DESC desc = {};
		desc.DepthEnable = false;
		desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		desc.DepthFunc = D3D11_COMPARISON_LESS;
		desc.StencilEnable = false;
		hr = m_pD3DDevice->CreateDepthStencilState(&desc, &m_pDepthStateNone);
		if (FAILED(hr))
		{
			AfxMessageBox(_T("Failed to create depth state!!"));
			return false;
		}
	}

	{
		D3D11_BLEND_DESC desc = {};
		desc.AlphaToCoverageEnable = false;
		desc.IndependentBlendEnable = false;
		desc.RenderTarget[0].BlendEnable = true;
		desc.RenderTarget[0].SrcBlend        = D3D11_BLEND_SRC_ALPHA;
		desc.RenderTarget[0].DestBlend       = D3D11_BLEND_INV_SRC_ALPHA;
		desc.RenderTarget[0].BlendOp         = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[0].SrcBlendAlpha   = D3D11_BLEND_ONE;
		desc.RenderTarget[0].DestBlendAlpha  = D3D11_BLEND_ZERO;
		desc.RenderTarget[0].BlendOpAlpha    = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		hr = m_pD3DDevice->CreateBlendState(&desc, &m_pBlendStateLinearAlpha);
		if (FAILED(hr))
		{
			AfxMessageBox(_T("Failed to create blend state!!"));
			return false;
		}
	}

	{
		D3D11_RASTERIZER_DESC desc = {};
		desc.FillMode              = D3D11_FILL_SOLID;
		desc.CullMode              = D3D11_CULL_NONE;
		desc.FrontCounterClockwise = true;
		desc.DepthBias             = 0;
		desc.DepthBiasClamp        = 0.0f;
		desc.SlopeScaledDepthBias  = 0.0f;
		desc.DepthClipEnable       = true;
		desc.ScissorEnable         = false;
		desc.MultisampleEnable     = false;
		desc.AntialiasedLineEnable = false;
		hr = m_pD3DDevice->CreateRasterizerState(&desc, &m_pRasterStateSolid);
		if (FAILED(hr))
		{
			AfxMessageBox(_T("Failed to create raster state!!"));
			return false;
		}
	}

	// パフォーマンス測定用の GPU クエリの作成。
	{
		D3D11_QUERY_DESC qDesc = {};
		qDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
		hr = m_pD3DDevice->CreateQuery(&qDesc, &m_pQueryDisjoint);
		ATLASSERT(SUCCEEDED(hr));
		qDesc.Query = D3D11_QUERY_TIMESTAMP;
		hr = m_pD3DDevice->CreateQuery(&qDesc, &m_pQueryStart);
		ATLASSERT(SUCCEEDED(hr));
		hr = m_pD3DDevice->CreateQuery(&qDesc, &m_pQueryEnd);
		ATLASSERT(SUCCEEDED(hr));
	}

	// シェーダーの作成。
	{
		TCHAR moduleFileName[MAX_PATH] = {};

		::GetModuleFileName(nullptr, moduleFileName, MAX_PATH);

		// EXE ファイルのある場所に、バイトコードが格納されたバイナリ ファイルがあると想定。

		ATL::CPath dirPath(moduleFileName);
		dirPath.RemoveFileSpec();

		std::vector<char> shaderBinBuf;

		using MyUtils::LoadBinaryFromFile;

		{
			ATL::CPath shaderBinFilePath(dirPath);
			shaderBinFilePath.Append(_T("vsParticle1.cso"));
			VERIFY(::PathFileExists(shaderBinFilePath.m_strPath.GetString()));

			if (!LoadBinaryFromFile(shaderBinFilePath, shaderBinBuf))
			{
				AfxMessageBox(_T("Failed to load vertex shader program!!"));
				return false;
			}

			hr = m_pD3DDevice->CreateVertexShader(&shaderBinBuf[0], shaderBinBuf.size(), nullptr,
				&m_pVertexShaderParticle1);
			if (FAILED(hr))
			{
				AfxMessageBox(_T("Failed to create vertex shader program!!"));
				return false;
			}

			hr = m_pD3DDevice->CreateInputLayout(VertexInputLayoutDescArrayParticle1, ARRAYSIZE(VertexInputLayoutDescArrayParticle1),
				&shaderBinBuf[0], shaderBinBuf.size(), &m_pVertexInputLayoutParticle1);
			if (FAILED(hr))
			{
				AfxMessageBox(_T("Failed to create vertex input layout!!"));
				return false;
			}
		}

		{
			ATL::CPath shaderBinFilePath(dirPath);
			shaderBinFilePath.Append(_T("vsParticle2.cso"));
			VERIFY(::PathFileExists(shaderBinFilePath.m_strPath.GetString()));

			if (!LoadBinaryFromFile(shaderBinFilePath, shaderBinBuf))
			{
				AfxMessageBox(_T("Failed to load vertex shader program!!"));
				return false;
			}

			hr = m_pD3DDevice->CreateVertexShader(&shaderBinBuf[0], shaderBinBuf.size(), nullptr,
				&m_pVertexShaderParticle2);
			if (FAILED(hr))
			{
				AfxMessageBox(_T("Failed to create vertex shader program!!"));
				return false;
			}

			hr = m_pD3DDevice->CreateInputLayout(VertexInputLayoutDescArrayParticle2, ARRAYSIZE(VertexInputLayoutDescArrayParticle2),
				&shaderBinBuf[0], shaderBinBuf.size(), &m_pVertexInputLayoutParticle2);
			if (FAILED(hr))
			{
				AfxMessageBox(_T("Failed to create vertex input layout!!"));
				return false;
			}
		}

		{
			ATL::CPath shaderBinFilePath(dirPath);
			shaderBinFilePath.Append(_T("vsParticle2SB.cso"));
			VERIFY(::PathFileExists(shaderBinFilePath.m_strPath.GetString()));

			if (!LoadBinaryFromFile(shaderBinFilePath, shaderBinBuf))
			{
				AfxMessageBox(_T("Failed to load vertex shader program!!"));
				return false;
			}

			hr = m_pD3DDevice->CreateVertexShader(&shaderBinBuf[0], shaderBinBuf.size(), nullptr,
				&m_pVertexShaderParticle2SB);
			if (FAILED(hr))
			{
				AfxMessageBox(_T("Failed to create vertex shader program!!"));
				return false;
			}
		}

		{
			ATL::CPath shaderBinFilePath(dirPath);
			shaderBinFilePath.Append(_T("gsParticle.cso"));
			VERIFY(::PathFileExists(shaderBinFilePath.m_strPath.GetString()));

			if (!LoadBinaryFromFile(shaderBinFilePath, shaderBinBuf))
			{
				AfxMessageBox(_T("Failed to load geometry shader program!!"));
				return false;
			}

			hr = m_pD3DDevice->CreateGeometryShader(&shaderBinBuf[0], shaderBinBuf.size(), nullptr,
				&m_pGeometryShaderParticle);
			if (FAILED(hr))
			{
				AfxMessageBox(_T("Failed to create geometry shader program!!"));
				return false;
			}
		}

		{
			ATL::CPath shaderBinFilePath(dirPath);
			shaderBinFilePath.Append(_T("psParticle.cso"));
			VERIFY(::PathFileExists(shaderBinFilePath.m_strPath.GetString()));

			if (!LoadBinaryFromFile(shaderBinFilePath, shaderBinBuf))
			{
				AfxMessageBox(_T("Failed to load pixel shader program!!"));
				return false;
			}

			hr = m_pD3DDevice->CreatePixelShader(&shaderBinBuf[0], shaderBinBuf.size(), nullptr,
				&m_pPixelShaderParticle);
			if (FAILED(hr))
			{
				AfxMessageBox(_T("Failed to create pixel shader program!!"));
				return false;
			}
		}

		{
			ATL::CPath shaderBinFilePath(dirPath);
#ifdef USE_SB_PARTICLE
			shaderBinFilePath.Append(_T("csParticleSB.cso"));
#else
			shaderBinFilePath.Append(_T("csParticleBAB.cso"));
#endif
			VERIFY(::PathFileExists(shaderBinFilePath.m_strPath.GetString()));

			if (!LoadBinaryFromFile(shaderBinFilePath, shaderBinBuf))
			{
				AfxMessageBox(_T("Failed to load compute shader program!!"));
				return false;
			}

			hr = m_pD3DDevice->CreateComputeShader(&shaderBinBuf[0], shaderBinBuf.size(), nullptr,
				&m_pComputeShaderParticle);
			if (FAILED(hr))
			{
				AfxMessageBox(_T("Failed to create compute shader program!!"));
				return false;
			}
		}
	}

	return true;
}

// MSVC CRT の std::rand() はマルチスレッド対応しているためにいくぶんオーバーヘッドがあるので、
// 毎フレーム使用すると Xorshift に比べて不利かと思い、CPU パーティクルの初期化にも Xorshift を使ってみたが、
// そこまで大きな差は見られない模様。
#define USE_XORSHIFT_RANDOM_FOR_CPU_PARTICLE

// Win32 高分解能パフォーマンス カウンターではなく ID3D11Query を使った計測方法だと、
// CPU 版パーティクルは一定時間（数秒）経過後にフレームレートの数値が極端に落ち込む。
// GPU 版パーティクルは一定時間経過後も比較的安定している模様。
#define USE_D3D_QUERY_FOR_PROFILING


void CMfcD3D11ParticleTest1Dlg::DrawByDirect3D()
{
	if (m_pD3DDeviceContext)
	{
#ifdef USE_D3D_QUERY_FOR_PROFILING
		m_pD3DDeviceContext->Begin(m_pQueryDisjoint.Get());
		m_pD3DDeviceContext->End(m_pQueryStart.Get()); // Insert the START timestamp
#else
		m_stopwatch.Start();
#endif

#if 0
		const XMFLOAT4 clearColor(0.0f, 0.5f, 0.5f, 1.0f); // Teal
#else
		const XMFLOAT4 clearColor = CreateColor4F(0x64, 0x95, 0xED); // CornflowerBlue
#endif

		D3D11_VIEWPORT viewport = {};
		viewport.Width = static_cast<float>(ScreenSizeX);
		viewport.Height = static_cast<float>(ScreenSizeY);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;

		m_pD3DDeviceContext->RSSetViewports(1, &viewport);

		m_pD3DDeviceContext->ClearRenderTargetView(m_pBackBufferRTV.Get(), &clearColor.x);

		m_pD3DDeviceContext->OMSetDepthStencilState(m_pDepthStateNone.Get(), 0);
		XMFLOAT4 blendFactor(0,0,0,0);
		m_pD3DDeviceContext->OMSetBlendState(m_pBlendStateLinearAlpha.Get(), &blendFactor.x, 0xFFFFFFFF);
		m_pD3DDeviceContext->RSSetState(m_pRasterStateSolid.Get());

		// CPU/GPU でパーティクルの色を変えたい場合、シェーダーで頂点のベース カラーを変えるなりすればいいが、
		// 色の違いによる目の錯覚（暗い色は重くて遅い、明るい色は軽くて速い）を引き起こすので、あえて同じ色にしておいたほうがよい。
		if (m_ddxcRadioCpuBased.GetCheck() == BST_CHECKED)
		{
			// CPU パーティクル。

			auto& spawnTargetParticleIndex = m_cbCommonHost.UniSpawnTargetParticleIndex;
			_ASSERTE(spawnTargetParticleIndex < m_vbParticleHost.size());
			// パーティクルの生成（再生成、リセット）。各々の寿命はパーティクルの総数に等しいフレーム数となる。
			auto& spawnParticle = m_vbParticleHost[spawnTargetParticleIndex];
			// 速度・角速度をランダムにする。
			spawnParticle.Position = XMFLOAT3(0, 0, 0);
#ifdef USE_XORSHIFT_RANDOM_FOR_CPU_PARTICLE
			const auto random0 = spawnParticle.RandomSeed;
			const auto random1 = Xorshift128Random::CreateNext(random0);
			const auto random2 = Xorshift128Random::CreateNext(random1);
			const auto random3 = Xorshift128Random::CreateNext(random2);
			const auto random4 = Xorshift128Random::CreateNext(random3);
#endif
			// Y 方向初速度は常に正。
			XMStoreFloat3(&spawnParticle.Velocity,
				XMVectorMultiply(
				XMLoadFloat3(&m_cbCommonHost.UniMaxParticleVelocity),
#ifdef USE_XORSHIFT_RANDOM_FOR_CPU_PARTICLE
				XMVectorSet(
				Xorshift128Random::GetRandomComponentSF(random0),
				Xorshift128Random::GetRandomComponentUF(random1),
				Xorshift128Random::GetRandomComponentSF(random2),
				0)
#else
				XMVectorSet(GetNextStdRandomSF(), GetNextStdRandomUF(), GetNextStdRandomSF(), 0)
#endif
				));
			spawnParticle.Angle = 0;
#ifdef USE_XORSHIFT_RANDOM_FOR_CPU_PARTICLE
			spawnParticle.DeltaAngle = m_cbCommonHost.UniMaxAngularVelocity * Xorshift128Random::GetRandomComponentSF(random3);
			spawnParticle.RandomSeed = random4;
#else
			spawnParticle.DeltaAngle = m_cbCommonHost.UniMaxAngularVelocity * GetNextStdRandomSF();
#endif
			spawnParticle.Size = m_cbCommonHost.UniParticleInitSize;
			spawnTargetParticleIndex = (spawnTargetParticleIndex + 1) % MaxParticleCount;

			D3D11_MAPPED_SUBRESOURCE mappedData = {};
			// GPU 側の頂点バッファは書込専用。以前のデータはすべて破棄する。
			// D3D11_MAP_WRITE は使えない。
			HRESULT hr = m_pD3DDeviceContext->Map(m_pVertexBufferParticles1.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);
			if (SUCCEEDED(hr))
			{
				// ホスト側の頂点データの更新と、頂点バッファへの反映。
				auto* pParticleArray = static_cast<MyVertexParticle1*>(mappedData.pData);
				for (uint32_t i = 0; i < MaxParticleCount; ++i)
				{
					auto& hostParticle = m_vbParticleHost[i];
					auto& devParticle = pParticleArray[i];
					XMStoreFloat3(&hostParticle.Position,
						XMVectorAdd(XMLoadFloat3(&hostParticle.Position), XMLoadFloat3(&hostParticle.Velocity)));
					devParticle.Position = hostParticle.Position;
					hostParticle.Velocity.y -= m_cbCommonHost.UniGravity;
					hostParticle.Angle += hostParticle.DeltaAngle;
					devParticle.Angle = hostParticle.Angle;
					hostParticle.Size = m_cbCommonHost.UniParticleInitSize; // HACK: とりあえず一定値だが、サイズも時間遷移などに応じて変える？
					devParticle.Size = hostParticle.Size;
				}
				m_pD3DDeviceContext->Unmap(m_pVertexBufferParticles1.Get(), 0);
			}
			// データ転送量削減のため、CPU バージョンは MyVertexParticle1 と ID3D11DeviceContext::Map() を採用したが、
			// ID3D11DeviceContext::UpdateSubresource() を使うならば GPU バージョンと同じ MyVertexParticle2 を使う必要がある。

			// GPU 側の定数バッファを書き換える。
			UpdateCBuffer(m_pD3DDeviceContext.Get(), m_pConstBufferCommon.Get(), &m_cbCommonHost);

			ID3D11Buffer* vbs[] =
			{ m_pVertexBufferParticles1.Get() };
			const UINT strides[] =
			{ sizeof(MyVertexParticle1) };
			const UINT offsets[] =
			{ 0 };
			ID3D11Buffer* cbs[] =
			{ m_pConstBufferCommon.Get() };
			ID3D11SamplerState* samplers[] =
			{ m_pSamplerLinearWrap.Get() };
			ID3D11ShaderResourceView* srvs[] =
			{ m_pParticleSpriteSRV.Get() };
			ID3D11RenderTargetView* rtvs[] =
			{ m_pBackBufferRTV.Get() };

			m_pD3DDeviceContext->VSSetConstantBuffers(0, ARRAYSIZE(cbs), cbs);
			m_pD3DDeviceContext->VSSetShader(m_pVertexShaderParticle1.Get(), nullptr, 0);
			m_pD3DDeviceContext->GSSetConstantBuffers(0, ARRAYSIZE(cbs), cbs);
			m_pD3DDeviceContext->GSSetShader(m_pGeometryShaderParticle.Get(), nullptr, 0);
			m_pD3DDeviceContext->PSSetConstantBuffers(0, ARRAYSIZE(cbs), cbs);
			m_pD3DDeviceContext->PSSetShader(m_pPixelShaderParticle.Get(), nullptr, 0);
			m_pD3DDeviceContext->PSSetSamplers(0, ARRAYSIZE(samplers), samplers);
			m_pD3DDeviceContext->PSSetShaderResources(0, ARRAYSIZE(srvs), srvs);

			m_pD3DDeviceContext->OMSetRenderTargets(ARRAYSIZE(rtvs), rtvs, nullptr);
			m_pD3DDeviceContext->IASetInputLayout(m_pVertexInputLayoutParticle1.Get());
			m_pD3DDeviceContext->IASetVertexBuffers(0, ARRAYSIZE(vbs), vbs, strides, offsets);
			m_pD3DDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
			m_pD3DDeviceContext->Draw(MaxParticleCount, 0);
		}
		else
		{
			// GPU パーティクル。

			// GPU 側の定数バッファを書き換える。
			UpdateCBuffer(m_pD3DDeviceContext.Get(), m_pConstBufferCommon.Get(), &m_cbCommonHost);

			ID3D11Buffer* vbs[] =
			{ m_pVertexBufferParticles2.Get() };
			UINT strides[] =
			{ sizeof(MyVertexParticle2) };
			const UINT offsets[] =
			{ 0 };
			ID3D11Buffer* cbs[] =
			{ m_pConstBufferCommon.Get() };
			ID3D11SamplerState* samplers[] =
			{ m_pSamplerLinearWrap.Get() };
			ID3D11ShaderResourceView* srvs[] =
			{
				m_pParticleSpriteSRV.Get(),
				m_pStructBufferParticles2SRV.Get(),
			};
			ID3D11RenderTargetView* rtvs[] =
			{ m_pBackBufferRTV.Get() };
			ID3D11UnorderedAccessView* uavs[] =
			{
#ifdef USE_SB_PARTICLE
				m_pStructBufferParticles2UAV.Get(),
#else
				m_pVertexBufferParticles2UAV.Get(),
#endif
			};

			m_pD3DDeviceContext->CSSetConstantBuffers(0, ARRAYSIZE(cbs), cbs);
			m_pD3DDeviceContext->CSSetShader(m_pComputeShaderParticle.Get(), nullptr, 0);
			m_pD3DDeviceContext->CSSetUnorderedAccessViews(0, ARRAYSIZE(uavs), uavs, nullptr);

			// オリジナルのプログラムでは、ローカル グループ サイズに中途半端な 100 という数が使われていたが、
			// 通常は2のべき乗（特に 32 の倍数）を使う。
			// というのも NVIDIA には 32 threads/unit を束ねる WARP、AMD には 64 threads/unit を束ねる WAVEFRONT という概念があるので、
			// その倍数に合わせておいたほうが効率が良くなることが多い。
			// Parallel Reduction などの並列アルゴリズムも2のべき乗のブロック サイズを使う。
			// また、グループ共有メモリやレジスタ数との兼ね合いにもなるが、
			// グループ サイズを 256, 512, 1024 などにして Occupancy を高めたほうが効率が良くなることが多い。
			// コンピュート シェーダーはスレッド グループ サイズや共有メモリなどハードウェア アーキテクチャに密接に関わる機能が多く、
			// OpenCL や CUDA 同様にプログラムの設計やパラメータ次第で性能が大きく変わるため、
			// 適切なチューニングを行なうには（従来のグラフィックス プログラミング以上に）ハードウェアに関する深い知識が必要となる。
			// cf. Kernel Occupancy of OpenCL or CUDA.

			// 1スレッドに1パーティクルのみを処理させるよりも、複数のパーティクルを処理させたほうがパフォーマンスが上がりやすい模様？
			// パーティクルの初期化のみを行なうスレッドをなくすことで、ワーク バランスを調整できる？

			// パーティクル総数がタスク ユニット サイズで割り切れない場合、パーティクル総数よりも多くスレッドを起動することになる。
			// 余ったスレッドは動的分岐で作業をさせないようにする。
			// http://wlog.flatlib.jp/item/1425
			const UINT dispatchX = UINT(std::ceil(float(MaxParticleCount) / float(MY_CS_LOCAL_GROUP_SIZE * MY_CS_PARTICLES_NUM_PER_THREAD)));
			m_pD3DDeviceContext->Dispatch(dispatchX, 1, 1);

			// UAV スロット (InOut) をリセット。
			uavs[0] = nullptr;
			m_pD3DDeviceContext->CSSetUnorderedAccessViews(0, ARRAYSIZE(uavs), uavs, nullptr);

			m_pD3DDeviceContext->VSSetConstantBuffers(0, ARRAYSIZE(cbs), cbs);
#ifdef USE_SB_PARTICLE
			m_pD3DDeviceContext->VSSetShader(m_pVertexShaderParticle2SB.Get(), nullptr, 0);
			m_pD3DDeviceContext->VSSetShaderResources(0, ARRAYSIZE(srvs), srvs);
#else
			m_pD3DDeviceContext->VSSetShader(m_pVertexShaderParticle2.Get(), nullptr, 0);
#endif
			m_pD3DDeviceContext->GSSetConstantBuffers(0, ARRAYSIZE(cbs), cbs);
			m_pD3DDeviceContext->GSSetShader(m_pGeometryShaderParticle.Get(), nullptr, 0);
			m_pD3DDeviceContext->PSSetConstantBuffers(0, ARRAYSIZE(cbs), cbs);
			m_pD3DDeviceContext->PSSetShader(m_pPixelShaderParticle.Get(), nullptr, 0);
			m_pD3DDeviceContext->PSSetSamplers(0, ARRAYSIZE(samplers), samplers);
			m_pD3DDeviceContext->PSSetShaderResources(0, ARRAYSIZE(srvs), srvs);

			m_pD3DDeviceContext->OMSetRenderTargets(ARRAYSIZE(rtvs), rtvs, nullptr);
			m_pD3DDeviceContext->IASetInputLayout(m_pVertexInputLayoutParticle2.Get());
			m_pD3DDeviceContext->IASetVertexBuffers(0, ARRAYSIZE(vbs), vbs, strides, offsets);
			m_pD3DDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
			// 構造化バッファを使う場合、頂点バッファはダミー入力。実際のパーティクル情報は構造化バッファにて管理されている。
			// 頂点をパーティクルの数だけ用意しておく方法に比べると、インスタンス描画は遅くなる模様。
#if defined(USE_SB_PARTICLE) && defined(USE_INSTANCE_ID_FOR_PARTICLE_DRAW)
			m_pD3DDeviceContext->DrawInstanced(1, MaxParticleCount, 0, 0);
#else
			m_pD3DDeviceContext->Draw(MaxParticleCount, 0);
#endif

#ifdef USE_SB_PARTICLE
			// SRV スロット (In) をリセット。
			srvs[1] = nullptr;
			m_pD3DDeviceContext->VSSetShaderResources(0, ARRAYSIZE(srvs), srvs);
			m_pD3DDeviceContext->PSSetShaderResources(0, ARRAYSIZE(srvs), srvs);
#else
			// VB スロット (In) をリセット。
			vbs[0] = nullptr;
			strides[0] = 0;
			m_pD3DDeviceContext->IASetVertexBuffers(0, ARRAYSIZE(vbs), vbs, strides, offsets);
#endif

			auto& spawnTargetParticleIndex = m_cbCommonHost.UniSpawnTargetParticleIndex;
			spawnTargetParticleIndex = (spawnTargetParticleIndex + 1) % MaxParticleCount;
		}

		// VSync は OFF。
		m_pSwapChain->Present(0, 0);

#ifdef USE_D3D_QUERY_FOR_PROFILING
		m_pD3DDeviceContext->End(m_pQueryEnd.Get()); // Insert the END timestamp
		m_pD3DDeviceContext->End(m_pQueryDisjoint.Get());
		{
			UINT64 startTime = 0;
			while (m_pD3DDeviceContext->GetData(m_pQueryStart.Get(), &startTime, sizeof(startTime), 0) == S_FALSE);
			UINT64 endTime = 0;
			while (m_pD3DDeviceContext->GetData(m_pQueryEnd.Get(), &endTime, sizeof(endTime), 0) == S_FALSE);
			D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData = {};
			while (m_pD3DDeviceContext->GetData(m_pQueryDisjoint.Get(), &disjointData, sizeof(disjointData), 0) == S_FALSE);
			if (!disjointData.Disjoint)
			{
				const double deltaTime = double(endTime - startTime);
				const double frequencyVal = double(disjointData.Frequency); // [Hz]
				m_elapsedTime += (deltaTime / frequencyVal);
				++m_frameCounter;
			}
		}
#else
		m_stopwatch.Stop();
		++m_frameCounter;
#endif

		const uint32_t FpsCalcFrameCount = 60;
		if (FpsCalcFrameCount <= m_frameCounter)
		{
#ifdef USE_D3D_QUERY_FOR_PROFILING
			const double fpsVal = m_frameCounter / m_elapsedTime;
#else
			const double elapsedTime = m_stopwatch.GetElapsedTimeInSeconds();
			const double fpsVal = m_frameCounter / elapsedTime;
#endif
			//ATLTRACE("FPS = %.1f\n", fpsVal);
			CString strFpsVal;
			strFpsVal.Format(_T("%.1f\n"), fpsVal);
			m_ddxcStaticFpsVal.SetWindowText(strFpsVal);
			m_stopwatch.Reset();
			m_frameCounter = 0;
			m_elapsedTime = 0;
		}
	}
}

// CMfcD3D11ParticleTest1Dlg メッセージ ハンドラー

BOOL CMfcD3D11ParticleTest1Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// "バージョン情報..." メニューをシステム メニューに追加します。

	// IDM_ABOUTBOX は、システム コマンドの範囲内になければなりません。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// このダイアログのアイコンを設定します。アプリケーションのメイン ウィンドウがダイアログでない場合、
	//  Framework は、この設定を自動的に行います。
	SetIcon(m_hIcon, TRUE);			// 大きいアイコンの設定
	SetIcon(m_hIcon, FALSE);		// 小さいアイコンの設定

	// TODO: 初期化をここに追加します。

	//m_ddxcRadioCpuBased.SetCheck(BST_CHECKED);
	//m_ddxcRadioGpuBased.SetCheck(BST_CHECKED);

	this->CheckRadioButton(IDC_RADIO_CPU_BASED, IDC_RADIO_GPU_BASED, IDC_RADIO_CPU_BASED);
	//this->CheckRadioButton(IDC_RADIO_CPU_BASED, IDC_RADIO_GPU_BASED, IDC_RADIO_GPU_BASED);

	m_ddxcPicture1.SetWindowPos(nullptr, 0, 0, ScreenSizeX, ScreenSizeY, SWP_NOMOVE | SWP_NOZORDER);
	m_ddxcPicture2.SetWindowPos(nullptr, 0, 0, ParticleSpriteTexSize, ParticleSpriteTexSize, SWP_NOMOVE | SWP_NOZORDER);

	// GDI を使って適当な文字をオフスクリーン レンダリングし、パーティクル スプライト用アルファマップ テクスチャに使うマスクデータを作成する。
	// GDI/DirectWrite でフォント メトリックを取得してマスクデータの DIB を作成する方法もあるが、レンダリングするより面倒なので割愛。
	{
		CClientDC clientDC(this);
		CDC memDC;
		memDC.CreateCompatibleDC(&clientDC);
		CFont textFont;
#if 0
		const LPCTSTR fontName = _T("メイリオ");
#else
		const LPCTSTR fontName = _T("Meiryo UI");
#endif
		//textFont.CreatePointFont(200, fontName);
		// グレースケールのマスクデータが欲しいので、ClearType アンチエイリアスは使わない。
		textFont.CreateFont(80, 0, 0, 0, FW_REGULAR, 0, 0, 0, SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			//CLEARTYPE_QUALITY,
			ANTIALIASED_QUALITY,
			FF_DONTCARE, fontName);
		// 互換ビットマップ作成時、CreateCompatibleBitmap() の第1引数にメモリ DC を使うとモノクロ (1bit) になる。
		// http://msdn.microsoft.com/ja-jp/library/cc428327.aspx
		m_spriteTexSrcBmp.CreateCompatibleBitmap(&clientDC, ParticleSpriteTexSize, ParticleSpriteTexSize);
		BITMAP bmpInfo = {};
		m_spriteTexSrcBmp.GetBitmap(&bmpInfo);
		_ASSERTE(bmpInfo.bmBitsPixel == 32);
		auto pOldBmp = memDC.SelectObject(m_spriteTexSrcBmp);
		auto pOldFont = memDC.SelectObject(textFont);
		// 黒地に白文字を描画。
		memDC.SetTextColor(RGB(255, 255, 255));
		memDC.SetBkMode(TRANSPARENT);
#if 1
		const LPCWSTR srcText = L"✦";
#elif 0
		const LPCWSTR srcText = L"✧";
#elif 0
		const LPCWSTR srcText = L"✸";
#elif 0
		const LPCWSTR srcText = L"★";
#else
		const LPCWSTR srcText = L"♥";
#endif
		memDC.DrawText(srcText, -1, CRect(1, 1, ParticleSpriteTexSize - 1, ParticleSpriteTexSize - 8),
			DT_SINGLELINE | DT_CENTER | DT_VCENTER);
		memDC.SelectObject(pOldBmp);
		memDC.SelectObject(pOldFont);
	}
	m_ddxcPicture2.SetBitmap(static_cast<HBITMAP>(m_spriteTexSrcBmp.GetSafeHandle()));

	if (!this->InitDirect3D(m_ddxcPicture1.GetSafeHwnd()))
	{
		//this->SendMessage(WM_CLOSE);
		//this->OnCancel();
		this->EndDialog(IDCANCEL);
		return true;
	}

	this->StartRenderingTimer();

	return TRUE;  // フォーカスをコントロールに設定した場合を除き、TRUE を返します。
}

void CMfcD3D11ParticleTest1Dlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// ダイアログに最小化ボタンを追加する場合、アイコンを描画するための
//  下のコードが必要です。ドキュメント/ビュー モデルを使う MFC アプリケーションの場合、
//  これは、Framework によって自動的に設定されます。

void CMfcD3D11ParticleTest1Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 描画のデバイス コンテキスト

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// クライアントの四角形領域内の中央
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// アイコンの描画
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();

		this->DrawByDirect3D();
	}
}

// ユーザーが最小化したウィンドウをドラッグしているときに表示するカーソルを取得するために、
//  システムがこの関数を呼び出します。
HCURSOR CMfcD3D11ParticleTest1Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CMfcD3D11ParticleTest1Dlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: ここにメッセージ ハンドラー コードを追加するか、既定の処理を呼び出します。

	if (nIDEvent == m_renderingTimerID)
	{
		InvalidateStatic(this, m_ddxcPicture1);
		//this->DrawByDirect3D();
	}

	CDialogEx::OnTimer(nIDEvent);
}


void CMfcD3D11ParticleTest1Dlg::OnDestroy()
{
	this->StopRenderingTimer();

	CDialogEx::OnDestroy();

	// TODO: ここにメッセージ ハンドラー コードを追加します。
}

#include "stdafx.h"
#include "MyD3DManager.h"


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
}

// 無印 Windows 8 環境に Windows 10 TP Build 9926 をアップグレード インストールしてテストしてみたが、
// NVIDIA 340.52 ドライバーと GeForce GTX 770 の組み合わせでは、Direct3D 12 デバイスを作成できない？
// Visual Studio と SDK のバージョンを CTP 6 に更新し、Windows 10 TP を Build 10041 に更新しても、
// やはり D3D12CreateDevice() が DXGI エラー（DXGI_ERROR_UNSUPPORTED, 0x887A0004）で失敗する。
// dxdiag で調べてみたら WDDM 1.3 (Windows 8.1) のままだった。340.52 ドライバーは 2014-07-29 リリースなので仕方ないかも。
// Windows Update 経由でデバイスに合わせた WDDM 2.0 ベータ ドライバーを取得できるらしいが、環境によっては WU が正常に動作しない。
// その後、Windows 10 向け GeForce ベータ ドライバー 352.63 のスタンドアロン インストーラーが 2015-04-29 にようやく公開された。
// http://www.nvidia.co.jp/download/driverResults.aspx/84257/jp
// こちらを Build 10074 にインストールすることで WDDM 2.0 になった。
// ちなみに Build 10041 にインストールしようとしたら、ドライバーインストール処理途中に OS がクラッシュする現象に遭遇した。

// ・Creating a basic Direct3D 12 component
// https://msdn.microsoft.com/en-us/library/dn859356.aspx
// 3月に公開された Direct3D 12 暫定ドキュメントと微妙に異なる部分が多い。
// 4月末に Visual Studio 2015 RC のリリースに合わせて実装とドキュメントがさらに更新された模様。
// しかし相変わらず暫定ドキュメントと実装とで微妙に異なる部分がある。

// 日本人が作成した Direct3D 12 with Visual Studio 2015 RC のサンプルとしては、下記が参考になる。
// https://github.com/shobomaru/HelloD3D12

void MyD3DManager::LoadPipeline(HWND hWnd, UINT width, UINT height)
{
#if _DEBUG
	const UINT dxgiFactoryFlag = DXGI_CREATE_FACTORY_DEBUG;
#else
	const UINT dxgiFactoryFlag = 0;
#endif

	// DXGI 1.3 Factory を生成。
	ComPtr<IDXGIFactory2> dxgiFactory;
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlag, IID_PPV_ARGS(&dxgiFactory)), _T("CreateDXGIFactory2"));

#if _DEBUG
	// Visual Studio 2015 RC の IDE にバグがあるっぽい。ただのブロックを使ってインデントをしようとすると、オート フォーマットが正常に機能しないことがある。
	{
		// 「D3D12GetDebugInterface: This method requiresthe D3D12 SDK Layers for Windows 10, but they are not present on the system.」
		// というメッセージが出力されてデバッグ レイヤーが有効にならない模様。32bit/64bit ともに発生。
		// ていうか requires と the の間のスペースが抜けてるし……
		// 0x887a002d すなわち DXGI_ERROR_SDK_COMPONENT_MISSING が返却される模様。
		// Windows 10 で Direct3D デバッグ レイヤーを使う場合、事前に Graphics Tools を明示的に有効化（インストール）する必要があるらしい。
		// これは Direct3D 12 に限らず、D3D11 や D3D10 でも同様らしい。
		// http://masafumi.cocolog-nifty.com/masafumis_diary/2015/07/windows-10direc.html
		// https://shobomaru.wordpress.com/2015/12/30/install-graphics-tools-on-windows-10/
		// https://msdn.microsoft.com/en-us/library/mt125501.aspx

		ComPtr<ID3D12Debug> debug;
		const HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
		ATLTRACE("D3D12GetDebugInterface = 0x%08lx\n", hr);
		if (SUCCEEDED(hr) && debug)
		{
			debug->EnableDebugLayer();
		}
	}
#endif

	// Visual Studio 2015 CTP 6 時点の関数シグネチャでは、D3D12_CREATE_DEVICE_FLAG や D3D_DRIVER_TYPE、D3D12_SDK_VERSION を指定するパラメータがあり、
	// D3D11CreateDevice() に近かった。
	// Visual Studio 2015 RC 時点の関数シグネチャではかなりパラメータが減ってシンプルになった。
	// なお、ついに D3D_FEATURE_LEVEL_12_0, D3D_FEATURE_LEVEL_12_1 が追加されたらしい。
	// ただし D3D_FEATURE_LEVEL_11_2, D3D_FEATURE_LEVEL_11_3 はやはり追加されない模様。
	ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_d3dDevice)), _T("D3D12CreateDevice"));

	// コマンド アロケーター、コマンド キューを作成。
	{
		ThrowIfFailed(m_d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_cmdAllocator)), _T("CreateCommandAllocator"));

		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		ThrowIfFailed(m_d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_cmdQueue)), _T("CreateCommandQueue"));
	}

	//DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = NumSwapBufs;
	//swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//swapChainDesc.BufferDesc.Width = width;
	//swapChainDesc.BufferDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	swapChainDesc.SampleDesc.Count = 1;
	//swapChainDesc.OutputWindow = hWnd;
	//swapChainDesc.Windowed = true;
	//swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// Direct3D 12 では DXGI Device の取得はサポートされないらしい。
	// DXGI デバイスは Direct2D/DirectWrite 連携をする場合に必要となるが、まず D3D11-D3D12 相互運用機能を使って Direct3D 11 デバイスを作成する必要があるらしい。
	// http://www.gamedev.net/topic/666986-direct3d-12-documentation-is-now-public/page-7
#if 0
	ComPtr<IDXGIAdapter> dxgiAdapter;
	ComPtr<IDXGIDevice> dxgiDevice;
	ComPtr<IDXGIFactory2> dxgiFactory;
	ThrowIfFailed(m_d3dDevice.As(&dxgiDevice), _T("Get DXGI device from D3D12 device")); // E_NOINTERFACE No such interface supported.
	ThrowIfFailed(dxgiDevice->GetAdapter(&dxgiAdapter), _T("Get DXGI adapter"));
	ThrowIfFailed(dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), &dxgiFactory), _T("Get DXGI factory"));
#endif

	// Create the swap chain.
	//ThrowIfFailed(dxgiFactory->CreateSwapChain(m_d3dDevice.Get(), &swapChainDesc, &m_dxgiSwapChain), _T("CreateSwapChain"));
	ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(m_cmdQueue.Get(), hWnd, &swapChainDesc, nullptr, nullptr, m_dxgiSwapChain.ReleaseAndGetAddressOf()), _T("CreateSwapChainForHwnd"));
}

// Direct3D Shader Compiler 6.3.9600.16384 (using C:\Program Files (x86)\Windows Kits\8.1\bin\x64\D3DCOMPILER_47.dll)
// Direct3D Shader Compiler 10.0.10011.0 (using C:\Program Files (x86)\Windows Kits\10\bin\x64\D3DCOMPILER_47.dll)
// 
// VS 2013 / Windows SDK 8.1 付属の fxc.exe 6.3 ではシェーダーモデル 5.1 のプロファイルは存在しなかったが、
// VS 2015 RC / Windows SDK 10.0.10069.0 付属の fxc.exe 10.0 ではシェーダーモデル 5.1 のプロファイルが存在する。
// cs_5_1, ds_5_1, gs_5_1, hs_5_1, ps_5_1, vs_5_1 などが追加されている。
// ただし VS 2015 RC に統合された HLSL コンパイラ ツールは、[シェーダーモデル] 欄で [/5_1] を選択できない。
// VS 2015 RTM のツールでは、シェーダーモデル 5.1 が選択できるようになっている模様。

void MyD3DManager::LoadAssets(UINT width, UINT height)
{
	const UINT nodeMask = 0;

	// Pipeline State Object (PSO) の作成。
	{
		ATL::CPathW dirPath;
		::GetModuleFileNameW(nullptr, dirPath.m_strPath.GetBuffer(MAX_PATH), MAX_PATH);
		dirPath.m_strPath.ReleaseBuffer();
		dirPath.RemoveFileSpec();

		// GLSL のように、D3DCompileFromFile() で実行時に毎回 HLSL ソースをコンパイルしてもいいが、
		// シェーダーのコンパイルはわりと時間がかかるので、Visual Studio の組み込みツールを使って
		// C++ コードとともに事前コンパイルしてバイトコードを生成しておく。

		std::vector<byte> vsSimpleBytecode;
		std::vector<byte> psSimpleBytecode;

		const auto& loadFunc = [dirPath](LPCWSTR fileName, std::vector<byte>& outBinBuf)
		{
			ATL::CPathW shaderBinFilePath(dirPath);
			shaderBinFilePath.Append(fileName);
			ATLVERIFY(::PathFileExistsW(shaderBinFilePath.m_strPath.GetString()));

			try
			{
				MyUtils::LoadBinaryFromFileImpl(shaderBinFilePath, outBinBuf);
			}
			catch (const std::exception& ex)
			{
				throw MyComException(E_FAIL, CString(ex.what()));
			}
			if (outBinBuf.empty())
			{
				throw MyComException(E_FAIL, _T("The shader object file is empty!!"));
			}
		};
		loadFunc(L"vsSimple1.cso", vsSimpleBytecode);
		loadFunc(L"psSimple.cso", psSimpleBytecode);

		// VS 2015 RC 用に更新された暫定ドキュメントでは、CD3DX12_ROOT_SIGNATURE_DESC, CD3DX12_RASTERIZER_DESC, CD3DX12_BLEND_DESC などの
		// ヘルパークラスがあたかも存在するかのように記述されたサンプルコードが記載されているが、実際の SDK には D3DX12 のヘッダーは含まれていない模様。
		// 最終製品版には同梱されるのかも。
		// https://msdn.microsoft.com/en-us/library/windows/desktop/dn708058.aspx
		// D3D11 では、CD3D11_RASTERIZER_DESC, CD3D11_BLEND_DESC などの形でヘルパークラスが標準ヘッダー d3d11.h に記述されていたが、
		// D3D12 では CD3DX12_XXX として d3dx12.h に分離される模様。しかし別の暫定ドキュメントでは d3d12.h に CD3D12_XXX が実装されると書いてある。どっちが正しいのか……
		// https://msdn.microsoft.com/en-us/library/windows/desktop/dn859316.aspx
		// https://msdn.microsoft.com/en-us/library/windows/desktop/dn859271.aspx

		ComPtr<ID3DBlob> pOutBlob, pErrorBlob;
		D3D12_DESCRIPTOR_RANGE descRangeCbvSrvUav[1] = {};
		descRangeCbvSrvUav[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		descRangeCbvSrvUav[0].NumDescriptors = 1;
		descRangeCbvSrvUav[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		D3D12_ROOT_PARAMETER rootParam[1] = {};
		rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParam[0].DescriptorTable.NumDescriptorRanges = ARRAYSIZE(descRangeCbvSrvUav);
		rootParam[0].DescriptorTable.pDescriptorRanges = descRangeCbvSrvUav;
		D3D12_ROOT_SIGNATURE_DESC descRootSignature = {};
		descRootSignature.NumParameters = ARRAYSIZE(rootParam);
		descRootSignature.pParameters = rootParam;
		descRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		ThrowIfFailed(D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, pOutBlob.ReleaseAndGetAddressOf(), pErrorBlob.ReleaseAndGetAddressOf()), _T("D3D12SerializeRootSignature"));
		// nodeMask パラメータはデバイスのアフィニティ マスクか？
		ThrowIfFailed(m_d3dDevice->CreateRootSignature(nodeMask, pOutBlob->GetBufferPointer(), pOutBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)), _T("CreateRootSignature"));

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout.NumElements = ARRAYSIZE(InputLayoutDescMyVertex);
		psoDesc.InputLayout.pInputElementDescs = InputLayoutDescMyVertex;
		psoDesc.pRootSignature = m_rootSignature.Get();
		psoDesc.VS = { &vsSimpleBytecode[0], vsSimpleBytecode.size() };
		psoDesc.PS = { &psSimpleBytecode[0], psSimpleBytecode.size() };
		psoDesc.RasterizerState =
		{
			D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_BACK,
			true,
			D3D12_DEFAULT_DEPTH_BIAS, D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
			D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
			true,
			false,
			false,
			0,
			D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
		};
		psoDesc.BlendState =
		{
			false,
			false,
		};
		const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
		{
			false, false,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_LOGIC_OP_NOOP,
			D3D12_COLOR_WRITE_ENABLE_ALL,
		};
		static_assert(ARRAYSIZE(psoDesc.BlendState.RenderTarget) == D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT, "Unexpected max render-target count!!");
		for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		{
			psoDesc.BlendState.RenderTarget[i] = defaultRenderTargetBlendDesc;
		}
		psoDesc.DepthStencilState.DepthEnable = false;
		psoDesc.DepthStencilState.StencilEnable = false;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;

		// Create the actual PSO.
		ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_simpleGraphicsPSO1)), _T("CreateGraphicsPipelineState"));

		loadFunc(L"vsSimple2.cso", vsSimpleBytecode);

		psoDesc.VS = { &vsSimpleBytecode[0], vsSimpleBytecode.size() };

		psoDesc.BlendState.RenderTarget[0].BlendEnable = true;
		psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;

		ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_simpleGraphicsPSO2)), _T("CreateGraphicsPipelineState"));

		// Direct3D 12 には、Direct3D 11 (Shader Model 5.0) の Dynamic Shader Linkage 相当機能や、
		// Direct3D 11.2 で追加された HLSL Shader Linking 相当機能が存在しないようだが、
		// 結局 Direct3D 10 までのように、シェーダージェネレーターよろしく PSO ジェネレーターを自作するしかないのか？
		// https://msdn.microsoft.com/en-us/library/windows/desktop/ff471420.aspx
		// https://msdn.microsoft.com/en-us/library/windows/desktop/dn312084.aspx
	}

	// create descriptor heap
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 1;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_descHeapRtv)), _T("CreateDescriptorHeap"));

		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_descHeapCbvSrvUav)), _T("CreateDescriptorHeap"));
	}

	// nodeMask パラメータはデバイスのアフィニティ マスクか？
	ThrowIfFailed(m_d3dDevice->CreateCommandList(nodeMask, D3D12_COMMAND_LIST_TYPE_DIRECT, m_cmdAllocator.Get(), m_simpleGraphicsPSO1.Get(), IID_PPV_ARGS(&m_graphicsCmdList)), _T("Create graphics command list"));

	// 2015年3月の暫定ドキュメントのサンプルコードでは下記のメソッド呼び出しが存在したが、VS 2015 RC 対応版でコレを実行してしまうと、
	// ID3D12GraphicsCommandList::Close() が 0x80070057 すなわち E_INVALIDARG で失敗する原因になる。
	//m_graphicsCmdList->SetDescriptorHeaps(1, m_descHeapRtv.GetAddressOf());

	// create backbuffer/rendertarget
	ThrowIfFailed(m_dxgiSwapChain->GetBuffer(0, IID_PPV_ARGS(&m_resRenderTarget)), _T("Get buffer from DXGI swap chain"));
	//auto cpuDesc = D3D12_CPU_DESCRIPTOR_HANDLE();
	//D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	//rtvDesc.Format = swapChainDesc.BufferDesc.Format;
	//rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_UNKNOWN;
	m_d3dDevice->CreateRenderTargetView(m_resRenderTarget.Get(), nullptr, m_descHeapRtv->GetCPUDescriptorHandleForHeapStart());

	{
		// set the viewport
		D3D12_VIEWPORT viewport = {};
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = float(width);
		viewport.Height = float(height);
		m_viewport = viewport;
		// create scissor rectangle
		D3D12_RECT rectScissor = {};
		rectScissor.left = 0;
		rectScissor.top = 0;
		rectScissor.right = width;
		rectScissor.bottom = height;
		m_rectScissor = rectScissor;
		//m_graphicsCmdList->RSSetViewports(1, &viewport);
		//m_graphicsCmdList->RSSetScissorRects(1, &rectScissor);
	}

	// 頂点バッファの作成。
	{
		// create geometry for a triangle

		// 右手系の CCW。
		const float outerRadius = 0.7f;
		const float posX = outerRadius * cos(DirectX::XMConvertToRadians(30));
		const float posY = outerRadius * sin(DirectX::XMConvertToRadians(30));
		const MyVertex triangleVerts[] =
		{
			{ MyVector3F{ 0.0f, +outerRadius, 0.0f }, MyVector4F{ 1.0f, 0.0f, 0.0f, 1.0f } },
			{ MyVector3F{ -posX, -posY, 0.0f }, MyVector4F{ 0.0f, 1.0f, 0.0f, 1.0f } },
			{ MyVector3F{ +posX, -posY, 0.0f }, MyVector4F{ 0.0f, 0.0f, 1.0f, 1.0f } },
		};

		// D3D12 ではバッファもテクスチャも同じインターフェイスで初期化を行なう。リソースとして本質的な違いがなくなっている。

		// CD3DX12_HEAP_PROPERTIES, CD3DX12_RESOURCE_DESC の定義されたヘッダーがまだ VS 2015 RC には付属していないようだが、
		// 設定例は CD3D12_HEAP_PROPERTIES, CD3D12_RESOURCE_DESC の暫定ドキュメントが一応参考になる。
		// https://msdn.microsoft.com/en-us/library/windows/desktop/dn859402.aspx
		// https://msdn.microsoft.com/en-us/library/windows/desktop/dn859330.aspx

		// actually create the vert buffer
		// Note: using upload heaps to transfer static data like vert buffers is not recommended.
		// Every time the GPU needs it, the upload heap will be marshalled over.  Please read up on Default Heap usage.
		// An upload heap is used here for code simplicity and because there are very few verts to actually transfer

		// D3D12_HEAP_TYPE_UPLOAD は Direct3D 11 でいうと Dynamic リソースに相当する。
		// 変化のない静的な頂点バッファの作成（生成時に初期値を流し込むだけ）には、D3D12_HEAP_TYPE_DEFAULT を使うべきらしい。
		// ちなみに Direct3D 11 までの Immutable はないらしい。
		D3D12_HEAP_PROPERTIES heapProps = {};
		heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		D3D12_RESOURCE_DESC resDesc = {};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resDesc.Width = ARRAYSIZE(triangleVerts) * sizeof(MyVertex);
		resDesc.Height = 1;
		resDesc.DepthOrArraySize = 1;
		resDesc.MipLevels = 1;
		resDesc.Format = DXGI_FORMAT_UNKNOWN;
		resDesc.SampleDesc.Count = 1;
		resDesc.SampleDesc.Quality = 0;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, // Clear value
			IID_PPV_ARGS(&m_resVertexBuffer)), _T("CreateCommittedResource"));

		// copy the triangle data to the vertex buffer

		void* dataBegin = nullptr;
		ThrowIfFailed(m_resVertexBuffer->Map(0, nullptr, &dataBegin), _T("Map vertex buffer"));
		_ASSERTE(dataBegin);
		memcpy(dataBegin, triangleVerts, sizeof(triangleVerts));
		m_resVertexBuffer->Unmap(0, nullptr);

		// create vertex buffer view

		m_vertexBufferView.BufferLocation = m_resVertexBuffer->GetGPUVirtualAddress();
		m_vertexBufferView.StrideInBytes = sizeof(MyVertex);
		m_vertexBufferView.SizeInBytes = sizeof(triangleVerts);
	}

	// 定数バッファの作成。
	{
		D3D12_HEAP_PROPERTIES heapProps = {};
		heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		D3D12_RESOURCE_DESC resDesc = {};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resDesc.Width = MyConstBufferSizeInBytes;
		resDesc.Height = 1;
		resDesc.DepthOrArraySize = 1;
		resDesc.MipLevels = 1;
		resDesc.Format = DXGI_FORMAT_UNKNOWN;
		resDesc.SampleDesc.Count = 1;
		resDesc.SampleDesc.Quality = 0;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, // Clear value
			IID_PPV_ARGS(&m_resConstBuffer)), _T("CreateCommittedResource"));

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = m_resConstBuffer->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = MyConstBufferSizeInBytes;
		const auto cbvSrvUavDescHandle = m_descHeapCbvSrvUav->GetCPUDescriptorHandleForHeapStart();
		// 複数の CBV と SRV と UAV を1つの Descriptor Heap にまとめる場合、
		// D3D12_CPU_DESCRIPTOR_HANDLE::ptr をビューごとにオフセットさせる必要があるらしい。
		//const UINT cbvSrvUavStep = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_d3dDevice->CreateConstantBufferView(&cbvDesc, cbvSrvUavDescHandle);

		ThrowIfFailed(m_resConstBuffer->Map(0, nullptr, &m_constBufferHostPtr), _T("Map constant buffer"));
	}

	// create fencing object

	ThrowIfFailed(m_d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_d3dFence)), _T("CreateFence"));
	m_currentFence = 1;

	// close the command list and use it to execute the initial GPU setup
	ThrowIfFailed(m_graphicsCmdList->Close(), _T("Close graphics command list"));
	ID3D12CommandList* ppCommandLists[] = { m_graphicsCmdList.Get(), };
	m_cmdQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// create event handle

	m_handleEvent = ::CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
	_ASSERTE(m_handleEvent);

	// wait for the command list to execute; we are reusing the same command list in our main loop but for now, 
	// we just want to wait for setup to complete before continuing

	m_isInitialized = true;

	this->WaitForGPU();
}

void MyD3DManager::WaitForGPU()
{
	if (!m_isInitialized)
	{
		return;
	}

	// signal and increment the fence value

	const UINT64 fence = m_currentFence;
	ThrowIfFailed(m_cmdQueue->Signal(m_d3dFence.Get(), fence), _T("Signal fence object"));
	m_currentFence++;

	// Let the previous frame finish before continuing

	if (m_d3dFence->GetCompletedValue() < fence)
	{
		ThrowIfFailed(m_d3dFence->SetEventOnCompletion(fence, m_handleEvent), _T("SetEventOnCompletion"));
		::WaitForSingleObject(m_handleEvent, INFINITE);
	}
}

void MyD3DManager::Render()
{
	if (!m_isInitialized)
	{
		return;
	}

	// record all the commands we need to render the scene into the command list

	this->PopulateCommandLists();

	// execute the command list

	ID3D12CommandList* ppCommandLists[] = { m_graphicsCmdList.Get(), };
	m_cmdQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// swap the back and front buffers

	ThrowIfFailed(m_dxgiSwapChain->Present(1, 0), _T("Present"));
	m_indexLastSwapBuf = (1 + m_indexLastSwapBuf) % NumSwapBufs;
	ThrowIfFailed(m_dxgiSwapChain->GetBuffer(m_indexLastSwapBuf, IID_PPV_ARGS(m_resRenderTarget.ReleaseAndGetAddressOf())), _T("Get buffer from DXGI swap chain"));
	m_d3dDevice->CreateRenderTargetView(m_resRenderTarget.Get(), nullptr, m_descHeapRtv->GetCPUDescriptorHandleForHeapStart());

	// wait and reset everything

	this->WaitForGPU();
}

void MyD3DManager::PopulateCommandLists()
{
	_ASSERTE(m_cmdAllocator);
	_ASSERTE(m_graphicsCmdList);

	// command list allocators can be only be reset when the associated command lists have finished execution on the GPU; 
	// apps should use fences to determine GPU execution progress

	ThrowIfFailed(m_cmdAllocator->Reset(), _T("Reset command allocator"));

	// HOWEVER, when ExecuteCommandList() is called on a particular command list, that command list can then be reset 
	// anytime and must be before rerecording

	ThrowIfFailed(m_graphicsCmdList->Reset(m_cmdAllocator.Get(), m_simpleGraphicsPSO1.Get()), _T("Reset graphics command list"));

	// set the graphics root signature

	m_graphicsCmdList->SetGraphicsRootSignature(m_rootSignature.Get());

	ID3D12DescriptorHeap* descHeaps[] = { m_descHeapCbvSrvUav.Get(), };
	m_graphicsCmdList->SetDescriptorHeaps(ARRAYSIZE(descHeaps), descHeaps);
	m_graphicsCmdList->SetGraphicsRootDescriptorTable(0, m_descHeapCbvSrvUav->GetGPUDescriptorHandleForHeapStart());

	// set the viewport and scissor rectangle

	m_graphicsCmdList->RSSetViewports(1, &m_viewport);
	m_graphicsCmdList->RSSetScissorRects(1, &m_rectScissor);

	// indicate this resource will be in use as a render target

	this->SetResourceBarrier(m_graphicsCmdList.Get(), m_resRenderTarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	// record commands

	if (m_constBufferHostPtr)
	{
		// Direct3D 11 時代のように、ドローコールごとに定数バッファを書き換えて描画する処理（効率は劣る）をコマンド リストにぶちこむように実装したいが、
		// Direct3D 11 で定数バッファを書き換えるのによく使っていた
		// ID3D11DeviceContext::Map() や ID3D11DeviceContext::UpdateSubresource() に直接相当するメソッドがない。
		// d3dx12.h に UpdateSubresources() がインライン実装されるらしいが、VS 2015 RC には付属していない。
		// https://msdn.microsoft.com/en-us/library/windows/desktop/dn903898%28v=vs.85%29.aspx
		// https://msdn.microsoft.com/en-us/library/windows/desktop/dn899230.aspx
		//this->SetResourceBarrier(m_graphicsCmdList.Get(), m_resRenderTarget.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		//UpdateSubresources<1>(...);
		//this->SetResourceBarrier(m_graphicsCmdList.Get(), m_resRenderTarget.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

		// 仕方ないので、とりあえずシェーダーごと分けて、さらにインスタンス描画する。
		// Direct3D 12 は非同期の描画効率を最優先する API なので、レンダリング エンジン（ユーザーコード）の設計も Direct3D 11 とは根本的に考え方を改める必要がありそう。
		auto pConstBuffer = static_cast<MyConstBuffer*>(m_constBufferHostPtr);
		DirectX::XMStoreFloat4x4(&pConstBuffer->UniWorld[0],
			DirectX::XMMatrixTranspose(DirectX::XMMatrixMultiply(DirectX::XMMatrixRotationRollPitchYaw(0, 0, +m_vRotAngle.z), DirectX::XMMatrixTranslation(+0.3f, 0, 0))));
		DirectX::XMStoreFloat4x4(&pConstBuffer->UniWorld[1],
			DirectX::XMMatrixTranspose(DirectX::XMMatrixMultiply(DirectX::XMMatrixRotationRollPitchYaw(0, 0, -m_vRotAngle.z), DirectX::XMMatrixTranslation(-0.3f, 0, 0))));

		// 縦横比の調整。
		DirectX::XMStoreFloat4x4(&pConstBuffer->UniProjection,
			DirectX::XMMatrixTranspose(DirectX::XMMatrixScaling(m_viewport.Height / m_viewport.Width, 1, 1)));
	}

	//const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	const MyVector4F clearColor(DirectX::Colors::CornflowerBlue);
	m_graphicsCmdList->ClearRenderTargetView(m_descHeapRtv->GetCPUDescriptorHandleForHeapStart(), &clearColor.x, 0, nullptr);
	m_graphicsCmdList->OMSetRenderTargets(1, &m_descHeapRtv->GetCPUDescriptorHandleForHeapStart(), true, nullptr);
	m_graphicsCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_graphicsCmdList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	// Direct3D 11 では非インスタンス描画の専用メソッドが存在したが、Direct3D 12 ではインスタンス描画のメソッドしか存在しない。
	m_graphicsCmdList->DrawInstanced(3, 1, 0, 0);

	if (m_drawsBlendingObjects)
	{
		m_graphicsCmdList->SetPipelineState(m_simpleGraphicsPSO2.Get());
		m_graphicsCmdList->DrawInstanced(3, 2, 0, 0);
	}

	// indicate that the render target will now be used to present when the command list is done executing

	this->SetResourceBarrier(m_graphicsCmdList.Get(), m_resRenderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	// all we need to do now is close the command list before executing it

	ThrowIfFailed(m_graphicsCmdList->Close(), _T("Close graphics command list"));
}

void MyD3DManager::SetResourceBarrier(ID3D12GraphicsCommandList* commandList, ID3D12Resource* resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter)
{
	_ASSERTE(commandList);
	_ASSERTE(resource);

	D3D12_RESOURCE_BARRIER descBarrier = {};

	descBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	descBarrier.Transition.pResource = resource;
	descBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	descBarrier.Transition.StateBefore = stateBefore;
	descBarrier.Transition.StateAfter = stateAfter;

	commandList->ResourceBarrier(1, &descBarrier);
}

void MyD3DManager::OnDestroy()
{
	// wait for the GPU to be done with all resources

	if (m_resConstBuffer && m_constBufferHostPtr)
	{
		m_resConstBuffer->Unmap(0, nullptr);
		m_constBufferHostPtr = nullptr;
	}

	// Direct3D 11 までと違い、Direct3D 12 ではリソースの解放時もアプリケーション コード側で明示的に GPU と同期をとってリソースを解放してやる必要がある。
	try
	{
		this->WaitForGPU();

		if (m_dxgiSwapChain)
		{
			ThrowIfFailed(m_dxgiSwapChain->SetFullscreenState(false, nullptr), _T("SetFullscreenState"));
		}
	}
	catch (const MyComException& ex)
	{
		CString strErr;
		strErr.Format(_T("hr = 0x%lx, %s"), ex.Code, ex.Message);
		ATLTRACE(_T("%s\n"), strErr);
	}

	if (m_handleEvent)
	{
		::CloseHandle(m_handleEvent);
		m_handleEvent = nullptr;
	}
}

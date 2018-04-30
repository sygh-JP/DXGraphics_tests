#pragma once

#include <afxwin.h>
#include <afxcmn.h>

#include "MyStopwatch.hpp"


// 簡単のため。

using Microsoft::WRL::ComPtr;
using namespace DirectX;


//! @brief  空のオーナードローを行なうスタティック コントロール。枠線すら描画しない。<br>
//! GDI を一切使わず、Direct3D や OpenGL のみでコントロール内部を描画する場合に使う。<br>
class CMyEmptyStatic: public CStatic
{
public:
	virtual void DrawItem(LPDRAWITEMSTRUCT /*lpDrawItemStruct*/) override
	{
	}
};


//! @brief  Xorshift による乱数を提供するクラス。<br>
//! 
//! シフト演算の結果が言語／処理系に依存しない uint で行なうのがコツ。<br>
//! HLSL にほぼコピー＆ペーストで移植できるように、互換性のある形にしておく。<br>
//! HLSL には Perlin ノイズを生成する noise() 組み込み関数が一応あるが、<br>
//! テクスチャ シェーダーおよび D3DX9 専用でしかも動的な用途ではない。<br>
//! GLSL にも規格で noise() 関数が定義されてあるが、実際に実装しているドライバーがないらしい？<br>
//! GPU で乱数をリアルタイムに動的生成する場合、なんらかのアルゴリズムを自前で実装する必要がある。<br>
class Xorshift128Random final
{
public:
	typedef uint32_t uint;
	typedef XMUINT4 uint4;

	// ちなみに OpenCL-C には C/C++ でおなじみの FLT_EPSILON や INT_MAX が事前定義されているらしい。
public:
	static const int IntMax = 0x7FFFFFFF;
	static const uint UintMax = 0xFFFFFFFFU;

	// 経験的な値。根拠はない。
	static const uint MaxVal = 10000;
public:
	static uint4 CreateNext(uint4 random)
	{
		const uint t = (random.x ^ (random.x << 11));
		random.x = random.y;
		random.y = random.z;
		random.z = random.w;
		random.w = (random.w = (random.w ^ (random.w >> 19)) ^ (t ^ (t >> 8)));
		return random;
	}

	static uint GetRandomComponentUI(uint4 random)
	{
		return random.w;
	}

	// 0.0～1.0 の範囲の実数乱数にする。
	static float GetRandomComponentUF(uint4 random)
	{
		//return float(GetRandomComponentUI(random)) / float(UintMax)); // NG.
		return float(GetRandomComponentUI(random) % MaxVal) / float(MaxVal);
	}

	// -1.0～+1.0 の範囲の実数乱数にする。
	static float GetRandomComponentSF(uint4 random)
	{
		return 2.0f * GetRandomComponentUF(random) - 1.0f;
	}

	// 1要素を符号なし 32bit 整数とした場合の Xorshift 乱数の最大値は、理論的には UINT32_MAX だが、
	// 周期が非常に長いため、逆に短い期間の乱数を [0.0, 1.0] 範囲の実数乱数に変換するときはまず比較的小さな数で剰余を求める。

	static uint4 CreateInitialNumber(uint seed)
	{
		// Xorshift の初期値は「すべてゼロ」でなければ何でも良いらしい。
		// http://d.hatena.ne.jp/jetbead/20110912/1315844091
		// http://ogawa-sankinkoutai.seesaa.net/article/108848981.html
		// http://meme.biology.tohoku.ac.jp/klabo-wiki/index.php?%B7%D7%BB%BB%B5%A1%2FC%2B%2B

		if (seed == 0)
		{
			seed += 11;
		}

		uint4 temp;

		temp.w = seed;
		temp.x = (seed << 16) + (seed >> 16);
		temp.y = temp.w + temp.x;
		temp.z = temp.x ^ temp.y;

		return temp;
	}
};


struct MyVertexPT final
{
	XMFLOAT3 Position;
	XMFLOAT2 TexCoord;
};

struct MyVertexPCT final
{
	XMFLOAT3 Position;
	XMFLOAT4 Color;
	XMFLOAT2 TexCoord;
};

//! @brief  CPU パーティクル用。<br>
struct MyVertexParticle1 final
{
	XMFLOAT3 Position;
	float Angle;
	float Size;
};

//! @brief  GPU パーティクル用。<br>
struct MyVertexParticle2 final
{
	XMFLOAT3 Position;
	XMFLOAT3 Velocity;
	float Angle;
	float DeltaAngle;
	float Size;
	//uint32_t Life;
	XMUINT4 RandomSeed;
};

// 頂点バッファの各要素に Xorshift の乱数シード（初期乱数）を仕込んでおき、
// コンピュート シェーダーで要素ごとに独立して乱数を生成し、ランダムな速度や角速度を与える。
// 寿命（年齢）を各パーティクルに持たせるなどしてもよい。
// この方法であれば、CPU の助けを借りることなく GPU 側ですべて完結できる。
// なお、GPU で頂点バッファを直接書き換える場合、RWByteAddressBuffer が必須。RWStructuredBuffer は残念ながら使えない。
// 下記の記事で言及されている、CAPCOM による手法は一見冗長に見えるが、何度か DirectX コンピュート シェーダーのプログラムを組めばその妥当性に気付く。
// http://news.mynavi.jp/articles/2010/09/06/cedec02/001.html
// 
// 今回このテスト プログラムにおいて乱数を使ってパーティクルを初期化するのは、1フレームにつき1つのパーティクルのみなので、
// パーティクルの初期化を行なっている GPU スレッドは、パーティクルの更新を行なっているほかのすべての GPU スレッドと作業負荷が異なることになる。
// こういった負荷の「むら」がパフォーマンスやスケジューラーに悪影響を及ぼすことは否定できない。
// 他に、パーティクルの初期化に関しては CPU で書き換えることのできるパーティクル1つ分の動的頂点バッファを用意しておき、
// GPU で書き換えることのできる BAB 頂点バッファの一部に対してピンポイントにコピー転送することで行ない、パーティクルの更新のみをシェーダーで担当する方法もある。
// これならば、すべての GPU スレッドの仕事量が均一になるが、CPU の仕事が増える。
// なお、下記で公開されているプログラムでは、この動的頂点バッファ＋BAB 頂点バッファ手法を用いている模様。
// https://sites.google.com/site/monshonosuana/directxno-hanashi-1/directx-108
// ちなみに上記ではテストに AMD Radeon HD 5870 を使ったらしいが、せっかくの DirectX 11 対応 GPU が泣いている。
// 比較に使ったとかいう CPU (Intel Core i5) の型番が不明だし、そもそも公開されているプログラム自体に謎のオレ様ライブラリが使われていて
// 第三者がビルド・検証することすらできない半端な状態なのではっきりとしたことは言えないが、
// おそらく GPU 版のスコアが低いとかいうのは Radeon が悪いんじゃない、テスト プログラムの設計がまずいのが原因。
// HD 5870 が DirectCompute 対応を最適化していないとは思えない。
// http://www.4gamer.net/games/085/G008506/20090922002/


// 定数バッファは更新頻度ごとに分けて作成するべきだが、簡単のため1つにまとめる。

struct MyCBufferCommon final
{
	XMFLOAT4X4 UniMatrixView;
	XMFLOAT4X4 UniMatrixProj;
	float UniNear;
	float UniFar;
	float UniGravity;
	uint32_t UniSpawnTargetParticleIndex;
	XMFLOAT3 UniMaxParticleVelocity;
	float UniMaxAngularVelocity; // [Radian]
	uint32_t UniMaxParticleCount;
	float UniParticleInitSize;
	XMFLOAT2 Dummy0;
};

static_assert(sizeof(MyCBufferCommon) % 16 == 0, "Invalid alignment!!");


// CMfcD3D11ParticleTest1Dlg ダイアログ
class CMfcD3D11ParticleTest1Dlg : public CDialogEx
{
	// コンストラクション
public:
	explicit CMfcD3D11ParticleTest1Dlg(CWnd* pParent = nullptr); // 標準コンストラクター

	// ダイアログ データ
	enum { IDD = IDD_MFCD3D11PARTICLETEST1_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override; // DDX/DDV サポート


private:
	static const uint32_t MaxParticleCount = 10000;
	static const uint32_t ParticleSpriteTexSize = 64;
	static const uint32_t ScreenSizeX = 800;
	static const uint32_t ScreenSizeY = 600;

private:
	ComPtr<ID3D11Device> m_pD3DDevice;
	ComPtr<ID3D11DeviceContext> m_pD3DDeviceContext;
	ComPtr<IDXGISwapChain> m_pSwapChain;
	ComPtr<ID3D11Texture2D> m_pBackBufferTex;
	ComPtr<ID3D11RenderTargetView> m_pBackBufferRTV;

	ComPtr<ID3D11Texture2D> m_pParticleSpriteTex;
	ComPtr<ID3D11ShaderResourceView> m_pParticleSpriteSRV;

	//ComPtr<ID3D11InputLayout> m_pVertexInputLayoutPT;
	//ComPtr<ID3D11InputLayout> m_pVertexInputLayoutPCT;

	ComPtr<ID3D11VertexShader> m_pVertexShaderParticle1;
	ComPtr<ID3D11VertexShader> m_pVertexShaderParticle2;
	ComPtr<ID3D11VertexShader> m_pVertexShaderParticle2SB;
	ComPtr<ID3D11GeometryShader> m_pGeometryShaderParticle;
	ComPtr<ID3D11PixelShader> m_pPixelShaderParticle;
	ComPtr<ID3D11ComputeShader> m_pComputeShaderParticle;

	ComPtr<ID3D11InputLayout> m_pVertexInputLayoutParticle1;
	ComPtr<ID3D11InputLayout> m_pVertexInputLayoutParticle2;
	ComPtr<ID3D11Buffer> m_pVertexBufferParticles1;
	ComPtr<ID3D11Buffer> m_pVertexBufferParticles2;
	ComPtr<ID3D11Buffer> m_pStructBufferParticles2;
	ComPtr<ID3D11UnorderedAccessView> m_pVertexBufferParticles2UAV;
	ComPtr<ID3D11UnorderedAccessView> m_pStructBufferParticles2UAV;
	ComPtr<ID3D11ShaderResourceView> m_pStructBufferParticles2SRV;
	ComPtr<ID3D11Buffer> m_pConstBufferCommon;
	ComPtr<ID3D11SamplerState> m_pSamplerLinearWrap;

	ComPtr<ID3D11DepthStencilState> m_pDepthStateNone;
	ComPtr<ID3D11BlendState> m_pBlendStateLinearAlpha;
	ComPtr<ID3D11RasterizerState> m_pRasterStateSolid;

	ComPtr<ID3D11Query> m_pQueryDisjoint;
	ComPtr<ID3D11Query> m_pQueryStart;
	ComPtr<ID3D11Query> m_pQueryEnd;

	double m_elapsedTime;

	MyCBufferCommon m_cbCommonHost;
	std::vector<MyVertexParticle2> m_vbParticleHost;

	// Windows メッセージのタイマーは精度も悪く、フレーム スキップもできないが、簡単のため。
private:
	static const UINT_PTR RenderingTimerID = 1;
	static const UINT FrameRate = 60;
	static const UINT RenderingPeriodPerFrameMs = 1000 / FrameRate;

	UINT_PTR m_renderingTimerID;

private:
	MyUtil::HRStopwatch m_stopwatch;
	uint32_t m_frameCounter;

private:
	CBitmap m_spriteTexSrcBmp;

private:
	void StartRenderingTimer()
	{
		if (!m_renderingTimerID)
		{
			m_renderingTimerID = this->SetTimer(RenderingTimerID, RenderingPeriodPerFrameMs, nullptr);
			ATLASSERT(m_renderingTimerID != 0);
		}
	}
	void StopRenderingTimer()
	{
		if (m_renderingTimerID)
		{
			ATLVERIFY(this->KillTimer(m_renderingTimerID));
			m_renderingTimerID = 0;
		}
	}

private:
	bool InitDirect3D(HWND hWnd);
	void DrawByDirect3D();

	// 実装
protected:
	HICON m_hIcon;

	// 生成された、メッセージ割り当て関数
	virtual BOOL OnInitDialog() override;
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
private:
	//CStatic m_ddxcPicture1;
	CMyEmptyStatic m_ddxcPicture1;
private:
	CStatic m_ddxcPicture2;
private:
	CStatic m_ddxcStaticFpsVal;
	CButton m_ddxcRadioCpuBased;
	CButton m_ddxcRadioGpuBased;
public:
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDestroy();
};

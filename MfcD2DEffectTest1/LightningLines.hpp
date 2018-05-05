#pragma once

// グローバル演算子オーバーロードのみで定義する方法もあるが、キャスト演算子オーバーロードなどは定義できない。
#if 0
inline D2D1_VECTOR_2F operator+(const D2D1_VECTOR_2F& a, const D2D1_VECTOR_2F& b)
{
	return D2D1::Vector2F(a.x + b.x, a.y + b.y);
}
inline D2D1_VECTOR_2F operator-(const D2D1_VECTOR_2F& a, const D2D1_VECTOR_2F& b)
{
	return D2D1::Vector2F(a.x - b.x, a.y - b.y);
}
inline D2D1_VECTOR_2F operator*(const D2D1_VECTOR_2F& a, float f)
{
	return D2D1::Vector2F(a.x * f, a.y * f);
}
inline D2D1_VECTOR_2F operator/(const D2D1_VECTOR_2F& a, float f)
{
	return D2D1::Vector2F(a.x / f, a.y / f);
}
inline D2D1_VECTOR_2F operator*(float f, const D2D1_VECTOR_2F& a)
{
	return D2D1::Vector2F(a.x * f, a.y * f);
}
inline D2D1_VECTOR_2F operator/(float f, const D2D1_VECTOR_2F& a)
{
	return D2D1::Vector2F(a.x / f, a.y / f);
}
inline D2D1_VECTOR_2F& operator*=(D2D1_VECTOR_2F& a, float f)
{
	a.x *= f;
	a.y *= f;
	return a;
}
inline D2D1_VECTOR_2F& operator/=(D2D1_VECTOR_2F& a, float f)
{
	a.x /= f;
	a.y /= f;
	return a;
}
#endif


namespace MyMath
{
	// 旧 D3DX の D3DXVECTOR2 ライクな派生クラスを定義する。
	// WPF の System.Windows.Vector に近い。

	class MyD2DVector2F : public D2D1_VECTOR_2F
	{
	public:

#pragma region // constructors
		MyD2DVector2F()
			: D2D1_VECTOR_2F()
		{}

		MyD2DVector2F(float inX, float inY)
			: D2D1_VECTOR_2F(D2D1::Vector2F(inX, inY))
		{}

		explicit MyD2DVector2F(const float inVal[])
			: D2D1_VECTOR_2F(D2D1::Vector2F(inVal[0], inVal[1]))
		{}

		explicit MyD2DVector2F(const D2D1_VECTOR_2F& inVal)
			: D2D1_VECTOR_2F(inVal)
		{}
#pragma endregion

#pragma region // casting
		operator float*()
		{
			return &this->x;
		}
		operator const float*() const
		{
			return &this->x;
		}
#pragma endregion
#pragma region // assignment operators
		MyD2DVector2F& operator+=(const MyD2DVector2F& inVal)
		{
			this->x += inVal.x;
			this->y += inVal.y;
			return *this;
		}
		MyD2DVector2F& operator-=(const MyD2DVector2F& inVal)
		{
			this->x -= inVal.x;
			this->y -= inVal.y;
			return *this;
		}
		MyD2DVector2F& operator*=(float f)
		{
			this->x *= f;
			this->y *= f;
			return *this;
		}
		MyD2DVector2F& operator/=(float f)
		{
			this->x /= f;
			this->y /= f;
			return *this;
		}
#pragma endregion
#pragma region // unary operators
		MyD2DVector2F operator+() const
		{
			return *this;
		}
		MyD2DVector2F operator-() const
		{
			return MyD2DVector2F(-this->x, -this->y);
		}
#pragma endregion
#pragma region // binary operators
		MyD2DVector2F operator+(const MyD2DVector2F& inVal) const
		{
			return MyD2DVector2F(this->x + inVal.x, this->y + inVal.y);
		}
		MyD2DVector2F operator-(const MyD2DVector2F& inVal) const
		{
			return MyD2DVector2F(this->x - inVal.x, this->y - inVal.y);
		}
		MyD2DVector2F operator*(float f) const
		{
			return MyD2DVector2F(this->x * f, this->y * f);
		}
		MyD2DVector2F operator/(float f) const
		{
			return MyD2DVector2F(this->x / f, this->y / f);
		}
#pragma endregion

		bool operator==(const MyD2DVector2F& inVal) const
		{
			return this->x == inVal.x && this->y == inVal.y;
		}
		bool operator!=(const MyD2DVector2F& inVal) const
		{
			return !(*this == inVal);
		}

		D2D1_POINT_2F ToPoint2F() const
		{
			return D2D1::Point2F(this->x, this->y);
		}

		float GetLengthSq() const
		{
			return this->x * this->x + this->y * this->y;
		}

		float GetLength() const
		{
			return sqrt(this->GetLengthSq());
		}

		float Dot(const MyD2DVector2F& inVal) const
		{
			return this->x * inVal.x + this->y * inVal.y;
		}

		void Normalize()
		{
			const float len = this->GetLength();
			if (len > FLT_EPSILON)
			{
				*this /= len;
			}
		}
	};

	inline MyD2DVector2F operator*(float f, const MyD2DVector2F& inVal)
	{
		return MyD2DVector2F(f * inVal.x, f * inVal.y);
	}
}


using MyMath::MyD2DVector2F;


// ライトニング（稲光、稲妻、雷光）をプロシージャル生成する。
class MyLightningGenerator final
{
	// 分割後の頂点数は最初の始点・終点を除くと、
	// 1 + 2 + 4 + 8 + ...
	// となり、Σ_(0, divLevel - 1) {2^n} で計算できる。

	// 稲光の形状は電磁気学的な物理ベースの解析手法のほか、フラクタルでも近似記述できるらしい。コッホ曲線を描画する要領。
	// http://www.dfx.co.jp/dftalk/?p=7690
	// 始点・終点を決めてから分割する方法のほか、
	// 始点から順番に指向性ランダムで方向を決めていく方法もあるのでは？
	// できれば頂点座標の計算はシェーダーで GPU アクセラレートしたい。
	// 枝分かれしたときはどういうふうにデータ（頂点バッファ）を格納していく？
	// データはすべて単一の頂点バッファに格納し、インデックス バッファで枝を表現する？
	// 幹の部分は線を太くするが、枝分かれした部分は線を細くする。
	// 頂点バッファは描画キックだけに使って、実際のデータは構造化バッファで管理する？

	// 理由や原理はよく知らないが、雷の稲光は赤紫に見えることが多い。

	static const int DivLevel = 4;
	static const int MaxBranchesCount = 10;

	std::mt19937 m_randGen;
	std::uniform_real_distribution<float> m_randDistF;
	std::uniform_int_distribution<uint32_t> m_randDistB;

	const MyD2DVector2F m_vAllStart;
	const MyD2DVector2F m_vAllEnd;

	std::vector<std::vector<MyD2DVector2F>> m_verticesArraysList;
	std::vector<boolean> m_branchExistenceList;
	int m_subVerticesCount;

public:
	const std::vector<std::vector<MyD2DVector2F>>& GetVerticesArraysList() const { return m_verticesArraysList; }
	const std::vector<boolean>& GetBranchExistenceList() const { return m_branchExistenceList; }

public:
	MyLightningGenerator()
		: m_randGen()
		, m_randDistF(0, 1)
		, m_randDistB(0, 1)
		, m_vAllStart(100, 20)
		, m_vAllEnd(500, 400)
		, m_verticesArraysList(MaxBranchesCount)
		, m_branchExistenceList(MaxBranchesCount)
		, m_subVerticesCount()
	{
		_ASSERTE(DivLevel > 0);
		for (int i = 0; i < DivLevel; ++i)
		{
			m_subVerticesCount += 1 << i;
		}
		_ASSERTE(m_subVerticesCount % 2 != 0); // 必ず奇数。
		int maxVerticesCountPerBranch = m_subVerticesCount + 2;
		auto& verticesArray = m_verticesArraysList[0];
		verticesArray.resize(maxVerticesCountPerBranch);
		verticesArray[0] = m_vAllStart;
		verticesArray[verticesArray.size() - 1] = m_vAllEnd;
		m_verticesArraysList[0] = verticesArray;
		m_branchExistenceList[0] = true;
		for (size_t a = 1; a < m_verticesArraysList.size(); ++a)
		{
			m_verticesArraysList[a].resize(maxVerticesCountPerBranch);
		}
		// 最終的に Direct3D プログラムへの移植を視野に入れるので、レンダリング時（フレームごと）にヒープを確保するのは NG。
		// あらかじめ余裕をもってバッファを確保しておき、その中でやりくりする。
		// TODO: Direct3D で扱いやすくするために、ジャグ配列ではなく1次元配列にする。

		this->Update();
	}

public:
	void Update()
	{
		// 頂点バッファ相当のデータ群を更新する。
		// レンダリング時のデータ更新は避け、データをもとに Draw 系メソッドをコールするだけにする。
		// TODO: 時刻の経過とともに先端に向かって漸次表示していく場合、頂点カラーのアルファ値の変更で対処する。
		size_t branchesCount = 0;
		for (size_t a = 1; a < m_branchExistenceList.size(); ++a)
		{
			m_branchExistenceList[a] = false;
		}
		for (size_t a = 0; a < m_verticesArraysList.size(); ++a)
		{
			if (m_branchExistenceList[a])
			{
				auto& verticesArray = m_verticesArraysList[a];
				size_t size = verticesArray.size();
				for (int i = 0; i < DivLevel; ++i)
				{
					size_t unit = size >> (i + 1); // size / 2^(i+1)
					int numMidPoints = 1 << i; // 2^i
					for (int j = 0; j < numMidPoints; ++j)
					{
						// P_i_j = { StartIndex, EndIndex, MidIndex }:
						// P_0_0 = { 0, Size - 1, Size / 2 },
						// P_1_0 = { 0, Size / 2, Size / 4 }, P_1_1 = { Size / 2, Size - 1, Size / 2 + Size / 4 },
						// ...
						size_t startIndex = unit * 2 * j;
						size_t endIndex = startIndex + unit * 2;
						size_t midIndex = startIndex + unit;
						const auto vStart = verticesArray[startIndex];
						const auto vEnd = verticesArray[endIndex];
						const auto randNum1 = m_randDistB(m_randGen);
						const auto randNum2 = m_randDistB(m_randGen);
						const auto sign = (randNum1 % 2 == 0) ? +1.0f : -1.0f;
						// 一定確率で分岐する。
						bool createsBranch = (a != 0 || j != 0) && (randNum1 % 2 == 0) && (randNum2 % 2 == 0);
						const auto coeff = m_randDistF(m_randGen);
						const auto vMid = CalcRandomMidPoint(vStart, vEnd, sign, coeff);
						verticesArray[midIndex] = vMid;
						if (createsBranch && branchesCount + 1 < m_verticesArraysList.size())
						{
							++branchesCount;
							m_branchExistenceList[branchesCount] = true;
							const auto vLast = verticesArray[verticesArray.size() - 1];
							const auto vDiff = vLast - vStart;
							auto vMid2 = CalcRandomMidPoint(vStart, vEnd, -sign, coeff);
							vMid2.Normalize();
							auto& nextArray = m_verticesArraysList[branchesCount];
							// まず両端のみを決めておく。
							nextArray[0] = vStart;
							nextArray[nextArray.size() - 1] = vStart + vMid2 * vDiff.GetLength();
						}
					}
				}
			}
		}
	}

private:
	static MyD2DVector2F CalcRandomMidPoint(MyD2DVector2F vStart, MyD2DVector2F vEnd, float sign, float coeff)
	{
		// TODO: 3次元への拡張。
		const auto vDiff = vEnd - vStart;
		const auto vOrtho = MyD2DVector2F(+vDiff.y, -vDiff.x);
		const auto vMid = (vStart + vEnd) * 0.5f;
		const auto vMidRandom = vMid + sign * 0.1f * (1.0f + 0.5f * coeff) * vOrtho;
		return vMidRandom;
	}
};


#include "MyVertex.hlsli"
#include "MyConstBuffer.hlsli"

#if 1
float4 main(MyVSOut psIn) : SV_Target
{
	return psIn.Color;
}
#else
float4 main() : SV_Target
{
	return float4(0.0f, 0.5f, 0.5f, 1.0f);
}
#endif

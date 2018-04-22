#include "MyVertex.hlsli"
#include "MyConstBuffer.hlsli"

#if 1
MyVSOut main(float3 pos : POSITION, float4 color : COLOR)
{
	MyVSOut outVal = (MyVSOut)0;
	outVal.Position = float4(pos, 1);
	outVal.Position = mul(outVal.Position, MyConstBuffer_UniProjection);
	outVal.Color = color;
	return outVal;
}
#else
float4 main(float4 pos : POSITION) : SV_Position
{
	return pos;
}
#endif

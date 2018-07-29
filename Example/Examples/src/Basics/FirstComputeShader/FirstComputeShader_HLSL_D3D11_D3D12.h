/*********************************************************\
 * Copyright (c) 2012-2018 The Unrimp Team
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
\*********************************************************/


//[-------------------------------------------------------]
//[ Shader start                                          ]
//[-------------------------------------------------------]
#if defined(RENDERER_DIRECT3D10) || defined(RENDERER_DIRECT3D11) || defined(RENDERER_DIRECT3D12)
if (renderer->getNameId() == Renderer::NameId::DIRECT3D10 || renderer->getNameId() == Renderer::NameId::DIRECT3D11 || renderer->getNameId() == Renderer::NameId::DIRECT3D12)
{


//[-------------------------------------------------------]
//[ Vertex shader source code                             ]
//[-------------------------------------------------------]
// One vertex shader invocation per vertex
vertexShaderSourceCode = R"(
// Attribute input/output
struct VS_OUTPUT
{
	float4 Position : SV_POSITION;	// Clip space vertex position as output, left/bottom is (-1,-1) and right/top is (1,1)
	float2 TexCoord : TEXCOORD0;	// Normalized texture coordinate as output
};

// Programs
VS_OUTPUT main(float2 Position : POSITION)	// Clip space vertex position as input, left/bottom is (-1,-1) and right/top is (1,1)
{
	// Pass through the clip space vertex position, left/bottom is (-1,-1) and right/top is (1,1)
	VS_OUTPUT output;
	output.Position = float4(Position, 0.5f, 1.0f);
	output.TexCoord = Position.xy;
	return output;
}
)";


//[-------------------------------------------------------]
//[ Fragment shader source code                           ]
//[-------------------------------------------------------]
// One fragment shader invocation per fragment
// "pixel shader" in Direct3D terminology
fragmentShaderSourceCode = R"(
// Uniforms
SamplerState SamplerLinear : register(s0);
Texture2D AlbedoMap : register(t0);

// Programs
float4 main(float4 Position : SV_POSITION, float2 TexCoord : TEXCOORD0) : SV_TARGET
{
	// Fetch the texel at the given texture coordinate and return its color
	return AlbedoMap.Sample(SamplerLinear, TexCoord);
}
)";


//[-------------------------------------------------------]
//[ Compute shader source code                            ]
//[-------------------------------------------------------]
computeShaderSourceCode = R"(
// Input
Texture2D<float4>	InputTexture2D		 : register(t0);
tbuffer				InputIndexBuffer	 : register(t1)
{
	uint inputIndexBuffer[3];
};
ByteAddressBuffer	InputVertexBuffer	 : register(t2);
tbuffer				InputIndirectBuffer  : register(t3)
{
	uint inputIndirectBuffer[5];
};
cbuffer				InputUniformBuffer	 : register(b0)
{
	float4 inputColor;
}

// Output
RWTexture2D<float4> OutputTexture2D		 : register(u0);
RWBuffer<uint>		OutputIndexBuffer    : register(u1);
RWByteAddressBuffer OutputVertexBuffer   : register(u2);
RWBuffer<uint>		OutputIndirectBuffer : register(u3);

// Programs
[numthreads(16, 16, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	// Fetch input texel
	float4 color = InputTexture2D.Load(dispatchThreadId) * inputColor;

	// Modify color
	color.g *= 1.0f - (float(dispatchThreadId.x) / 16.0f);
	color.g *= 1.0f - (float(dispatchThreadId.y) / 16.0f);

	// Output texel
	OutputTexture2D[dispatchThreadId.xy] = color;

	// Output buffer
	if (0 == dispatchThreadId.x && 0 == dispatchThreadId.y && 0 == dispatchThreadId.z)
	{
		// Output indices
		for (int indexIndex = 0; indexIndex < 3; ++indexIndex)
		{
			OutputIndexBuffer[indexIndex] = inputIndexBuffer[indexIndex];
		}

		// Output vertices
		// -> Using a structured vertex buffer would be handy inside shader source codes, sadly this isn't possible with Direct3D 11 and will result in the following error:
		//    D3D11 ERROR: ID3D11Device::CreateBuffer: Buffers created with D3D11_RESOURCE_MISC_BUFFER_STRUCTURED cannot specify any of the following listed bind flags.  The following BindFlags bits (0x9) are set: D3D11_BIND_VERTEX_BUFFER (1), D3D11_BIND_INDEX_BUFFER (0), D3D11_BIND_CONSTANT_BUFFER (0), D3D11_BIND_STREAM_OUTPUT (0), D3D11_BIND_RENDER_TARGET (0), or D3D11_BIND_DEPTH_STENCIL (0). [ STATE_CREATION ERROR #68: CREATEBUFFER_INVALIDMISCFLAGS]
		for (int vertexIndex = 0; vertexIndex < 3; ++vertexIndex)
		{
			float2 position = asfloat(InputVertexBuffer.Load2(vertexIndex * 8));
			OutputVertexBuffer.Store2(vertexIndex * 8, asuint(position));
		}

		// Output draw call
		// -> Using a structured indirect buffer would be handy inside shader source codes, sadly this isn't possible with Direct3D 11 and will result in the following error:
		//    "D3D11 ERROR: ID3D11Device::CreateBuffer: A resource cannot created with both D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS and D3D11_RESOURCE_MISC_BUFFER_STRUCTURED. [ STATE_CREATION ERROR #68: CREATEBUFFER_INVALIDMISCFLAGS]"
		OutputIndirectBuffer[0] = inputIndirectBuffer[0];	// Renderer::DrawIndexedInstancedArguments::indexCountPerInstance
		OutputIndirectBuffer[1] = inputIndirectBuffer[1];	// Renderer::DrawIndexedInstancedArguments::instanceCount
		OutputIndirectBuffer[2] = inputIndirectBuffer[2];	// Renderer::DrawIndexedInstancedArguments::startIndexLocation
		OutputIndirectBuffer[3] = inputIndirectBuffer[3];	// Renderer::DrawIndexedInstancedArguments::baseVertexLocation
		OutputIndirectBuffer[4] = inputIndirectBuffer[4];	// Renderer::DrawIndexedInstancedArguments::startInstanceLocation

		// Output uniform not possible by design
	}
}
)";


//[-------------------------------------------------------]
//[ Shader end                                            ]
//[-------------------------------------------------------]
}
else
#endif
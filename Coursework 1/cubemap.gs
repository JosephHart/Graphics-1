//geometry shader for cube mapping


struct PS_CUBEMAP_IN
{
	float4 Pos : SV_POSITION; // Projection coord
	float2 Tex : TEXCOORD0; // Texture coord
	uint RTIndex : SV_RenderTargetArrayIndex;
};

[maxvertexcount(18)]

void GS_CubeMap(triangle GS_CUBEMAP_IN input[3], inout TriangleStream<PS_CUBEMAP_IN> CubeMapStream)
{
	// For each triangle
	for(int f = 0; f < 6; ++f)
	{
		// Compute screen coordinates
		PS_CUBEMAP_IN output;
		// Assign the ith triangle to the ith render target.
		output.RTIndex = f;
		// For each vertex in the triangle
		for(int v = 0; v < 3; v++)
		{
			// Transform to the view space of the ith cube face.
			output.Pos = mul(input[v].Pos, g_mViewCM[f]);
			// Transform to homogeneous clip space.
			output.Pos = mul(output.Pos, mProj);
			output.Tex = input[v].Tex;
			CubeMapStream.Append(output);
		}
		CubeMapStream.RestartStrip();
	}
}
//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "ERenderer.h"
#include "ERGBColor.h"
#include <iostream>
#include "Parser.h"

Elite::Renderer::Renderer(SDL_Window * pWindow)
	:m_TextureDiffuse{"../Resources/vehicle_diffuse.png"}
	,m_TextureNormal{"../Resources/vehicle_normal.png"}
	,m_TextureGlossiness{"../Resources/vehicle_gloss.png"}
	,m_TexureSpecularMap{"../Resources/vehicle_specular.png"}
{
	//Initialize
	m_pWindow = pWindow;
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	int width, height = 0;
	SDL_GetWindowSize(pWindow, &width, &height);
	m_Width = static_cast<uint32_t>(width);
	m_Height = static_cast<uint32_t>(height);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	m_PreviousRotation = MakeRotationY((0.f));

	//Get triangles
	m_Topology = PrimitiveTopology::TriangleList;

	if (m_Topology == PrimitiveTopology::TriangleList)
	{
		Parser parser;

		std::vector<std::vector<IPoint3>> indexBuffer;

		parser.ParseObject(m_VertixBuffer, indexBuffer, "../Resources/vehicle.obj");

		for (int i{}; i < indexBuffer.size(); i++)
		{
			m_Triangles.push_back(Triangle{ ColorVertix{m_VertixBuffer[indexBuffer[i][0].x - 1].pos,m_VertixBuffer[indexBuffer[i][0].x - 1].color, m_VertixBuffer[indexBuffer[i][1].x - 1].uv, m_VertixBuffer[indexBuffer[i][2].x - 1].normal},
				ColorVertix{m_VertixBuffer[indexBuffer[i][0].y - 1].pos,m_VertixBuffer[indexBuffer[i][0].y - 1].color, m_VertixBuffer[indexBuffer[i][1].y - 1].uv, m_VertixBuffer[indexBuffer[i][2].y - 1].normal},
				ColorVertix{m_VertixBuffer[indexBuffer[i][0].z - 1].pos,m_VertixBuffer[indexBuffer[i][0].z - 1].color, m_VertixBuffer[indexBuffer[i][1].z - 1].uv, m_VertixBuffer[indexBuffer[i][2].z - 1].normal} });
		}
	}
	else if (m_Topology == PrimitiveTopology::TriangleStrip)
	{
		for (int i{}; i < m_IndexBuffer.size() - 2; i++)
		{
			if (i % 2 == 0)
			{
				m_Triangles.push_back(Triangle{ m_VertixBuffer[m_IndexBuffer[i]], m_VertixBuffer[m_IndexBuffer[i + 1]], m_VertixBuffer[m_IndexBuffer[i + 2]] });
			}
			else
			{
				m_Triangles.push_back(Triangle{ m_VertixBuffer[m_IndexBuffer[i]], m_VertixBuffer[m_IndexBuffer[i + 2]], m_VertixBuffer[m_IndexBuffer[i + 1]] });
			}
		}
	}

	//Calculate Tangents
	for (uint32_t i = 0; i < m_Triangles.size(); i++)
	{
		const FPoint3& p0 = m_Triangles[i].v0.pos.xyz;
		const FPoint3& p1 = m_Triangles[i].v1.pos.xyz;
		const FPoint3& p2 = m_Triangles[i].v2.pos.xyz;
		const FVector2& uv0 = m_Triangles[i].v0.uv;
		const FVector2& uv1 = m_Triangles[i].v1.uv;
		const FVector2& uv2 = m_Triangles[i].v2.uv;

		const FVector3& edge0 = p1 - p0;
		const FVector3& edge1 = p2 - p0;
		const FVector2& diffX = FVector2(uv1.x - uv0.x, uv2.x - uv0.x);
		const FVector2& diffY = FVector2(uv1.y - uv0.y, uv2.y - uv0.y);

		float r = 1.f / Cross(diffX, diffY);

		FVector3 tangent = (edge0 * diffY.y - edge1 * diffY.x) * r;
		m_Triangles[i].v0.tangent += tangent;
		m_Triangles[i].v1.tangent += tangent;
		m_Triangles[i].v2.tangent += tangent;
	}

	for (Triangle& t : m_Triangles)
	{
		t.v0.tangent = GetNormalized(Reject(t.v0.tangent, t.v0.normal));
		t.v1.tangent = GetNormalized(Reject(t.v1.tangent, t.v1.normal));
		t.v2.tangent = GetNormalized(Reject(t.v2.tangent, t.v2.normal));
	}

	m_TransformedTriangles.resize(m_Triangles.size());

	m_AspectRatio = float(m_Width) / m_Height;

	m_DepthBuffer.resize(m_Height * m_Width);
	std::fill(m_DepthBuffer.begin(), m_DepthBuffer.end(), FLT_MAX);
}

void Elite::Renderer::SetCamera(Camera* pCamera)
{
	m_pCamera = pCamera;

	//Camera specifications
	m_Fov = pCamera->GetFov();
	float near = pCamera->GetNear();
	float far = pCamera->GetFar();

	m_ProjectionMatrix[0] = { 1 / (m_AspectRatio * m_Fov), 0 , 0, 0 };
	m_ProjectionMatrix[1] = { 0, 1 / m_Fov , 0, 0 };
	m_ProjectionMatrix[2] = { 0, 0 ,  far / (near - far), (far * near) / (near - far) };
	m_ProjectionMatrix[3] = { 0, 0, -1, 0 };
}

void Elite::Renderer::Render()
{
	SDL_LockSurface(m_pBackBuffer);
	SDL_FillRect(m_pBackBuffer, NULL, 0x1E1E1E);
	
	//Projection stage
	for (int i{}; i < m_Triangles.size(); i++)
	{
		auto transformedTriangle = GetScreenSpace({ m_Triangles[i].v0, m_Triangles[i].v1, m_Triangles[i].v2 });
		m_TransformedTriangles[i] = Triangle{ transformedTriangle[0], transformedTriangle[1], transformedTriangle[2] };
	}

	for (const Triangle& t : m_TransformedTriangles)
	{
		std::vector<Elite::ColorVertix> NDCVertices = { t.v0, t.v1, t.v2 };

		//Frustum culling
		bool culling = false;
		for (int i{}; i < NDCVertices.size(); i++)
		{
			if (NDCVertices[i].pos.x < -1.0f || NDCVertices[i].pos.x > 1.0f)
			{
				culling = true;
			}
			if (NDCVertices[i].pos.y < -1.0f || NDCVertices[i].pos.y > 1.0f)
			{
				culling = true;
			}
			if (NDCVertices[i].pos.z < 0.0f || NDCVertices[i].pos.z > 1.0f)
			{
				culling = true;
			}

			//Rasterization stage
			NDCVertices[i].pos.x = ((NDCVertices[i].pos.x + 1) / 2.0f) * m_Width;
			NDCVertices[i].pos.y = ((1 - NDCVertices[i].pos.y) / 2.0f) * m_Height;
		}


		if (!culling)
		{
			//Bounding Box
			Elite::FPoint2 topLeft = Elite::FPoint2{ std::min(std::min(NDCVertices[0].pos.x, NDCVertices[1].pos.x), NDCVertices[2].pos.x),
				std::min(std::min(NDCVertices[0].pos.y, NDCVertices[1].pos.y), NDCVertices[2].pos.y) };

			Elite::FPoint2 bottomRight = Elite::FPoint2{ std::max(std::max(NDCVertices[0].pos.x, NDCVertices[1].pos.x), NDCVertices[2].pos.x),
				std::max(std::max(NDCVertices[0].pos.y, NDCVertices[1].pos.y), NDCVertices[2].pos.y) };

			topLeft.x = Elite::Clamp(topLeft.x, 0.f, float(m_Width));
			topLeft.y = Elite::Clamp(topLeft.y, 0.f, float(m_Height));
			bottomRight.x = Elite::Clamp(bottomRight.x, 0.f, float(m_Width));
			bottomRight.y = Elite::Clamp(bottomRight.y, 0.f, float(m_Height));

			for (uint32_t r = uint32_t(topLeft.y); r < bottomRight.y; ++r)
			{
				for (uint32_t c = uint32_t(topLeft.x); c < bottomRight.x; ++c)
				{
					Elite::ColorVertix pixel{};
					pixel.pos = { float(c), float(r), 0, 0 };

					if (IsInTriangle(pixel, NDCVertices))
					{
						if (pixel.pos.z < m_DepthBuffer[c + (r * m_Width)])
						{
							m_DepthBuffer[c + (r * m_Width)] = pixel.pos.z;
							Elite::RGBColor finalColor{};
							finalColor += PixelShading(pixel);

							finalColor.MaxToOne();
							m_pBackBufferPixels[c + (r * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
								static_cast<uint8_t>(uint8_t(finalColor.r * 255)),
								static_cast<uint8_t>(uint8_t(finalColor.g * 255)),
								static_cast<uint8_t>(uint8_t(finalColor.b * 255)));
						}

					}
				}
			}
		}

	}

	std::fill(m_DepthBuffer.begin(), m_DepthBuffer.end(), FLT_MAX);

	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Elite::Renderer::Update(float dt)
{
	m_Timer += dt;
}

bool Elite::Renderer::SaveBackbufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "BackbufferRender.bmp");
}

std::vector<Elite::ColorVertix> Elite::Renderer::GetScreenSpace(const std::vector<Elite::ColorVertix>& originalVertices)
{
	FMatrix4 ONB = (m_pCamera->GetWorldToView());

	FMatrix4 rotation{};

	if (m_Rotating)
	{
		rotation = MakeRotationY((m_Timer));
		m_PreviousRotation = rotation;
	}
	else
	{
		rotation = m_PreviousRotation;
	}
	FMatrix4 worldMatrix{};
	worldMatrix[0] = { 1.f, 0.f,0.f,0.f };
	worldMatrix[1] = { 0.f, 1.f,0.f,0.f };
	worldMatrix[2] = { 0.f, 0.f, 1.f,0.f };
	worldMatrix[3] = { 0.f, 0.f,0.f,1.f };

	worldMatrix *= rotation;

	FMatrix4 worldViewProjectionMatrix = m_ProjectionMatrix * (ONB) * worldMatrix;
	std::vector<Elite::ColorVertix> transformedVertices{};

	for (int i{}; i < originalVertices.size(); i++)
	{
		//worldViewProjectionMatrix
		transformedVertices.push_back({ worldViewProjectionMatrix * originalVertices[i].pos, originalVertices[i].color, originalVertices[i].uv,
			(worldMatrix * Elite::FVector4{originalVertices[i].normal.x, originalVertices[i].normal.y, originalVertices[i].normal.z, 1.f}).xyz,
			(worldMatrix * Elite::FVector4{originalVertices[i].tangent.x, originalVertices[i].tangent.y, originalVertices[i].tangent.z, 1.f}).xyz,
			Elite::FVector3((worldMatrix * originalVertices[i].pos - ONB[3]).xyz)});

		//Make the mesh visible
		transformedVertices[i].pos.w *= 10;

		//Perspective divide
		transformedVertices[i].pos.x = transformedVertices[i].pos.x / transformedVertices[i].pos.w;
		transformedVertices[i].pos.y = transformedVertices[i].pos.y / transformedVertices[i].pos.w;
		transformedVertices[i].pos.z = transformedVertices[i].pos.z / transformedVertices[i].pos.w;

		transformedVertices[i].viewDirection = GetNormalized(transformedVertices[i].viewDirection);
	}

	return transformedVertices;
}

bool Elite::Renderer::IsInTriangle(Elite::ColorVertix& pointToHit, const std::vector<Elite::ColorVertix>& ndcPoints) const
{
	Elite::FVector2 pointToSideA = FVector2((pointToHit.pos - ndcPoints[0].pos));
	Elite::FVector2 pointToSideB = FVector2((pointToHit.pos - ndcPoints[1].pos));
	Elite::FVector2 pointToSideC = FVector2((pointToHit.pos - ndcPoints[2].pos));

	const Elite::FVector2 edgeA{ ndcPoints[1].pos - ndcPoints[0].pos };
	const Elite::FVector2 edgeB{ ndcPoints[2].pos - ndcPoints[1].pos };
	const Elite::FVector2 edgeC{ ndcPoints[0].pos - ndcPoints[2].pos };

	float W0 = Elite::Cross(pointToSideB, edgeB);
	float W1 = Elite::Cross(pointToSideC, edgeC);
	float W2 = Elite::Cross(pointToSideA, edgeA);


	if (W2 < 0 || W1 < 0 || W0 < 0)
	{
		return false;
	}

	//Interpolate between vertex values
	W0 = abs(W0 / Elite::Cross(Elite::FVector2(ndcPoints[0].pos - ndcPoints[1].pos), edgeC));
	W1 = abs(W1 / Elite::Cross(Elite::FVector2(ndcPoints[0].pos - ndcPoints[1].pos), edgeC));
	W2 = abs(W2 / Elite::Cross(Elite::FVector2(ndcPoints[0].pos - ndcPoints[1].pos), edgeC));

	pointToHit.color = ndcPoints[0].color * W0 + ndcPoints[1].color * W1 + ndcPoints[2].color * W2;

	auto interpolatedZ = (1 / (((1 / (ndcPoints[0].pos.z)) * W0) + ((1 / (ndcPoints[1].pos.z)) * W1) + ((1 / (ndcPoints[2].pos.z)) * W2)));
	pointToHit.pos.z = interpolatedZ;

	auto interpolatedW = (1 / (((1 / (ndcPoints[0].pos.w)) * W0) + ((1 / (ndcPoints[1].pos.w)) * W1) + ((1 / (ndcPoints[2].pos.w)) * W2)));

	pointToHit.uv = (((ndcPoints[0].uv / ndcPoints[0].pos.w) * W0) + ((ndcPoints[1].uv / ndcPoints[1].pos.w) * W1)
		+ ((ndcPoints[2].uv / ndcPoints[2].pos.w) * W2)) * interpolatedW;

	pointToHit.normal = ndcPoints[0].normal * W0 + ndcPoints[1].normal * W1 + ndcPoints[2].normal * W2;
	pointToHit.normal = GetNormalized(pointToHit.normal);

	pointToHit.tangent = ndcPoints[0].tangent * W0 + ndcPoints[1].tangent * W1 + ndcPoints[2].tangent * W2;

	pointToHit.viewDirection = ndcPoints[0].viewDirection * W0 + ndcPoints[1].viewDirection * W1 + ndcPoints[2].viewDirection * W2;
	pointToHit.viewDirection = GetNormalized(pointToHit.viewDirection);
	return true;
}

Elite::RGBColor Elite::Renderer::PixelShading(const Elite::ColorVertix& v) const
{
	//Direction light
	FVector3 lightDir = { 0.577f, -0.577f, -0.577f };
	Elite::RGBColor lightColor = { 1.f,1.f,1.f };
	float intensity = 2.f;

	float observedArea{};

	if (m_UsingNormalMap)
	{
		//Calculate normals in tangent space
		Elite::RGBColor normalRgb = m_TextureNormal.Sample(v.uv);
		Elite::FVector3 normal{ normalRgb.r, normalRgb.g, normalRgb.b };
		Elite::FVector3 binormal = Cross(v.tangent, v.normal);
		Elite::FMatrix3 tangentSpaceAxis = FMatrix3(v.tangent, binormal, v.normal);

		normal.x = 2.f * normal.x - 1.f;
		normal.y = 2.f * normal.y - 1.f;
		normal.z = 2.f * normal.z - 1.f;

		normal = tangentSpaceAxis * normal;
		normal = GetNormalized(normal);

		observedArea = Elite::Dot(normal, -lightDir);
	}
	else
	{
		observedArea = Elite::Dot(v.normal, -lightDir);
	}

	observedArea = Clamp(observedArea, 0.f, 1.f);


	if (m_UsingDepthBufferColor)
	{
		Elite::RGBColor depthBufferColor = { Clamp(Remap(v.pos.z, 0.9f,1.f), 0.f,1.f),Clamp(Remap(v.pos.z, 0.9f,1.f), 0.f,1.f),Clamp(Remap(v.pos.z, 0.9f,1.f), 0.f,1.f) };

		return lightColor * intensity * depthBufferColor * observedArea;
	}
	else
	{
		float shininess = 25.f;
		auto phongColor = BRDF::Phong(m_TexureSpecularMap.Sample(v.uv), shininess * m_TextureGlossiness.Sample(v.uv).r, lightDir, v.viewDirection, v.normal);
		auto diffuseColor = m_TextureDiffuse.Sample(v.uv);
		auto ambientColor = Elite::RGBColor{ 0.05f, 0.05f, 0.05f };

		diffuseColor += phongColor + ambientColor;
		diffuseColor.MaxToOne();

		auto finalColor = lightColor * intensity * diffuseColor * observedArea;
		return finalColor;
	}
}

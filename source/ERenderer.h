/*=============================================================================*/
// Copyright 2017-2019 Elite Engine
// Authors: Matthieu Delaere
/*=============================================================================*/
// ERenderer.h: class that holds the surface to render to, does traverse the pixels 
// and traces the rays using a tracer
/*=============================================================================*/
#ifndef ELITE_RAYTRACING_RENDERER
#define	ELITE_RAYTRACING_RENDERER

#include <cstdint>
#include "EMath.h"
#include "ETriangle.h"
#include "ECamera.h"
#include "EColorVertix.h"
#include "Texture.h"
#include <vector>
#include "EBRDF.h"

struct SDL_Window;
struct SDL_Surface;

namespace Elite
{
	enum class PrimitiveTopology
	{
		TriangleList,
		TriangleStrip
	};

	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer() = default;

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void SetCamera(Camera* pCamera);

		void Render();
		void Update(float dt);
		bool SaveBackbufferToImage() const;
		
		void SetUsingDepthBuffer() { m_UsingDepthBufferColor = !m_UsingDepthBufferColor; }
		void SetRotation() { m_Rotating = !m_Rotating; }
		void SetNormalMapping() { m_UsingNormalMap = !m_UsingNormalMap; }

	private:
		SDL_Window* m_pWindow = nullptr;
		SDL_Surface* m_pFrontBuffer = nullptr;
		SDL_Surface* m_pBackBuffer = nullptr;
		uint32_t* m_pBackBufferPixels = nullptr;
		uint32_t m_Width = 0;
		uint32_t m_Height = 0;

		PrimitiveTopology m_Topology{};

		std::vector<Triangle> m_Triangles;
		std::vector<Triangle> m_TransformedTriangles;

		std::vector<ColorVertix> m_VertixBuffer;
		std::vector<uint32_t> m_IndexBuffer;
		std::vector<Elite::IPoint3> m_IndexBufferList;

		Camera* m_pCamera{};

		std::vector<float> m_DepthBuffer{};
		Texture m_TextureDiffuse;
		Texture m_TextureNormal;
		Texture m_TexureSpecularMap;
		Texture m_TextureGlossiness;

		FMatrix4 m_ProjectionMatrix{};

		FMatrix4 m_PreviousRotation{};

		float m_Timer{};

		bool m_UsingDepthBufferColor{false};
		bool m_Rotating{false};
		bool m_UsingNormalMap{ false };

		float m_Fov{};
		float m_AspectRatio{};

		std::vector<Elite::ColorVertix> GetScreenSpace(const std::vector<Elite::ColorVertix>& originalVertices);

		Elite::RGBColor PixelShading(const Elite::ColorVertix& v) const;
		
		bool IsInTriangle(Elite::ColorVertix& pointToHit, const std::vector<Elite::ColorVertix>& ndcPoints) const;
	};
}

#endif
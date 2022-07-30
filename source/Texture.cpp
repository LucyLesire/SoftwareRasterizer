#include "Texture.h"
#include <iostream>


Elite::Texture::Texture(const char* filePath)
{
	m_pTexture = IMG_Load(filePath);
}

Elite::Texture::~Texture()
{
	SDL_FreeSurface(m_pTexture);
}

Elite::RGBColor Elite::Texture::Sample(const Elite::FVector2& uv) const
{
	Elite::FVector2 convertedUv{};
	convertedUv.x = uv.x * m_pTexture->w;
	convertedUv.y = (uv.y) * m_pTexture->h;
	Elite::RGBColor finalColor;

	Uint8 r{};
	Uint8 g{};
	Uint8 b{};

	Uint32 pixelCoord = Uint32(roundf(convertedUv.x)) + Uint32((roundf(convertedUv.y) * m_pTexture->w));
	pixelCoord = Clamp(pixelCoord, Uint32(0), Uint32(m_pTexture->w * m_pTexture->h));

	SDL_GetRGB(static_cast<Uint32*>(m_pTexture->pixels)[pixelCoord], m_pTexture->format, &r, &g, &b);

	finalColor.r = static_cast<float>(float(r)) / 255.f;
	finalColor.g = static_cast<float>(float(g)) / 255.f;
	finalColor.b = static_cast<float>(float(b)) / 255.f;

	return finalColor;
}

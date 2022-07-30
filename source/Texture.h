#pragma once
#include<string>
#include "EMath.h"
#include <SDL_image.h>
#include "ERGBColor.h"
#include "EColorVertix.h"

namespace Elite
{
	class Texture
	{
	public:
		Texture(const char* filePath);
		~Texture();

		Elite::RGBColor Sample(const Elite::FVector2& uv) const;

	private:
		SDL_Surface* m_pTexture;
	};
}



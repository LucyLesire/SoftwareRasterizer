#pragma once
#include "EMath.h"
#include "ERGBColor.h"

namespace Elite
{
	struct ColorVertix
	{
		Elite::FPoint4 pos;
		Elite::RGBColor color;
		Elite::FVector2 uv;
		Elite::FVector3 normal;
		Elite::FVector3 tangent;
		Elite::FVector3 viewDirection;
	};
}
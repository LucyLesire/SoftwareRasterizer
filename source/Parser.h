#pragma once
#include "EMath.h"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <cassert>
#include <vector>
#include <sstream>
#include "EColorVertix.h"

namespace Elite
{
	class Parser
	{
	public:
		Parser();
		~Parser();

		void ParseObject(std::vector<ColorVertix>& vertexBuffer, std::vector<std::vector<IPoint3>>& indexBuffer, const std::string& fileName) const;
	};
}

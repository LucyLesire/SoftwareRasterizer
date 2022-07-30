#include "Parser.h"
#include "EColorVertix.h"

Elite::Parser::Parser()
{
}

Elite::Parser::~Parser()
{
}

void Elite::Parser::ParseObject(std::vector<ColorVertix>& vertexBuffer, std::vector<std::vector<IPoint3>>& indexBuffer, const std::string& fileName) const
{
	std::string line;
	std::ifstream input{};

	std::vector<FPoint4> vertices;
	std::vector<FVector3> normals;
	std::vector<FVector2> uvs;
	input.open(fileName, std::ios::in | std::ios::binary);
	if (input.is_open())
	{
		while (!input.eof())
		{
			std::getline(input, line);
			if (line[0] == 'v')
			{
				if (line[1] == 'n')
				{
					std::stringstream ss(line);
					std::string v, x, y, z;
					std::getline(ss, v, ' ');
					std::getline(ss, x, ' ');
					std::getline(ss, y, ' ');
					std::getline(ss, z, '\r');
					FVector3 normal;
					normal.x = std::stof(x);
					normal.y = std::stof(y);
					normal.z = std::stof(z);
					normals.push_back(normal);
				}
				else if (line[1] == 't')
				{
					std::stringstream ss(line);
					std::string v, x, y, z;
					std::getline(ss, v, ' ');
					std::getline(ss, x, ' ');
					std::getline(ss, y, ' ');
					std::getline(ss, z, '\r');
					FVector2 uv;
					uv.x = std::stof(x);
					uv.y = std::stof(y);
					uvs.push_back(uv);
				}
				else
				{
					std::stringstream ss(line);
					std::string v, x, y, z;
					std::getline(ss, v, ' ');
					std::getline(ss, v, ' ');
					std::getline(ss, x, ' ');
					std::getline(ss, y, ' ');
					std::getline(ss, z, '\r');
					FPoint4 vertex;
					vertex.x = std::stof(x);
					vertex.y = std::stof(y);
					vertex.z = std::stof(z);
					vertices.push_back(vertex);
				}
			}
			else if (line[0] == 'f')
			{
				std::stringstream ss(line);
				std::string f, x, y, z;
				std::getline(ss, f, ' ');
				std::getline(ss, x, '/');
				std::getline(ss, y, '/');
				std::getline(ss, z, ' ');
				IPoint3 triangle;
				IPoint3 uv;
				IPoint3 normal;
				triangle.x = std::stoi(x);
				uv.x = std::stoi(y);
				normal.x = std::stoi(z);

				std::getline(ss, x, '/');
				std::getline(ss, y, '/');
				std::getline(ss, z, ' ');
				triangle.y = std::stoi(x);
				uv.y = std::stoi(y);
				normal.y = std::stoi(z);

				std::getline(ss, x, '/');
				std::getline(ss, y, '/');
				std::getline(ss, z, '\r');
				triangle.z = std::stoi(x);
				uv.z = std::stoi(y);
				normal.z = std::stoi(z);

				indexBuffer.push_back({ triangle, uv, normal });
			}
		}

		size_t j = 0;
		size_t k = 0;
		size_t l = 0;

		size_t size = vertices.size();
		if (uvs.size() > size)
		{
			size = uvs.size();
		}
		if (normals.size() > size)
		{
			size = normals.size();
		}

		for (int i{}; i < size; i++)
		{
			vertexBuffer.push_back(Elite::ColorVertix{ vertices[j], RGBColor{ 0.f,0.f,0.f }, Elite::FVector2{uvs[k].x, 1 - uvs[k].y}, normals[l] });
			if (j < vertices.size() - 1)
				j++;
			if (k < uvs.size() - 1)
				k++;
			if (l < normals.size() - 1)
				l++;
		}

		input.close();
	}
}

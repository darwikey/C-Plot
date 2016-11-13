#pragma once
#include <string>

struct Tweakable
{

	Tweakable(const std::string &name);
	~Tweakable();


	std::string name;
	double value = 0;
	double min = -10.0;
	double max = 10.0;
};

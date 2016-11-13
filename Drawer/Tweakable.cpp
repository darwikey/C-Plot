#include "Tweakable.h"
#include <algorithm> 

Tweakable::Tweakable(const std::string &_name) : name(_name)
{
	std::transform(name.begin(), name.end(), name.begin(), ::toupper);
}

Tweakable::~Tweakable()
{
}
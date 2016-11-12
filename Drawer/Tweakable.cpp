#include "Tweakable.h"
#include <algorithm> 

Tweakable::Tweakable(const std::string &name) : mName(name)
{
	std::transform(mName.begin(), mName.end(), mName.begin(), ::toupper);

	//std::string toInsert = "//double " + mName + ";\n";
	//if (sourceCode.find(toInsert) == std::string::npos)
	//{
	//	sourceCode.insert(0, toInsert);
	//}
}

Tweakable::~Tweakable()
{
}
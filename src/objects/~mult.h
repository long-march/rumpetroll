#pragma once

#include "../AudioObject.h"

namespace Yggdrasil {

class MultObject : public AudioObject
{
	float default_value = 1;
	void run(buf&, buf&, int) override;

public:
	MultObject(std::string);
};

}

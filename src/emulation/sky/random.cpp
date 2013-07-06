#include "random.hpp"
#include <iostream>
#include "library/sha256.hpp"
#include "library/serialization.hpp"
#include "interface/callbacks.hpp"

namespace sky
{
	random::random()
	{
		initialized = false;
	}

	void random::init()
	{
		memset(state, 0, 32);
		write64ule(state, ecore_callbacks->get_randomseed());
		initialized = true;
	}

	void random::push(uint32_t x)
	{
		if(!initialized)
			init();
		uint8_t buf[4];
		sha256 h;
		h.write(state, 32);
		write32ule(buf, x);
		h.write(buf, 4);
		h.read(state);
	}

	uint32_t random::pull()
	{
		if(!initialized)
			init();
		uint32_t val = read32ule(state);
		sha256 h;
		h.write(state, 32);
		h.read(state);
		return val;
	}
}

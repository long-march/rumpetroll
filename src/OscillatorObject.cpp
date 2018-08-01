
#include "Yggdrasil.h"
#include "Block.h"
#include "OscillatorObject.h"

Block OscillatorObject::make_block()
{
	static uint block_index = 0;

	Block b;
	for (uint i = 0; i < BLOCKSIZE; i++)
	{
		b[i] = sinf((float)(i + block_index*BLOCKSIZE) / SAMPLE_RATE * 110 * TAU);
	}

	block_index++;
	return b;
}

void OscillatorObject::run()
{
	write_block(make_block(), 0);
}

OscillatorObject::OscillatorObject()
{
	outputs.push_back({});
}
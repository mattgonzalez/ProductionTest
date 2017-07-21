#include "base.h"
#include "ehw.h"

bool MidiTest(ehw *dev, bool &passed)
{
	uint64 sernum;

	sernum = dev->GetSerialNumber();
	sernum &= 0x000000ffffffffff;
	if(sernum == 0)
		passed = true;
	else
		passed = false;
	return true;
}

#include "LINChecksum.h"

LINChecksum::LINChecksum()
: mChecksum(0)
{
}

LINChecksum::~LINChecksum()
{
}

void LINChecksum::clear()
{
	mChecksum = 0;
}

U8 LINChecksum::add(U8 byte)
{
	mChecksum += byte;
	if ( mChecksum & 0x100 ) // carry?
		mChecksum += 1;
	return (U8)( mChecksum & 0xFF );
}

U8 LINChecksum::result()
{
	U8 rc = (U8)( mChecksum & 0xFF );
	rc = ~rc;
	return rc;
}


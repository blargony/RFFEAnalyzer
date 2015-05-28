#ifndef LIN_CHECKSUM_H
#define LIN_CHECKSUM_H

#include <LogicPublicTypes.h>

class LINChecksum
{
public:
	LINChecksum();
	~LINChecksum();

	void	clear();
	U8		add( U8 byte );
	U8		result();

private:
	U16 mChecksum;
};

#endif // LINCHECKSUM_H

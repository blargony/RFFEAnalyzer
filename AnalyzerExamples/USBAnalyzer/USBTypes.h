#ifndef USB_TYPES_H
#define USB_TYPES_H

#include <LogicPublicTypes.h>

enum USBFrameTypes	// analyzer frames, NOT USB 1ms frame
{
	FT_Signal,		// low-level signals: J, K, SE0 or SE1

	FT_SYNC,		// USB Sync field
	FT_PID,			// USB PID
	FT_FrameNum,	// SOF's frame number field
	FT_AddrEndp,	// address & endpoint
	FT_EOP,			// USB End Of Packet
	FT_Reset,		// USB reset
	FT_CRC5,
	FT_CRC16,
	FT_Idle,
	FT_KeepAlive,	// Low speed keep alive signals

	FT_Byte,		// used in Bytes decode mode and for data payload of packets

	FT_Error,		// invalid packet
};

// valid USB 1.1 PIDs
// USB 2.0 PIDs are commented out
enum USB_PID
{
	PID_IN		= 0x69,
	PID_OUT		= 0xE1,
	PID_SOF		= 0xA5,
	PID_SETUP	= 0x2D,

	PID_DATA0	= 0xC3,
	PID_DATA1	= 0x4B,
	//PID_DATA2	= 0x87,
	//PID_MDATA	= 0x0F,

	PID_ACK		= 0xD2,
	PID_NAK		= 0x5A,
	PID_STALL	= 0x1E,
	//PID_NYET	= 0x96,

	PID_PRE		= 0x3C,
	//PID_ERR	= 0x3C,
	//PID_SPLIT	= 0x78,
	//PID_PING	= 0xB4,
};

std::string GetPIDName(USB_PID pid);

class USBAnalyzerResults;

struct USBPacket
{
	U64		sample_begin;
	U64		sample_end;

	// all the data in the packet starting from the SYNC field all the way to the CRC
	// so:
	// data[0] SYNC
	// data[1] PID
	//   data[2..n-3] payload (for packets > 4 bytes)
	//   data[n-2..n-1] CRC (for packets > 4 bytes)
	//   data[2..3] address, endpoint and CRC (for packets == 4 bytes)
	std::vector<U8>		data;

	// sample number of the individual bits
	// the last element if this vector will hold the ending sample of the last data bit
	// this means the number of elements in the vector will be == num_bytes*8 + 1
	std::vector<U64>	bit_begin_samples;

	U8			PID;
	U16			CRC;	// both 16 and 5 bits

	void Clear();

	U16 GetLastWord() const
	{
		return (data.back() << 8) | *(data.end() - 2);
	}

	U8 GetAddress() const
	{
		return data[2] & 0x7F;
	}

	U8 GetEndpoint() const
	{
		return (GetLastWord() >> 7) & 0xf;
	}

	U16 GetFrameNum() const
	{
		return GetLastWord() & 0x7ff;
	}

	bool IsTokenPacket() const
	{
		return PID == PID_IN  ||  PID == PID_OUT  ||  PID == PID_SETUP;
	}

	bool IsSOFPacket() const
	{
		return PID == PID_SOF;
	}

	bool IsDataPacket() const
	{
		return PID == PID_DATA0  ||  PID == PID_DATA1;
	}

	bool IsHandshakePacket() const
	{
		return PID == PID_ACK  ||  PID == PID_NAK  ||  PID == PID_STALL;
	}

	bool IsPIDValid() const;

	static U8 CalcCRC5(U16 data);
	U16 CalcCRC16() const;

	U64 AddFrames(USBAnalyzerResults* pResults);
	U64 AddRawByteFrames(USBAnalyzerResults* pResults);
	U64 AddErrorFrame(USBAnalyzerResults* pResults);
};

enum USBState
{
	S_K,
	S_J,
	S_SE0,
	S_SE1,
};

enum USBSpeed
{
	LOW_SPEED,		// 1.5 mbit/s
	FULL_SPEED,		// 12 mbit/s
};

enum USBDecodeLevel
{
	OUT_PACKETS,
	OUT_BYTES,
	OUT_SIGNALS,
};

const double FS_BIT_DUR = (1000 / 12.0);	// 83.3 ns
const double LS_BIT_DUR = (1000 / 1.5);		// 666.7 ns

struct USBSignalState
{
	U64			sample_begin;
	U64			sample_end;

	USBState	state;
	double		dur;	// in nano sec

	bool IsDurationFS() const
	{
		return dur > FS_BIT_DUR * .3  &&  dur < FS_BIT_DUR * 7.5;
	}

	bool IsDurationLS() const
	{
		return dur > LS_BIT_DUR * .7  &&  dur < LS_BIT_DUR * 7.3;
	}

	bool IsData(const USBSpeed speed) const
	{
		return (speed == LOW_SPEED ? IsDurationLS() : IsDurationFS())
					&&  (state == S_J  ||  state == S_K);
	}

	int GetNumBits(USBSpeed speed) const
	{
		const double BIT_DUR = (speed == FULL_SPEED ? FS_BIT_DUR : LS_BIT_DUR);
		return int(dur / BIT_DUR + 0.5);
	}

	void AddFrame(USBAnalyzerResults* res);
};

class USBAnalyzer;
class USBAnalyzerResults;
class USBAnalyzerSettings;

class USBSignalFilter
{
private:
	AnalyzerChannelData*	mDP;
	AnalyzerChannelData*	mDM;

	USBAnalyzer*			mAnalyzer;
	USBAnalyzerResults*		mResults;
	USBAnalyzerSettings*	mSettings;

	USBSpeed		mSpeed;				// LS or FS
	const double	mSampleDur;			// in ns
	U64				mStateStartSample;	// used for filtered signal state iterating

	bool SkipGlitch(AnalyzerChannelData* pFurther);
	U64 DoFilter(AnalyzerChannelData* pFurther, AnalyzerChannelData* pNearer);

public:
	USBSignalFilter(USBAnalyzer* pAnalyzer, USBAnalyzerResults* pResults, USBAnalyzerSettings* pSettings,
						AnalyzerChannelData* pDP, AnalyzerChannelData* pDM, USBSpeed speed);

	bool HasMoreData();
	USBSignalState GetState();
	bool GetPacket(USBPacket& pckt, USBSignalState& sgnl);
};

std::string int2str_sal(const U64 i, DisplayBase base, const int max_bits = 8);

/*#ifdef _WINDOWS
# include <windows.h>
#endif

inline std::string int2str(const U64 i)
{
	return int2str_sal(i, Decimal, 64);
}

// debugging helper functions -- Windows only!!!
inline void debug(const std::string& str)
{
#if !defined(NDEBUG)  &&  defined(_WINDOWS)
	::OutputDebugStringA(("----- " + str + "\n").c_str());
#endif
}

inline void debug(const char* str)
{
#if !defined(NDEBUG)  &&  defined(_WINDOWS)
	debug(std::string(str));
#endif
}*/

#endif	// USB_TYPES_H
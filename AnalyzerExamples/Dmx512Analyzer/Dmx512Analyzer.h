#ifndef DMX512_ANALYZER_H
#define DMX512_ANALYZER_H

#include <Analyzer.h>
#include <AnalyzerHelpers.h>
#include "Dmx512AnalyzerResults.h"
#include "Dmx512SimulationDataGenerator.h"

class Dmx512AnalyzerSettings;
class ANALYZER_EXPORT Dmx512Analyzer : public Analyzer2
{
public:
	Dmx512Analyzer();
	virtual ~Dmx512Analyzer();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();

    virtual void SetupResults();

	enum { BREAK, MAB, START_CODE, DATA, MBB, MARK, STOP, START } data_type;

#pragma warning( push )
#pragma warning( disable : 4251 ) //warning C4251: 'SerialAnalyzer::<...>' : class <...> needs to have dll-interface to be used by clients of class
protected: //vars
	std::auto_ptr< Dmx512AnalyzerSettings > mSettings;
	std::auto_ptr< Dmx512AnalyzerResults > mResults;
	AnalyzerChannelData* mSerial;
	ClockGenerator mClockGenerator;

	Dmx512SimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitilized;

	//Serial analysis vars:
	U32 mSampleRateHz;
	U32 mStartOfStopBitOffset;
	U32 mEndOfStopBitOffset;
	U32 mSamplesPerBit;

	U64 ReadMAB(U64 start);
	U64 ReadByte(U64 start, U8 type, U64 data1);
	U64 ReadSlot(U64 start, U8 type, int count);

	void PassFrame(U64 data,U8 type,U8 flag,U64 start,U64 end,U8); // helper
	// Would make more sense to re-order the parameters
	//void PassFrame(U8 type, U64 start, U64 end, U8 flag, U64 data, U8 data2); // helper
#pragma warning( pop )
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //DMX512_ANALYZER_H

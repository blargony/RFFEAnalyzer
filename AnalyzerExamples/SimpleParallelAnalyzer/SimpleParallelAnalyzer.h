#ifndef SIMPLEPARALLEL_ANALYZER_H
#define SIMPLEPARALLEL_ANALYZER_H

#include <Analyzer.h>
#include "SimpleParallelAnalyzerResults.h"
#include "SimpleParallelSimulationDataGenerator.h"

class SimpleParallelAnalyzerSettings;
class ANALYZER_EXPORT SimpleParallelAnalyzer : public Analyzer2
{
public:
	SimpleParallelAnalyzer();
	virtual ~SimpleParallelAnalyzer();
	virtual void SetupResults();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();

protected: //vars

#pragma warning( push )
#pragma warning( disable : 4251 ) //warning C4251: 'SerialAnalyzer::<...>' : class <...> needs to have dll-interface to be used by clients of class

	std::auto_ptr< SimpleParallelAnalyzerSettings > mSettings;
	std::auto_ptr< SimpleParallelAnalyzerResults > mResults;

	std::vector< AnalyzerChannelData* > mData;
	std::vector< U16 > mDataMasks;
	std::vector< Channel > mDataChannels;
	AnalyzerChannelData* mClock;

	SimpleParallelSimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitilized;

	//Serial analysis vars:
	U32 mSampleRateHz;

#pragma warning( pop )
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //SIMPLEPARALLEL_ANALYZER_H

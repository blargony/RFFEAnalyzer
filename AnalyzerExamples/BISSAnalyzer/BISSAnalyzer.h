#ifndef BISS_ANALYZER_H
#define BISS_ANALYZER_H

#include <Analyzer.h>
#include "BISSAnalyzerResults.h"
#include "BISSSimulationDataGenerator.h"

class BISSAnalyzerSettings;
class ANALYZER_EXPORT BISSAnalyzer : public Analyzer2
{
public:
	BISSAnalyzer();
	virtual ~BISSAnalyzer();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
    virtual void SetupResults();
	virtual bool NeedsRerun();

protected: //functions
	void FindStartBit();
	void GetData();
	void GetCdm( BitState bit_state_cdm );
	void GetCds( BitState bit_state_cds );
	void BuiltCdmFrames();
	void BuiltCdsFrames();
	void AddMyFrame(Frame frame, int CDM_CDS);
	void ShowCdmCds();

protected: //vars
	std::auto_ptr< BISSAnalyzerSettings > mSettings;
	std::auto_ptr< BISSAnalyzerResults > mResults;
	AnalyzerChannelData* mMa;
	AnalyzerChannelData* mSlo;

	BISSSimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitilized;

	//Serial analysis vars:
	U32 mSampleRateHz;
	U32 mStartOfStopBitOffset;
	U32 mEndOfStopBitOffset;
	U64 Trigger;
	
	//CDM vars
	U32 m_counter_cdm;
	U64 m_cdm_daten;		
	U64 m_start_cmd;
	U64 m_end_cdm;
	DataBuilder m_cdm;
	BitState m_cdm_array[2000];
	U64 m_cdm_samplenummer_array[2000];
	Frame m_faCDM[1000];
	
	//CDS vars
	U32 m_counter_cds;
	U64 m_cds_daten;		
	U64 m_start_cds;
	U64 m_end_cds;
	DataBuilder m_cds;
	BitState m_cds_array[2000];
	U64 m_cds_samplenummer_array[2000];	
	Frame m_faCDS[1000];

	int m_counter_newframe;
	unsigned long mul_cdmframe_index;
	unsigned long mul_cdsframe_index;

	U64 m_startperiod;
	U64 m_endperiod;
	int m_samplesperperiod;

	BitState m_sdata_array[500];
	U64 m_sdata_samplenummer_array[500];

	
	//U32 m_ulInternalValue;
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //BISS_ANALYZER_H

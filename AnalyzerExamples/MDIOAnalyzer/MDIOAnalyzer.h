#ifndef MDIO_ANALYZER_H
#define MDIO_ANALYZER_H

#include <Analyzer.h>
#include "MDIOAnalyzerResults.h"
#include "MDIOSimulationDataGenerator.h"

enum MDIOPacketClauseType { MDIO_C22_PACKET, MDIO_C45_PACKET };
enum MDIOPacketC45Type { MDIO_C45_PACKET_ADDR, MDIO_C45_PACKET_DATA };
enum MDIOPacketOperation { MDIO_PACKET_READ, MDIO_PACKET_WRITE };

// struct to take actions depending on the type of the current MDIO packet
struct PacketType 
{
	MDIOPacketClauseType clause;
	MDIOPacketC45Type c45Type;
	MDIOPacketOperation operation;
};

class MDIOAnalyzerSettings;
class ANALYZER_EXPORT MDIOAnalyzer : public Analyzer2
{
public:
	MDIOAnalyzer();
	virtual ~MDIOAnalyzer();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();
    virtual void SetupResults();
protected: 
	void AdvanceToStartFrame();
	
	void ProcessStartFrame();
	void ProcessOpcodeFrame();
	void ProcessPhyAddrFrame();
	void ProcessRegAddrDevTypeFrame();
	void ProcessTAFrame();
	void ProcessAddrDataFrame();
	
	void AdvanceToHighMDIO();
	
	void ProcessTAFrameInReadPacket();
	void ProcessTAFrameInWritePacket();

	void AddArrowMarkers();
	void GetBit( BitState& bit_state, std::vector<U64> & arrows );
	MDIOFrameType GetDevType(const U64 & value);

protected: 
	std::auto_ptr< MDIOAnalyzerSettings > mSettings;
	std::auto_ptr< MDIOAnalyzerResults > mResults;

	AnalyzerChannelData* mMdio;
	AnalyzerChannelData* mMdc;

	MDIOSimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitialized;

	// U8 mPacketInTransaction;
	// U64 mTransactionID;

	PacketType mCurrentPacket;

	std::vector<U64> mMdcPosedgeArrows;
	std::vector<U64> mMdcNegedgeArrows;

	U32 mSampleRateHz;
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //MDIO_ANALYZER_H

#ifndef HDMICEC_ANALYZER_H
#define HDMICEC_ANALYZER_H

#include <Analyzer.h>
#include "HdmiCecAnalyzerResults.h"
#include "HdmiCecSimulationDataGenerator.h"

class HdmiCecAnalyzerSettings;

class ANALYZER_EXPORT HdmiCecAnalyzer : public Analyzer2
{
public:
    HdmiCecAnalyzer();
    virtual ~HdmiCecAnalyzer();
    virtual void WorkerThread();

    virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
    virtual U32 GetMinimumSampleRateHz();

    virtual void SetupResults();

    virtual const char* GetAnalyzerName() const;
    virtual bool NeedsRerun();

protected:
    // Reads the start sequence, returns false on error
    bool ReadStartSequence( Frame& frame );
    // Functions to read fields from a 10-bit CEC block
    // All functions return false on error
    bool ReadBlockByte( int blockIndex, Frame& byteFrame );
    bool ReadBlockEOM( Frame& eomFrame );
    bool ReadBlockACK( Frame& ackFrame );
    // Reads one bit written by the initiatior. *firstSample and *lastSample are
    // set if the pointers are not null. Returns false on error.
    bool ReadBit( bool& value, S64* firstSample=0, S64* lastSample=0 );

    // Returns the elapsed time in msecs since another sample.
    // TimeSince will return a negative number if "sample" is in the future.
    float TimeSince( S64 sample );
    // Adds an error marker to the current position
    void MarkErrorPosition();

    std::auto_ptr< HdmiCecAnalyzerSettings > mSettings;
    std::auto_ptr< HdmiCecAnalyzerResults > mResults;
    AnalyzerChannelData* mCec;

    HdmiCecSimulationDataGenerator mSimulationDataGenerator;
    bool mSimulationInitilized;
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //HDMICEC_ANALYZER_H

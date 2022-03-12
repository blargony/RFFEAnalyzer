#ifndef RFFE_ANALYZER_H
#define RFFE_ANALYZER_H

#include <Analyzer.h>
#include "RFFEAnalyzerResults.h"
#include "RFFESimulationDataGenerator.h"

#pragma warning(push)
// warning C4275: non dll-interface class 'Analyzer2' used
//               as base for dll-interface class 'RFFEAnalyzer'
#pragma warning(disable : 4275)

#define UNEXPECTED_SSC_EXCEPTION 101

class RFFEAnalyzerSettings;

class ANALYZER_EXPORT RFFEAnalyzer : public Analyzer2 {
public:
  RFFEAnalyzer();
  virtual ~RFFEAnalyzer();
  virtual void SetupResults();
  virtual void WorkerThread();

  virtual U32 GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor **simulation_channels);
  virtual U32 GetMinimumSampleRateHz();

  virtual const char *GetAnalyzerName() const;
  virtual bool NeedsRerun();

#pragma warning(push)
// warning C4251: 'RFFEAnalyzer::<...>' : class <...> needs to have dll-interface
//               to be used by clients of class
#pragma warning(disable : 4251)

protected: // vars
  std::auto_ptr<RFFEAnalyzerSettings> mSettings;
  std::auto_ptr<RFFEAnalyzerResults> mResults;
  AnalyzerChannelData *mSclk;
  AnalyzerChannelData *mSdata;

  RFFESimulationDataGenerator mSimulationDataGenerator;
  bool mSimulationInitilized;

  //
  U32 mSampleRateHz;

protected:
  // Packet Level Searchs
  U8 FindStartSeqCondition();
  U8 FindCommandFrame();
  void FindByteFrame(RFFEAnalyzerResults::RffeFrameType type);
  U8 FindISI();
  void FindInterruptSlots();
  void FindParity(bool expParity, U64 extra_data);
  void FindBusPark();

  // Bit Level Waveform Parsing
  void GotoNextTransition();
  void GotoSclkEdge(BitState);
  BitState GetNextBit(U8 idx);
  U64 GetBitStream(U8 len);

  // Physical Layer Checks
  bool CheckClockRate();

  // Interface to the Results/Output
  void FillInFrame(RFFEAnalyzerResults::RffeFrameType type, U64 frame_data, U64 extra_data, U32 idx_start, U32 idx_end, U8 flags);

private:
  // --------------------------------------
  // The sampling and storing of chunks of
  // RFFE Data are managed in member variables.
  // These get handed back and forth between
  // a variety of methods so keep them here as
  // a common working set of variables
  // --------------------------------------
  // RFFE Cmd Type we are in the middle of - ReadExtended, Write0, etc...
  RFFEAnalyzerResults::RffeCommandFieldType mRffeCmdType;

  // Used to store the current and previous state of the bus
  // (on a transition by transition basis)
  U64 mSamplePosition;

  BitState mSclkCurrent;
  BitState mSdataCurrent;
  BitState mSclkPrevious;
  BitState mSdataPrevious;

  bool mUnexpectedSSC;
  U64 mUnexpectedSSCStart;

  // Used to store sample offsets that need to be handed to the AnalyzerResults
  // objects to indicate the start/stop sample points for annotations in the
  // waveform display.
  U64 mSampleClkOffsets[16];
  U64 mSampleDataOffsets[16];
  AnalyzerResults::MarkerType mSampleMarker[16];

#pragma warning(pop)
};
#pragma warning(pop)

extern "C" ANALYZER_EXPORT const char *__cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer *__cdecl CreateAnalyzer();
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer(Analyzer *analyzer);

#endif // RFFE_ANALYZER_H

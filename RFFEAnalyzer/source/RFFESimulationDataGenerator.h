#ifndef RFFE_SIMULATION_DATA_GENERATOR
#define RFFE_SIMULATION_DATA_GENERATOR

#include <AnalyzerHelpers.h>
#include <SimulationChannelDescriptor.h>
#include <string>
#include "RFFEAnalyzerResults.h"

class RFFEAnalyzerSettings;

class RFFESimulationDataGenerator {
public:
  RFFESimulationDataGenerator();
  ~RFFESimulationDataGenerator();

  void Initialize(U32 simulation_sample_rate, RFFEAnalyzerSettings *settings);
  U32 GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor **simulation_channels);

protected:
  RFFEAnalyzerSettings *mSettings;
  U32 mSimulationSampleRateHz;

protected: // RFFE specific functions
  void CreateRffeTransaction();
  void CreateSSC();
  void CreateSlaveAddress(U8 addr);
  void CreateByteFrame(U8 byte);
  void CreateByte(U8 cmd);
  void CreateBits(U8 bits, U8 data);
  void CreateParity(U8 byte);
  void CreateBusPark();
  U8 CreateRandomData();

protected: // RFFE specific vars
  ClockGenerator mClockGenerator;
  SimulationChannelDescriptorGroup mRffeSimulationChannels;
  SimulationChannelDescriptor *mSclk;
  SimulationChannelDescriptor *mSdata;

private:
  U8 mLSFRData;
};

// Parity Lookup Table
static const U8 ParityTable256[256] = {
#define P2(n) n, n ^ 1, n ^ 1, n
#define P4(n) P2(n), P2(n ^ 1), P2(n ^ 1), P2(n)
#define P6(n) P4(n), P4(n ^ 1), P4(n ^ 1), P4(n)
    P6(1), P6(0), P6(0), P6(1)};

#endif // RFFE_SIMULATION_DATA_GENERATOR

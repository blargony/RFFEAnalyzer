#ifndef HD44780_SIMULATION_DATA_GENERATOR
#define HD44780_SIMULATION_DATA_GENERATOR

#include <SimulationChannelDescriptor.h>
#include <string>

#include <AnalyzerHelpers.h>

class HD44780AnalyzerSettings;

class HD44780SimulationDataGenerator
{
public:
	HD44780SimulationDataGenerator();
	~HD44780SimulationDataGenerator();

	void Initialize( U32 simulation_sample_rate, HD44780AnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel );

protected:
	HD44780AnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;

  bool bitmode8;
  U32 value;

  void Init();
  void Output();
  void DoTransfer(bool ARS, bool ARW, U8 AData);
  void DoOperation(bool ARS, bool ARW, U8 AData);
  void DoTransferString(bool ARW, char *AString);
  U32 TimeToSamplesOrMore(double AS);
  U32 TimeToSamplesOrLess(double AS);


protected:
	SimulationChannelDescriptorGroup mSimulationChannels;
	SimulationChannelDescriptor *mE, *mRS, *mRW, *mDB[8];

};
#endif //HD44780_SIMULATION_DATA_GENERATOR
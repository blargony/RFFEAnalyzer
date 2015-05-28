#ifndef PS2KEYBOARD_SIMULATION_DATA_GENERATOR
#define PS2KEYBOARD_SIMULATION_DATA_GENERATOR

#include <AnalyzerHelpers.h>
#include <SimulationChannelDescriptor.h>
#include <string>
class PS2KeyboardAnalyzerSettings;

class PS2KeyboardSimulationDataGenerator
{
public:
	PS2KeyboardSimulationDataGenerator();
	~PS2KeyboardSimulationDataGenerator();

	void Initialize( U32 simulation_sample_rate, PS2KeyboardAnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );

protected:
	PS2KeyboardAnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;

protected:

	SimulationChannelDescriptorGroup mPS2KeyboardSimulationChannels;
	SimulationChannelDescriptor* mClock;
	SimulationChannelDescriptor* mData;

	ClockGenerator mClockGenerator;

	void SendByte(U8 payload);
	void SendStandardKey(U8 keycode);
	void SendExtendedKey(U8 keycode);
	void SendPrintScreenKey();
	void SendPauseBreakKey();
	void SendMovementPacket(double MouseType);
	void SendMouseDeviceID(double MouseType);
	void HostSendByte(U8 payload);

};
#endif //PS2KEYBOARD_SIMULATION_DATA_GENERATOR

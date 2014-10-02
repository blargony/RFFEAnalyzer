#include <AnalyzerChannelData.h>

#include <vector>
#include <algorithm>

#include "USBAnalyzer.h"
#include "USBAnalyzerSettings.h"

USBAnalyzer::USBAnalyzer()
	: mSimulationInitilized(false)
{
	SetAnalyzerSettings(&mSettings);
}

USBAnalyzer::~USBAnalyzer()
{
	KillThread();
}

void USBAnalyzer::Setup()
{
	// reset the results
	mResults.reset(new USBAnalyzerResults(this, &mSettings));
	SetAnalyzerResults(mResults.get());

	// get the channel pointers
	mDP = GetAnalyzerChannelData(mSettings.mDPChannel);
	mDM = GetAnalyzerChannelData(mSettings.mDMChannel);

	// set which channels will carry bubbles
	mResults->AddChannelBubblesWillAppearOn(mSettings.mDPChannel);
	mResults->AddChannelBubblesWillAppearOn(mSettings.mDMChannel);
}

void USBAnalyzer::WorkerThread()
{
	Setup();

	USBSignalFilter sf(this, mResults.get(), &mSettings, mDP, mDM, mSettings.mSpeed);

	const double BIT_DUR = mSettings.mSpeed == FULL_SPEED ? FS_BIT_DUR : LS_BIT_DUR;
	const double SAMPLE_DUR = 1e9 / GetSampleRate();	// 1 sample duration in ns
	const double BIT_SAMPLES = BIT_DUR / SAMPLE_DUR;

	USBSignalState s;
	USBPacket pckt;
	U64 lastFrameEnd = 0;
	while (sf.HasMoreData())
	{
		s = sf.GetState();

		if (mSettings.mDecodeLevel == OUT_SIGNALS)
		{
			s.AddFrame(mResults.get());

		} else {
			if (lastFrameEnd == 0)
				lastFrameEnd = s.sample_begin;

			// could this be a valid packet?
			if (s.IsData(mSettings.mSpeed)  &&  s.GetNumBits(mSettings.mSpeed) == 1)
			{
				if (sf.GetPacket(pckt, s))
				{
					//static int c = 0;
					//debug("OK packet #" + int2str(++c) + " time: " + GetTimeStr(pckt.sample_begin) + " begin sample: " + int2str(pckt.sample_begin) + " end sample: " + int2str(pckt.sample_end));

					if (mSettings.mDecodeLevel == OUT_PACKETS)
						lastFrameEnd = pckt.AddFrames(mResults.get());
					else
						lastFrameEnd = pckt.AddRawByteFrames(mResults.get());

					//if (!pckt.IsPIDValid())
					//	debug("bad PID " + int2str(pckt.PID) + "   " + int2str(pckt.sample_begin));
				} else {
					//static int c = 0;
					//debug("Error packet #" + int2str(++c) + "  time: " + GetTimeStr(pckt.sample_begin) + " begin sample: " + int2str(pckt.sample_begin) + " end sample: " + int2str(pckt.sample_end));

					lastFrameEnd = pckt.AddErrorFrame(mResults.get());
				}

			} else if (mSettings.mSpeed == LOW_SPEED			// is this a LS Keep-alive?
						&&  s.state == S_SE0
						&&  s.GetNumBits(LOW_SPEED) == 2) {

				Frame f;
				f.mStartingSampleInclusive = lastFrameEnd;
				f.mEndingSampleInclusive = s.sample_end;
				f.mType = FT_KeepAlive;
				f.mFlags = 0;
				f.mData1 = f.mData2 = 0;

				mResults->AddFrame(f);
				mResults->CommitResults();

				lastFrameEnd = s.sample_end;

				//debug("SIM_NEW_FRAME,");

			} else if (s.state == S_SE0  &&  s.dur > 1e7) {		// Reset?   dur > 10 ms

				Frame f;
				f.mStartingSampleInclusive = lastFrameEnd;
				f.mEndingSampleInclusive = s.sample_end;
				f.mType = FT_Reset;
				f.mFlags = 0;
				f.mData1 = f.mData2 = 0;

				mResults->AddFrame(f);
				mResults->CommitResults();

				lastFrameEnd = s.sample_end;

				//debug("Reset");

			} else if (s.state == S_J) {		// Idle

				lastFrameEnd = s.sample_end;
			}
		}

		ReportProgress(s.sample_end);
	}
}

bool USBAnalyzer::NeedsRerun()
{
	return false;
}

U32 USBAnalyzer::GenerateSimulationData(U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels)
{
	if (!mSimulationInitilized)
	{
		mSimulationDataGenerator.Initialize(GetSimulationSampleRate(), &mSettings);
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData(minimum_sample_index, device_sample_rate, simulation_channels);
}

U32 USBAnalyzer::GetMinimumSampleRateHz()
{
	return 24000000;	// full 24MHz
}

const char* USBAnalyzer::GetAnalyzerName() const
{
	return ::GetAnalyzerName();
}

const char* GetAnalyzerName()
{
	return "USB 1.1";
}

Analyzer* CreateAnalyzer()
{
	return new USBAnalyzer();
}

void DestroyAnalyzer(Analyzer* analyzer)
{
	delete analyzer;
}

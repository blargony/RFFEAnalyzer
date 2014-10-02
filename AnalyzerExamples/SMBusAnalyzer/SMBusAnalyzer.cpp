#include <AnalyzerChannelData.h>

#include <cassert>

#include <vector>
#include <algorithm>

#include "SMBusAnalyzer.h"
#include "SMBusAnalyzerSettings.h"

SMBusAnalyzer::SMBusAnalyzer()
	: mSimulationInitilized(false)
{
	SetAnalyzerSettings(&mSettings);
}

SMBusAnalyzer::~SMBusAnalyzer()
{
	KillThread();
}

void SMBusAnalyzer::Setup()
{
	// reset the results
	mResults.reset(new SMBusAnalyzerResults(this, &mSettings));
	SetAnalyzerResults(mResults.get());

	// get the channel pointers
	mSMBDAT = GetAnalyzerChannelData(mSettings.mSMBDAT);
	mSMBCLK = GetAnalyzerChannelData(mSettings.mSMBCLK);

	// set which channels will carry bubbles
	mResults->AddChannelBubblesWillAppearOn(mSettings.mSMBDAT);
	mResults->AddChannelBubblesWillAppearOn(mSettings.mSMBCLK);
}

AnalyzerChannelData* SMBusAnalyzer::AdvanceAllTo(U64 toSample)
{
	assert(mSMBCLK->GetSampleNumber() <= toSample);
	assert(mSMBDAT->GetSampleNumber() <= toSample);
	
	mSMBCLK->AdvanceToAbsPosition(toSample);
	mSMBDAT->AdvanceToAbsPosition(toSample);

	return GetNearestTransitionChannel();
}

AnalyzerChannelData* SMBusAnalyzer::GetNearestTransitionChannel()
{
	if (!mSMBDAT->DoMoreTransitionsExistInCurrentData())
		return mSMBCLK;

	if (!mSMBCLK->DoMoreTransitionsExistInCurrentData())
		return mSMBDAT;

	if (mSMBDAT->GetSampleOfNextEdge() < mSMBCLK->GetSampleOfNextEdge())
		return mSMBDAT;

	return mSMBCLK;
}

bool SMBusAnalyzer::GetSignal(SMBusSignalState& state)
{

func_begin:

	bool ret_val = true;

	state.Clear();

	AnalyzerChannelData* nearestEdgeChannel = GetNearestTransitionChannel();

	// first check for a start or stop
	if (mSMBCLK->GetBitState() == BIT_HIGH)
	{
		U64 start_sample = mSMBCLK->GetSampleNumber();
		if (nearestEdgeChannel == mSMBDAT)
		{
			ret_val = false;

			// remember where we began in case this is the last packet
			U64 frame_begin = mSMBDAT->GetSampleNumber();

			state.sample_marker = mSMBDAT->GetSampleOfNextEdge();

			if (mSMBDAT->GetBitState() == BIT_HIGH)
				state.bus_signal = SMB_Start;
			else
				state.bus_signal = SMB_Stop;

			// move all to the SMBDAT transition
			nearestEdgeChannel = AdvanceAllTo(state.sample_marker);

			// set the begin and ending samples for the frame
			U64 frame_offset = 0;
			if (nearestEdgeChannel->DoMoreTransitionsExistInCurrentData())
			{
				U64 end_sample = nearestEdgeChannel->GetSampleOfNextEdge();
				if (end_sample - state.sample_marker < state.sample_marker - start_sample)
					frame_offset = (end_sample - state.sample_marker) / 2;
				else
					frame_offset = (state.sample_marker - start_sample) / 2;
			} else {
				frame_offset = state.sample_marker - frame_begin;
			}

			state.sample_begin = state.sample_marker - frame_offset;
			state.sample_end = state.sample_marker + frame_offset;

			// skip the falling SMBCLK if we have a start
			// this prepares us for the first bit
			if (state.bus_signal == SMB_Start  &&  nearestEdgeChannel == mSMBCLK)
				AdvanceAllTo(mSMBCLK->GetSampleOfNextEdge());
		} else {
			// SMBCLK falls - skip to it
			nearestEdgeChannel = AdvanceAllTo(nearestEdgeChannel->GetSampleOfNextEdge());
		}
	}

	// if we didn't find a start or stop above try parsing a bit
	if (ret_val)
	{
		// we must have a low SMBCLK at this point
		assert(mSMBCLK->GetBitState() == BIT_LOW);

		// skip the preparing of SMBDAT for the rising SMBCLK egde
		if (nearestEdgeChannel == mSMBDAT)
		{
			state.sample_begin = mSMBDAT->GetSampleOfNextEdge();
			nearestEdgeChannel = AdvanceAllTo(state.sample_begin);
		} else {
			state.sample_begin = (mSMBCLK->GetSampleNumber() + mSMBCLK->GetSampleOfNextEdge()) / 2;
		}

		// do we have a rising SMBCLK at this point?
		if (nearestEdgeChannel == mSMBCLK)
		{
			nearestEdgeChannel = AdvanceAllTo(mSMBCLK->GetSampleOfNextEdge());

			state.sample_rising_clk = mSMBCLK->GetSampleNumber();

			// do we have a falling SMBCLK edge next?
			if (nearestEdgeChannel == mSMBCLK)
			{
				// sample the state of SMBDAT
				state.sample_marker = (mSMBCLK->GetSampleOfNextEdge() + state.sample_rising_clk) / 2;

				if (state.sample_marker > state.sample_rising_clk  &&  state.sample_marker < mSMBCLK->GetSampleOfNextEdge())
					nearestEdgeChannel = AdvanceAllTo(state.sample_marker);

				state.bus_signal = mSMBDAT->GetBitState() == BIT_HIGH ? SMB_One : SMB_Zero;

				// advance to SMBCLK falling edge
				nearestEdgeChannel = AdvanceAllTo(mSMBCLK->GetSampleOfNextEdge());

				// where to put the end of state depends on where the next transition happens
				if (nearestEdgeChannel == mSMBCLK)
					state.sample_end = (mSMBCLK->GetSampleNumber() + mSMBCLK->GetSampleOfNextEdge()) / 2;
				else
					state.sample_end = mSMBDAT->GetSampleOfNextEdge();

			} else {
				// go back to parsing start/stop
				goto func_begin;
			}

		} else {
			// go back to parsing start/stop
			goto func_begin;
		}
	}

	return ret_val;
}

void SMBusAnalyzer::WorkerThread()
{
	Setup();

	const bool calcPEC = mSettings.CalcPEC();
	bool is_bit;
	SMBusByte byte;
	SMBusPacket packet;
	SMBusSignalState s;
	for (;;)
	{
		is_bit = GetSignal(s);

		if (is_bit)
		{
			// is this a data bit?
			if (byte.signals.size() < 8)
			{
				byte.value <<= 1;
				if (s.bus_signal == SMB_One)
					byte.value |= 1;

				byte.signals.push_back(s);
			} else {
				// translate zero/one to ack/nack
				if (s.bus_signal == SMB_Zero)
				{
					byte.is_acked = true;
					s.bus_signal = SMB_ACK;
				} else {
					byte.is_acked = false;
					s.bus_signal = SMB_NACK;
				}

				// remember the ACK/NACK signal
				byte.signals.push_back(s);

				// add byte to command/packet
				if (!packet.chunks.empty())
					packet.chunks.back().push_back(byte);
			}

		} else {

			// on a Start signal begin a new chunk
			if (s.bus_signal == SMB_Start)
			{
				if (packet.chunks.empty())
					packet.first_start = s;

				packet.chunks.push_back(std::vector<SMBusByte>());
			}
		}

		// create the markers
		s.AddMarkers(mResults.get(), mSettings.mSMBCLK, mSettings.mSMBDAT);

		// branch on decode level
		if (mSettings.mDecodeLevel == DL_Signals)
		{
			mResults->AddFrame(s.ToFrame());

		} else if (mSettings.mDecodeLevel == DL_Bytes) {

			if (byte.IsComplete())
			{
				if (packet.chunks.back().size() == 1)	// if this is the first byte of the packet
					mResults->AddFrame(byte.ToAddrFrame());
				else
					mResults->AddFrame(byte.ToFrame());
			}

		} else if (mSettings.mDecodeLevel == DL_SMBus
					|| mSettings.mDecodeLevel == DL_PMBus
					|| mSettings.mDecodeLevel == DL_SmartBattery) {

			if (s.bus_signal == SMB_Stop)
			{
				// make frames for the transaction

				packet.stop = s;

				// first try the SMBus protocols
				bool found_match = false;
				int pcnt;
				for (pcnt = 0; pcnt < NUM_SMBUS_PROTOCOLS  &&  !found_match; ++pcnt)
				{
					if (packet.MatchesProtocol(SMBusProtocols + pcnt, calcPEC))
					{
						packet.CreateDescFrames(mResults.get(), SMBusProtocols + pcnt, 0);
						packet.CreateFrames(mResults.get(), SMBusProtocols + pcnt, calcPEC, mSettings.mDecodeLevel);
						found_match = true;
					}
				}

				// do the PMBus specific protocols
				if (!found_match  &&  mSettings.mDecodeLevel == DL_PMBus)
				{
					for (pcnt = 0; pcnt < NUM_PMBUS_PROTOCOLS  &&  !found_match; ++pcnt)
					{
						if (packet.MatchesProtocol(PMBusProtocols + pcnt, calcPEC))
						{
							packet.CreateDescFrames(mResults.get(), PMBusProtocols + pcnt, 0);
							packet.CreateFrames(mResults.get(), PMBusProtocols + pcnt, calcPEC, mSettings.mDecodeLevel);
							found_match = true;
						}
					}

					if (!found_match  &&  packet.MatchesGroupCommand(calcPEC))
					{
						packet.CreateDescFrames(mResults.get(), NULL, 1);
						packet.CreateFramesForGroupCommand(mResults.get(), calcPEC);
						found_match = true;
					}
				}

				// if we have not recognized this packet, just add raw data frames
				if (!found_match)
				{
					packet.CreateDescFrames(mResults.get(), NULL, 2);
					packet.CreateFramesForRawData(mResults.get(), calcPEC);
				}

				packet.Clear();
			}
		}

		if (byte.signals.size() == 9  ||  !is_bit)
			byte.Clear();

		mResults->CommitResults();

		ReportProgress(mSMBDAT->GetSampleNumber());
	}
}

bool SMBusAnalyzer::NeedsRerun()
{
	return false;
}

U32 SMBusAnalyzer::GenerateSimulationData(U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels)
{
	if (!mSimulationInitilized)
	{
		mSimulationDataGenerator.Initialize(GetSimulationSampleRate(), &mSettings);
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData(minimum_sample_index, device_sample_rate, simulation_channels);
}

U32 SMBusAnalyzer::GetMinimumSampleRateHz()
{
	return 1000000;
}

const char* SMBusAnalyzer::GetAnalyzerName() const
{
	return ::GetAnalyzerName();
}

const char* GetAnalyzerName()
{
	return "SMBus";
}

Analyzer* CreateAnalyzer()
{
	return new SMBusAnalyzer();
}

void DestroyAnalyzer(Analyzer* analyzer)
{
	delete analyzer;
}

#include <AnalyzerChannelData.h>

#include <vector>
#include <algorithm>

#include "AtmelSWIAnalyzer.h"
#include "AtmelSWIAnalyzerSettings.h"

AtmelSWIAnalyzer::AtmelSWIAnalyzer()
	: mSimulationInitilized(false)
{
	SetAnalyzerSettings(&mSettings);
}

AtmelSWIAnalyzer::~AtmelSWIAnalyzer()
{
	KillThread();
}

void AtmelSWIAnalyzer::Setup()
{
    // get the channel data pointer
	mSDA = GetAnalyzerChannelData(mSettings.mSDAChannel);
}

void AtmelSWIAnalyzer::AddFrame(U64 SampleBegin, U64 SampleEnd, AtmelSWIFrameType FrameType, U64 Data1, U64 Data2)
{
	Frame frm;

	frm.mFlags = 0;
	frm.mStartingSampleInclusive = SampleBegin;
	frm.mEndingSampleInclusive = SampleEnd - 1;
	frm.mData1 = Data1;
	frm.mData2 = Data2;
	frm.mType = FrameType;

	mResults->AddFrame(frm);
	mResults->CommitResults();
}

void AtmelSWIAnalyzer::ResyncToWake(SWI_WaveParser& tokenizer)
{
	U64 SampleBegin, SampleEnd;
	U8 byte;
	bool is_wake;

	// just read the raw bytes until GetByteWithExceptions finds a wake token and throws an exception
	for (;;)
	{
		byte = tokenizer.GetByte(SampleBegin, SampleEnd, is_wake);
		if (is_wake)
		{
			AddFrame(SampleBegin, SampleEnd, FrameToken, SWI_Wake);
			throw WakeException();
		}

		AddFrame(SampleBegin, SampleEnd, FrameByte, byte);
	}
}

void AtmelSWIAnalyzer::WorkerThread()
{
    Setup();

	SWI_WaveParser tokenizer(mSDA, GetSampleRate());

	Frame frm;

	U64 SampleBegin, SampleEnd;
	SWI_Token token;

	if (mSettings.mDecodeLevel == DL_Tokens)
	{
		// just read all the tokens
		for (;;)
		{
			token = tokenizer.GetToken(SampleBegin, SampleEnd);
			
			AddFrame(SampleBegin, SampleEnd, FrameToken, token);
		}

	} else if (mSettings.mDecodeLevel == DL_Bytes) {

		// sync to the first wake token
		tokenizer.GetWake(SampleBegin, SampleEnd);

		AddFrame(SampleBegin, SampleEnd, FrameToken, SWI_Wake);

		// now read bytes or wake tokens
		bool IsWake;
		for (;;)
		{
			U8 DataByte = tokenizer.GetByte(SampleBegin, SampleEnd, IsWake);

			if (IsWake)
				AddFrame(SampleBegin, SampleEnd, FrameToken, SWI_Wake);
			else
				AddFrame(SampleBegin, SampleEnd, FrameByte, DataByte);
		}

	} else if (mSettings.mDecodeLevel == DL_Packets) {

		// sync to the first wake token
		tokenizer.GetWake(SampleBegin, SampleEnd);

		AddFrame(SampleBegin, SampleEnd, FrameToken, SWI_Wake);

		// now read bytes and pack them into Flag frames and I/O blocks

		std::vector<std::pair<U64, U64> > ByteSamples;
		SWI_Block block;
		SWI_Opcode PrevOpcode = SWIO_Undefined;
		bool is_wake = false;
		U8 byte;

		for (;;)
		{
			try {
				// get the flag
				U8 Flag = tokenizer.GetByte(SampleBegin, SampleEnd, is_wake);

				if (is_wake)
				{
					AddFrame(SampleBegin, SampleEnd, FrameToken, SWI_Wake);
					throw WakeException();
				} else if (Flag == SWIF_Command  ||  Flag == SWIF_Transmit  ||  Flag == SWIF_Idle  ||  Flag == SWIF_Sleep) {
					AddFrame(SampleBegin, SampleEnd, FrameFlag, Flag, 1);
				} else {
					// add the error flag
					AddFrame(SampleBegin, SampleEnd, FrameFlag, Flag, 0);

					ResyncToWake(tokenizer);
				}

				// read the I/O block after the command or transmit flags
				if (Flag == SWIF_Command  ||  Flag == SWIF_Transmit)
				{
					ByteSamples.clear();
					block.Clear();

					// set the block opcode
					block.IsCommand = (Flag == SWIF_Command);
					if (!block.IsCommand)
						block.Opcode = PrevOpcode;

					// read the block byte count
					U8 Count = tokenizer.GetByte(SampleBegin, SampleEnd, is_wake);

					if (is_wake)
					{
						AddFrame(SampleBegin, SampleEnd, FrameToken, SWI_Wake);
						throw WakeException();
					}

					block.Data.push_back(Count);
					ByteSamples.push_back(std::make_pair(SampleBegin, SampleEnd));

					// we can't have a block without a data packet of at least
					// one byte meaning the block has to be at least 4 bytes long
					if (Count < 4)
						ResyncToWake(tokenizer);

					// read the bytes of the IO block
					int c;
					for (c = 0; c < Count - 1; c++)
					{
						byte = tokenizer.GetByte(SampleBegin, SampleEnd, is_wake);
						if (is_wake  &&  !ByteSamples.empty())
						{
							// make raw byte frames from the data read so far
							std::vector<std::pair<U64, U64> >::const_iterator bs_i(ByteSamples.begin());
							std::vector<U8>::const_iterator di(block.Data.begin());
							while (bs_i != ByteSamples.end())
							{
								AddFrame(bs_i->first, bs_i->second, FrameByte, *di);

								++di;
								++bs_i;
							}

							// add the wake token
							AddFrame(SampleBegin, SampleEnd, FrameToken, SWI_Wake);

							throw WakeException();
						}

						block.Data.push_back(byte);
						ByteSamples.push_back(std::make_pair(SampleBegin, SampleEnd));
					}

					// set the opcode in the block object
					if (block.IsCommand)
						block.Opcode = static_cast<SWI_Opcode>(block.Data[1]);

					// Add the Count frame
					AddFrame(ByteSamples.front().first, ByteSamples.front().second, FrameCount, Count);

					// add the block to the container
					size_t block_ndx = mResults->AddBlock(block);

					// parse the packet data
					ParsePacket(block, block_ndx, ByteSamples);

					// Add the checksum frame
					AddFrame((ByteSamples.end() - 2)->first, ByteSamples.back().second,
								FrameChecksum,
								block.GetChecksum(),
								block.CalcCRC());

					// remember the command for the next iteration
					if (block.IsCommand)
						PrevOpcode = block.Opcode;
					else
						PrevOpcode = SWIO_Undefined;
				}

			} catch (WakeException&) {
				// Just ignore the wake token. The exception thrown on an unexpected
				// wake token will only re-sync the parser to the next flag.
			}
		}
	}
}

void AtmelSWIAnalyzer::ParsePacket(const SWI_Block& block, size_t block_ndx, const std::vector<std::pair<U64, U64> >& ByteSamples)
{
	const U8 Count = block.GetCount();

	// is this a status/response block?
	if (Count == 4)
	{
		// make a status block frame
		AddFrame(ByteSamples[1].first, ByteSamples[1].second, FramePacketSegment, 0, 0x100000000ull | (U64)block_ndx);
	} else {
		int offset = 1;		// I/O block offset - we are skipping the Count byte
		int param_cnt = 0;
		SWI_PacketParam* param;
		for (param_cnt = 0; PacketParams[param_cnt].Name != NULL; ++param_cnt)
		{
			param = PacketParams + param_cnt;

			if (param->IsCommand == block.IsCommand  &&  param->ForOpcode == block.Opcode
					&&  (Count == param->ValidIfCount  ||  param->ValidIfCount == 0))
			{
				// pack the offset and block index into mData2
				U64 data2 = offset;
				data2 <<= 32;
				data2 |= block_ndx;

				AddFrame(ByteSamples[offset].first, ByteSamples[offset + param->Length - 1].second, FramePacketSegment, 
								param_cnt, data2);

				offset += param->Length;
			}

			param++;
		}
	}
}

bool AtmelSWIAnalyzer::NeedsRerun()
{
	return false;
}

U32 AtmelSWIAnalyzer::GenerateSimulationData(U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels)
{
	if (!mSimulationInitilized)
	{
		mSimulationDataGenerator.Initialize(GetSimulationSampleRate(), &mSettings);
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData(minimum_sample_index, device_sample_rate, simulation_channels);
}

U32 AtmelSWIAnalyzer::GetMinimumSampleRateHz()
{
    return 4000000;		// 4 MHz
}

void AtmelSWIAnalyzer::SetupResults()
{
    // reset the results
    mResults.reset(new AtmelSWIAnalyzerResults(this, &mSettings));
    SetAnalyzerResults(mResults.get());

    // set which channels will carry bubbles
    mResults->AddChannelBubblesWillAppearOn(mSettings.mSDAChannel);
}

const char* AtmelSWIAnalyzer::GetAnalyzerName() const
{
	return ::GetAnalyzerName();
}

const char* GetAnalyzerName()
{
	return "Atmel SWI";
}

Analyzer* CreateAnalyzer()
{
	return new AtmelSWIAnalyzer();
}

void DestroyAnalyzer(Analyzer* analyzer)
{
	delete analyzer;
}

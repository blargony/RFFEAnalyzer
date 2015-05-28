#include <AnalyzerChannelData.h>
#include <AnalyzerHelpers.h>

#include <algorithm>

#include "USBAnalyzer.h"
#include "USBAnalyzerResults.h"
#include "USBTypes.h"

std::string GetPIDName(USB_PID pid)
{
	switch (pid)
	{
	case PID_IN:		return "IN";
	case PID_OUT:		return "OUT";
	case PID_SOF:		return "SOF";
	case PID_SETUP:		return "SETUP";
	case PID_DATA0:		return "DATA0";
	case PID_DATA1:		return "DATA1";
	case PID_ACK:		return "ACK";
	case PID_NAK:		return "NAK";
	case PID_STALL:		return "STALL";
	case PID_PRE:		return "PRE";
	}

	return "<invalid>";
}

void USBPacket::Clear()
{
	data.clear();
	bit_begin_samples.clear();
	sample_begin = sample_end = 0;
	PID = 0;
	CRC = 0;
}

bool USBPacket::IsPIDValid() const
{
	return PID == PID_IN
			|| PID == PID_OUT
			|| PID == PID_SOF
			|| PID == PID_SETUP
			|| PID == PID_DATA0
			|| PID == PID_DATA1
			|| PID == PID_ACK
			|| PID == PID_NAK
			|| PID == PID_STALL
			|| PID == PID_PRE;
}

U8 USBPacket::CalcCRC5(U16 data)
{
	U8 crc_register = 0x1f;
	U8 polynom = 0x14;		// 0b10100

	U8 data_bit, crc_bit;
	U16 shift_register;

	// only the lower 11 bits of the 16 bit number
	for (shift_register = 1; shift_register <= 0x400; shift_register <<= 1)
	{
		data_bit = (data & shift_register) ? 1 : 0;
		crc_bit = crc_register & 1;

		crc_register >>= 1;

		if (data_bit ^ crc_bit)
			crc_register ^= polynom;
	}

	return (~crc_register) & 0x1f;
}

U16 USBPacket::CalcCRC16() const
{
	size_t counter;
	U16 crc_register = 0xffff;
	U16 polynom = 0xA001;
	U8 shift_register, data_bit, crc_bit;

	for (counter = 2; counter < data.size() - 2; counter++)
	{
		for (shift_register = 0x01; shift_register > 0x00; shift_register <<= 1)
		{
			data_bit = (data[counter] & shift_register) ? 1 : 0;
			crc_bit = crc_register & 1;

			crc_register >>= 1;

			if (data_bit ^ crc_bit)
				crc_register ^= polynom;
		}
	}

	return ~crc_register;
}

U64 USBPacket::AddFrames(USBAnalyzerResults* pResults)
{
	// make the analyzer frames for this packet
	Frame f;

	// SYNC
	f.mStartingSampleInclusive = bit_begin_samples.front();
	f.mEndingSampleInclusive = *(bit_begin_samples.begin() + 8);
	f.mType = FT_SYNC;
	f.mData1 = f.mData2 = 0;
	f.mFlags = 0;
	pResults->AddFrame(f);

	// PID
	f.mStartingSampleInclusive = *(bit_begin_samples.begin() + 8);
	f.mEndingSampleInclusive = *(bit_begin_samples.begin() + 16);
	f.mType = FT_PID;
	f.mData1 = PID;
	f.mData2 = 0;
	f.mFlags = 0;
	pResults->AddFrame(f);

	// do the payload & CRC frames
	if (IsTokenPacket()  ||  IsSOFPacket())
	{
		// address/endpoint  or  frame number
		f.mStartingSampleInclusive = *(bit_begin_samples.begin() + 16);
		f.mEndingSampleInclusive = *(bit_begin_samples.begin() + 27);

		// is this a SOF packet?
		if (PID == PID_SOF)
		{
			f.mType = FT_FrameNum;
			f.mData1 = GetFrameNum();
			f.mData2 = 0;
		} else {
			f.mType = FT_AddrEndp;
			f.mData1 = GetAddress();
			f.mData2 = GetEndpoint();
		}

		pResults->AddFrame(f);

		// CRC5
		f.mStartingSampleInclusive = *(bit_begin_samples.begin() + 27);
		f.mEndingSampleInclusive = bit_begin_samples.back();

		f.mType = FT_CRC5;
		f.mData1 = CRC;
		f.mData2 = CalcCRC5(GetLastWord() & 0x7ff);
		pResults->AddFrame(f);

	} else if (IsDataPacket()) {

		//debug("DATA: " + int2str(bit_begin_samples.back()));

		// raw data
		size_t bc;
		f.mType = FT_Byte;
		for (bc = 2; bc < data.size() - 2; ++bc)
		{
			f.mStartingSampleInclusive = *(bit_begin_samples.begin() + bc * 8);
			f.mEndingSampleInclusive = *(bit_begin_samples.begin() + (bc + 1) * 8);
			f.mData1 = data[bc];
			pResults->AddFrame(f);
		}

		// CRC16
		f.mStartingSampleInclusive = *(bit_begin_samples.end() - 17);
		f.mEndingSampleInclusive = bit_begin_samples.back();

		f.mType = FT_CRC16;
		f.mData1 = CRC;
		f.mData2 = CalcCRC16();
		pResults->AddFrame(f);
	}

	// debug invalid CRCs
	//if (data.size() > 2  &&  f.mData1 != f.mData2)
	//	debug("bad CRC " + GetPIDName((USB_PID) PID) + "  " + int2str(bit_begin_samples.back()) + "  rcvd: " + int2str(f.mData1) + "  calc: " + int2str(f.mData2));

	// add the EOP frame
	f.mStartingSampleInclusive = bit_begin_samples.back();
	// EOP is 2 bits SE0 and one bit J, so add another bit
	f.mEndingSampleInclusive = sample_end;
	f.mData1 = f.mData2 = 0;
	f.mFlags = 0;
	f.mType = FT_EOP;
	pResults->AddFrame(f);

	pResults->CommitResults();

	return f.mEndingSampleInclusive;
}

U64 USBPacket::AddRawByteFrames(USBAnalyzerResults* pResults)
{
	// raw data
	size_t bc;
	Frame f;
	f.mType = FT_Byte;
	f.mData2 = 0;
	f.mFlags = 0;
	std::string bytes_row;
	for (bc = 0; bc < data.size(); ++bc)
	{
		bytes_row += int2str_sal(data[bc], Hexadecimal, 8) + ", ";

		f.mStartingSampleInclusive = *(bit_begin_samples.begin() + bc * 8);
		f.mEndingSampleInclusive = *(bit_begin_samples.begin() + (bc + 1) * 8);
		f.mData1 = data[bc];
		pResults->AddFrame(f);
	}

	// add the EOP frame
	f.mStartingSampleInclusive = bit_begin_samples.back();
	// EOP is 2 bits SE0 and one bit J, so add another bit
	f.mEndingSampleInclusive = sample_end;
	f.mData1 = f.mData2 = 0;
	f.mFlags = 0;
	f.mType = FT_EOP;
	pResults->AddFrame(f);

	pResults->CommitResults();

	return f.mEndingSampleInclusive;
}

U64 USBPacket::AddErrorFrame(USBAnalyzerResults* pResults)
{
	// add the Error frame -- parser can't decode the packet
	Frame f;
	f.mStartingSampleInclusive = sample_begin;
	f.mEndingSampleInclusive = sample_end;
	f.mData1 = f.mData2 = 0;
	f.mFlags = 0;
	f.mType = FT_Error;
	pResults->AddFrame(f);

	pResults->CommitResults();

	return f.mEndingSampleInclusive;
}

void USBSignalState::AddFrame(USBAnalyzerResults* res)
{
	Frame f;
	f.mStartingSampleInclusive = sample_begin;
	f.mEndingSampleInclusive = sample_end;
	f.mType = FT_Signal;
	f.mData1 = state;
	f.mData2 = 0;

	res->AddFrame(f);
	res->CommitResults();
}

USBSignalFilter::USBSignalFilter(USBAnalyzer* pAnalyzer, USBAnalyzerResults* pResults, USBAnalyzerSettings* pSettings,
					AnalyzerChannelData* pDP, AnalyzerChannelData* pDM, USBSpeed speed)
	: mAnalyzer(pAnalyzer), mResults(pResults), mSettings(pSettings), mDP(pDP), mDM(pDM), mSpeed(speed),
		mSampleDur(1e9 / mAnalyzer->GetSampleRate())
{
	mStateStartSample = mDP->GetSampleNumber();
}

bool USBSignalFilter::SkipGlitch(AnalyzerChannelData* pChannel)
{
	if (mSampleDur > 20		// sample rate < 50Mhz?
			||  mSpeed == FULL_SPEED)
		return false;

	// up to 20ns
	const int IGNORE_PULSE_SAMPLES = mSampleDur == 10 ? 2 : 1;

	// skip the glitch
	if (pChannel->WouldAdvancingCauseTransition(IGNORE_PULSE_SAMPLES))
	{
		//debug("start: " + int2str(pChannel->GetSampleNumber()));
		pChannel->AdvanceToNextEdge();
		//debug("end: " + int2str(pChannel->GetSampleNumber()));

		return true;
	}

	return false;
}

U64 USBSignalFilter::DoFilter(AnalyzerChannelData* mDP, AnalyzerChannelData* mDM)
{
	AnalyzerChannelData* pFurther;
	AnalyzerChannelData* pNearer;
	U64 next_edge_further;
	U64 next_edge_nearer;

	// this loop consumes all short (1 sample) pulses caused by noise
	do {
		if (mDP->GetSampleOfNextEdge() > mDM->GetSampleOfNextEdge())
			pFurther = mDP, pNearer = mDM;
		else
			pFurther = mDM, pNearer = mDP;

		next_edge_further = pFurther->GetSampleOfNextEdge();
		next_edge_nearer = pNearer->GetSampleOfNextEdge();

		pNearer->AdvanceToNextEdge();
		pFurther->AdvanceToAbsPosition(next_edge_nearer);
	} while (SkipGlitch(pNearer));

	const int FILTER_THLD = mSettings->mSpeed == LOW_SPEED ? 200 : 50;	// filtering threshold in ns

	U64 diff_samples = next_edge_further - next_edge_nearer;
	// if there's a pulse on pNearer and no transition on pFurther
	if (!pNearer->WouldAdvancingToAbsPositionCauseTransition(next_edge_further)	
				// if the transitions happened withing FILTER_THLD time of eachother
				&&  diff_samples * mSampleDur <= FILTER_THLD)	
	{
		for (;;)
		{
			pNearer->AdvanceToAbsPosition(next_edge_further);
			pFurther->AdvanceToAbsPosition(next_edge_further);

			if (!SkipGlitch(pFurther))
				break;

			//debug("time: " + mAnalyzer->GetTimeStr(pFurther->GetSampleNumber()) + " sample: " + int2str(pFurther->GetSampleNumber()));
			pFurther->AdvanceToNextEdge();
			next_edge_further = pFurther->GetSampleNumber();
		}

		// return the filtered position of the transition
		return (next_edge_nearer + next_edge_further) / 2;
	}

	// return state up until the nearer transition
	return next_edge_nearer;
}

USBSignalState USBSignalFilter::GetState()
{
	USBSignalState ret_val;

	ret_val.sample_begin = mStateStartSample;

	// determine the USB signal state
	BitState dp_state = mDP->GetBitState();
	BitState dm_state = mDM->GetBitState();
	if (dp_state == dm_state)
		ret_val.state = dp_state == BIT_LOW ? S_SE0 : S_SE1;
	else
		ret_val.state = (mSpeed == LOW_SPEED ?
							(dp_state == BIT_LOW ? S_J : S_K)
							:	(dp_state == BIT_LOW ? S_K : S_J));

	// do the filtering and remember the sample begin for the next iteration
	mStateStartSample = DoFilter(mDP, mDM);

	ret_val.dur = (mStateStartSample - ret_val.sample_begin) * mSampleDur;
	ret_val.sample_end = mStateStartSample;

	return ret_val;
}

bool USBSignalFilter::HasMoreData()
{
	return mDP->DoMoreTransitionsExistInCurrentData()  ||  mDM->DoMoreTransitionsExistInCurrentData();
}

bool USBSignalFilter::GetPacket(USBPacket& pckt, USBSignalState& sgnl)
{
	pckt.Clear();

	const U8 bits2add[] = {0, 1, 1, 1, 1, 1, 1};

	const double BIT_DUR = (mSpeed == FULL_SPEED ? FS_BIT_DUR : LS_BIT_DUR);
	const double SAMPLE_DUR = 1e9 / mAnalyzer->GetSampleRate();	// 1 sample duration in ns
	const double BIT_SAMPLES = BIT_DUR / SAMPLE_DUR;

	std::vector<U8> bits;

	pckt.sample_begin = sgnl.sample_begin;	// default for bad packets

	bool is_stuff_bit = false;
	while (sgnl.IsData(mSpeed))
	{
		// get the number of bits in this signal
		int num_bits = sgnl.GetNumBits(mSpeed);

		const U8* add_begin = bits2add;
		const U8* add_end = bits2add + num_bits;

		// mind the stuffing bit
		if (is_stuff_bit)
			++add_begin;

		// now add the data bits
		bits.insert(bits.end(), add_begin, add_end);

		// do the bit markers and remember the samples at which a bit begins
		int bc;
		for (bc = 0; bc < num_bits; ++bc)
		{
			if (!is_stuff_bit  ||  bc > 0)
				pckt.bit_begin_samples.push_back(sgnl.sample_begin + U64(BIT_SAMPLES * bc + .5));

			mResults->AddMarker(sgnl.sample_begin + U64(BIT_SAMPLES * (bc + .5) + .5),
										bc == 0 ? (is_stuff_bit ? AnalyzerResults::ErrorX : AnalyzerResults::Zero) 
												: AnalyzerResults::One,
												mSettings->mDPChannel);
		}

		// set the stuff bit flag for the next state
		is_stuff_bit = (num_bits == 7);

		// get the next signal state
		sgnl = GetState();
	}

	pckt.sample_end = sgnl.sample_end;		// default for bad packets

	// add another element to bit_begin_samples to mark the end of the last bit
	pckt.bit_begin_samples.push_back(sgnl.sample_begin);

	// check the number of bits
	if (bits.empty()
			||  (bits.size() % 8) != 0
			||  (pckt.bit_begin_samples.size() % 8) != 1)
	{
		//debug("incorrect number of bits " + int2str(bits.size()));
		return false;
	}

	// remember the begin & end samples for the entire packet
	pckt.sample_begin = pckt.bit_begin_samples.front();
	pckt.sample_end = sgnl.sample_end + S64(BIT_DUR / SAMPLE_DUR + 0.5);

	// make bytes out of these bits
	U8 val = 0;
	std::vector<U8>::iterator i(bits.begin());
	while (i != bits.end())
	{
		// least significant bit first
		val >>= 1;
		if (*i)
			val |= 0x80;

		++i;

		// do we have a full byte?
		if (((i - bits.begin()) % 8) == 0)
			pckt.data.push_back(val);
	}

	// extract the PID
	if (pckt.data.size() > 1)
		pckt.PID = pckt.data[1];

	// extract the CRC fields
	if (pckt.data.size() >= 4)
	{
		U16 last_word = (pckt.data.back() << 8) | *(pckt.data.end() - 2);

		if (pckt.data.size() == 4)
			pckt.CRC = last_word >> 11;
		else
			pckt.CRC = last_word;
	}

	//debug("bit_begin_samples.size()=" + int2str(pckt.bit_begin_samples.size()));
	//debug("data.size()=" + int2str(pckt.data.size()));

	return (pckt.IsTokenPacket()  &&  pckt.data.size() == 4)
				||  (pckt.IsDataPacket()  &&  pckt.data.size() >= 4)
				||  (pckt.IsHandshakePacket()  &&  pckt.data.size() == 2)
				||  (pckt.IsSOFPacket()  &&  pckt.data.size() == 4);
}

std::string int2str_sal(const U64 i, DisplayBase base, const int max_bits)
{
	char number_str[256];
	AnalyzerHelpers::GetNumberString(i, base, max_bits, number_str, sizeof(number_str));
	return number_str;
}
#include <cassert>

#include "SMBusSimulationDataGenerator.h"
#include "SMBusAnalyzerSettings.h"
#include "SMBusTypes.h"
#include "SMBusCommands.h"

SMBusSimulationDataGenerator::SMBusSimulationDataGenerator()
{}

SMBusSimulationDataGenerator::~SMBusSimulationDataGenerator()
{}

void SMBusSimulationDataGenerator::Initialize(U32 simulation_sample_rate, SMBusAnalyzerSettings* settings)
{
	mSimulationSampleRateHz = simulation_sample_rate;
	mSettings = settings;

	// 50KHz, but it's not really used for AdvanceByHalfPeriod
	mClockGenerator.Init(50000, simulation_sample_rate);

	mSMBDAT = mSMBSimulationChannels.Add(settings->mSMBDAT, mSimulationSampleRateHz, BIT_HIGH);
	mSMBCLK = mSMBSimulationChannels.Add(settings->mSMBCLK, mSimulationSampleRateHz, BIT_HIGH);

	AdvanceAllBySec(ONE_MS * 0.5);	// insert a pause before we start
}

U32 SMBusSimulationDataGenerator::GenerateSimulationData(U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels)
{
	U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample( largest_sample_requested, sample_rate, mSimulationSampleRateHz );

	while (mSMBCLK->GetCurrentSampleNumber() < adjusted_largest_sample_requested)
	{
		CreateSMBusTransaction();
		AdvanceAllBySec(ONE_MS * 2);
	}

	*simulation_channels = mSMBSimulationChannels.GetArray();

	return mSMBSimulationChannels.GetCount();
}

void SMBusSimulationDataGenerator::CreateSMBusTransaction()
{
	OutputQuickCommand();
	OutputSendByte(0x01);
	OutputRecvByte(0x02);

	if (mSettings->mDecodeLevel == DL_PMBus)
	{
		OutputProcessCallPMBusCoefficients();
		OutputReadByte(CAPABILITY, 0xB0);
		OutputBlockProcessCallPMBusQuery();

		OutputWriteByte(WRITE_PROTECT, 0x20);
		OutputReadByte(WRITE_PROTECT, 0x00);

		OutputWriteByte(OPERATION, 0x20);
		OutputReadByte(OPERATION, 0x00);

		OutputWriteByte(ON_OFF_CONFIG, 0xff);
		OutputReadByte(ON_OFF_CONFIG, 0xff);

		OutputWriteByte(VOUT_MODE, 0xff);
		OutputReadByte(VOUT_MODE, 0xff);

		OutputPMBusGroupCommand();

	} else if (mSettings->mDecodeLevel == DL_SmartBattery) {

		OutputWriteWord(SBC_ManufacturerAccess, 0x1234);
		OutputReadWord(SBC_ManufacturerAccess, 0x5678);

		OutputWriteWord(SBC_BatteryMode, 0xabcd);
		OutputReadWord(SBC_BatteryStatus, 0xabcd);

		OutputReadWord(SBC_SpecificationInfo, 0xabcd);
		OutputReadWord(SBC_ManufactureDate, 0x1234);

	} else {

		OutputQuickCommand();
		OutputSendByte(0xae);
		OutputRecvByte(0x66);
		OutputWriteByte(0x83, 0x73);
		OutputReadByte(0x84, 0x91);
		OutputWriteWord(0x12, 0x92);
		OutputReadWord(0x05, 0x62);
	}
}

void SMBusSimulationDataGenerator::OutputStart()
{
	// get SMBDAT high
	if (mSMBDAT->GetCurrentBitState() == BIT_LOW)
	{
		AdvanceAllBySec(ONE_US * 5);
		mSMBDAT->Transition();
	}

	// get SMBCLK high
	if (mSMBCLK->GetCurrentBitState() == BIT_LOW)
	{
		AdvanceAllBySec(TSU_STA_MIN);
		mSMBCLK->Transition();
	}

	// SMBDAT goes low
	AdvanceAllBySec(ONE_US * 10);
	mSMBDAT->Transition();

	// SMBCLK goes low
	AdvanceAllBySec(ONE_US * 10);
	mSMBCLK->Transition();

	// pause a little
	AdvanceAllBySec(TBUF_MIN * 2);
}

void SMBusSimulationDataGenerator::OutputStop()
{
	// SMBCLK must be low
	assert(mSMBCLK->GetCurrentBitState() == BIT_LOW);

	// get SMBDAT low
	if (mSMBDAT->GetCurrentBitState() == BIT_HIGH)
	{
		AdvanceAllBySec(ONE_US * 8);	// insert a pause before we start
		mSMBDAT->Transition();
	}

	// SMBCLK goes high
	AdvanceAllBySec(ONE_US * 8);
	mSMBCLK->Transition();

	// SMBDAT goes high
	AdvanceAllBySec(ONE_US * 8);
	mSMBDAT->Transition();
}

void SMBusSimulationDataGenerator::OutputBit(BitState state)
{
	if (state == BIT_HIGH)
		mSMBDAT->TransitionIfNeeded(BIT_HIGH);
	else
		mSMBDAT->TransitionIfNeeded(BIT_LOW);

	AdvanceAllBySec(ONE_US * 5);

	mSMBCLK->Transition();	// go high
	AdvanceAllBySec(ONE_US * 10);
	mSMBCLK->Transition();	// go low

	AdvanceAllBySec(ONE_US * 5);
}

U8 SMBusSimulationDataGenerator::OutputByte(const U8 byte, bool is_ack)
{
	// SMBCLK must be low
	assert(mSMBCLK->GetCurrentBitState() == BIT_LOW);

	for (int mask = 0x80; mask; mask >>= 1)
		OutputBit((byte & mask) ? BIT_HIGH : BIT_LOW);

	OutputBit(is_ack ? BIT_LOW : BIT_HIGH);

	// add a small pause
	AdvanceAllBySec(ONE_US * 15);

	return byte;
}

U8 SMBusSimulationDataGenerator::OutputAddr(U8 addr, bool is_read, bool is_ack)
{
	U8 byte = (addr << 1) | (is_read ? 1 : 0);
	OutputByte(byte, is_ack);
	return byte;
}

void SMBusSimulationDataGenerator::OutputQuickCommand()
{
	// quick command has no PEC

	OutputStart();
	OutputAddr(0x41, true);
	OutputStop();

	AdvanceAllBySec(ONE_MS * 0.5);
}

void SMBusSimulationDataGenerator::OutputSendByte(U8 byte)
{
	U8 pec = 0;

	OutputStart();
	pec = SMBusCRCLookup[pec ^ OutputAddr(0x42, false)];
	pec = SMBusCRCLookup[pec ^ OutputByte(byte, mSettings->CalcPEC())];
	if (mSettings->CalcPEC())
		OutputByte(pec, false);
	OutputStop();

	AdvanceAllBySec(ONE_MS * 0.5);
}

void SMBusSimulationDataGenerator::OutputRecvByte(U8 byte)
{
	U8 pec = 0;

	OutputStart();
	pec = SMBusCRCLookup[pec ^ OutputAddr(0x43, true)];
	pec = SMBusCRCLookup[pec ^ OutputByte(byte, mSettings->CalcPEC())];
	if (mSettings->CalcPEC())
		OutputByte(pec, false);
	OutputStop();

	AdvanceAllBySec(ONE_MS * 0.5);
}

void SMBusSimulationDataGenerator::OutputWriteByte(U8 command, U8 data_byte)
{
	U8 pec = 0;

	OutputStart();
	pec = SMBusCRCLookup[pec ^ OutputAddr(0x43, false)];
	pec = SMBusCRCLookup[pec ^ OutputByte(command)];
	pec = SMBusCRCLookup[pec ^ OutputByte(data_byte, mSettings->CalcPEC())];
	if (mSettings->CalcPEC())
		OutputByte(pec, false);
	OutputStop();

	AdvanceAllBySec(ONE_MS * 0.5);
}

void SMBusSimulationDataGenerator::OutputReadByte(U8 command, U8 data_byte)
{
	U8 pec = 0;

	OutputStart();
	pec = SMBusCRCLookup[pec ^ OutputAddr(0x43, false)];
	pec = SMBusCRCLookup[pec ^ OutputByte(command)];
	OutputStart();
	pec = SMBusCRCLookup[pec ^ OutputAddr(0x43, true)];
	pec = SMBusCRCLookup[pec ^ OutputByte(data_byte, mSettings->CalcPEC())];
	if (mSettings->CalcPEC())
		OutputByte(pec, false);
	OutputStop();

	AdvanceAllBySec(ONE_MS * 0.5);
}

void SMBusSimulationDataGenerator::OutputWriteWord(U8 command, U16 data_word)
{
	U8 pec = 0;

	OutputStart();
	pec = SMBusCRCLookup[pec ^ OutputAddr(0x43, false)];
	pec = SMBusCRCLookup[pec ^ OutputByte(command)];
	pec = SMBusCRCLookup[pec ^ OutputByte(U8(data_word >> 8))];
	pec = SMBusCRCLookup[pec ^ OutputByte(U8(data_word & 0xff), mSettings->CalcPEC())];
	if (mSettings->CalcPEC())
		OutputByte(pec, false);
	OutputStop();

	AdvanceAllBySec(ONE_MS * 0.5);
}

void SMBusSimulationDataGenerator::OutputReadWord(U8 command, U16 data_word)
{
	U8 pec = 0;

	OutputStart();
	pec = SMBusCRCLookup[pec ^ OutputAddr(0x43, false)];
	pec = SMBusCRCLookup[pec ^ OutputByte(command)];
	OutputStart();
	pec = SMBusCRCLookup[pec ^ OutputAddr(0x43, true)];
	pec = SMBusCRCLookup[pec ^ OutputByte(U8(data_word >> 8))];
	pec = SMBusCRCLookup[pec ^ OutputByte(U8(data_word & 0xff), mSettings->CalcPEC())];
	if (mSettings->CalcPEC())
		OutputByte(pec, false);
	OutputStop();

	AdvanceAllBySec(ONE_MS * 0.5);
}

void SMBusSimulationDataGenerator::OutputProcessCallPMBusRevision()
{
	U8 pec = 0;

	OutputStart();
	pec = SMBusCRCLookup[pec ^ OutputAddr(0x43, false)];
	pec = SMBusCRCLookup[pec ^ OutputByte(PMBUS_REVISION)];
	OutputStart();
	pec = SMBusCRCLookup[pec ^ OutputAddr(0x43, true)];
	pec = SMBusCRCLookup[pec ^ OutputByte(0x11, mSettings->CalcPEC())];
	if (mSettings->CalcPEC())
		OutputByte(pec, false);
	OutputStop();

	AdvanceAllBySec(ONE_MS * 0.5);
}

void SMBusSimulationDataGenerator::OutputBlockProcessCallPMBusQuery()
{
	U8 pec = 0;

	OutputStart();
	pec = SMBusCRCLookup[pec ^ OutputAddr(0x43, false)];
	pec = SMBusCRCLookup[pec ^ OutputByte(QUERY)];
	pec = SMBusCRCLookup[pec ^ OutputByte(0x01)];			// block command byte count
	pec = SMBusCRCLookup[pec ^ OutputByte(CLEAR_FAULTS)];	// 1 data byte is the command to query
	OutputStart();
	pec = SMBusCRCLookup[pec ^ OutputAddr(0x43, true)];
	pec = SMBusCRCLookup[pec ^ OutputByte(0x01)];			// block command byte count
	pec = SMBusCRCLookup[pec ^ OutputByte(0x98, mSettings->CalcPEC())];			// the result
	if (mSettings->CalcPEC())
		OutputByte(pec, false);
	OutputStop();

	AdvanceAllBySec(ONE_MS * 0.5);
}

void SMBusSimulationDataGenerator::OutputPMBusGroupCommand()
{
	U8 pec;

	OutputStart();
	pec = 0;
	pec = SMBusCRCLookup[pec ^ OutputAddr(0x43, false)];
	pec = SMBusCRCLookup[pec ^ OutputByte(VOUT_MARGIN_HIGH)];
	pec = SMBusCRCLookup[pec ^ OutputByte(0x01)];
	pec = SMBusCRCLookup[pec ^ OutputByte(0x24)];
	if (mSettings->CalcPEC())
		OutputByte(pec);

	OutputStart();
	pec = 0;
	pec = SMBusCRCLookup[pec ^ OutputAddr(0x44, false)];
	pec = SMBusCRCLookup[pec ^ OutputByte(VOUT_MARGIN_HIGH)];
	pec = SMBusCRCLookup[pec ^ OutputByte(0x73)];
	pec = SMBusCRCLookup[pec ^ OutputByte(0x28)];
	if (mSettings->CalcPEC())
		OutputByte(pec);

	OutputStart();
	pec = 0;
	pec = SMBusCRCLookup[pec ^ OutputAddr(0x45, false)];
	pec = SMBusCRCLookup[pec ^ OutputByte(VOUT_MARGIN_HIGH)];
	pec = SMBusCRCLookup[pec ^ OutputByte(0x24)];
	pec = SMBusCRCLookup[pec ^ OutputByte(0x88)];
	if (mSettings->CalcPEC())
		OutputByte(pec);

	OutputStart();
	pec = 0;
	pec = SMBusCRCLookup[pec ^ OutputAddr(0x46, false)];
	pec = SMBusCRCLookup[pec ^ OutputByte(VOUT_MODE)];
	pec = SMBusCRCLookup[pec ^ OutputByte(0x11)];
	if (mSettings->CalcPEC())
		OutputByte(pec);

	OutputStop();

	AdvanceAllBySec(ONE_MS * 0.5);
}

void SMBusSimulationDataGenerator::OutputProcessCallPMBusCoefficients()
{
	U8 pec = 0;

	OutputStart();
	pec = SMBusCRCLookup[pec ^ OutputAddr(0x43, false)];
	pec = SMBusCRCLookup[pec ^ OutputByte(COEFFICIENTS)];
	pec = SMBusCRCLookup[pec ^ OutputByte(0x02)];
	pec = SMBusCRCLookup[pec ^ OutputByte(0x03)];
	pec = SMBusCRCLookup[pec ^ OutputByte(0x24)];
	OutputStart();
	pec = SMBusCRCLookup[pec ^ OutputAddr(0x43, true)];
	pec = SMBusCRCLookup[pec ^ OutputByte(0x05)];	// byte count
	pec = SMBusCRCLookup[pec ^ OutputByte(0x12)];	// m: low
	pec = SMBusCRCLookup[pec ^ OutputByte(0x23)];	// m: high
	pec = SMBusCRCLookup[pec ^ OutputByte(0x34)];	// b: low
	pec = SMBusCRCLookup[pec ^ OutputByte(0x45)];	// b: high
	pec = SMBusCRCLookup[pec ^ OutputByte(0x56)];	// R
	if (mSettings->CalcPEC())
		OutputByte(pec, false);
	OutputStop();

	AdvanceAllBySec(ONE_MS * 0.5);
}

#include "AtmelSWISimulationDataGenerator.h"
#include "AtmelSWIAnalyzerSettings.h"

#include "AtmelSWITypes.h"

#define MICRO_SEC	1e-6
#define MILLI_SEC	1e-3

// This is the data to be sent to/from the ATSHA204

U8 StatusBlock[] = {0x04, 0x11, 0x33, 0x43};

U8 CheckMacBlockSent[] =
{	0x54,			// Count
	0x28,			// Opcode
	0x00,			// Mode
	0x00, 0x00,		// KeyID
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,		// ClientChal
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,		// ClientResp
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// OtherData

	0x7F, 0x59		// Checksum
};

U8 CheckMacBlockReceived[] =	{0x04, 0x01, 0x00, 0xC3};

U8 DeriveKeyBlockSent[] =		{0x07, 0x1C, 0x00, 0x00, 0x00, 0x0A, 0x4D};
U8 DeriveKeyBlockReceived[] =	{0x04, 0x0F, 0x23, 0x42};

U8 DevRevBlockSent[] =			{0x07, 0x30, 0x00, 0x00, 0x00, 0x03, 0x5D};
U8 DevRevBlockReceived[] =		{0x07, 0x00, 0x00, 0x00, 0x01, 0x00, 0x2E};

U8 GenDigBlockSent[] =			{0x07, 0x15, 0x02, 0x00, 0x00, 0x30, 0x08};
U8 GenDigBlockReceived[] =		{0x04, 0x0F, 0x23, 0x42};

U8 HMACBlockSent[] =			{0x07, 0x11, 0x00, 0x00, 0x00, 0x3F, 0x0D};
U8 HMACBlockReceived[] =		{0x04, 0x0F, 0x23, 0x42};

U8 LockBlockSent[] =			{0x07, 0x17, 0x00, 0x00, 0x00, 0x2E, 0x0D};
U8 LockBlockReceived[] =		{0x04, 0x0F, 0x23, 0x42};

U8 MacBlockSent[] =				{0x07, 0x08, 0x00, 0x00, 0x00, 0x05, 0xED};
U8 MacBlockReceived[] =			{0x23, 0xBD, 0x8C, 0x06, 0x10, 0xB1, 0x1D, 0xE5, 0x79, 0x24, 0x60, 0x0A, 0xD3, 0x1D, 0xEA, 0x2E, 0x01, 0x61, 0x23, 0x28, 0x9C, 0x5C, 0xCD, 0x88, 0xFB, 0x45, 0x6B, 0x13, 0x8B, 0x4C, 0x88, 0xEE, 0x59, 0xD7, 0xA6};

U8 NonceBlockSent[] =			{0x1B, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7D, 0xE0};
U8 NonceBlockReceived[] =		{0x23, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x41, 0x1A};

U8 RandomBlockSent[] =			{0x07, 0x1B, 0x00, 0x00, 0x00, 0x24, 0xCD};
U8 RandomBlockReceived[] =		{0x23, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x41, 0x1A};

U8 ReadBlockSent[] =			{0x07, 0x02, 0x00, 0x00, 0x00, 0x1E, 0x2D};
U8 ReadBlockReceived[] =		{0x07, 0x01, 0x23, 0x4A, 0xB6, 0x04, 0xCF};

U8 WriteBlockSent[] =			{0x0B, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA7, 0xCF};
U8 WriteBlockReceived[] =		{0x04, 0x03, 0x83, 0x42};

U8 UpdateExtraBlockSent[] =		{0x07, 0x20, 0x00, 0x00, 0x00, 0x00, 0x7D};
U8 UpdateExtraBlockReceived[] =	{0x04, 0x00, 0x03, 0x40};

U8 PauseBlockSent[] =			{0x07, 0x01, 0x00, 0x00, 0x00, 0x3C, 0x2D};
U8 PauseBlockReceived[] =		{0x04, 0x00, 0x03, 0x40};


AtmelSWISimulationDataGenerator::AtmelSWISimulationDataGenerator()
{}

AtmelSWISimulationDataGenerator::~AtmelSWISimulationDataGenerator()
{}

void AtmelSWISimulationDataGenerator::Initialize(U32 simulation_sample_rate, AtmelSWIAnalyzerSettings* settings)
{
	mSimulationSampleRateHz = simulation_sample_rate;
	mSettings = settings;

	mClockGenerator.Init(simulation_sample_rate / 10, simulation_sample_rate);

	mSDA = mSWISimulationChannels.Add(settings->mSDAChannel, mSimulationSampleRateHz, BIT_HIGH);

	mSWISimulationChannels.AdvanceAll(mClockGenerator.AdvanceByTimeS(MILLI_SEC));	// insert a pause before we start
}

U32 AtmelSWISimulationDataGenerator::GenerateSimulationData(U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels)
{
	U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample(largest_sample_requested, sample_rate, mSimulationSampleRateHz);

	while (mSDA->GetCurrentSampleNumber() < adjusted_largest_sample_requested)
	{
		CreateSWITransaction(CheckMacBlockSent, CheckMacBlockReceived);
		CreateSWITransaction(DeriveKeyBlockSent, DeriveKeyBlockReceived);
		CreateSWITransaction(DevRevBlockSent, DevRevBlockReceived);
		CreateSWITransaction(GenDigBlockSent, GenDigBlockReceived);
		CreateSWITransaction(HMACBlockSent, HMACBlockReceived);
		CreateSWITransaction(LockBlockSent, LockBlockReceived);
		CreateSWITransaction(MacBlockSent, MacBlockReceived);
		CreateSWITransaction(NonceBlockSent, NonceBlockReceived);
		CreateSWITransaction(PauseBlockSent, PauseBlockReceived);
		CreateSWITransaction(RandomBlockSent, RandomBlockReceived);
		CreateSWITransaction(ReadBlockSent, ReadBlockReceived);
		CreateSWITransaction(ReadBlockSent, ReadBlockReceived);
		CreateSWITransaction(UpdateExtraBlockSent, UpdateExtraBlockReceived);
		CreateSWITransaction(WriteBlockSent, WriteBlockReceived);
	}

	*simulation_channels = mSWISimulationChannels.GetArray();

	return mSWISimulationChannels.GetCount();
}

void AtmelSWISimulationDataGenerator::OutputTokenWake()
{
	mSDA->Transition();		// to low
	mSWISimulationChannels.AdvanceAll(mClockGenerator.AdvanceByTimeS(MICRO_SEC * 70));
	mSDA->Transition();		// to high
	mSWISimulationChannels.AdvanceAll(mClockGenerator.AdvanceByTimeS(MILLI_SEC * 3));
}

void AtmelSWISimulationDataGenerator::OutputTokenZero()
{
	mSDA->Transition();		// to low
	mSWISimulationChannels.AdvanceAll(mClockGenerator.AdvanceByTimeS(MICRO_SEC * 4.34));
	mSDA->Transition();		// to high
	mSWISimulationChannels.AdvanceAll(mClockGenerator.AdvanceByTimeS(MICRO_SEC * 4.34));
	mSDA->Transition();		// to low
	mSWISimulationChannels.AdvanceAll(mClockGenerator.AdvanceByTimeS(MICRO_SEC * 4.34));
	mSDA->Transition();		// to high
	mSWISimulationChannels.AdvanceAll(mClockGenerator.AdvanceByTimeS(MICRO_SEC * 26));
}

void AtmelSWISimulationDataGenerator::OutputTokenOne()
{
	mSDA->Transition();		// to low
	mSWISimulationChannels.AdvanceAll(mClockGenerator.AdvanceByTimeS(MICRO_SEC * 4.34));
	mSDA->Transition();		// to high
	mSWISimulationChannels.AdvanceAll(mClockGenerator.AdvanceByTimeS(MICRO_SEC * 34.66));
}

void AtmelSWISimulationDataGenerator::OutputByte(U8 byte)
{
	int c;
	for (c = 0; c < 8; ++c)
	{
		if (byte & 1)
			OutputTokenOne();
		else
			OutputTokenZero();

		byte >>= 1;
	}
}

void AtmelSWISimulationDataGenerator::OutputIOBlock(U8* pBytes)
{
	int c;
	for (c = 0; c < *pBytes; ++c)
		OutputByte(pBytes[c]);
}

void AtmelSWISimulationDataGenerator::OutputFlag(SWI_Flag flag)
{
	OutputByte(flag);
	mSWISimulationChannels.AdvanceAll(mClockGenerator.AdvanceByTimeS(MICRO_SEC * 100));
}

void AtmelSWISimulationDataGenerator::CreateSWITransaction(U8 BlockSent[], U8 BlockReceived[])
{
	// wake
	OutputTokenWake();

	// get the status
	OutputFlag(SWIF_Transmit);
	OutputIOBlock(StatusBlock);

	mSWISimulationChannels.AdvanceAll(mClockGenerator.AdvanceByTimeS(MILLI_SEC * 1));

	// send the command
	OutputFlag(SWIF_Command);
	OutputIOBlock(BlockSent);
	mSWISimulationChannels.AdvanceAll(mClockGenerator.AdvanceByTimeS(MILLI_SEC * 14));

	// get the response
	OutputFlag(SWIF_Transmit);
	OutputIOBlock(BlockReceived);

	mSWISimulationChannels.AdvanceAll(mClockGenerator.AdvanceByTimeS(MILLI_SEC * 10));

	// add the idle flag
	OutputFlag(SWIF_Idle);

	// make a gap
	mSWISimulationChannels.AdvanceAll(mClockGenerator.AdvanceByTimeS(MILLI_SEC * 100));
}

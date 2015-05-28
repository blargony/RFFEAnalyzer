#include "MDIOSimulationDataGenerator.h"

MDIOSimulationDataGenerator::MDIOSimulationDataGenerator()
{
}

MDIOSimulationDataGenerator::~MDIOSimulationDataGenerator()
{
}

void MDIOSimulationDataGenerator::Initialize( U32 simulation_sample_rate, MDIOAnalyzerSettings* settings )
{
	mSimulationSampleRateHz = simulation_sample_rate;
	mSettings = settings;

	// 1.7 Mhz is a commonly used frequency
	mClockGenerator.Init( 1700000*2, mSimulationSampleRateHz );

	// Set the simulation channels
	mMdio = mSimulationChannels.Add( mSettings->mMdioChannel, mSimulationSampleRateHz, BIT_HIGH );
	mMdc = mSimulationChannels.Add( mSettings->mMdcChannel, mSimulationSampleRateHz, BIT_LOW );

	// start the simulation with 10 periods of idle
	mSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 20.0 ) );
}

U32 MDIOSimulationDataGenerator::GenerateSimulationData( U64 largest_sample_requested, U32 sample_rate,
		SimulationChannelDescriptor** simulation_channels )
{
	U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample( largest_sample_requested,
											sample_rate,
											mSimulationSampleRateHz );

	MdioOpCode opcodeValues[3] = { C45_WRITE, C22_READ, C45_READ }; // address
	U8 opIndex = 0;

	MdioDevType devTypeValues[7] = { DEV_RESERVED, DEV_PMD_PMA, DEV_WIS, DEV_PCS, DEV_PHY_XS, DEV_DTE_XS, DEV_OTHER };
	U8 devTypeIndex = 0;

	U16 regAddrValue = 0;
	U8 regAddrValue5b = 0;

	U16 dataValueC22 = 0;
	U16 dataValueC45 = 0;
	U8 phyAddressValue = 0;

	while( mMdc->GetCurrentSampleNumber() < adjusted_largest_sample_requested )
	{

		CreateMdioC45Transaction( opcodeValues[opIndex % 3], 			// OpCode
								  phyAddressValue++,					// PHY Address
								  devTypeValues[devTypeIndex++ % 7], 	// DevType
								  regAddrValue++,     					// Register Address
								  dataValueC45++     					// Data
								);

		mSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) ); //insert 5 periods of idle

		CreateMdioC22Transaction( opcodeValues[opIndex++ % 3],	// OpCode
								  phyAddressValue++,			// PHY Address
								  regAddrValue5b++,				// Register Address
								  dataValueC22++				// Data
								);

		mSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 20.0 ) ); //insert 10 periods of idle

	}

	*simulation_channels = mSimulationChannels.GetArray();
	return mSimulationChannels.GetCount();
}

void MDIOSimulationDataGenerator::CreateMdioC22Transaction( MdioOpCode opCode, U8 phyAddress, U8 regAddress, U16 data )
{
	// A Clause 22 transaction consists of ONE packet containing a 5 bit register address, and a 16 bit data
	CreateStart(C22_START);
	CreateOpCode(opCode);
	CreatePhyAddress(phyAddress);
	CreateRegAddress(regAddress);
	CreateTurnAround(IsReadOperation(opCode));
	CreateData(data);
}

void MDIOSimulationDataGenerator::CreateMdioC45Transaction( MdioOpCode opCode, U8 phyAddress,
		MdioDevType devType, U16 regAddress, U16 data )
{
	// A Clause 45 transaction consists of TWO packets.
	// The first frame contains a 16 bit register address, the second has the 16 bit data

	// First packet
	CreateStart(C45_START);
	CreateOpCode(C45_ADDRESS);
	CreatePhyAddress(phyAddress);
	CreateDevType(devType);
	CreateTurnAround(IsReadOperation(opCode));
	CreateAddressOrData(regAddress);

	mSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 2.0 ) ); //insert 1 periods of idle

	// Second packet
	CreateStart(C45_START);
	CreateOpCode(opCode);
	CreatePhyAddress(phyAddress);
	CreateDevType(devType);
	CreateTurnAround(IsReadOperation(opCode));
	CreateAddressOrData(data);
}

bool MDIOSimulationDataGenerator::IsReadOperation( MdioOpCode opcode )
{
	return ( opcode == C22_READ ||
			 opcode == C45_READ_AND_ADDR ||
			 opcode == C45_READ );
}

void MDIOSimulationDataGenerator::CreateStart( MdioStart start )
{
	if( mMdc->GetCurrentBitState() == BIT_HIGH )
	{
		mSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 1.0 ) );
		mMdc->Transition();
		mSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 0.5 ) );
	}

	BitExtractor bit_extractor( start, AnalyzerEnums::MsbFirst, 2 );

	for( U32 i=0; i<2; i++ )
	{
		CreateBit( bit_extractor.GetNextBit() );
	}

}

void MDIOSimulationDataGenerator::CreateOpCode( MdioOpCode opCode )
{
	BitExtractor bit_extractor( opCode, AnalyzerEnums::MsbFirst, 2 );

	for( U32 i=0; i<2; i++ )
	{
		CreateBit( bit_extractor.GetNextBit() );
	}
}

void MDIOSimulationDataGenerator::CreatePhyAddress( U8 address )
{
	BitExtractor bit_extractor( address, AnalyzerEnums::MsbFirst, 5 );

	for( U32 i=0; i<5; i++ )
	{
		CreateBit( bit_extractor.GetNextBit() );
	}
}

void MDIOSimulationDataGenerator::CreateRegAddress( U8 address )
{

	BitExtractor bit_extractor( address, AnalyzerEnums::MsbFirst, 5 );

	for( U32 i=0; i<5; i++ )
	{
		CreateBit( bit_extractor.GetNextBit() );
	}

}

void MDIOSimulationDataGenerator::CreateDevType( MdioDevType devType )
{
	BitExtractor bit_extractor( devType, AnalyzerEnums::MsbFirst, 5 );

	for( U32 i=0; i<5; i++ )
	{
		CreateBit( bit_extractor.GetNextBit() );
	}
}

void MDIOSimulationDataGenerator::CreateTurnAround( bool isReadOperation )
{
	if( isReadOperation )
	{
		CreateTAForRead();
	}
	else
	{
		CreateTAForWrite();
	}
}

void MDIOSimulationDataGenerator::CreateTAForRead()
{
	mMdio->TransitionIfNeeded(BIT_HIGH);
	mSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 0.5 ) );
	mMdc->Transition(); // MDC posedge
	mSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 0.5 ) );
	mMdio->Transition(); // MDIO line goes down (by the slave)
	mSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 0.5 ) );
	mMdc->Transition(); // MDC negedge
	mSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 1.0 ) );
	mMdc->Transition(); // MDC posedge
	mSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 0.5 ) );
}

void MDIOSimulationDataGenerator::CreateTAForWrite()
{
	mMdio->TransitionIfNeeded(BIT_HIGH);
	mSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 0.5 ) );
	mMdc->Transition(); // MDC posedge
	mSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 1.0 ) );
	mMdc->Transition(); // MDC negedge
	mSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 0.5 ) );
	mMdio->Transition(); // value 0
	mSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 0.5 ) );
	mMdc->Transition(); // MDC posedge
	mSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 1.0 ) );
	mMdc->Transition(); // MDC negedge
	mSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 0.5 ) );
}

void MDIOSimulationDataGenerator::CreateData(U16 data)
{
	BitExtractor bit_extractor( data, AnalyzerEnums::MsbFirst, 16 );

	for( U32 i=0; i<16; i++ )
	{
		CreateBit( bit_extractor.GetNextBit() );
	}

	mMdio->TransitionIfNeeded( BIT_HIGH );  // Release the bus (normally pulled up)

	mSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 0.5 ) );
	mMdc->TransitionIfNeeded( BIT_LOW );    // clk must remain low on idle
}

void MDIOSimulationDataGenerator::CreateAddressOrData(U16 data)
{
	CreateData(data);
}

void MDIOSimulationDataGenerator::CreateBit( BitState bit_state )
{
	mMdio->TransitionIfNeeded( bit_state );
	mSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 0.5 ) );
	mMdc->Transition(); // MDC posedge
	mSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 1.0 ) );
	mMdc->Transition(); // MDC negedge
	mSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 0.5 ) );
}

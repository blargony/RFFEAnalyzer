
#include "UnioSimulationDataGenerator.h"

UnioSimulationDataGenerator::UnioSimulationDataGenerator()
:	mBitRate( 40000 ),  //100000.0
	mStandyPulseTimeS( 600E-6 ),
	mStartHeaderLowPulseTimeS( 5E-6 ),
	mHoldDelayS( 100E-6 ),
	mTssMinimumSameAddressDelayS( 10E-6 )
{
}

UnioSimulationDataGenerator::~UnioSimulationDataGenerator()
{
}

void UnioSimulationDataGenerator::Initialize( U32 simulation_sample_rate, UnioAnalyzerSettings* settings )
{

	mSimulationSampleRateHz = simulation_sample_rate;
	mSettings = settings;

	mClockGenerator.Init( mBitRate, mSimulationSampleRateHz );
	
	mUnioSimulationData.SetChannel( mSettings->mScioChannel );
	mUnioSimulationData.SetSampleRate( mSimulationSampleRateHz );
	mUnioSimulationData.SetInitialBitState( BIT_HIGH );
	mUnioSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10 ) );
}



U32 UnioSimulationDataGenerator::GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample( newest_sample_requested, sample_rate, mSimulationSampleRateHz );

	while( mUnioSimulationData.GetCurrentSampleNumber() < adjusted_largest_sample_requested )
	{
		WriteStandbyPulse();
		WriteHeader();
		WriteByte( 0xA0, MAK, SAK ); //device address
		WriteByte( 0x03, MAK, SAK ); //READ command for MC serial eeprom
		WriteByte( 0x00, MAK, SAK ); //address MSB
		WriteByte( 0x00, MAK, SAK ); //address LSB
		WriteByte( 0x01, MAK, SAK ); //data
		WriteByte( 0x02, NoMAK, SAK ); //data, stop.

		WriteMinimumSameAddressDelay();
		WriteHeader();
		WriteByte( 0xA0, MAK, SAK ); //device address
		WriteByte( 0x03, MAK, SAK ); //READ command for MC serial eeprom
		WriteByte( 0x00, MAK, SAK ); //address MSB
		WriteByte( 0x00, MAK, SAK ); //address LSB
		WriteByte( 0x01, MAK, SAK ); //data
		WriteByte( 0x02, NoMAK, SAK ); //data, stop.

		WriteMinimumSameAddressDelay();
		WriteHeader();
		WriteByte( 0xF0, MAK, NoSAK ); //device address
		WriteByte( 0x08, MAK, SAK ); //device address
		WriteByte( 0x03, MAK, SAK ); //READ command for MC serial eeprom
		WriteByte( 0x00, MAK, SAK, true, true ); //address MSB
		WriteByte( 0x00, MAK, SAK ); //address LSB
		WriteByte( 0x01, MAK, SAK ); //data
		WriteByte( 0x02, NoMAK, SAK ); //data, stop.
	}

	*simulation_channels = &mUnioSimulationData;
	return 1; // we are retuning the size of the SimulationChannelDescriptor array.  In our case, the "array" is length 1.
}

void UnioSimulationDataGenerator::WriteStandbyPulse()
{
	mUnioSimulationData.TransitionIfNeeded( BIT_HIGH );
	mUnioSimulationData.Advance( mClockGenerator.AdvanceByTimeS( mStandyPulseTimeS ) );

	//we leave this funciton with the line still high.
}

void UnioSimulationDataGenerator::WriteMinimumSameAddressDelay()
{
	mUnioSimulationData.Advance( mClockGenerator.AdvanceByTimeS( mTssMinimumSameAddressDelayS ) );
}

void UnioSimulationDataGenerator::WriteByteWithHold( U8 data, MasterAcknowkedge master_ack, SlaveAcknowkedge slave_ack )
{
	BitExtractor bit_values( data, AnalyzerEnums::MsbFirst, 8 );

	for( U32 i=0; i<8; i++ )
	{
		WriteBit( bit_values.GetNextBit() );
	}

	if( master_ack == NoMAK )
		WriteNak();
	else
		WriteAck();

	//enum SlaveAcknowkedge { NoSAK, SAK, NonStandardNoSAK };
	switch( slave_ack )
	{
	case NoSAK:
		WriteNak();
		break;
	case SAK:
		WriteAck();
		break;
	case NonStandardNoSAK:
		WriteNonStandardNak();
		break;
	}
}

void UnioSimulationDataGenerator::WriteByte( U8 data, MasterAcknowkedge master_ack, SlaveAcknowkedge slave_ack, bool valid, bool mak_hold )
{
	BitExtractor bit_values( data, AnalyzerEnums::MsbFirst, 8 );

	for( U32 i=0; i<8; i++ )
	{
		if( valid == false && i == 3 )
		{
			WriteNonStandardNak();
			bit_values.GetNextBit();
		}else
		{
			WriteBit( bit_values.GetNextBit() );
		}
	}

	if( mak_hold )
	{
		WriteAckWithHold();
	}else
	{
		if( master_ack == NoMAK )
			WriteNak();
		else
			WriteAck();
	}


	//enum SlaveAcknowkedge { NoSAK, SAK, NonStandardNoSAK };
	switch( slave_ack )
	{
	case NoSAK:
		WriteNak();
		break;
	case SAK:
		WriteAck();
		break;
	case NonStandardNoSAK:
		WriteNonStandardNak();
		break;
	}
}

void UnioSimulationDataGenerator::WriteHeader()
{
	//we enter this function with the line high
	if( mUnioSimulationData.GetCurrentBitState() == BIT_LOW )
		AnalyzerHelpers::Assert( "unexpected" );

	//LOW
	mUnioSimulationData.Transition();
	mUnioSimulationData.Advance( mClockGenerator.AdvanceByTimeS( mStartHeaderLowPulseTimeS ) );

	WriteBit( BIT_LOW );
	WriteBit( BIT_HIGH );
	WriteBit( BIT_LOW );
	WriteBit( BIT_HIGH );

	WriteBit( BIT_LOW );
	WriteBit( BIT_HIGH );
	WriteBit( BIT_LOW );
	WriteBit( BIT_HIGH );

	WriteAck();
	WriteNonStandardNak();
}

void UnioSimulationDataGenerator::WriteNonStandardNak()
{
	//don't do anything for one bit period
	mUnioSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 1.0 ) );
}

void UnioSimulationDataGenerator::WriteNak()
{
	WriteBit( BIT_LOW );
}

void UnioSimulationDataGenerator::WriteAck()
{
	WriteBit( BIT_HIGH );
}

void UnioSimulationDataGenerator::WriteAckWithHold()
{
	//start out low
	mUnioSimulationData.TransitionIfNeeded( BIT_LOW );

	//hold...
	mUnioSimulationData.Advance( mClockGenerator.AdvanceByTimeS( mHoldDelayS ) );

	//posedge -- MAK
	mUnioSimulationData.Transition();
	
	//wait 1/2 a bit.
	mUnioSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( .5 ) );
}

void UnioSimulationDataGenerator::WriteBit( BitState bit_state )
{
	if( bit_state == BIT_HIGH )
	{
		//we will need to do a rising edge. 
		mUnioSimulationData.TransitionIfNeeded( BIT_LOW );
	}else
	{
		//we will need to do a falling edge. 
		mUnioSimulationData.TransitionIfNeeded( BIT_HIGH );
	}

	//wait 1/2 a bit.
	mUnioSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( .5 ) );
	mUnioSimulationData.Transition();
	mUnioSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( .5 ) );
}

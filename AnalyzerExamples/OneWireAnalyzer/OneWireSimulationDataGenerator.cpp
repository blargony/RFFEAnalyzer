#include "OneWireSimulationDataGenerator.h"
#include "OneWireAnalyzerSettings.h"

OneWireSimulationDataGenerator::OneWireSimulationDataGenerator()
{

}
OneWireSimulationDataGenerator::~OneWireSimulationDataGenerator()
{

}

void OneWireSimulationDataGenerator::Initialize( U32 simulation_sample_rate, OneWireAnalyzerSettings* settings )
{
	mSimulationSampleRateHz = simulation_sample_rate;
	mSettings = settings;

	mOneWireSimulationData.SetChannel( mSettings->mOneWireChannel );
	mOneWireSimulationData.SetSampleRate( simulation_sample_rate );

	mOneWireSimulationData.SetInitialBitState( BIT_HIGH );
	//mOneWireSimulationData.Advance( 1000 ); 
	mSimOverdrive = false;


}

U32 OneWireSimulationDataGenerator::GenerateSimulationData( U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample( largest_sample_requested, sample_rate, mSimulationSampleRateHz );

	while( mOneWireSimulationData.GetCurrentSampleNumber() < adjusted_largest_sample_requested )
	{
		U32 delay;
		if(mSimOverdrive == true)
			delay = 30;
		else
			delay = 250;

		mOneWireSimulationData.Advance( U32(UsToSamples(delay * 3 )) );
		//mSimulationSampleIndex += UsToSamples(delay * 3, true);

		SimResetPacket();
		SimReadRom(0x8877665544332211ull);

		//mSimulationSampleIndex += UsToSamples(delay, true);
		mOneWireSimulationData.Advance( U32( UsToSamples(delay ) ) );
		SimWriteByte(0x37);
		//mSimulationSampleIndex += UsToSamples(delay, true);
		mOneWireSimulationData.Advance( U32( UsToSamples(delay ) ) );
		SimWriteByte(0xF0);

		std::vector<U64> device_ROMs;
		device_ROMs.push_back(0x8877665544332211ull);
		device_ROMs.push_back(0x1122334455667788ull);
		
		SimResetPacket();
		U64 found_device;
		found_device = SimSearchRom( device_ROMs );

		//mSimulationSampleIndex += UsToSamples(delay, true);
		mOneWireSimulationData.Advance( U32( UsToSamples(delay ) ) );
		SimWriteByte(0x0F);
		//mSimulationSampleIndex += UsToSamples(delay, true);
		mOneWireSimulationData.Advance( U32( UsToSamples(delay ) ) );
		SimWriteByte(0xF0);
		//mSimulationSampleIndex += UsToSamples(delay, true);
		mOneWireSimulationData.Advance( U32( UsToSamples(delay ) ) );
		SimWriteByte(0x55);
		//mSimulationSampleIndex += UsToSamples(delay, true);
		mOneWireSimulationData.Advance( U32( UsToSamples(delay ) ) );
		SimWriteByte(0x18);
		
		SimResetPacket();
		SimMatchRom(0xF0E1D2C3B4A59687ull);

		//mSimulationSampleIndex += UsToSamples(delay, true);
		mOneWireSimulationData.Advance( U32( UsToSamples(delay ) ) );
		SimWriteByte(0x55);
		//mSimulationSampleIndex += UsToSamples(delay, true);
		mOneWireSimulationData.Advance( U32( UsToSamples(delay ) ) );
		SimWriteByte(0x18);
				
		if( mSimOverdrive == true )
			mSimOverdrive = false;
		else
		{
			SimResetPacket();
			SimOverdriveSkipRom();
		}

	}

	*simulation_channels = &mOneWireSimulationData;



	return 1;  // we are retuning the size of the SimulationChannelDescriptor array.  In our case, the "array" is length 1.
}


void OneWireSimulationDataGenerator::SimReadRom ( U64 rom )
{	//ROM: CRC:SERIAL:FAMILY, 1:6:1
	//Code: 0x33
	SimWriteByte(0x33);
	SimWriteByte(U32((rom & 0x00000000000000FFull))); //family code
	SimWriteByte(U32((rom & 0x000000000000FF00ull)>>8)); //Serial 1
	SimWriteByte(U32((rom & 0x0000000000FF0000ull)>>16)); //Serial 2
	SimWriteByte(U32((rom & 0x00000000FF000000ull)>>24)); //Serial 3
	SimWriteByte(U32((rom & 0x000000FF00000000ull)>>32)); //Serial 4
	SimWriteByte(U32((rom & 0x0000FF0000000000ull)>>40)); //Serial 5
	SimWriteByte(U32((rom & 0x00FF000000000000ull)>>48)); //Serial 6
	SimWriteByte(U32((rom & 0xFF00000000000000ull)>>56)); //CRC

}

void OneWireSimulationDataGenerator::SimSkipRom ()
{
	//Code: 0xCC
	SimWriteByte(0xCC);

}

void OneWireSimulationDataGenerator::SimMatchRom( U64 rom, bool overdrive )
{
	//Code: 0x55
	if( overdrive == false )
		SimWriteByte(0x55);
	SimWriteByte(U32((rom & 0x00000000000000FFull))); //family code
	SimWriteByte(U32((rom & 0x000000000000FF00ull)>>8)); //Serial 1
	SimWriteByte(U32((rom & 0x0000000000FF0000ull)>>16)); //Serial 2
	SimWriteByte(U32((rom & 0x00000000FF000000ull)>>24)); //Serial 3
	SimWriteByte(U32((rom & 0x000000FF00000000ull)>>32)); //Serial 4
	SimWriteByte(U32((rom & 0x0000FF0000000000ull)>>40)); //Serial 5
	SimWriteByte(U32((rom & 0x00FF000000000000ull)>>48)); //Serial 6
	SimWriteByte(U32((rom & 0xFF00000000000000ull)>>56)); //CRC
}

U64 OneWireSimulationDataGenerator::SimSearchRom( std::vector<U64>& roms )
{
	U64 selected_device = 0;
	//Code: 0xF0
	SimWriteByte(0xF0);
	//for each bit posision in each of the roms, we will check to see if they all match, or if they conflict.
	for( U32 i = 0; i<64; i++ )
	{
		std::vector<U64>::iterator it = roms.begin();

		U32 rom_bit = (*it >> i) & 0x1;
		U32 comp_bit = rom_bit ^ 0x1;
		for(; it != roms.end(); ++it)
		{
			U32 current_bit = (*it >> i) & 0x1;
			if (current_bit != rom_bit)
			{
				rom_bit = 0;
				comp_bit = 0;
				//Eliminate all remaining roms with a '1' at posision i.
				for( std::vector<U64>::iterator delete_it = roms.begin(); delete_it != roms.end(); ++delete_it )
				{
					if( ((*delete_it >> i) & 0x1) == 0x1 )
					{
						delete_it = roms.erase(delete_it);
						it = roms.begin();
					}

				}

			}
		}
		SimWriteBit(rom_bit);
		SimWriteBit(comp_bit);
		if(rom_bit == 1)
		{
			selected_device |= U64(0x1) << i;
			SimWriteBit(1);
		}
		else
		{
			SimWriteBit(0);
		}
		
	}
	return selected_device;
}

void OneWireSimulationDataGenerator::SimOverdriveSkipRom()
{
	//Code: 0x3C
	SimWriteByte(0x3C);
	mSimOverdrive = true;
}	

void OneWireSimulationDataGenerator::SimOverdriveMatchRom( U64 rom )
{
	//Code: 0x69
	SimWriteByte(0x69);
	mSimOverdrive = true;
	SimMatchRom( rom, true );
}

void OneWireSimulationDataGenerator::SimResetPacket()
{
	U32 reset_window_length = 960;
	U32 reset_pulse_length = 480;
	if ( ((mOneWireSimulationData.GetCurrentSampleNumber() >> 5) % 20) > 15 )
		reset_pulse_length = 440;
	U32 device_responce_delay = 30;
	U32 device_responce_length = 120;

	if( mSimOverdrive == true )
	{
		reset_window_length = 96;
		reset_pulse_length = 48;
		device_responce_delay = 4;
		device_responce_length = 12;
	}
	
	//mOneWireTransitions->push_back(mSimulationSampleIndex);
	mOneWireSimulationData.Transition();
	//mSimulationSampleIndex += UsToSamples(reset_pulse_length );		//Reset Pulse Length
	mOneWireSimulationData.Advance( U32( UsToSamples( reset_pulse_length ) ) );
	//mOneWireTransitions->push_back(mSimulationSampleIndex);
	mOneWireSimulationData.Transition();
	//mSimulationSampleIndex += UsToSamples(device_responce_delay );		//Typical delay before responce
	mOneWireSimulationData.Advance( U32( UsToSamples( device_responce_delay ) ) );
	//mOneWireTransitions->push_back(mSimulationSampleIndex);
	mOneWireSimulationData.Transition();
	//mSimulationSampleIndex += UsToSamples(device_responce_length );		//Typical responce length
	mOneWireSimulationData.Advance( U32( UsToSamples( device_responce_length ) ) );
	//mOneWireTransitions->push_back(mSimulationSampleIndex);
	mOneWireSimulationData.Transition();
	//mSimulationSampleIndex += UsToSamples( reset_window_length - reset_pulse_length - device_responce_delay - device_responce_length, true );	//Required Time for Reset and Presence opperation.
	mOneWireSimulationData.Advance( U32( UsToSamples( reset_window_length - reset_pulse_length - device_responce_delay - device_responce_length ) ) );
}

void OneWireSimulationDataGenerator::SimWriteBit( U32 bit )
{
	U32 slot_length = 70;
	U32 logic_low_length = 60;
	U32 logic_high_length = 6;

	if( mSimOverdrive == true )
	{
		slot_length = 8;
		logic_low_length = 6;
		logic_high_length = 2;
	}
	//mOneWireTransitions->push_back(mSimulationSampleIndex);
	mOneWireSimulationData.Transition();
	if ( bit == BIT_LOW ) //0 (long)
	{
		//mSimulationSampleIndex += UsToSamples(logic_low_length );
		mOneWireSimulationData.Advance( U32( UsToSamples( logic_low_length ) ) );
		//mOneWireTransitions->push_back(mSimulationSampleIndex);
		mOneWireSimulationData.Transition();
		//mSimulationSampleIndex += UsToSamples(slot_length - logic_low_length );
		mOneWireSimulationData.Advance( U32( UsToSamples( slot_length - logic_low_length ) ) );
	}
	else if( bit == BIT_HIGH ) //1 (short)
	{
		//mSimulationSampleIndex += UsToSamples(logic_high_length );
		mOneWireSimulationData.Advance( U32( UsToSamples( logic_high_length ) ) );
		//mOneWireTransitions->push_back(mSimulationSampleIndex);
		mOneWireSimulationData.Transition();
		//mSimulationSampleIndex += UsToSamples(slot_length - logic_high_length );
		mOneWireSimulationData.Advance( U32( UsToSamples( slot_length - logic_high_length ) ) );
	}
}

void OneWireSimulationDataGenerator::SimWriteByte( U32 byte )
{
	//LSB first
	U32 mask = 1; 
	for( U32 i = 0; i < 8; i++ )
	{
		U32 mask_result = byte & mask;
		if( mask_result != 0 )
			SimWriteBit( BIT_HIGH );
		else
			SimWriteBit( BIT_LOW );
		mask = mask << 1;
	}
	if( mSimOverdrive == true )
		//mSimulationSampleIndex += UsToSamples( 12 );
		mOneWireSimulationData.Advance( U32( UsToSamples( 12 ) ) );
	else
		//mSimulationSampleIndex += UsToSamples(120 );
		mOneWireSimulationData.Advance( U32( UsToSamples( 120 ) ) );
}

U64 OneWireSimulationDataGenerator::UsToSamples( U64 us )
{
	return ( mSimulationSampleRateHz * us ) / 1000000;
}

U64 OneWireSimulationDataGenerator::SamplesToUs( U64 samples )
{
	return( samples * 1000000 ) / mSimulationSampleRateHz;
}

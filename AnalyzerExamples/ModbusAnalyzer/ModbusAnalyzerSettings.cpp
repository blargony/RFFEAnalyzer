#include "ModbusAnalyzerSettings.h"

#include <AnalyzerHelpers.h>
#include <sstream>

#ifndef __GNUC__
	#pragma warning(disable: 4800) //warning C4800: 'U32' : forcing value to bool 'true' or 'false' (performance warning)
#endif


ModbusAnalyzerSettings::ModbusAnalyzerSettings()
:	mInputChannel( UNDEFINED_CHANNEL ),
	mBitRate( 9600 ),
	mBitsPerTransfer( 8 ),
	mShiftOrder( AnalyzerEnums::LsbFirst ),
	mStopBits( 1.0 ),
	mParity( AnalyzerEnums::None ),
	mInverted( false ),
	mUseAutobaud( false ),
	mModbusMode( ModbusAnalyzerEnums::ModbusRTUMaster )
{
/*
	mUseAutobaudInterface.reset( new AnalyzerSettingInterfaceBool() );
	mUseAutobaudInterface->SetTitleAndTooltip( "", "With Autobaud turned on, the analyzer will run as usual, with the current bit rate.  At the same time, it will also keep track of the shortest pulse it detects.  After analyzing all the data, if the bit rate implied by this shortest pulse is different by more than 10% from the specified bit rate, the bit rate will be changed and the analysis run again." );
	mUseAutobaudInterface->SetCheckBoxText( "Use Autobaud" );
	mUseAutobaudInterface->SetValue( mUseAutobaud );

	mBitsPerTransferInterface.reset( new AnalyzerSettingInterfaceNumberList() );
	mBitsPerTransferInterface->SetTitleAndTooltip( "", "Select the number of bits per frame" ); 
	for( U32 i = 1; i <= 64; i++ )
	{
		std::stringstream ss; 

		if( i == 1 )
			ss << "1 Bit per Transfer";
		else
			if( i == 8 )
				ss << "8 Bits per Transfer (Standard)";
			else
				ss << i << " Bits per Transfer";

		mBitsPerTransferInterface->AddNumber( i, ss.str().c_str(), "" );
	}
	mBitsPerTransferInterface->SetNumber( mBitsPerTransfer );


	mStopBitsInterface.reset( new AnalyzerSettingInterfaceNumberList() );
	mStopBitsInterface->SetTitleAndTooltip( "", "Specify the number of stop bits." );
	mStopBitsInterface->AddNumber( 1.0, "1 Stop Bit (Standard)", "" );
	mStopBitsInterface->AddNumber( 1.5, "1.5 Stop Bits", "" );
	mStopBitsInterface->AddNumber( 2.0, "2 Stop Bits", "" );
	mStopBitsInterface->SetNumber( mStopBits ); 


	mParityInterface.reset( new AnalyzerSettingInterfaceNumberList() );
	mParityInterface->SetTitleAndTooltip( "", "Specify None, Even, or Odd Parity." );
	mParityInterface->AddNumber( AnalyzerEnums::None, "No Parity Bit (Standard)", "" );
	mParityInterface->AddNumber( AnalyzerEnums::Even, "Even Parity Bit", "" );
	mParityInterface->AddNumber( AnalyzerEnums::Odd, "Odd Parity Bit", "" ); 
	mParityInterface->SetNumber( mParity );


	mShiftOrderInterface.reset( new AnalyzerSettingInterfaceNumberList() );
	mShiftOrderInterface->SetTitleAndTooltip( "", "Select if the most significant bit or least significant bit is transmitted first" );
	mShiftOrderInterface->AddNumber( AnalyzerEnums::LsbFirst, "Least Significant Bit Sent First (Standard)", "" );
	mShiftOrderInterface->AddNumber( AnalyzerEnums::MsbFirst, "Most Significant Bit Sent First", "" );
	mShiftOrderInterface->SetNumber( mShiftOrder );
*/

	mInputChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mInputChannelInterface->SetTitleAndTooltip( "Modbus", "Modbus" );		// Todo: Change to reflect specific variant of Modbus later in program.
	mInputChannelInterface->SetChannel( mInputChannel );


	mModbusModeInterface.reset( new AnalyzerSettingInterfaceNumberList() );
	mModbusModeInterface->SetTitleAndTooltip( "Modbus Mode", "Specify which mode of Modbus this is" );
	mModbusModeInterface->AddNumber( ModbusAnalyzerEnums::ModbusRTUMaster, "Modbus/RTU - Master", "(messages are transmitted in binary)" );
	mModbusModeInterface->AddNumber( ModbusAnalyzerEnums::ModbusRTUSlave, "Modbus/RTU - Slave", "(messages are transmitted in binary)" );
	mModbusModeInterface->AddNumber( ModbusAnalyzerEnums::ModbusASCIIMaster, "Modbus/ASCII - Master", "(messages are transmitted in ASCII-readable format)" );
	mModbusModeInterface->AddNumber( ModbusAnalyzerEnums::ModbusASCIISlave, "Modbus/ASCII - Slave", "(messages are transmitted in ASCII-readable format)");
	mModbusModeInterface->SetNumber( mModbusMode );


	mBitRateInterface.reset( new AnalyzerSettingInterfaceInteger() );
	mBitRateInterface->SetTitleAndTooltip( "Bit Rate (Bits/S)",  "Specify the bit rate in bits per second." );
	mBitRateInterface->SetMax( 6000000 );
	mBitRateInterface->SetMin( 1 );
	mBitRateInterface->SetInteger( mBitRate );


	mInvertedInterface.reset( new AnalyzerSettingInterfaceNumberList() );
	mInvertedInterface->SetTitleAndTooltip( "", "Specify if the serial signal is inverted" );
	mInvertedInterface->AddNumber( false, "Non Inverted (Standard)", "" );
	mInvertedInterface->AddNumber( true, "Inverted", "" );
	mInvertedInterface->SetNumber( mInverted );
	enum Mode { Normal, MpModeRightZeroMeansAddress, MpModeRightOneMeansAddress, MpModeLeftZeroMeansAddress, MpModeLeftOneMeansAddress };	// FIXME: unused?
	
	
	
	
	/*
	AddInterface( mUseAutobaudInterface.get() );
	AddInterface( mBitsPerTransferInterface.get() );
	AddInterface( mStopBitsInterface.get() );
	AddInterface( mParityInterface.get() );
	AddInterface( mShiftOrderInterface.get() );
	*/
	AddInterface( mInputChannelInterface.get() );
	AddInterface( mModbusModeInterface.get() );
	AddInterface( mBitRateInterface.get() );
	AddInterface( mInvertedInterface.get() );




	//AddExportOption( 0, "Export as text/csv file", "text (*.txt);;csv (*.csv)" );
	AddExportOption( 0, "Export as text/csv file" );
	AddExportExtension( 0, "text", "txt" );
	AddExportExtension( 0, "csv", "csv" );

	ClearChannels();
	AddChannel( mInputChannel, "Modbus", false );
}

ModbusAnalyzerSettings::~ModbusAnalyzerSettings()
{
}

bool ModbusAnalyzerSettings::SetSettingsFromInterfaces()
{
/*
 * 	if( AnalyzerEnums::Parity( U32( mParityInterface->GetNumber() ) ) != AnalyzerEnums::None )
		if( ModbusAnalyzerEnums::Mode( U32( mModbusModeInterface->GetNumber() ) ) != ModbusAnalyzerEnums::Normal )
		{
			SetErrorText( "Sorry, but we don't support using parity at the same time as MP mode." );
			return false;
		}
*/
	mInputChannel = mInputChannelInterface->GetChannel();
	mBitRate = mBitRateInterface->GetInteger();
	//mBitsPerTransfer = U32( mBitsPerTransferInterface->GetNumber() );
	//mStopBits = mStopBitsInterface->GetNumber();
	//mParity = AnalyzerEnums::Parity( U32( mParityInterface->GetNumber() ) );
	//mShiftOrder =  AnalyzerEnums::ShiftOrder( U32( mShiftOrderInterface->GetNumber() ) );
	mInverted = bool( U32( mInvertedInterface->GetNumber() ) );
	//mUseAutobaud = mUseAutobaudInterface->GetValue();
	mModbusMode = ModbusAnalyzerEnums::Mode( U32( mModbusModeInterface->GetNumber() ) );

	ClearChannels();
	AddChannel( mInputChannel, "Modbus", true );

	return true;
}

void ModbusAnalyzerSettings::UpdateInterfacesFromSettings()
{
	mInputChannelInterface->SetChannel( mInputChannel );
	mBitRateInterface->SetInteger( mBitRate );
	//mBitsPerTransferInterface->SetNumber( mBitsPerTransfer );
	//mStopBitsInterface->SetNumber( mStopBits );
	//mParityInterface->SetNumber( mParity );
	//mShiftOrderInterface->SetNumber( mShiftOrder );
	mInvertedInterface->SetNumber( mInverted );
	//mUseAutobaudInterface->SetValue( mUseAutobaud );
	mModbusModeInterface->SetNumber( mModbusMode );
}

void ModbusAnalyzerSettings::LoadSettings( const char* settings )
{
	// Example: $3 = 0x1647478 "22 serialization::archive 5 22 ModbusAnalyzerSettings 0 8 1324695933 0 9600"
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	const char* name_string;	//the first thing in the archive is the name of the protocol analyzer that the data belongs to.
	text_archive >> &name_string;
	if( strcmp( name_string, "ModbusAnalyzerSettings" ) != 0 ) {
		if( strcmp( name_string, "SaleaeAsyncModbusAnalyzer" ) != 0 ) {	// The old string; treat them the same for now.
			AnalyzerHelpers::Assert( "ModbusAnalyzerSettings: Provided with a settings string that doesn't belong to us;" );
		}
	}
	text_archive >> mInputChannel;
	text_archive >> mBitRate;
	//text_archive >> mBitsPerTransfer;
	//text_archive >> mStopBits;
	//text_archive >> *(U32*)&mParity;
	//text_archive >> *(U32*)&mShiftOrder;
	text_archive >> mInverted;

	//check to make sure loading it actual works befor assigning the result -- do this when adding settings to an anylzer which has been previously released.
	//bool use_autobaud;
	//if( text_archive >> use_autobaud )
	//	mUseAutobaud = use_autobaud;

	ModbusAnalyzerEnums::Mode mode;
	if( text_archive >> *(U32*)&mode )
		mModbusMode = mode;

	ClearChannels();
	AddChannel( mInputChannel, "Modbus", true );

	UpdateInterfacesFromSettings();
}

const char* ModbusAnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

	text_archive << "ModbusAnalyzerSettings";
	text_archive << mInputChannel;
	text_archive << mBitRate;
	//text_archive << mBitsPerTransfer;
	//text_archive << mStopBits;
	//text_archive << mParity;
	//text_archive << mShiftOrder;
	text_archive << mInverted;

	//text_archive << mUseAutobaud;

	text_archive << mModbusMode;

	return SetReturnString( text_archive.GetString() );
}

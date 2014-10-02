#include "ManchesterAnalyzerSettings.h"

#include <AnalyzerHelpers.h>
#include <sstream>
#include <cstring>

#pragma warning(disable: 4800) //warning C4800: 'U32' : forcing value to bool 'true' or 'false' (performance warning)
#pragma warning(disable: 4996) //warning C4996: 'sprintf': This function or variable may be unsafe. Consider using sprintf_s instead.

ManchesterAnalyzerSettings::ManchesterAnalyzerSettings()
:	mInputChannel( UNDEFINED_CHANNEL ),
	mMode( MANCHESTER ),
	mBitRate( 1500 ),
	mInverted( false ),
	mBitsPerTransfer( 8 ),
	mShiftOrder( AnalyzerEnums::LsbFirst ),
	mBitsToIgnore( 0 ),
	mTolerance( TOL25 )
{
	mInputChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mInputChannelInterface->SetTitleAndTooltip( "Manchester", "Manchester" );
	mInputChannelInterface->SetChannel( mInputChannel );

	mModeInterface.reset( new AnalyzerSettingInterfaceNumberList() );
	mModeInterface->SetTitleAndTooltip( "Mode", "Specify the Manchester Mode" );
	mModeInterface->AddNumber( MANCHESTER, "Manchester", "" );
	mModeInterface->AddNumber( DIFFERENTIAL_MANCHESTER, "Differential Manchester", "" );
	mModeInterface->AddNumber( BI_PHASE_MARK, "Bi-Phase Mark Code (FM1)", "" );
	mModeInterface->AddNumber( BI_PHASE_SPACE, "Bi-Phase Space Code (FM0)", "" );
	mModeInterface->SetNumber( mMode );

	mBitRateInterface.reset( new AnalyzerSettingInterfaceInteger() );
	mBitRateInterface->SetTitleAndTooltip( "Bit Rate (Bits/S)",  "Specify the bit rate in bits per second." );
	mBitRateInterface->SetMax( 6000000 );
	mBitRateInterface->SetMin( 1 );
	mBitRateInterface->SetInteger( mBitRate );

	mInvertedInterface.reset( new AnalyzerSettingInterfaceNumberList() );
	mInvertedInterface->SetTitleAndTooltip( "", "Specify the Manchester edge polarity (Normal Manchester mode only)" );
	mInvertedInterface->AddNumber( false, "negative edge is binary one", "" );
	mInvertedInterface->AddNumber( true, "negative edge is binary zero", "" );
	mInvertedInterface->SetNumber( mInverted );

	mBitsPerTransferInterface.reset( new AnalyzerSettingInterfaceNumberList() );
	mBitsPerTransferInterface->SetTitleAndTooltip( "", "Select the number of bits per frame" ); 
	for( U32 i = 1; i <= 64; i++ )
	{
		std::stringstream ss; 

		if( i == 1 )
			ss << "1 Bit per Transfer";
		else
			ss << i << " Bits per Transfer";

		mBitsPerTransferInterface->AddNumber( i, ss.str().c_str(), "" );
	}
	mBitsPerTransferInterface->SetNumber( mBitsPerTransfer );

	mShiftOrderInterface.reset( new AnalyzerSettingInterfaceNumberList() );
	mShiftOrderInterface->SetTitleAndTooltip( "", "Select if the most significant bit or least significant bit is transmitted first" );
	mShiftOrderInterface->AddNumber( AnalyzerEnums::LsbFirst, "Least Significant Bit Sent First", "" );
	mShiftOrderInterface->AddNumber( AnalyzerEnums::MsbFirst, "Most Significant Bit Sent First", "" );
	mShiftOrderInterface->SetNumber( mShiftOrder );


	mNumBitsIgnoreInterface.reset( new AnalyzerSettingInterfaceInteger() );
	mNumBitsIgnoreInterface->SetTitleAndTooltip( "Preamble bits to ignore",  "Specify the number of preamble bits to ignore." );
	mNumBitsIgnoreInterface->SetMax( 1000 );
	mNumBitsIgnoreInterface->SetMin( 0 );
	mNumBitsIgnoreInterface->SetInteger( mBitsToIgnore );

	mToleranceInterface.reset( new AnalyzerSettingInterfaceNumberList() );
	mToleranceInterface->SetTitleAndTooltip( "Tolerance", "Specify the Manchester Tolerance as a percentage of period" );
	mToleranceInterface->AddNumber( TOL25, "25% of period (default)", "Maximum allowed tolerance, +- 50% of one half period" );
	mToleranceInterface->AddNumber( TOL5, "5% of period", "Required more than 10x over sampling" );
	mToleranceInterface->AddNumber( TOL05, "0.5% of period", "Requires more than 200x over sampling" );
	mToleranceInterface->SetNumber( mTolerance );

	AddInterface( mInputChannelInterface.get() );
	AddInterface( mModeInterface.get() );
	AddInterface( mBitRateInterface.get() );
	AddInterface( mInvertedInterface.get() );
	AddInterface( mBitsPerTransferInterface.get() );
	AddInterface( mShiftOrderInterface.get() );
	AddInterface( mNumBitsIgnoreInterface.get() );
	AddInterface( mToleranceInterface.get() );

	AddExportOption( 0, "Export as text/csv file" );
	AddExportExtension( 0, "text", "txt" );
	AddExportExtension( 0, "csv", "csv" );

	ClearChannels();
	AddChannel( mInputChannel, "Manchester", false );
}

ManchesterAnalyzerSettings::~ManchesterAnalyzerSettings()
{

}

bool ManchesterAnalyzerSettings::SetSettingsFromInterfaces()
{
	mInputChannel = mInputChannelInterface->GetChannel();
	mMode = ManchesterMode( U32( mModeInterface->GetNumber() ) );
	mBitRate = mBitRateInterface->GetInteger();
	mInverted = bool( U32( mInvertedInterface->GetNumber() ) );
	mBitsPerTransfer = U32( mBitsPerTransferInterface->GetNumber() );
	mShiftOrder =  AnalyzerEnums::ShiftOrder( U32( mShiftOrderInterface->GetNumber() ) );
	mBitsToIgnore = mNumBitsIgnoreInterface->GetInteger();
	mTolerance = ManchesterTolerance( U32( mToleranceInterface->GetNumber() ) );
	ClearChannels();
	AddChannel( mInputChannel, "Manchester", true );

	return true;
}
void ManchesterAnalyzerSettings::LoadSettings( const char* settings )
{
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	const char* name_string;	//the first thing in the archive is the name of the protocol analyzer that the data belongs to.
	text_archive >> &name_string;
	if( strcmp( name_string, "SaleaeManchesterAnalyzer" ) != 0 )
		AnalyzerHelpers::Assert( "SaleaeManchesterAnalyzer: Provided with a settings string that doesn't belong to us;" );

	text_archive >> mInputChannel;
	text_archive >> *(U32*)&mMode;
	text_archive >> mBitRate;
	text_archive >> mInverted;
	text_archive >> mBitsPerTransfer;
	text_archive >> *(U32*)&mShiftOrder;
	text_archive >> mBitsToIgnore;

	ManchesterTolerance tolerance;
	if( text_archive >> *(U32*)&tolerance )
		mTolerance = tolerance;

	ClearChannels();
	AddChannel( mInputChannel, "Manchester", true );

	UpdateInterfacesFromSettings();
}

const char* ManchesterAnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

	text_archive << "SaleaeManchesterAnalyzer";
	text_archive << mInputChannel;
	text_archive << U32( mMode );
	text_archive << mBitRate;
	text_archive << mInverted;
	text_archive << mBitsPerTransfer;
	text_archive << U32( mShiftOrder );
	text_archive << mBitsToIgnore;
	text_archive << U32( mTolerance );

	return SetReturnString( text_archive.GetString() );
}

void ManchesterAnalyzerSettings::UpdateInterfacesFromSettings()
{
	mInputChannelInterface->SetChannel( mInputChannel );
	mModeInterface->SetNumber( mMode );
	mBitRateInterface->SetInteger( mBitRate );
	mInvertedInterface->SetNumber( mInverted );
	mBitsPerTransferInterface->SetNumber( mBitsPerTransfer );
	mShiftOrderInterface->SetNumber( mShiftOrder );
	mNumBitsIgnoreInterface->SetInteger( mBitsToIgnore );
	mToleranceInterface->SetNumber( mTolerance );
}

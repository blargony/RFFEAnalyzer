#include "HdlcSimulationDataGenerator.h"
#include "HdlcAnalyzerSettings.h"
#include <AnalyzerHelpers.h>
#include <algorithm>

HdlcSimulationDataGenerator::HdlcSimulationDataGenerator() :
	mSettings ( 0 ), mSimulationSampleRateHz ( 0 ), mFrameNumber ( 0 ), mAbortByte ( 0 ),
	mWrongFramesSeparation ( 0 ), mControlValue ( 0 ), mAddresByteValue ( 0 ), mInformationByteValue ( 0 ),
	mSamplesInHalfPeriod ( 0 ), mSamplesInAFlag ( 0 )

{
	mFrameTypes[ 0 ] = HDLC_I_FRAME;
	mFrameTypes[ 1 ] = HDLC_S_FRAME;
	mFrameTypes[ 2 ] = HDLC_U_FRAME;
}

HdlcSimulationDataGenerator::~HdlcSimulationDataGenerator()
{
}

void HdlcSimulationDataGenerator::Initialize ( U32 simulation_sample_rate, HdlcAnalyzerSettings* settings )
{
	mSimulationSampleRateHz = simulation_sample_rate;
	mSettings = settings;

	mHdlcSimulationData.SetChannel ( mSettings->mInputChannel );
	mHdlcSimulationData.SetSampleRate ( simulation_sample_rate );
	mHdlcSimulationData.SetInitialBitState ( BIT_LOW );

	// Initialize rng seed
	srand ( 5 );

	mSamplesInHalfPeriod = U64 ( simulation_sample_rate / double ( mSettings->mBitRate ) );
	mSamplesInAFlag = mSamplesInHalfPeriod * 7;

	mHdlcSimulationData.Advance ( mSamplesInHalfPeriod * 8 ); // Advance 4 periods
	GenerateAbortFramesIndexes();
	mAbortByte = 0;
	mFrameNumber = 0;
	mWrongFramesSeparation = ( rand() % 10 ) + 10; // [15..30]

	mControlValue=0;
	mAddresByteValue=0;
	mInformationByteValue=0;

}

void HdlcSimulationDataGenerator::GenerateAbortFramesIndexes()
{
	mAbortFramesIndexes.push_back ( rand() % 50 );
	mAbortFramesIndexes.push_back ( rand() % 50 );
	mAbortFramesIndexes.push_back ( rand() % 50 );
	mAbortFramesIndexes.push_back ( rand() % 50 );
	mAbortFramesIndexes.push_back ( rand() % 50 );
	mAbortFramesIndexes.push_back ( rand() % 50 );
}

bool HdlcSimulationDataGenerator::ContainsElement ( U32 index ) const
{
	for ( U32 i=0; i < mAbortFramesIndexes.size(); ++i )
	{
		if ( mAbortFramesIndexes.at ( i ) == index )
		{
			return true;
		}
	}
	return false;
}

U32 HdlcSimulationDataGenerator::GenerateSimulationData ( U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel )
{
	U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample ( largest_sample_requested, sample_rate, mSimulationSampleRateHz );

	while ( mHdlcSimulationData.GetCurrentSampleNumber() < adjusted_largest_sample_requested )
	{

		// Two consecutive flags
		CreateFlag();
		CreateFlag();

		HdlcFrameType frameType = mFrameTypes[ mFrameNumber%3 ];
		U32 sizeOfInformation = ( frameType == HDLC_S_FRAME ) ? 0 : ( ( rand() % 4 ) + 1 );
		U64 addressBytes= ( ( rand() % 4 ) + 1 );

		vector<U8> address = GenAddressField ( mSettings->mHdlcAddr, addressBytes, mAddresByteValue++ );
		vector<U8> control = GenControlField ( frameType, mSettings->mHdlcControl, mControlValue++ );
		vector<U8> information = GenInformationField ( sizeOfInformation, mInformationByteValue++ );

		CreateHDLCFrame ( address, control, information );

		// Two consecutive flags
		CreateFlag();
		CreateFlag();

		mFrameNumber++;
	}

	*simulation_channel = &mHdlcSimulationData;
	return 1;
}

void HdlcSimulationDataGenerator::CreateFlag()
{
	if ( mSettings->mTransmissionMode == HDLC_TRANSMISSION_BIT_SYNC )
	{
		CreateFlagBitSeq();
	}
	else // HDLC_TRANSMISSION_BYTE_ASYNC
	{
		CreateAsyncByte ( HDLC_FLAG_VALUE );
	}
}


vector<U8> HdlcSimulationDataGenerator::GenAddressField ( HdlcAddressType addressType,
        U64 addressBytes,
        U8 value ) const
{
	vector<U8> addrRet;
	if ( addressType == HDLC_BASIC_ADDRESS_FIELD )
	{
		addrRet.push_back ( value );
	}
	else // addressType == HDLC_EXTENDED_ADDRESS_FIELD
	{
		for ( U32 i=0; i < addressBytes; ++i )
		{
			U8 mask = ( i == addressBytes - 1 ) ? 0x00 : 0x01; // EA bit (Lsb is set to 1 to extend the address recursively)
			U8 extValue =  ( value & 0xFE ) | mask;
			addrRet.push_back ( extValue );
		}
	}
	return addrRet;
}

// ISO/IEC 13239:2002(E) page 26
vector<U8> HdlcSimulationDataGenerator::GenControlField ( HdlcFrameType frameType,
        HdlcControlType controlType,
        U8 value ) const
{
	vector<U8> controlRet;
	U8 ctrl;
	switch ( frameType )
	{
	case HDLC_I_FRAME:
		ctrl = ( value & 0xFE ) | U8 ( frameType );
		break;
	case HDLC_S_FRAME:
		ctrl = ( value & 0xFC ) | U8 ( frameType );
		break;
	case HDLC_U_FRAME:
		ctrl = value | U8 ( HDLC_U_FRAME );
	}

	switch ( frameType )
	{
	case HDLC_I_FRAME:
	case HDLC_S_FRAME:
	{
		// first byte
		controlRet.push_back ( ctrl );
		switch ( controlType )
		{
		case HDLC_EXTENDED_CONTROL_FIELD_MOD_128:
			controlRet.push_back ( value ); // second byte
			break;
		case HDLC_EXTENDED_CONTROL_FIELD_MOD_32768:
			controlRet.push_back ( value ); // second byte
			controlRet.push_back ( value ); // third byte
			controlRet.push_back ( value ); // fourth byte
			break;
		case HDLC_EXTENDED_CONTROL_FIELD_MOD_2147483648:
			controlRet.push_back ( value ); // second byte
			controlRet.push_back ( value ); // third byte
			controlRet.push_back ( value ); // fourth byte
			controlRet.push_back ( value ); // fifth byte
			controlRet.push_back ( value ); // sixth byte
			controlRet.push_back ( value ); // seventh byte
			controlRet.push_back ( value ); // eighth byte
			break;
		}
		break;
	}
	case HDLC_U_FRAME: // U frames are always of 8 bits
	{
		controlRet.push_back ( ctrl );
		break;
	}
	}
	return controlRet;
}

vector<U8> HdlcSimulationDataGenerator::GenInformationField ( U16 size, U8 value ) const
{
	vector<U8> informationRet ( size, value );
	return informationRet;
}

void HdlcSimulationDataGenerator::CreateHDLCFrame ( const vector<U8> & address, const vector<U8> & control,
        const vector<U8> & information )
{
	vector<U8> allFields;

	allFields.insert ( allFields.end(), address.begin(), address.end() );
	allFields.insert ( allFields.end(), control.begin(), control.end() );
	allFields.insert ( allFields.end(), information.begin(), information.end() );

	// Calculate the crc of the address, control and data fields
	vector<U8> fcs = GenFcs ( mSettings->mHdlcFcs, allFields );
	allFields.insert ( allFields.end(), fcs.begin(), fcs.end() );

	// Transmit the frame in bit-sync or byte-async
	if ( mSettings->mTransmissionMode == HDLC_TRANSMISSION_BIT_SYNC )
	{
		TransmitBitSync ( allFields );
	}
	else
	{
		TransmitByteAsync ( allFields );
	}
}

vector<U8> HdlcSimulationDataGenerator::GenFcs ( HdlcFcsType fcsType, const vector<U8> & stream ) const
{
	vector<U8> crcRet;
	switch ( fcsType )
	{
	case HDLC_CRC8:
		crcRet = Crc8 ( stream );
		break;
	case HDLC_CRC16:
		crcRet = Crc16 ( stream );
		break;
	case HDLC_CRC32:
		crcRet = Crc32 ( stream );
		break;
	}
	return crcRet;
}

void HdlcSimulationDataGenerator::TransmitBitSync ( const vector<U8> & stream )
{
	// Opening flag
	CreateFlagBitSeq();

	bool abortFrame = ContainsElement ( mFrameNumber );

	U8 consecutiveOnes = 0;
	BitState previousBit = BIT_LOW;
	// For each byte of the stream
	U32 index=0;
	for ( U32 s=0; s<stream.size(); ++s )
	{

		bool abortThisByte = ( mAbortByte == s );
		if ( abortFrame && abortThisByte )
		{
			// Sync bit abort sequence = 7 or more consecutive 1
			for ( U32 j=0; j < 7; ++j )
			{
				CreateSyncBit ( BIT_HIGH );
			}
			mAbortByte++;
			return;
		}

		// For each bit of the byte stream
		BitExtractor bit_extractor ( stream[ s ], AnalyzerEnums::LsbFirst, 8 );
		for ( U32 i=0; i<8; ++i )
		{
			BitState bit = bit_extractor.GetNextBit();
			CreateSyncBit ( bit );

			if ( bit == BIT_HIGH )
			{
				if ( previousBit == BIT_HIGH )
				{
					consecutiveOnes++;
				}
				else
				{
					consecutiveOnes = 0;
				}
			}
			else // bit low
			{
				consecutiveOnes = 0;
			}

			if ( consecutiveOnes == 4 ) // if five 1s in a row, then insert a 0 and continue
			{
				CreateSyncBit ( BIT_LOW );
				consecutiveOnes = 0;
				previousBit = BIT_LOW;
			}
			else
			{
				previousBit = bit;
			}
			index++;
		}
	}

	// Closing flag
	CreateFlagBitSeq();

}

void HdlcSimulationDataGenerator::CreateFlagBitSeq()
{
	mHdlcSimulationData.Transition();

	mHdlcSimulationData.Advance ( mSamplesInAFlag );
	mHdlcSimulationData.Transition();

	mHdlcSimulationData.Advance ( mSamplesInHalfPeriod );

}

// Maps the bit to the signal using NRZI
void HdlcSimulationDataGenerator::CreateSyncBit ( BitState bitState )
{
	if ( bitState == BIT_LOW ) // BIT_LOW == transition, BIT_HIGH == no transition
	{
		mHdlcSimulationData.Transition();
	}
	mHdlcSimulationData.Advance ( mSamplesInHalfPeriod );
}

void HdlcSimulationDataGenerator::TransmitByteAsync ( const vector<U8> & stream )
{
	// Opening flag
	CreateAsyncByte ( HDLC_FLAG_VALUE );

	bool abortFrame = ContainsElement ( mFrameNumber );

	for ( U32 i=0; i < stream.size(); ++i )
	{

		bool abortThisByte = ( mAbortByte == i );
		if ( abortFrame && abortThisByte ) // Abort the frame: ABORT SEQUENCE = ESCAPE + FLAG
		{
			CreateAsyncByte ( HDLC_ESCAPE_SEQ_VALUE );
			CreateAsyncByte ( HDLC_FLAG_VALUE );
			AsyncByteFill ( 7 );
			mAbortByte++;
			return;
		}

		const U8 byte = stream[ i ];
		switch ( byte )
		{
		case HDLC_FLAG_VALUE: // 0x7E
			CreateAsyncByte ( HDLC_ESCAPE_SEQ_VALUE );			// 7D escape
			CreateAsyncByte ( HdlcAnalyzerSettings::Bit5Inv ( HDLC_FLAG_VALUE ) );		// 5E
			break;
		case HDLC_ESCAPE_SEQ_VALUE: // 0x7D
			CreateAsyncByte ( HDLC_ESCAPE_SEQ_VALUE );			// 7D escape
			CreateAsyncByte ( HdlcAnalyzerSettings::Bit5Inv ( HDLC_ESCAPE_SEQ_VALUE ) );	// 5D
			break;
		default:
			CreateAsyncByte ( byte );							// normal byte
		}

		// Fill between bytes (0 to 7 bits of value 1)
		AsyncByteFill ( rand() % 8 );
	}

	// Closing flag
	CreateAsyncByte ( HDLC_FLAG_VALUE );

}

void HdlcSimulationDataGenerator::AsyncByteFill ( U32 N )
{
	// 0) If the line is not high we must set it high
	if ( mHdlcSimulationData.GetCurrentBitState() == BIT_LOW )
	{
		mHdlcSimulationData.Transition();
	}
	// 1) Fill N high periods
	mHdlcSimulationData.Advance ( mSamplesInHalfPeriod * N );
}

// ISO/IEC 13239:2002(E) page 17
void HdlcSimulationDataGenerator::CreateAsyncByte ( U8 byte )
{

	// 0) If the line is not high we must set it high
	if ( mHdlcSimulationData.GetCurrentBitState() == BIT_LOW )
	{
		mHdlcSimulationData.Transition();
		mHdlcSimulationData.Advance ( mSamplesInHalfPeriod );
	}

	// 1) Start bit (BIT_HIGH -> BIT_LOW)
	mHdlcSimulationData.TransitionIfNeeded ( BIT_LOW );
	mHdlcSimulationData.Advance ( mSamplesInHalfPeriod );

	// 2) Transmit byte
	BitExtractor bit_extractor ( byte, AnalyzerEnums::LsbFirst, 8 );
	for ( U32 i=0; i < 8; ++i )
	{
		BitState bit = bit_extractor.GetNextBit();
		mHdlcSimulationData.TransitionIfNeeded ( bit );
		mHdlcSimulationData.Advance ( mSamplesInHalfPeriod );
	}

	// 3) Stop bit (BIT_LOW -> BIT_HIGH)
	mHdlcSimulationData.TransitionIfNeeded ( BIT_HIGH );
	mHdlcSimulationData.Advance ( mSamplesInHalfPeriod );

}

//
////////////////////// Static functions /////////////////////////////////////////////////////
//

vector<BitState> HdlcSimulationDataGenerator::BytesVectorToBitsVector ( const vector<U8> & v, U32 numberOfBits )
{
	vector<BitState> bitsRet;
	U32 vectorIndex = 0;
	U8 byte;
	bool getByte = true;
	U8 bytePos = 0x80;
	for ( U32 i=0; i < numberOfBits; ++i )
	{
		if ( getByte )
		{
			byte = v.at ( vectorIndex );
			bytePos = 0x80;
			vectorIndex++;
		}

		BitState bit = ( byte & bytePos ) ? BIT_HIGH : BIT_LOW;
		bitsRet.push_back ( bit );

		bytePos >>= 1;

		getByte = ( ( i+1 ) % 8 == 0 );

	}

	return bitsRet;
}

vector<U8> HdlcSimulationDataGenerator::CrcDivision ( const vector<U8> & stream, const vector<U8> & genPoly, U32 crcNumber )
{

	vector<BitState> dataBits = BytesVectorToBitsVector ( stream, stream.size() * 8 );
	vector<BitState> polyBits = BytesVectorToBitsVector ( genPoly, crcNumber + 1 );

	U32 dataIndex=0;
	U32 dataLimit = dataBits.size() - ( polyBits.size() - 1 );
	while ( dataIndex < dataLimit )
	{

		// Advance one-position or 0-bits
		bool zeroBits = true;
		while ( zeroBits )
		{
			zeroBits = ( ( dataBits.at ( dataIndex ) == BIT_LOW ) &&
			             ( dataIndex < dataLimit ) );
			if ( zeroBits )
			{
				dataIndex++;
			}
		}

		if ( dataIndex < dataLimit )
		{
			for ( U32 bitIndex = 0; bitIndex < polyBits.size(); ++bitIndex )
			{
				BitState bit = dataBits.at ( dataIndex + bitIndex );
				BitState polyBit = polyBits.at ( bitIndex );

				dataBits[ dataIndex + bitIndex ] = BitState ( bit ^ polyBit );
			}
		}

		dataIndex++;

	}

	// put the crc result in the vector of bytes
	vector<U8> crcRet;
	U8 offset = crcNumber;
	for ( U32 s=0; s < crcNumber / 8; ++s )
	{
		U64 byteValue= 0;
		DataBuilder dbyte;
		dbyte.Reset ( &byteValue, AnalyzerEnums::MsbFirst, 8 );
		for ( U32 i=dataBits.size() - offset; i < dataBits.size() - offset + 8; ++i )
		{
			dbyte.AddBit ( dataBits.at ( i ) );
		}
		offset -= 8;
		crcRet.push_back ( byteValue );
	}

	return crcRet;

}

vector<U8> HdlcSimulationDataGenerator::Crc8 ( const vector<U8> & stream )
{
	vector<U8> result = stream;
	result.push_back ( 0x00 );

	// ISO/IEC 13239:2002(E) page 14
	// CRC8 Divisor (9 bits) - x**8 + x**2 + x + 1
	vector<U8> divisor;
	divisor.push_back ( 0x83 );
	divisor.push_back ( 0x80 );

	vector<U8> crc8Ret = CrcDivision ( result, divisor, 8 );
	return crc8Ret;
}

vector<U8> HdlcSimulationDataGenerator::Crc16 ( const vector<U8> & stream )
{
	vector<U8> result = stream;

	// Append 16 0-bits
	result.push_back ( 0x00 );
	result.push_back ( 0x00 );

	// ISO/IEC 13239:2002(E) page 14
	// CRC16 Divisor (17 bits) - x**16 + x**12 + x**5 + 1 (0x1021)
	vector<U8> divisor;
	divisor.push_back ( 0x88 );
	divisor.push_back ( 0x10 );
	divisor.push_back ( 0x80 );

	vector<U8> crc16Ret = CrcDivision ( result, divisor, 16 );

	return crc16Ret;
}

vector<U8> HdlcSimulationDataGenerator::Crc32 ( const vector<U8> & stream )
{
	vector<U8> result = stream;
	// Append 32 0-bits
	result.push_back ( 0x00 );
	result.push_back ( 0x00 );
	result.push_back ( 0x00 );
	result.push_back ( 0x00 );

	// ISO/IEC 13239:2002(E) page 13
	// CRC32 Divisor (33 bits)
	vector<U8> divisor;
	divisor.push_back ( 0x82 );
	divisor.push_back ( 0x60 );
	divisor.push_back ( 0x8E );
	divisor.push_back ( 0xDB );
	divisor.push_back ( 0x80 );

	vector<U8> crc32Ret = CrcDivision ( result, divisor, 32 );
	return crc32Ret;

}

#include "HdmiCecAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "HdmiCecAnalyzer.h"
#include "HdmiCecAnalyzerSettings.h"
#include "HdmiCecProtocol.h"

#include <fstream>
#include <sstream>

HdmiCecAnalyzerResults::HdmiCecAnalyzerResults( HdmiCecAnalyzer* analyzer, HdmiCecAnalyzerSettings* settings )
:	AnalyzerResults(),
    mSettings( settings ),
    mAnalyzer( analyzer )
{
}

HdmiCecAnalyzerResults::~HdmiCecAnalyzerResults()
{
}

void HdmiCecAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& /*channel*/, DisplayBase display_base )
{
    GenBubbleText( frame_index, display_base, false );
}

void HdmiCecAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
    Frame frame = GetFrame( frame_index );
    mDisplayBase = display_base;

    ClearTabularText();
  
    switch( frame.mType )
    {
        case HdmiCec::FrameType_StartSeq:
            AddTabularText( "Start Sequence" );
            break;
        case HdmiCec::FrameType_Header:
			{
				const U8 src = ( frame.mData1 >> 4 ) & 0xF;
				const U8 dst = ( frame.mData1 >> 0 ) & 0xF;
				std::string srcStr = GetNumberString( src, 4 );
				std::string dstStr = GetNumberString( dst, 4 );
				std::string srcName = HdmiCec::GetDevAddressString(
                static_cast<HdmiCec::DevAddress>( src) );
				std::string dstName = HdmiCec::GetDevAddressString(
                static_cast<HdmiCec::DevAddress>( dst ) );
				std::string str = "Header SRC=" + srcStr + " (" + srcName + "), DST=" + dstStr + " ("+dstName+")";
				AddTabularText( str.c_str() );
			}
            break;

        case HdmiCec::FrameType_OpCode:
			{
				HdmiCec::OpCode opCode = static_cast<HdmiCec::OpCode>( frame.mData1 );
				std::string opCodeStr = GetNumberString( frame.mData1, 8 );
				std::string opCodeText = HdmiCec::GetOpCodeString( opCode );

				std::string str = "Opcode " + opCodeStr + " (" + opCodeText + ")";
			    AddTabularText( str.c_str() );
			}
            break;
        case HdmiCec::FrameType_Operand:
			{
				std::string dataStr = GetNumberString( frame.mData1, 8 );

				std::string str = "Data " + dataStr;
				
			}
            
            break;
        case HdmiCec::FrameType_EOM:
			{
				 bool eom = frame.mData1;

				 std::string str = "End of Message = " + std::string(eom ? "1" : "0");
				 AddTabularText( str.c_str() );
			}
            break;
        case HdmiCec::FrameType_ACK:
			{
				 bool ack = frame.mData1;
				 std::string str = "Acknowledgment = " + std::string(ack ? "1" : "0");
				 AddTabularText( str.c_str() );
			}
            break;
        default:
            break;
    }
}

void HdmiCecAnalyzerResults::GeneratePacketTabularText( U64 /*packet_id*/, DisplayBase /*display_base*/ )
{
    ClearResultStrings();
    AddResult( "Not supported" );
}

void HdmiCecAnalyzerResults::GenerateTransactionTabularText( U64 /*transaction_id*/, DisplayBase /*display_base*/ )
{
    ClearResultStrings();
    AddResult( "Not supported" );
}

void HdmiCecAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 /*export_type_user_id*/ )
{
    std::ofstream file_stream( file, std::ios::out );

    mDisplayBase = display_base;
    U64 trigger_sample = mAnalyzer->GetTriggerSample();
    U32 sample_rate = mAnalyzer->GetSampleRate();

    file_stream << "Time [s],Block ID,Type,Data,Data Desc,EOM,ACK" << std::endl;

    // String to fill "not applicable" fields
    const std::string naString = "N/A";

    const U64 frameCount = GetNumFrames();        

    U64 frameId = 0;
    U64 blockId = 0;
    int blockField = 0; // 0->byte, 1->EOM, 2->ACK

    // Loop through all the frames, add one line per block
    while( frameId < frameCount )
    {
        Frame frame = GetFrame( frameId );
        HdmiCec::FrameType frameType = static_cast<HdmiCec::FrameType>( frame.mType );

        // Skip start sequence frames
        if( frameType == HdmiCec::FrameType_StartSeq )
        {
            frameId++;
            continue;
        }

        if( blockField == 0 )
        {
            // Add timestamp and block Id
            const int strLen = 128;
            char timeStr[strLen];
            AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample,
                                            sample_rate, timeStr, strLen );

            std::string typeDesc = HdmiCec::GetFrameTypeString( frameType );
            std::string dataCode = GetNumberString( frame.mData1, 8 );

            std::string dataDesc = naString;
            if( frameType == HdmiCec::FrameType_Header )
            {
                const U8 src = ( frame.mData1 >> 4 ) & 0xF;
                const U8 dst = ( frame.mData1 >> 0 ) & 0xF;
                std::string srcStr = GetNumberString( src, 4 );
                std::string dstStr = GetNumberString( dst, 4 );
                std::string srcName = HdmiCec::GetDevAddressString( static_cast<HdmiCec::DevAddress>( src ) );
                std::string dstName = HdmiCec::GetDevAddressString( static_cast<HdmiCec::DevAddress>( dst ) );
                dataDesc = "SRC=" + srcStr + " (" + srcName + ") DST=" + dstStr + " ("+dstName+")";
            }
            else if( frameType == HdmiCec::FrameType_OpCode )
            {
                HdmiCec::OpCode opCode = static_cast<HdmiCec::OpCode>( frame.mData1 );
                dataDesc = HdmiCec::GetOpCodeString( opCode );
            }
            else if( frameType == HdmiCec::FrameType_Operand )
            {
                dataDesc = naString;
            }
            else
            {
                // This should not happen
                frameId++;
                continue;
            }

            file_stream << timeStr << ",";
            file_stream << blockId << ",";
            file_stream << typeDesc << ",";
            file_stream << dataCode << ",";
            file_stream << dataDesc << ",";

            blockField++;
        }
        else if( blockField == 1 && frameType == HdmiCec::FrameType_EOM )
        {
            file_stream << frame.mData1 << ",";
            blockField++;
        }
        else if( blockField == 2 && frameType == HdmiCec::FrameType_ACK )
        {
            file_stream << frame.mData1 << std::endl;
            blockField = 0;
            blockId++;
        }
        else
        {
            // There was an error
            blockField = 0;
            blockId++;
            frameId++;
            continue;
        }

        if( UpdateExportProgressAndCheckForCancel( frameId, frameCount ) )
        {
            file_stream.close();
            return;
        }

        frameId++;
    }

    UpdateExportProgressAndCheckForCancel( frameCount, frameCount );
    file_stream.close();
}

//
// Bubble generation
//

void HdmiCecAnalyzerResults::GenBubbleText( U64 frame_index, DisplayBase display_base, bool tabular )
{
    ClearResultStrings();
    Frame frame = GetFrame( frame_index );
    mDisplayBase = display_base;
    mTabular = tabular;

    switch( frame.mType )
    {
        case HdmiCec::FrameType_StartSeq:
            GenStartSeqBubble();
            break;
        case HdmiCec::FrameType_Header:
            GenHeaderBubble( frame );
            break;
        case HdmiCec::FrameType_OpCode:
            GenOpCodeBubble( frame );
            break;
        case HdmiCec::FrameType_Operand:
            GenOperandBubble( frame );
            break;
        case HdmiCec::FrameType_EOM:
            GenEOMBubble( frame );
            break;
        case HdmiCec::FrameType_ACK:
            GenACKBubble( frame );
            break;
        default:
            break;
    }
}

void HdmiCecAnalyzerResults::GenStartSeqBubble()
{
    if( !mTabular )
    {
        AddResult( "S" );
        AddResult( "Start" );
        AddResult( "Start Seq." );
    }
    AddResult( "Start Sequence" );
}

void HdmiCecAnalyzerResults::GenHeaderBubble( const Frame& frame )
{
    const U8 src = ( frame.mData1 >> 4 ) & 0xF;
    const U8 dst = ( frame.mData1 >> 0 ) & 0xF;
    std::string srcStr = GetNumberString( src, 4 );
    std::string dstStr = GetNumberString( dst, 4 );

    if( !mTabular )
    {
        AddResult( "H" );
        AddResult( "H " + srcStr + " to " + dstStr );
        AddResult( "Header SRC=" + srcStr + ", DST=" + dstStr );
    }

    std::string srcName = HdmiCec::GetDevAddressString(
                static_cast<HdmiCec::DevAddress>( src) );
    std::string dstName = HdmiCec::GetDevAddressString(
                static_cast<HdmiCec::DevAddress>( dst ) );

    AddResult( "Header SRC=" + srcStr + " (" + srcName + "), DST=" + dstStr + " ("+dstName+")" );
}

void HdmiCecAnalyzerResults::GenOpCodeBubble( const Frame& frame )
{
    HdmiCec::OpCode opCode = static_cast<HdmiCec::OpCode>( frame.mData1 );
    std::string opCodeStr = GetNumberString( frame.mData1, 8 );
    std::string opCodeText = HdmiCec::GetOpCodeString( opCode );

    if( !mTabular )
    {
        AddResult( "O" );
        AddResult( "Op. " + opCodeStr );
        AddResult( "Opcode " + opCodeStr );
    }
    AddResult( "Opcode " + opCodeStr + " (" + opCodeText + ")" );
}

void HdmiCecAnalyzerResults::GenOperandBubble( const Frame& frame )
{
    std::string dataStr = GetNumberString( frame.mData1, 8 );

    if( !mTabular )
    {
        AddResult( "D" );
        AddResult( "Data" );
    }
    AddResult( "Data " + dataStr );
}

void HdmiCecAnalyzerResults::GenEOMBubble( const Frame& frame )
{
    bool eom = frame.mData1;

    if( !mTabular )
    {
        AddResult( "E" );
        AddResult( eom ? "E=1" : "E=0" );
        AddResult( eom ? "EOM=1" : "EOM=0" );
    }
    AddResult( "End of Message = " + std::string(eom ? "1" : "0") );
}

void HdmiCecAnalyzerResults::GenACKBubble( const Frame& frame )
{
    bool ack = frame.mData1;

    if( !mTabular )
    {
        AddResult( "A" );
        AddResult( ack ? "A=1" : "A=0" );
        AddResult( ack ? "ACK=1" : "ACK=0" );
    }
    AddResult( "Acknowledgment = " + std::string(ack ? "1" : "0") );
}

//
// std::string wrappers
//

std::string HdmiCecAnalyzerResults::GetNumberString( U64 number, int bits )
{
    const int strLen = 128;
    char str[strLen];
    AnalyzerHelpers::GetNumberString( number, mDisplayBase, bits, str, strLen );
    return std::string( str );
}

void HdmiCecAnalyzerResults::AddResult( const std::string& str )
{
    AddResultString( str.c_str() );
}

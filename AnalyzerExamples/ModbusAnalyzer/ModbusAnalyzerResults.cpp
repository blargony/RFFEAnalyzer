#include "ModbusAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "ModbusAnalyzer.h"
#include "ModbusAnalyzerSettings.h"
#include <iostream>
#include <sstream>


#pragma warning(disable: 4996) //warning C4996: 'sprintf': This function or variable may be unsafe. Consider using sprintf_s instead.

ModbusAnalyzerResults::ModbusAnalyzerResults( ModbusAnalyzer* analyzer, ModbusAnalyzerSettings* settings )
:	AnalyzerResults(),
	mSettings( settings ),
	mAnalyzer( analyzer )
{
}

ModbusAnalyzerResults::~ModbusAnalyzerResults()
{
}

void ModbusAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& /*channel*/, DisplayBase display_base )  //unrefereced vars commented out to remove warnings.
{
	//we only need to pay attention to 'channel' if we're making bubbles for more than one channel (as set by AddChannelBubblesWillAppearOn)
	ClearResultStrings();
	Frame frame = GetFrame( frame_index );

	bool framing_error = false;
	if( ( frame.mFlags & FRAMING_ERROR_FLAG ) != 0 )
		framing_error = true;

	bool parity_error = false;
	if( ( frame.mFlags & PARITY_ERROR_FLAG ) != 0 )
		parity_error = true;

	U32 bits_per_transfer = mSettings->mBitsPerTransfer;
	if( mSettings->mModbusMode != ModbusAnalyzerEnums::Normal )
		bits_per_transfer--;

	if(mSettings->mModbusMode==ModbusAnalyzerEnums::ModbusASCIIMaster || mSettings->mModbusMode==ModbusAnalyzerEnums::ModbusASCIISlave || mSettings->mModbusMode==ModbusAnalyzerEnums::ModbusRTUMaster || mSettings->mModbusMode==ModbusAnalyzerEnums::ModbusRTUSlave)
	{
		char DeviceAddrStr[128];
		U8 DeviceAddr = (frame.mData1 & 0xFF00000000000000)>>56;
		AnalyzerHelpers::GetNumberString( DeviceAddr, display_base, bits_per_transfer, DeviceAddrStr, 128 );

		char FunctionCodeStr[128];
		U8 FunctionCode = (frame.mData1 & 0x00FF000000000000)>>48;
		AnalyzerHelpers::GetNumberString( FunctionCode, display_base, bits_per_transfer, FunctionCodeStr, 128 );

		char Payload1Str[128];
		U16 Payload1 = (frame.mData1 & 0x0000FFFF00000000)>>32;
		AnalyzerHelpers::GetNumberString( Payload1, display_base, bits_per_transfer, Payload1Str, 128 );

		char Payload2Str[128];
		U16 Payload2 = (frame.mData1 & 0x00000000FFFF0000)>>16;
		AnalyzerHelpers::GetNumberString( Payload2, display_base, bits_per_transfer, Payload2Str, 128 );

		char Payload3Str[128];
		U16 Payload3 = (frame.mData2 & 0x000000000000FFFF);
		AnalyzerHelpers::GetNumberString( Payload3, display_base, bits_per_transfer, Payload3Str, 128 );

		char Payload4Str[128];
		U16 Payload4 = (frame.mData2 & 0x00000000FFFF0000)>>16;
		AnalyzerHelpers::GetNumberString( Payload4, display_base, bits_per_transfer, Payload4Str, 128 );

		char ChecksumStr[128];
		U16 Checksum = (frame.mData1 & 0x000000000000FFFF);
		AnalyzerHelpers::GetNumberString( Checksum, display_base, bits_per_transfer, ChecksumStr, 128 );

		char result_str[256];
		char Error_str[128];

		if(frame.mFlags&FLAG_REQUEST_FRAME)
		{
			AddResultString( FunctionCodeStr);
			switch(FunctionCode)
			{
			case FUNCCODE_READ_COILS:
				AddResultString( "Read Coils" );
				sprintf( result_str, "DeviceID: %s, Func: Read Coils (%s), StartAddr: %s, Qty: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
				break;
			case FUNCCODE_READ_DISCRETE_INPUTS:
				AddResultString( "Read Discrete Inputs" );
				sprintf( result_str, "DeviceID: %s, Func: Read Discrete Inputs (%s), StartAddr: %s, Qty: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
				break;
			case FUNCCODE_READ_HOLDING_REGISTERS:
				AddResultString( "Read Holding Registers" );
				sprintf( result_str, "DeviceID: %s, Func: Holding Registers (%s), StartAddr: %s, Qty: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
				break;
			case FUNCCODE_READ_INPUT_REGISTER:
				AddResultString( "Read Input Registers" );
				sprintf( result_str, "DeviceID: %s, Func: Input Registers (%s), StartAddr: %s, Qty: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
				break;
			case FUNCCODE_WRITE_SINGLE_COIL:
				AddResultString( "Write Single Coil" );
				sprintf( result_str, "DeviceID: %s, Func: Write Single Coil (%s), Addr: %s, Value: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
				break;
			case FUNCCODE_WRITE_SINGLE_REGISTER:
				AddResultString( "Write Single Register" );
				sprintf( result_str, "DeviceID: %s, Func: Write Single Register (%s), Addr: %s, Value: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
				break;
			case FUNCCODE_READ_EXCEPTION_STATUS:
				AddResultString( "Read Exception Status" );
				sprintf( result_str, "DeviceID: %s, Func: Read Exception Status (%s), ChkSum: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
				break;
			case FUNCCODE_DIAGNOSTIC:

				switch(Payload1)
				{
				case RETURN_QUERY_DATA:
					AddResultString( "Diagnostics" );
					AddResultString( "Diagnostics - Return Query Data" );
					sprintf( result_str, "DeviceID: %s, Func: Diagnostics (%s), SubFunc: Return Query Data (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case RESTART_COMMUNICATIONS_OPTION:
					AddResultString( "Diagnostics" );
					AddResultString( "Diagnostics - Restart Comms Option" );
					sprintf( result_str, "DeviceID: %s, Func: Diagnostics (%s), SubFunc: Restart Comms Option (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case RETURN_DIAGNOSTIC_REGISTER:
					AddResultString( "Diagnostics" );
					AddResultString( "Diagnostics - Return Diag Reg" );
					sprintf( result_str, "DeviceID: %s, Func: Diagnostics (%s), SubFunc: Return Diag Reg (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case CHANGE_ASCII_INPUT_DELIM:
					AddResultString( "Diagnostics" );
					AddResultString( "Diagnostics - Change ASCII Input Delim" );
					sprintf( result_str, "DeviceID: %s, Func: Diagnostics (%s), SubFunc: Change ASCII Input Delim (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case FORCE_LISTEN_ONLY_MODE:
					AddResultString( "Diagnostics" );
					AddResultString( "Diagnostics - Force Listen Only Mode" );
					sprintf( result_str, "DeviceID: %s, Func: Diagnostics (%s), SubFunc: Force Listen Only Mode (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case CLEAR_COUNTERS_AND_DIAG_REGISTER:
					AddResultString( "Diagnostics" );
					AddResultString( "Diagnostics - Clr Counters And Diag Reg" );
					sprintf( result_str, "DeviceID: %s, Func: Diagnostics (%s), SubFunc: Clr Counters And Diag Reg (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case RETURN_BUS_MESSAGE_COUNT:
					AddResultString( "Diagnostics" );
					AddResultString( "Diagnostics - Bus Msg Cnt" );
					sprintf( result_str, "DeviceID: %s, Func: Diagnostics (%s), SubFunc: Bus Msg Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case RETURN_BUS_COMM_ERROR_COUNT:
					AddResultString( "Diagnostics" );
					AddResultString( "Diagnostics - Bus Comm Err Cnt" );
					sprintf( result_str, "DeviceID: %s, Func: Diagnostics (%s), SubFunc: Bus Comm Err Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case RETURN_BUS_EXCEPTION_ERROR_COUNT:
					AddResultString( "Diagnostics" );
					AddResultString( "Diagnostics - Bus Excpt Err Cnt" );
					sprintf( result_str, "DeviceID: %s, Func: Diagnostics (%s), SubFunc: Bus Excpt Err Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case RETURN_SLAVE_MESSAGE_COUNT:
					AddResultString( "Diagnostics" );
					AddResultString( "Diagnostics - Slave Msg Cnt" );
					sprintf( result_str, "DeviceID: %s, Func: Diagnostics (%s), SubFunc: Slave Msg Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case RETURN_SLAVE_NO_RESPONSE_COUNT:
					AddResultString( "Diagnostics" );
					AddResultString( "Diagnostics - Slave No Resp Cnt" );
					sprintf( result_str, "DeviceID: %s, Func: Diagnostics (%s), SubFunc: Slave No Resp Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case RETURN_SLAVE_NAK_COUNT:
					AddResultString( "Diagnostics" );
					AddResultString( "Diagnostics - Slave NAK Cnt" );
					sprintf( result_str, "DeviceID: %s, Func: Diagnostics (%s), SubFunc: Slave NAK Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case RETURN_SLAVE_BUSY_COUNT:
					AddResultString( "Diagnostics" );
					AddResultString( "Diagnostics - Slave Busy Cnt" );
					sprintf( result_str, "DeviceID: %s, Func: Diagnostics (%s), SubFunc: Slave Busy Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case RETURN_BUS_CHAR_OVERRUN_COUNT:
					AddResultString( "Diagnostics" );
					AddResultString( "Diagnostics - Bus Char Overrun Cnt" );
					sprintf( result_str, "DeviceID: %s, Func: Diagnostics (%s), SubFunc: Bus Char Overrun Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case CLEAR_OVERRUN_COUNTER_AND_FLAG:
					AddResultString( "Diagnostics" );
					AddResultString( "Diagnostics - Clear Overrun Cnt" );
					sprintf( result_str, "DeviceID: %s, Func: Diagnostics (%s), SubFunc: Clear Overrun Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				}
				break;
			case FUNCCODE_GET_COM_EVENT_COUNTER:
				AddResultString( "Get Comm Event Counter" );
				sprintf( result_str, "DeviceID: %s, Func: Get Comm Event Counter (%s), ChkSum: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
				break;
			case FUNCCODE_GET_COM_EVENT_LOG:
				AddResultString( "Get Comm Event Log" );
				sprintf( result_str, "DeviceID: %s, Func: Get Comm Event Log (%s), ChkSum: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
				break;
			case FUNCCODE_WRITE_MULTIPLE_COILS:
				AddResultString( "Write Multiple Coils" );
				sprintf( result_str, "DeviceID: %s, Func: Write Multiple Coils (%s), StartAddr: %s, Qty: %s, ByteCount: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
				break;
			case FUNCCODE_WRITE_MULTIPLE_REGISTERS:
				AddResultString( "Write Multiple Registers" );
				sprintf( result_str, "DeviceID: %s, Func: Write Multiple Registers (%s), StartAddr: %s, Qty: %s, ByteCount: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
				break;
			case FUNCCODE_REPORT_SLAVE_ID:
				AddResultString( "Report Slave ID" );
				sprintf( result_str, "DeviceID: %s, Func: Report Slave ID (%s), ChkSum: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
				break;
			case FUNCCODE_READ_FILE_RECORD:
				AddResultString( "Read File Record" );
				sprintf( result_str, "DeviceID: %s, Func: Read File Record (%s), ByteCount: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
				break;
			case FUNCCODE_WRITE_FILE_RECORD:
				AddResultString( "Write File Record" );
				sprintf( result_str, "DeviceID: %s, Func: Write File Record (%s), ByteCount: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
				break;
			case FUNCCODE_MASK_WRITE_REGISTER:
				AddResultString( "Mask Write Register" );
				sprintf( result_str, "DeviceID: %s, Func: Mask Write Register (%s), RefAddr: %s, And_Mask: %s, OR_Mask: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, Payload3Str, ChecksumStr );
				break;
			case FUNCCODE_READWRITE_MULTIPLE_REGISTERS:
				AddResultString( "Read/Write Multiple Registers" );
				sprintf( result_str, "DeviceID: %s, Func: Read/Write Multiple Registers (%s), ReadStartAddr: %s, ReadQty: %s, WriteStartAddr: %s, WriteQty: %s, ByteCount: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, Payload4Str, Payload3Str, ChecksumStr );
				break;
			case FUNCCODE_READ_FIFO_QUEUE:
				AddResultString( "Read FIFO Queue" );
				sprintf( result_str, "DeviceID: %s, Func: Read FIFO Queue (%s), Addr: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
				break;
			case FUNCCODE_READ_DEVICE_ID:
				AddResultString( "Read Device ID" );
				sprintf( result_str, "DeviceID: %s, Func: Read Device ID (%s), MEI: %s, ReadIDCode: %s, ObjID: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, Payload3Str, ChecksumStr );
				break;
			}
		}
		else if(frame.mFlags&FLAG_RESPONSE_FRAME)
		{
			AddResultString( FunctionCodeStr);
			switch(FunctionCode)
			{
			case FUNCCODE_READ_COILS:
				AddResultString( "Read Coils [ACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Read Coils [ACK] (%s), ByteCount: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr);
				break;
			case FUNCCODE_READ_DISCRETE_INPUTS:
				AddResultString( "Read Discrete Inputs [ACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Read Discrete Inputs [ACK] (%s), ByteCount: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
				break;
			case FUNCCODE_READ_HOLDING_REGISTERS:
				AddResultString( "Read Holding Registers [ACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Holding Registers [ACK] (%s), ByteCount: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
				break;
			case FUNCCODE_READ_INPUT_REGISTER:
				AddResultString( "Read Input Registers [ACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Input Registers [ACK] (%s), ByteCount: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
				break;
			case FUNCCODE_WRITE_SINGLE_COIL:
				AddResultString( "Write Single Coil [ACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Write Single Coil [ACK] (%s), Addr: %s, Value: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
				break;
			case FUNCCODE_WRITE_SINGLE_REGISTER:
				AddResultString( "Write Single Register [ACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Write Single Register [ACK] (%s), Addr: %s, Value: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
				break;
			case FUNCCODE_READ_EXCEPTION_STATUS:
				AddResultString( "Read Exception Status [ACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Read Exception Status [ACK] (%s), Value: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
				break;
			case FUNCCODE_DIAGNOSTIC:

				switch(Payload1)
				{
				case RETURN_QUERY_DATA:
					AddResultString( "Diagnostics [ACK]" );
					AddResultString( "Diagnostics [ACK] - Return Query Data" );
					sprintf( result_str, "DeviceID: %s, Func: Diagnostics [ACK] (%s), SubFunc: Return Query Data (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case RESTART_COMMUNICATIONS_OPTION:
					AddResultString( "Diagnostics [ACK]" );
					AddResultString( "Diagnostics [ACK]- Restart Comms Option" );
					sprintf( result_str, "DeviceID: %s, Func: Diagnostics [ACK] (%s), SubFunc: Restart Comms Option (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case RETURN_DIAGNOSTIC_REGISTER:
					AddResultString( "Diagnostics [ACK]" );
					AddResultString( "Diagnostics [ACK] - Return Diag Reg" );
					sprintf( result_str, "DeviceID: %s, Func: Diagnostics [ACK] (%s), SubFunc: Return Diag Reg (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case CHANGE_ASCII_INPUT_DELIM:
					AddResultString( "Diagnostics [ACK]" );
					AddResultString( "Diagnostics [ACK] - Change ASCII Input Delim" );
					sprintf( result_str, "DeviceID: %s, Func: Diagnostics [ACK] (%s), SubFunc: Change ASCII Input Delim (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case FORCE_LISTEN_ONLY_MODE:
					AddResultString( "Diagnostics [ACK]" );
					AddResultString( "Diagnostics [ACK] - Force Listen Only Mode" );
					sprintf( result_str, "DeviceID: %s, Func: Diagnostics [ACK] (%s), SubFunc: Force Listen Only Mode (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case CLEAR_COUNTERS_AND_DIAG_REGISTER:
					AddResultString( "Diagnostics [ACK]" );
					AddResultString( "Diagnostics [ACK] - Clr Counters And Diag Reg" );
					sprintf( result_str, "DeviceID: %s, Func: Diagnostics [ACK] (%s), SubFunc: Clr Counters And Diag Reg (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case RETURN_BUS_MESSAGE_COUNT:
					AddResultString( "Diagnostics [ACK]" );
					AddResultString( "Diagnostics [ACK] - Bus Msg Cnt" );
					sprintf( result_str, "DeviceID: %s, Func: Diagnostics [ACK] (%s), SubFunc: Bus Msg Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case RETURN_BUS_COMM_ERROR_COUNT:
					AddResultString( "Diagnostics [ACK]" );
					AddResultString( "Diagnostics [ACK] - Bus Comm Err Cnt" );
					sprintf( result_str, "DeviceID: %s, Func: Diagnostics [ACK] (%s), SubFunc: Bus Comm Err Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case RETURN_BUS_EXCEPTION_ERROR_COUNT:
					AddResultString( "Diagnostics [ACK]" );
					AddResultString( "Diagnostics [ACK] - Bus Excpt Err Cnt" );
					sprintf( result_str, "DeviceID: %s, Func: Diagnostics [ACK] (%s), SubFunc: Bus Excpt Err Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case RETURN_SLAVE_MESSAGE_COUNT:
					AddResultString( "Diagnostics [ACK]" );
					AddResultString( "Diagnostics [ACK] - Slave Msg Cnt" );
					sprintf( result_str, "DeviceID: %s, Func: Diagnostics [ACK] (%s), SubFunc: Slave Msg Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case RETURN_SLAVE_NO_RESPONSE_COUNT:
					AddResultString( "Diagnostics [ACK]" );
					AddResultString( "Diagnostics [ACK] - Slave No Resp Cnt" );
					sprintf( result_str, "DeviceID: %s, Func: Diagnostics [ACK] (%s), SubFunc: Slave No Resp Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case RETURN_SLAVE_NAK_COUNT:
					AddResultString( "Diagnostics [ACK]" );
					AddResultString( "Diagnostics [ACK] - Slave NAK Cnt" );
					sprintf( result_str, "DeviceID: %s, Func: Diagnostics [ACK] (%s), SubFunc: Slave NAK Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case RETURN_SLAVE_BUSY_COUNT:
					AddResultString( "Diagnostics [ACK]" );
					AddResultString( "Diagnostics [ACK] - Slave Busy Cnt" );
					sprintf( result_str, "DeviceID: %s, Func: Diagnostics [ACK] (%s), SubFunc: Slave Busy Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case RETURN_BUS_CHAR_OVERRUN_COUNT:
					AddResultString( "Diagnostics [ACK]" );
					AddResultString( "Diagnostics [ACK] - Bus Char Overrun Cnt" );
					sprintf( result_str, "DeviceID: %s, Func: Diagnostics [ACK] (%s), SubFunc: Bus Char Overrun Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case CLEAR_OVERRUN_COUNTER_AND_FLAG:
					AddResultString( "Diagnostics [ACK]" );
					AddResultString( "Diagnostics [ACK] - Clear Overrun Cnt" );
					sprintf( result_str, "DeviceID: %s, Func: Diagnostics [ACK] (%s), SubFunc: Clear Overrun Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				}
				break;
			case FUNCCODE_GET_COM_EVENT_COUNTER:
				AddResultString( "Get Comm Event Counter [ACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Get Comm Event Counter [ACK] (%s), Status: %s, Count: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
				break;
			case FUNCCODE_GET_COM_EVENT_LOG:
				AddResultString( "Get Comm Event Log [ACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Get Comm Event Log [ACK] (%s), Status: %s, EventCnt: %s, MsgCnt: %s, ByteCount: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload4Str, Payload3Str, ChecksumStr );
				break;
			case FUNCCODE_WRITE_MULTIPLE_COILS:
				AddResultString( "Write Multiple Coils [ACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Write Multiple Coils [ACK] (%s), StartAddr: %s, Qty: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
				break;
			case FUNCCODE_WRITE_MULTIPLE_REGISTERS:
				AddResultString( "Write Multiple Registers [ACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Write Multiple Registers [ACK] (%s), StartAddr: %s, Qty: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
				break;
			case FUNCCODE_REPORT_SLAVE_ID:
				AddResultString( "Report Slave ID [ACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Report Slave ID [ACK] (%s), ByteCount: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
				break;
			case FUNCCODE_READ_FILE_RECORD:
				AddResultString( "Read File Record [ACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Read File Record [ACK] (%s), ByteCount: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
				break;
			case FUNCCODE_WRITE_FILE_RECORD:
				AddResultString( "Write File Record [ACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Write File Record [ACK] (%s), ByteCount: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
				break;
			case FUNCCODE_MASK_WRITE_REGISTER:
				AddResultString( "Mask Write Register [ACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Mask Write Register [ACK] (%s), RefAddr: %s, And_Mask: %s, OR_Mask: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, Payload3Str, ChecksumStr );
				break;
			case FUNCCODE_READWRITE_MULTIPLE_REGISTERS:
				AddResultString( "Read/Write Multiple Registers [ACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Read/Write Multiple Registers [ACK] (%s), ByteCount: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
				break;
			case FUNCCODE_READ_FIFO_QUEUE:
				AddResultString( "Read FIFO Queue [ACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Read FIFO Queue [ACK] (%s), ByteCount: %s, FIFO Count: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr, Payload2Str );
				break;
			case FUNCCODE_READ_DEVICE_ID:
				AddResultString( "Read Device ID" );
				sprintf( result_str, "DeviceID: %s, Func: Read Device ID (%s), MEI: %s, ReadIDCode: %s, ObjID: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, Payload3Str, ChecksumStr );
				break;
			}
		
		}
		else if(frame.mFlags&FLAG_EXCEPTION_FRAME)
		{
			AddResultString( "NAK" );
			switch(FunctionCode-0x80)
			{
			case FUNCCODE_READ_COILS:
				AddResultString( "Read Coils [NACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Read Coils [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr);
				break;
			case FUNCCODE_READ_DISCRETE_INPUTS:
				AddResultString( "Read Discrete Inputs [NACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Read Discrete Inputs [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
				break;
			case FUNCCODE_READ_HOLDING_REGISTERS:
				AddResultString( "Read Holding Registers [NACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Holding Registers [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
				break;
			case FUNCCODE_READ_INPUT_REGISTER:
				AddResultString( "Read Input Registers [NACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Input Registers [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
				break;
			case FUNCCODE_WRITE_SINGLE_COIL:
				AddResultString( "Write Single Coil [NACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Write Single Coil [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
				break;
			case FUNCCODE_WRITE_SINGLE_REGISTER:
				AddResultString( "Write Single Register [NACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Write Single Register [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
				break;
			case FUNCCODE_READ_EXCEPTION_STATUS:
				AddResultString( "Read Exception Status [NACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Read Exception Status [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
				break;
			case FUNCCODE_DIAGNOSTIC:
				AddResultString( "Diagnostics [NACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Diagnostics [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
				break;
			case FUNCCODE_GET_COM_EVENT_COUNTER:
				AddResultString( "Get Comm Event Counter [NACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Get Comm Event Counter [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
				break;
			case FUNCCODE_GET_COM_EVENT_LOG:
				AddResultString( "Get Comm Event Log [NACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Get Comm Event Log [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
				break;
			case FUNCCODE_WRITE_MULTIPLE_COILS:
				AddResultString( "Write Multiple Coils [NACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Write Multiple Coils [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
				break;
			case FUNCCODE_WRITE_MULTIPLE_REGISTERS:
				AddResultString( "Write Multiple Registers [NACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Write Multiple Registers [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
				break;
			case FUNCCODE_REPORT_SLAVE_ID:
				AddResultString( "Report Slave ID [NACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Report Slave ID [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
				break;
			case FUNCCODE_READ_FILE_RECORD:
				AddResultString( "Read File Record [NACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Read File Record [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr);
				break;
			case FUNCCODE_WRITE_FILE_RECORD:
				AddResultString( "Write File Record [NACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Write File Record [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr);
				break;
			case FUNCCODE_MASK_WRITE_REGISTER:
				AddResultString( "Mask Write Register [NACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Mask Write Register [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
				break;
			case FUNCCODE_READWRITE_MULTIPLE_REGISTERS:
				AddResultString( "Read/Write Multiple Registers [NACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Read/Write Multiple Registers [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
				break;
			case FUNCCODE_READ_FIFO_QUEUE:
				AddResultString( "Read FIFO Queue [NACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Read FIFO Queue [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr);
				break;
			case FUNCCODE_READ_DEVICE_ID:
				AddResultString( "Read Device ID [NACK]" );
				sprintf( result_str, "DeviceID: %s, Func: Read Device ID [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
				break;
			default:
				AddResultString( "User Defined Function [NACK]" );
				sprintf( result_str, "DeviceID: %s, Func: User Defined Function [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
				break;
			}
		}
		else if(frame.mFlags&FLAG_FILE_SUBREQ)
		{
			AddResultString( "SubRequest Data" );
			sprintf( result_str, "SubRequest - RefType: %s, FileNum: %s, RecordNum: %s, RecordLen: %s", FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
		}
		else if(frame.mFlags&FLAG_DATA_FRAME)
		{
			AddResultString( Payload1Str );
			AddResultString( "Data" );
			sprintf( result_str, "Value: %s", Payload1Str );
		}
		else if(frame.mFlags&FLAG_END_FRAME)
		{
			AddResultString( ChecksumStr );
			AddResultString( "ChkSum" );
			sprintf( result_str, " Checksum: %s", ChecksumStr );
		}

		if(frame.mFlags&FLAG_CHECKSUM_ERROR)
			sprintf( result_str, "%s (Invalid Checksum!)", result_str );

		AddResultString( result_str );
	}
	else
	{
		char number_str[128];
		AnalyzerHelpers::GetNumberString( frame.mData1, display_base, bits_per_transfer, number_str, 128 );

		char result_str[128];

		//MP mode address case:
		bool mp_mode_address_flag = false;
		if( ( frame.mFlags & MP_MODE_ADDRESS_FLAG ) != 0 )
		{
			mp_mode_address_flag = true;

			AddResultString( "A" );
			AddResultString( "Addr" );

			if( framing_error == false )
			{
				sprintf( result_str, "Addr: %s", number_str );
				AddResultString( result_str );

				sprintf( result_str, "Address: %s", number_str );
				AddResultString( result_str );

			}else
			{
				sprintf( result_str, "Addr: %s (framing error)", number_str );
				AddResultString( result_str );

				sprintf( result_str, "Address: %s (framing error)", number_str );
				AddResultString( result_str );
			}
			return;
		}

		//normal case:
		if( ( parity_error == true ) || ( framing_error == true ) )
		{
			AddResultString( "!" );

			sprintf( result_str, "%s (error)", number_str );
			AddResultString( result_str );

			if( parity_error == true && framing_error == false )
				sprintf( result_str, "%s (parity error)", number_str );
			else
			if( parity_error == false && framing_error == true )
				sprintf( result_str, "%s (framing error)", number_str );
			else
				sprintf( result_str, "%s (framing error & parity error)", number_str );

			AddResultString( result_str );
				
		}else
		{
			AddResultString( number_str );
		}
	}
}

void ModbusAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 /*export_type_user_id*/ )
{
	//export_type_user_id is only important if we have more than one export type.
	std::stringstream ss;

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();
	U64 num_frames = GetNumFrames();

	void* f = AnalyzerHelpers::StartFile( file );

	if( mSettings->mModbusMode == ModbusAnalyzerEnums::Normal )
	{
		//Normal case -- not MP mode.
		ss << "Time [s],Value,Parity Error,Framing Error" << std::endl;

		for( U32 i=0; i < num_frames; i++ )
		{
			Frame frame = GetFrame( i );
			
			//static void GetTimeString( U64 sample, U64 trigger_sample, U32 sample_rate_hz, char* result_string, U32 result_string_max_length );
			char time_str[128];
			AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );

			char number_str[128];
			AnalyzerHelpers::GetNumberString( frame.mData1, display_base, mSettings->mBitsPerTransfer, number_str, 128 );

			ss << time_str << "," << number_str;

			if( ( frame.mFlags & PARITY_ERROR_FLAG ) != 0 )
				ss << ",Error,";
			else
				ss << ",,";

			if( ( frame.mFlags & FRAMING_ERROR_FLAG ) != 0 )
				ss << "Error";


			ss << std::endl;

			AnalyzerHelpers::AppendToFile( (U8*)ss.str().c_str(), ss.str().length(), f );
			ss.str( std::string() );

			if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
			{
				AnalyzerHelpers::EndFile( f );
				return;
			}
		}
	}
	else if(mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusRTUMaster || mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusRTUSlave || mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusASCIIMaster || mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusASCIISlave)
	{
		//Modbus Mode
		ss << "Time [s], DeviceID, Function Code, Message" << std::endl;

		for( U32 i=0; i < num_frames; i++ )
		{
			Frame frame = GetFrame( i );

			char DeviceAddrStr[128];
			U8 DeviceAddr = (frame.mData1 & 0xFF00000000000000)>>56;
			AnalyzerHelpers::GetNumberString( DeviceAddr, display_base, mSettings->mBitsPerTransfer, DeviceAddrStr, 128 );

			char FunctionCodeStr[128];
			U8 FunctionCode = (frame.mData1 & 0x00FF000000000000)>>48;
			AnalyzerHelpers::GetNumberString( FunctionCode, display_base, mSettings->mBitsPerTransfer, FunctionCodeStr, 128 );

			char Payload1Str[128];
			U16 Payload1 = (frame.mData1 & 0x0000FFFF00000000)>>32;
			AnalyzerHelpers::GetNumberString( Payload1, display_base, mSettings->mBitsPerTransfer, Payload1Str, 128 );

			char Payload2Str[128];
			U16 Payload2 = (frame.mData1 & 0x00000000FFFF0000)>>16;
			AnalyzerHelpers::GetNumberString( Payload2, display_base, mSettings->mBitsPerTransfer, Payload2Str, 128 );

			char Payload3Str[128];
			U16 Payload3 = (frame.mData2 & 0x000000000000FFFF);
			AnalyzerHelpers::GetNumberString( Payload3, display_base, mSettings->mBitsPerTransfer, Payload3Str, 128 );

			char Payload4Str[128];
			U16 Payload4 = (frame.mData2 & 0x00000000FFFF0000)>>16;
			AnalyzerHelpers::GetNumberString( Payload4, display_base, mSettings->mBitsPerTransfer, Payload4Str, 128 );

			char ChecksumStr[128];
			U16 Checksum = (frame.mData1 & 0x000000000000FFFF);
			AnalyzerHelpers::GetNumberString( Checksum, display_base, mSettings->mBitsPerTransfer, ChecksumStr, 128 );

			char time_str[128];
			AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );

			char result_str[256];
			char Error_str[128];

			if(frame.mFlags&FLAG_REQUEST_FRAME)
			{
				switch(FunctionCode)
				{
				case FUNCCODE_READ_COILS:
					sprintf( result_str, "%s, Read Coils (%s), StartAddr: %s, Qty: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case FUNCCODE_READ_DISCRETE_INPUTS:
					sprintf( result_str, "%s, Read Discrete Inputs (%s), StartAddr: %s, Qty: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case FUNCCODE_READ_HOLDING_REGISTERS:
					sprintf( result_str, "%s, Holding Registers (%s), StartAddr: %s, Qty: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case FUNCCODE_READ_INPUT_REGISTER:
					sprintf( result_str, "%s, Input Registers (%s), StartAddr: %s, Qty: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case FUNCCODE_WRITE_SINGLE_COIL:
					sprintf( result_str, "%s, Write Single Coil (%s), Addr: %s, Value: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case FUNCCODE_WRITE_SINGLE_REGISTER:
					sprintf( result_str, "%s, Write Single Register (%s), Addr: %s, Value: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case FUNCCODE_READ_EXCEPTION_STATUS:
					sprintf( result_str, "%s, Read Exception Status (%s), ChkSum: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
					break;
				case FUNCCODE_DIAGNOSTIC:

					switch(Payload1)
					{
					case RETURN_QUERY_DATA:
						sprintf( result_str, "%s, Diagnostics (%s), SubFunc: Return Query Data (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
						break;
					case RESTART_COMMUNICATIONS_OPTION:
						sprintf( result_str, "%s, Diagnostics (%s), SubFunc: Restart Comms Option (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
						break;
					case RETURN_DIAGNOSTIC_REGISTER:
						sprintf( result_str, "%s, Diagnostics (%s), SubFunc: Return Diag Reg (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
						break;
					case CHANGE_ASCII_INPUT_DELIM:
						sprintf( result_str, "%s, Diagnostics (%s), SubFunc: Change ASCII Input Delim (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
						break;
					case FORCE_LISTEN_ONLY_MODE:
						sprintf( result_str, "%s, Diagnostics (%s), SubFunc: Force Listen Only Mode (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
						break;
					case CLEAR_COUNTERS_AND_DIAG_REGISTER:
						sprintf( result_str, "%s, Diagnostics (%s), SubFunc: Clr Counters And Diag Reg (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
						break;
					case RETURN_BUS_MESSAGE_COUNT:
						sprintf( result_str, "%s, Diagnostics (%s), SubFunc: Bus Msg Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
						break;
					case RETURN_BUS_COMM_ERROR_COUNT:
						sprintf( result_str, "%s, Diagnostics (%s), SubFunc: Bus Comm Err Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
						break;
					case RETURN_BUS_EXCEPTION_ERROR_COUNT:
						sprintf( result_str, "%s, Diagnostics (%s), SubFunc: Bus Excpt Err Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
						break;
					case RETURN_SLAVE_MESSAGE_COUNT:
						sprintf( result_str, "%s, Diagnostics (%s), SubFunc: Slave Msg Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
						break;
					case RETURN_SLAVE_NO_RESPONSE_COUNT:
						sprintf( result_str, "%s, Diagnostics (%s), SubFunc: Slave No Resp Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
						break;
					case RETURN_SLAVE_NAK_COUNT:
						sprintf( result_str, "%s, Diagnostics (%s), SubFunc: Slave NAK Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
						break;
					case RETURN_SLAVE_BUSY_COUNT:
						sprintf( result_str, "%s, Diagnostics (%s), SubFunc: Slave Busy Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
						break;
					case RETURN_BUS_CHAR_OVERRUN_COUNT:
						sprintf( result_str, "%s, Diagnostics (%s), SubFunc: Bus Char Overrun Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
						break;
					case CLEAR_OVERRUN_COUNTER_AND_FLAG:
						sprintf( result_str, "%s, Diagnostics (%s), SubFunc: Clear Overrun Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
						break;
					}
					break;
				case FUNCCODE_GET_COM_EVENT_COUNTER:
					sprintf( result_str, "%s, Get Comm Event Counter (%s), ChkSum: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
					break;
				case FUNCCODE_GET_COM_EVENT_LOG:
					sprintf( result_str, "%s, Get Comm Event Log (%s), ChkSum: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
					break;
				case FUNCCODE_WRITE_MULTIPLE_COILS:
					sprintf( result_str, "%s, Write Multiple Coils (%s), StartAddr: %s, Qty: %s, ByteCount: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case FUNCCODE_WRITE_MULTIPLE_REGISTERS:
					sprintf( result_str, "%s, Write Multiple Registers (%s), StartAddr: %s, Qty: %s, ByteCount: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
					break;
				case FUNCCODE_REPORT_SLAVE_ID:
					sprintf( result_str, "%s, Report Slave ID (%s), ChkSum: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
					break;
				case FUNCCODE_READ_FILE_RECORD:
					sprintf( result_str, "%s, Read File Record (%s), ByteCount: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
					break;
				case FUNCCODE_WRITE_FILE_RECORD:
					sprintf( result_str, "%s, Write File Record (%s), ByteCount: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
					break;
				case FUNCCODE_MASK_WRITE_REGISTER:
					sprintf( result_str, "%s, Mask Write Register (%s), RefAddr: %s, And_Mask: %s, OR_Mask: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, Payload3Str, ChecksumStr );
					break;
				case FUNCCODE_READWRITE_MULTIPLE_REGISTERS:
					sprintf( result_str, "%s, Read/Write Multiple Registers (%s), ReadStartAddr: %s, ReadQty: %s, WriteStartAddr: %s, WriteQty: %s, ByteCount: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, Payload4Str, Payload3Str, ChecksumStr );
					break;
				case FUNCCODE_READ_FIFO_QUEUE:
					sprintf( result_str, "%s, Read FIFO Queue (%s), Addr: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
					break;
				case FUNCCODE_READ_DEVICE_ID:
					sprintf( result_str, "%s, Read Device ID (%s), MEI: %s, ReadIDCode: %s, ObjID: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, Payload3Str, ChecksumStr );
					break;
				}
				}
				else if(frame.mFlags&FLAG_RESPONSE_FRAME)
				{
					switch(FunctionCode)
					{
					case FUNCCODE_READ_COILS:
						sprintf( result_str, "%s, Read Coils [ACK] (%s), ByteCount: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr);
						break;
					case FUNCCODE_READ_DISCRETE_INPUTS:
						sprintf( result_str, " %s, Read Discrete Inputs [ACK] (%s), ByteCount: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
						break;
					case FUNCCODE_READ_HOLDING_REGISTERS:
						sprintf( result_str, "%s, Holding Registers [ACK] (%s), ByteCount: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
						break;
					case FUNCCODE_READ_INPUT_REGISTER:
						sprintf( result_str, " %s, Input Registers [ACK] (%s), ByteCount: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
						break;
					case FUNCCODE_WRITE_SINGLE_COIL:
						sprintf( result_str, "%s, Write Single Coil [ACK] (%s), Addr: %s, Value: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
						break;
					case FUNCCODE_WRITE_SINGLE_REGISTER:
						sprintf( result_str, "%s, Write Single Register [ACK] (%s), Addr: %s, Value: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
						break;
					case FUNCCODE_READ_EXCEPTION_STATUS:
						sprintf( result_str, "%s, Read Exception Status [ACK] (%s), Value: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
						break;
					case FUNCCODE_DIAGNOSTIC:

						switch(Payload1)
						{
						case RETURN_QUERY_DATA:
							sprintf( result_str, "%s, Diagnostics [ACK] (%s), SubFunc: Return Query Data (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
							break;
						case RESTART_COMMUNICATIONS_OPTION:
							sprintf( result_str, "%s, Diagnostics [ACK] (%s), SubFunc: Restart Comms Option (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
							break;
						case RETURN_DIAGNOSTIC_REGISTER:
							sprintf( result_str, "%s, Diagnostics [ACK] (%s), SubFunc: Return Diag Reg (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
							break;
						case CHANGE_ASCII_INPUT_DELIM:
							sprintf( result_str, "%s, Diagnostics [ACK] (%s), SubFunc: Change ASCII Input Delim (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
							break;
						case FORCE_LISTEN_ONLY_MODE:
							sprintf( result_str, " %s, Diagnostics [ACK] (%s), SubFunc: Force Listen Only Mode (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
							break;
						case CLEAR_COUNTERS_AND_DIAG_REGISTER:
							sprintf( result_str, "%s, Diagnostics [ACK] (%s), SubFunc: Clr Counters And Diag Reg (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
							break;
						case RETURN_BUS_MESSAGE_COUNT:
							sprintf( result_str, "%s, Diagnostics [ACK] (%s), SubFunc: Bus Msg Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
							break;
						case RETURN_BUS_COMM_ERROR_COUNT:
							sprintf( result_str, "%s, Diagnostics [ACK] (%s), SubFunc: Bus Comm Err Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
							break;
						case RETURN_BUS_EXCEPTION_ERROR_COUNT:
							sprintf( result_str, "%s, Diagnostics [ACK] (%s), SubFunc: Bus Excpt Err Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
							break;
						case RETURN_SLAVE_MESSAGE_COUNT:
							sprintf( result_str, "%s, Diagnostics [ACK] (%s), SubFunc: Slave Msg Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
							break;
						case RETURN_SLAVE_NO_RESPONSE_COUNT:
							sprintf( result_str, "%s, Diagnostics [ACK] (%s), SubFunc: Slave No Resp Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
							break;
						case RETURN_SLAVE_NAK_COUNT:
							sprintf( result_str, "%s, Diagnostics [ACK] (%s), SubFunc: Slave NAK Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
							break;
						case RETURN_SLAVE_BUSY_COUNT:
							sprintf( result_str, "%s, Diagnostics [ACK] (%s), SubFunc: Slave Busy Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
							break;
						case RETURN_BUS_CHAR_OVERRUN_COUNT:
							sprintf( result_str, "%s, Diagnostics [ACK] (%s), SubFunc: Bus Char Overrun Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
							break;
						case CLEAR_OVERRUN_COUNTER_AND_FLAG:
							sprintf( result_str, "%s, Diagnostics [ACK] (%s), SubFunc: Clear Overrun Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
							break;
						}
						break;
					case FUNCCODE_GET_COM_EVENT_COUNTER:
						sprintf( result_str, " %s, Get Comm Event Counter [ACK] (%s), Status: %s, Count: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
						break;
					case FUNCCODE_GET_COM_EVENT_LOG:
						sprintf( result_str, "%s, Get Comm Event Log [ACK] (%s), Status: %s, EventCnt: %s, MsgCnt: %s, ByteCount: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload4Str, Payload3Str, ChecksumStr );
						break;
					case FUNCCODE_WRITE_MULTIPLE_COILS:
						sprintf( result_str, "%s, Write Multiple Coils [ACK] (%s), StartAddr: %s, Qty: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
						break;
					case FUNCCODE_WRITE_MULTIPLE_REGISTERS:
						sprintf( result_str, "%s, Write Multiple Registers [ACK] (%s), StartAddr: %s, Qty: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
						break;
					case FUNCCODE_REPORT_SLAVE_ID:
						sprintf( result_str, "%s, Report Slave ID [ACK] (%s), ByteCount: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
						break;
					case FUNCCODE_READ_FILE_RECORD:
						sprintf( result_str, "%s, Read File Record [ACK] (%s), ByteCount: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
						break;
					case FUNCCODE_WRITE_FILE_RECORD:
						sprintf( result_str, "%s, Write File Record [ACK] (%s), ByteCount: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
						break;
					case FUNCCODE_MASK_WRITE_REGISTER:
						sprintf( result_str, "%s, Mask Write Register [ACK] (%s), RefAddr: %s, And_Mask: %s, OR_Mask: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, Payload3Str, ChecksumStr );
						break;
					case FUNCCODE_READWRITE_MULTIPLE_REGISTERS:
						sprintf( result_str, "%s, Read/Write Multiple Registers [ACK] (%s), ByteCount: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
						break;
					case FUNCCODE_READ_FIFO_QUEUE:
						sprintf( result_str, "%s, Read FIFO Queue [ACK] (%s), ByteCount: %s, FIFO Count: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr, Payload2Str );
						break;
					case FUNCCODE_READ_DEVICE_ID:
						sprintf( result_str, "%s, Read Device ID (%s), MEI: %s, ReadIDCode: %s, ObjID: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, Payload3Str, ChecksumStr );
						break;
					}
				}
				else if(frame.mFlags&FLAG_EXCEPTION_FRAME)
				{
					switch(FunctionCode-0x80)
					{
					case FUNCCODE_READ_COILS:
						sprintf( result_str, "%s, Read Coils [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr);
						break;
					case FUNCCODE_READ_DISCRETE_INPUTS:
						sprintf( result_str, "%s, Read Discrete Inputs [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
						break;
					case FUNCCODE_READ_HOLDING_REGISTERS:
						sprintf( result_str, "%s, Holding Registers [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
						break;
					case FUNCCODE_READ_INPUT_REGISTER:
						sprintf( result_str, "%s, Input Registers [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
						break;
					case FUNCCODE_WRITE_SINGLE_COIL:
						sprintf( result_str, "%s, Write Single Coil [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
						break;
					case FUNCCODE_WRITE_SINGLE_REGISTER:
						sprintf( result_str, "%s, Write Single Register [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
						break;
					case FUNCCODE_READ_EXCEPTION_STATUS:
						sprintf( result_str, "%s, Read Exception Status [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
						break;
					case FUNCCODE_DIAGNOSTIC:
						sprintf( result_str, "%s, Diagnostics [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
						break;
					case FUNCCODE_GET_COM_EVENT_COUNTER:
						sprintf( result_str, "%s, Get Comm Event Counter [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
						break;
					case FUNCCODE_GET_COM_EVENT_LOG:
						sprintf( result_str, "%s, Get Comm Event Log [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
						break;
					case FUNCCODE_WRITE_MULTIPLE_COILS:
						sprintf( result_str, "%s, Write Multiple Coils [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
						break;
					case FUNCCODE_WRITE_MULTIPLE_REGISTERS:
						sprintf( result_str, "%s, Write Multiple Registers [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
						break;
					case FUNCCODE_REPORT_SLAVE_ID:
						sprintf( result_str, "%s, Report Slave ID [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
						break;
					case FUNCCODE_READ_FILE_RECORD:
						sprintf( result_str, "%s, Read File Record [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr);
						break;
					case FUNCCODE_WRITE_FILE_RECORD:
						sprintf( result_str, "%s, Write File Record [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr);
						break;
					case FUNCCODE_MASK_WRITE_REGISTER:
						sprintf( result_str, "%s, Mask Write Register [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
						break;
					case FUNCCODE_READWRITE_MULTIPLE_REGISTERS:
						sprintf( result_str, "%s, Read/Write Multiple Registers [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
						break;
					case FUNCCODE_READ_FIFO_QUEUE:
						sprintf( result_str, "%s, Read FIFO Queue [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr);
						break;
					case FUNCCODE_READ_DEVICE_ID:
						sprintf( result_str, "%s, Read Device ID [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
						break;
					default:
						sprintf( result_str, "%s, User Defined Function [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
						break;
					}
				}
				else if(frame.mFlags&FLAG_FILE_SUBREQ)
				{
					sprintf( result_str, ",,SubRequest - RefType: %s, FileNum: %s, RecordNum: %s, RecordLen: %s", FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
				}
				else if(frame.mFlags&FLAG_DATA_FRAME)
				{
					sprintf( result_str, ",, Data, Value: %s", Payload1Str );
				}
				else if(frame.mFlags&FLAG_END_FRAME)
				{
					sprintf( result_str, ",, End of Frame, Checksum: %s", ChecksumStr );
				}

				if(frame.mFlags&FLAG_CHECKSUM_ERROR)
					sprintf( result_str, "%s (Invalid Checksum!)", result_str );

				ss << time_str << "," << result_str << std::endl;
	

			AnalyzerHelpers::AppendToFile( (U8*)ss.str().c_str(), ss.str().length(), f );
			ss.str( std::string() );


			if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
			{
				AnalyzerHelpers::EndFile( f );
				return;
			}
		}
	}
	else
	{
		//MP mode.
		ss << "Time [s],Packet ID,Address,Data,Framing Error" << std::endl;
		U64 address = 0;

		for( U32 i=0; i < num_frames; i++ )
		{
			Frame frame = GetFrame( i );

			if( ( frame.mFlags & MP_MODE_ADDRESS_FLAG ) != 0 )
			{
				address = frame.mData1;
				continue;
			}

			U64 packet_id = GetPacketContainingFrameSequential( i ); 

			//static void GetTimeString( U64 sample, U64 trigger_sample, U32 sample_rate_hz, char* result_string, U32 result_string_max_length );
			char time_str[128];
			AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );

			char address_str[128];
			AnalyzerHelpers::GetNumberString( address, display_base, mSettings->mBitsPerTransfer - 1, address_str, 128 );

			char number_str[128];
			AnalyzerHelpers::GetNumberString( frame.mData1, display_base, mSettings->mBitsPerTransfer - 1, number_str, 128 );
			if( packet_id == INVALID_RESULT_INDEX )
				ss << time_str << "," << "" << "," << address_str << "," << number_str << ",";
			else
				ss << time_str << "," << packet_id << "," << address_str << "," << number_str << ",";

			if( ( frame.mFlags & FRAMING_ERROR_FLAG ) != 0 )
				ss << "Error";

			ss << std::endl;

			AnalyzerHelpers::AppendToFile( (U8*)ss.str().c_str(), ss.str().length(), f );
			ss.str( std::string() );


			if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
			{
				AnalyzerHelpers::EndFile( f );
				return;
			}


		}
	}

	UpdateExportProgressAndCheckForCancel( num_frames, num_frames );
	AnalyzerHelpers::EndFile( f );
}

void ModbusAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
	Frame frame = GetFrame( frame_index );
    ClearTabularText();

	bool framing_error = false;
	if( ( frame.mFlags & FRAMING_ERROR_FLAG ) != 0 )
		framing_error = true;

	bool parity_error = false;
	if( ( frame.mFlags & PARITY_ERROR_FLAG ) != 0 )
		parity_error = true;


    U32 bits_per_transfer = mSettings->mBitsPerTransfer;
    if( mSettings->mModbusMode != ModbusAnalyzerEnums::Normal )
        bits_per_transfer--;

    if(mSettings->mModbusMode==ModbusAnalyzerEnums::ModbusASCIIMaster || mSettings->mModbusMode==ModbusAnalyzerEnums::ModbusASCIISlave || mSettings->mModbusMode==ModbusAnalyzerEnums::ModbusRTUMaster || mSettings->mModbusMode==ModbusAnalyzerEnums::ModbusRTUSlave)
    {
        char DeviceAddrStr[128];
        U8 DeviceAddr = (frame.mData1 & 0xFF00000000000000)>>56;
        AnalyzerHelpers::GetNumberString( DeviceAddr, display_base, bits_per_transfer, DeviceAddrStr, 128 );

        char FunctionCodeStr[128];
        U8 FunctionCode = (frame.mData1 & 0x00FF000000000000)>>48;
        AnalyzerHelpers::GetNumberString( FunctionCode, display_base, bits_per_transfer, FunctionCodeStr, 128 );

        char Payload1Str[128];
        U16 Payload1 = (frame.mData1 & 0x0000FFFF00000000)>>32;
        AnalyzerHelpers::GetNumberString( Payload1, display_base, bits_per_transfer, Payload1Str, 128 );

        char Payload2Str[128];
        U16 Payload2 = (frame.mData1 & 0x00000000FFFF0000)>>16;
        AnalyzerHelpers::GetNumberString( Payload2, display_base, bits_per_transfer, Payload2Str, 128 );

        char Payload3Str[128];
        U16 Payload3 = (frame.mData2 & 0x000000000000FFFF);
        AnalyzerHelpers::GetNumberString( Payload3, display_base, bits_per_transfer, Payload3Str, 128 );

        char Payload4Str[128];
        U16 Payload4 = (frame.mData2 & 0x00000000FFFF0000)>>16;
        AnalyzerHelpers::GetNumberString( Payload4, display_base, bits_per_transfer, Payload4Str, 128 );

        char ChecksumStr[128];
        U16 Checksum = (frame.mData1 & 0x000000000000FFFF);
        AnalyzerHelpers::GetNumberString( Checksum, display_base, bits_per_transfer, ChecksumStr, 128 );

        char result_str[256];
        char Error_str[128];

        if(frame.mFlags&FLAG_REQUEST_FRAME)
        {
            switch(FunctionCode)
            {
            case FUNCCODE_READ_COILS:
                sprintf( result_str, "DeviceID: %s, Func: Read Coils (%s), StartAddr: %s, Qty: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                break;
            case FUNCCODE_READ_DISCRETE_INPUTS:
                sprintf( result_str, "DeviceID: %s, Func: Read Discrete Inputs (%s), StartAddr: %s, Qty: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                break;
            case FUNCCODE_READ_HOLDING_REGISTERS:
                sprintf( result_str, "DeviceID: %s, Func: Holding Registers (%s), StartAddr: %s, Qty: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                break;
            case FUNCCODE_READ_INPUT_REGISTER:
                sprintf( result_str, "DeviceID: %s, Func: Input Registers (%s), StartAddr: %s, Qty: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                break;
            case FUNCCODE_WRITE_SINGLE_COIL:
                sprintf( result_str, "DeviceID: %s, Func: Write Single Coil (%s), Addr: %s, Value: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                break;
            case FUNCCODE_WRITE_SINGLE_REGISTER:
                sprintf( result_str, "DeviceID: %s, Func: Write Single Register (%s), Addr: %s, Value: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                break;
            case FUNCCODE_READ_EXCEPTION_STATUS:
                sprintf( result_str, "DeviceID: %s, Func: Read Exception Status (%s), ChkSum: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
                break;
            case FUNCCODE_DIAGNOSTIC:

                switch(Payload1)
                {
                case RETURN_QUERY_DATA:
                    sprintf( result_str, "DeviceID: %s, Func: Diagnostics (%s), SubFunc: Return Query Data (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                    break;
                case RESTART_COMMUNICATIONS_OPTION:
                    sprintf( result_str, "DeviceID: %s, Func: Diagnostics (%s), SubFunc: Restart Comms Option (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                    break;
                case RETURN_DIAGNOSTIC_REGISTER:
                    sprintf( result_str, "DeviceID: %s, Func: Diagnostics (%s), SubFunc: Return Diag Reg (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                    break;
                case CHANGE_ASCII_INPUT_DELIM:
                    sprintf( result_str, "DeviceID: %s, Func: Diagnostics (%s), SubFunc: Change ASCII Input Delim (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                    break;
                case FORCE_LISTEN_ONLY_MODE:
                    sprintf( result_str, "DeviceID: %s, Func: Diagnostics (%s), SubFunc: Force Listen Only Mode (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                    break;
                case CLEAR_COUNTERS_AND_DIAG_REGISTER:
                    sprintf( result_str, "DeviceID: %s, Func: Diagnostics (%s), SubFunc: Clr Counters And Diag Reg (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                    break;
                case RETURN_BUS_MESSAGE_COUNT:
                    sprintf( result_str, "DeviceID: %s, Func: Diagnostics (%s), SubFunc: Bus Msg Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                    break;
                case RETURN_BUS_COMM_ERROR_COUNT:
                    sprintf( result_str, "DeviceID: %s, Func: Diagnostics (%s), SubFunc: Bus Comm Err Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                    break;
                case RETURN_BUS_EXCEPTION_ERROR_COUNT:
                    sprintf( result_str, "DeviceID: %s, Func: Diagnostics (%s), SubFunc: Bus Excpt Err Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                    break;
                case RETURN_SLAVE_MESSAGE_COUNT:
                    sprintf( result_str, "DeviceID: %s, Func: Diagnostics (%s), SubFunc: Slave Msg Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                    break;
                case RETURN_SLAVE_NO_RESPONSE_COUNT:
                    sprintf( result_str, "DeviceID: %s, Func: Diagnostics (%s), SubFunc: Slave No Resp Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                    break;
                case RETURN_SLAVE_NAK_COUNT:
                    sprintf( result_str, "DeviceID: %s, Func: Diagnostics (%s), SubFunc: Slave NAK Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                    break;
                case RETURN_SLAVE_BUSY_COUNT:
                    sprintf( result_str, "DeviceID: %s, Func: Diagnostics (%s), SubFunc: Slave Busy Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                    break;
                case RETURN_BUS_CHAR_OVERRUN_COUNT:
                    sprintf( result_str, "DeviceID: %s, Func: Diagnostics (%s), SubFunc: Bus Char Overrun Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                    break;
                case CLEAR_OVERRUN_COUNTER_AND_FLAG:
                    sprintf( result_str, "DeviceID: %s, Func: Diagnostics (%s), SubFunc: Clear Overrun Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                    break;
                }
                break;
            case FUNCCODE_GET_COM_EVENT_COUNTER:
                sprintf( result_str, "DeviceID: %s, Func: Get Comm Event Counter (%s), ChkSum: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
                break;
            case FUNCCODE_GET_COM_EVENT_LOG:
                sprintf( result_str, "DeviceID: %s, Func: Get Comm Event Log (%s), ChkSum: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
                break;
            case FUNCCODE_WRITE_MULTIPLE_COILS:
                sprintf( result_str, "DeviceID: %s, Func: Write Multiple Coils (%s), StartAddr: %s, Qty: %s, ByteCount: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                break;
            case FUNCCODE_WRITE_MULTIPLE_REGISTERS:
                sprintf( result_str, "DeviceID: %s, Func: Write Multiple Registers (%s), StartAddr: %s, Qty: %s, ByteCount: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                break;
            case FUNCCODE_REPORT_SLAVE_ID:
                sprintf( result_str, "DeviceID: %s, Func: Report Slave ID (%s), ChkSum: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
                break;
            case FUNCCODE_READ_FILE_RECORD:
                sprintf( result_str, "DeviceID: %s, Func: Read File Record (%s), ByteCount: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
                break;
            case FUNCCODE_WRITE_FILE_RECORD:
                AddTabularText( "Write File Record" );
                sprintf( result_str, "DeviceID: %s, Func: Write File Record (%s), ByteCount: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
                break;
            case FUNCCODE_MASK_WRITE_REGISTER:
                sprintf( result_str, "DeviceID: %s, Func: Mask Write Register (%s), RefAddr: %s, And_Mask: %s, OR_Mask: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, Payload3Str, ChecksumStr );
                break;
            case FUNCCODE_READWRITE_MULTIPLE_REGISTERS:
                sprintf( result_str, "DeviceID: %s, Func: Read/Write Multiple Registers (%s), ReadStartAddr: %s, ReadQty: %s, WriteStartAddr: %s, WriteQty: %s, ByteCount: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, Payload4Str, Payload3Str, ChecksumStr );
                break;
            case FUNCCODE_READ_FIFO_QUEUE:
                sprintf( result_str, "DeviceID: %s, Func: Read FIFO Queue (%s), Addr: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
                break;
            case FUNCCODE_READ_DEVICE_ID:
                sprintf( result_str, "DeviceID: %s, Func: Read Device ID (%s), MEI: %s, ReadIDCode: %s, ObjID: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, Payload3Str, ChecksumStr );
                break;
            }
        }
        else if(frame.mFlags&FLAG_RESPONSE_FRAME)
        {
            switch(FunctionCode)
            {
            case FUNCCODE_READ_COILS:
                sprintf( result_str, "DeviceID: %s, Func: Read Coils [ACK] (%s), ByteCount: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr);
                break;
            case FUNCCODE_READ_DISCRETE_INPUTS:
                sprintf( result_str, "DeviceID: %s, Func: Read Discrete Inputs [ACK] (%s), ByteCount: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
                break;
            case FUNCCODE_READ_HOLDING_REGISTERS:
                sprintf( result_str, "DeviceID: %s, Func: Holding Registers [ACK] (%s), ByteCount: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
                break;
            case FUNCCODE_READ_INPUT_REGISTER:
                sprintf( result_str, "DeviceID: %s, Func: Input Registers [ACK] (%s), ByteCount: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
                break;
            case FUNCCODE_WRITE_SINGLE_COIL:
                sprintf( result_str, "DeviceID: %s, Func: Write Single Coil [ACK] (%s), Addr: %s, Value: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                break;
            case FUNCCODE_WRITE_SINGLE_REGISTER:
                sprintf( result_str, "DeviceID: %s, Func: Write Single Register [ACK] (%s), Addr: %s, Value: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                break;
            case FUNCCODE_READ_EXCEPTION_STATUS:
                sprintf( result_str, "DeviceID: %s, Func: Read Exception Status [ACK] (%s), Value: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
                break;
            case FUNCCODE_DIAGNOSTIC:

                switch(Payload1)
                {
                case RETURN_QUERY_DATA:
                    sprintf( result_str, "DeviceID: %s, Func: Diagnostics [ACK] (%s), SubFunc: Return Query Data (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                    break;
                case RESTART_COMMUNICATIONS_OPTION:
                    sprintf( result_str, "DeviceID: %s, Func: Diagnostics [ACK] (%s), SubFunc: Restart Comms Option (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                    break;
                case RETURN_DIAGNOSTIC_REGISTER:
                    sprintf( result_str, "DeviceID: %s, Func: Diagnostics [ACK] (%s), SubFunc: Return Diag Reg (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                    break;
                case CHANGE_ASCII_INPUT_DELIM:
                    sprintf( result_str, "DeviceID: %s, Func: Diagnostics [ACK] (%s), SubFunc: Change ASCII Input Delim (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                    break;
                case FORCE_LISTEN_ONLY_MODE:
                    sprintf( result_str, "DeviceID: %s, Func: Diagnostics [ACK] (%s), SubFunc: Force Listen Only Mode (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                    break;
                case CLEAR_COUNTERS_AND_DIAG_REGISTER:
                    sprintf( result_str, "DeviceID: %s, Func: Diagnostics [ACK] (%s), SubFunc: Clr Counters And Diag Reg (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                    break;
                case RETURN_BUS_MESSAGE_COUNT:
                    sprintf( result_str, "DeviceID: %s, Func: Diagnostics [ACK] (%s), SubFunc: Bus Msg Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                    break;
                case RETURN_BUS_COMM_ERROR_COUNT:
                    sprintf( result_str, "DeviceID: %s, Func: Diagnostics [ACK] (%s), SubFunc: Bus Comm Err Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                    break;
                case RETURN_BUS_EXCEPTION_ERROR_COUNT:
                    sprintf( result_str, "DeviceID: %s, Func: Diagnostics [ACK] (%s), SubFunc: Bus Excpt Err Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                    break;
                case RETURN_SLAVE_MESSAGE_COUNT:
                    sprintf( result_str, "DeviceID: %s, Func: Diagnostics [ACK] (%s), SubFunc: Slave Msg Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                    break;
                case RETURN_SLAVE_NO_RESPONSE_COUNT:
                    sprintf( result_str, "DeviceID: %s, Func: Diagnostics [ACK] (%s), SubFunc: Slave No Resp Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                    break;
                case RETURN_SLAVE_NAK_COUNT:
                    sprintf( result_str, "DeviceID: %s, Func: Diagnostics [ACK] (%s), SubFunc: Slave NAK Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                    break;
                case RETURN_SLAVE_BUSY_COUNT:
                    sprintf( result_str, "DeviceID: %s, Func: Diagnostics [ACK] (%s), SubFunc: Slave Busy Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                    break;
                case RETURN_BUS_CHAR_OVERRUN_COUNT:
                    sprintf( result_str, "DeviceID: %s, Func: Diagnostics [ACK] (%s), SubFunc: Bus Char Overrun Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                    break;
                case CLEAR_OVERRUN_COUNTER_AND_FLAG:
                    sprintf( result_str, "DeviceID: %s, Func: Diagnostics [ACK] (%s), SubFunc: Clear Overrun Cnt (%s), Data: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                    break;
                }
                break;
            case FUNCCODE_GET_COM_EVENT_COUNTER:
                sprintf( result_str, "DeviceID: %s, Func: Get Comm Event Counter [ACK] (%s), Status: %s, Count: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                break;
            case FUNCCODE_GET_COM_EVENT_LOG:
                sprintf( result_str, "DeviceID: %s, Func: Get Comm Event Log [ACK] (%s), Status: %s, EventCnt: %s, MsgCnt: %s, ByteCount: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload4Str, Payload3Str, ChecksumStr );
                break;
            case FUNCCODE_WRITE_MULTIPLE_COILS:
                sprintf( result_str, "DeviceID: %s, Func: Write Multiple Coils [ACK] (%s), StartAddr: %s, Qty: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                break;
            case FUNCCODE_WRITE_MULTIPLE_REGISTERS:
                sprintf( result_str, "DeviceID: %s, Func: Write Multiple Registers [ACK] (%s), StartAddr: %s, Qty: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
                break;
            case FUNCCODE_REPORT_SLAVE_ID:
                sprintf( result_str, "DeviceID: %s, Func: Report Slave ID [ACK] (%s), ByteCount: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
                break;
            case FUNCCODE_READ_FILE_RECORD:
                sprintf( result_str, "DeviceID: %s, Func: Read File Record [ACK] (%s), ByteCount: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
                break;
            case FUNCCODE_WRITE_FILE_RECORD:
                sprintf( result_str, "DeviceID: %s, Func: Write File Record [ACK] (%s), ByteCount: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
                break;
            case FUNCCODE_MASK_WRITE_REGISTER:
                sprintf( result_str, "DeviceID: %s, Func: Mask Write Register [ACK] (%s), RefAddr: %s, And_Mask: %s, OR_Mask: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, Payload3Str, ChecksumStr );
                break;
            case FUNCCODE_READWRITE_MULTIPLE_REGISTERS:
                sprintf( result_str, "DeviceID: %s, Func: Read/Write Multiple Registers [ACK] (%s), ByteCount: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr );
                break;
            case FUNCCODE_READ_FIFO_QUEUE:
                sprintf( result_str, "DeviceID: %s, Func: Read FIFO Queue [ACK] (%s), ByteCount: %s, FIFO Count: %s", DeviceAddrStr, FunctionCodeStr, ChecksumStr, Payload2Str );
                break;
            case FUNCCODE_READ_DEVICE_ID:
                sprintf( result_str, "DeviceID: %s, Func: Read Device ID (%s), MEI: %s, ReadIDCode: %s, ObjID: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, Payload2Str, Payload3Str, ChecksumStr );
                break;
            }

        }
        else if(frame.mFlags&FLAG_EXCEPTION_FRAME)
        {
            switch(FunctionCode-0x80)
            {
            case FUNCCODE_READ_COILS:
                sprintf( result_str, "DeviceID: %s, Func: Read Coils [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr);
                break;
            case FUNCCODE_READ_DISCRETE_INPUTS:
                sprintf( result_str, "DeviceID: %s, Func: Read Discrete Inputs [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
                break;
            case FUNCCODE_READ_HOLDING_REGISTERS:
                sprintf( result_str, "DeviceID: %s, Func: Holding Registers [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
                break;
            case FUNCCODE_READ_INPUT_REGISTER:
                sprintf( result_str, "DeviceID: %s, Func: Input Registers [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
                break;
            case FUNCCODE_WRITE_SINGLE_COIL:
                sprintf( result_str, "DeviceID: %s, Func: Write Single Coil [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
                break;
            case FUNCCODE_WRITE_SINGLE_REGISTER:
                sprintf( result_str, "DeviceID: %s, Func: Write Single Register [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
                break;
            case FUNCCODE_READ_EXCEPTION_STATUS:
                sprintf( result_str, "DeviceID: %s, Func: Read Exception Status [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
                break;
            case FUNCCODE_DIAGNOSTIC:
                sprintf( result_str, "DeviceID: %s, Func: Diagnostics [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
                break;
            case FUNCCODE_GET_COM_EVENT_COUNTER:
                sprintf( result_str, "DeviceID: %s, Func: Get Comm Event Counter [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
                break;
            case FUNCCODE_GET_COM_EVENT_LOG:
                sprintf( result_str, "DeviceID: %s, Func: Get Comm Event Log [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
                break;
            case FUNCCODE_WRITE_MULTIPLE_COILS:
                sprintf( result_str, "DeviceID: %s, Func: Write Multiple Coils [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
                break;
            case FUNCCODE_WRITE_MULTIPLE_REGISTERS:
                sprintf( result_str, "DeviceID: %s, Func: Write Multiple Registers [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
                break;
            case FUNCCODE_REPORT_SLAVE_ID:
                sprintf( result_str, "DeviceID: %s, Func: Report Slave ID [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
                break;
            case FUNCCODE_READ_FILE_RECORD:
                sprintf( result_str, "DeviceID: %s, Func: Read File Record [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr);
                break;
            case FUNCCODE_WRITE_FILE_RECORD:
                sprintf( result_str, "DeviceID: %s, Func: Write File Record [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr);
                break;
            case FUNCCODE_MASK_WRITE_REGISTER:
                sprintf( result_str, "DeviceID: %s, Func: Mask Write Register [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
                break;
            case FUNCCODE_READWRITE_MULTIPLE_REGISTERS:
                sprintf( result_str, "DeviceID: %s, Func: Read/Write Multiple Registers [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
                break;
            case FUNCCODE_READ_FIFO_QUEUE:
                sprintf( result_str, "DeviceID: %s, Func: Read FIFO Queue [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr);
                break;
            case FUNCCODE_READ_DEVICE_ID:
                sprintf( result_str, "DeviceID: %s, Func: Read Device ID [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
                break;
            default:
                sprintf( result_str, "DeviceID: %s, Func: User Defined Function [NACK] (%s), ExceptionCode: %s, ChkSum: %s", DeviceAddrStr, FunctionCodeStr, Payload1Str, ChecksumStr );
                break;
            }
        }
        else if(frame.mFlags&FLAG_FILE_SUBREQ)
        {
            sprintf( result_str, "SubRequest - RefType: %s, FileNum: %s, RecordNum: %s, RecordLen: %s", FunctionCodeStr, Payload1Str, Payload2Str, ChecksumStr );
        }
        else if(frame.mFlags&FLAG_DATA_FRAME)
        {
            sprintf( result_str, "Value: %s", Payload1Str );
        }
        else if(frame.mFlags&FLAG_END_FRAME)
        {
            sprintf( result_str, " Checksum: %s", ChecksumStr );
        }

        if(frame.mFlags&FLAG_CHECKSUM_ERROR)
            sprintf( result_str, "%s (Invalid Checksum!)", result_str );

        AddTabularText( result_str );
    }
    else
    {
        char number_str[128];
        AnalyzerHelpers::GetNumberString( frame.mData1, display_base, bits_per_transfer, number_str, 128 );

        char result_str[128];

        //MP mode address case:
        bool mp_mode_address_flag = false;
        if( ( frame.mFlags & MP_MODE_ADDRESS_FLAG ) != 0 )
        {
            mp_mode_address_flag = true;
            if( framing_error == false )
            {
               sprintf( result_str, "Address: %s", number_str );

            }else
            {
                sprintf( result_str, "Address: %s (framing error)", number_str );
            }
            return;
        }

        //normal case:
        if( ( parity_error == true ) || ( framing_error == true ) )
        {
            sprintf( result_str, "%s (error)", number_str );
            AddTabularText( result_str );

            if( parity_error == true && framing_error == false )
                sprintf( result_str, "%s (parity error)", number_str );
            else
            if( parity_error == false && framing_error == true )
                sprintf( result_str, "%s (framing error)", number_str );
            else
                sprintf( result_str, "%s (framing error & parity error)", number_str );

            AddTabularText( result_str );

        }else
        {
            AddTabularText( number_str );
        }
    }
}

void ModbusAnalyzerResults::GeneratePacketTabularText( U64 /*packet_id*/, DisplayBase /*display_base*/ )  //unrefereced vars commented out to remove warnings.
{
	ClearResultStrings();
	AddResultString( "not supported" );
}

void ModbusAnalyzerResults::GenerateTransactionTabularText( U64 /*transaction_id*/, DisplayBase /*display_base*/ )  //unrefereced vars commented out to remove warnings.
{
	ClearResultStrings();
	AddResultString( "not supported" );
}

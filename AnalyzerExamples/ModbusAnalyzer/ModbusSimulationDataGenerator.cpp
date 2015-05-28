#include "ModbusSimulationDataGenerator.h"
#include "ModbusAnalyzerSettings.h"
#include "ModbusAnalyzerModbusExtension.h"

ModbusSimulationDataGenerator::ModbusSimulationDataGenerator()
{
}

ModbusSimulationDataGenerator::~ModbusSimulationDataGenerator()
{
}

void ModbusSimulationDataGenerator::Initialize( U32 simulation_sample_rate, ModbusAnalyzerSettings* settings )
{
	mSimulationSampleRateHz = simulation_sample_rate;
	mSettings = settings;

	mClockGenerator.Init( mSettings->mBitRate, simulation_sample_rate );
	mModbusSimulationData.SetChannel( mSettings->mInputChannel );
	mModbusSimulationData.SetSampleRate( simulation_sample_rate );

	if( mSettings->mInverted == false )
	{
		mBitLow = BIT_LOW;
		mBitHigh = BIT_HIGH;	
	}
	else
	{
		mBitLow = BIT_HIGH;
		mBitHigh = BIT_LOW;
	}

	mModbusSimulationData.SetInitialBitState( mBitHigh );
	mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

	mValue = 0;
	mNumBitsMask = 0;

	U32 num_bits = mSettings->mBitsPerTransfer;
	for( U32 i = 0; i < num_bits; i++ )
	{
		mNumBitsMask <<= 1;
		mNumBitsMask |= 0x1;
	}

	//used for calculating Modbus Checksum values
	init_crc16_tab();
}

U32 ModbusSimulationDataGenerator::GenerateSimulationData( U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample( largest_sample_requested, sample_rate, mSimulationSampleRateHz );

	while( mModbusSimulationData.GetCurrentSampleNumber() < adjusted_largest_sample_requested )
	{
		if (mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusRTUMaster || mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusASCIIMaster)
		{
			//Simulate the Master Device on a Modbus channel
		
			SendGenericRequest(0x01, 0x01, 0x0013, 0x0013 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendGenericRequest(0x01, 0x02, 0x00C4, 0x0016 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendGenericRequest(0x01, 0x03, 0x006B, 0x0003 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendGenericRequest(0x01, 0x04, 0x0008, 0x0001 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendGenericRequest(0x01, 0x05, 0x00AC, 0xFF00 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendGenericRequest(0x01, 0x06, 0x0001, 0x0003 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendGenDiagnosticRequest(0x01, 0x07);
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );
			
			//all available subcodes used for Diagnostics command
			SendRequest_Diagnostics(0x01, RETURN_QUERY_DATA, 0xA537 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendRequest_Diagnostics(0x01, RESTART_COMMUNICATIONS_OPTION, 0x0000 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendRequest_Diagnostics(0x01, RESTART_COMMUNICATIONS_OPTION, 0xFF00 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendRequest_Diagnostics(0x01, RETURN_DIAGNOSTIC_REGISTER, 0x0000 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendRequest_Diagnostics(0x01, CHANGE_ASCII_INPUT_DELIM, 0x4300 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendRequest_Diagnostics(0x01, FORCE_LISTEN_ONLY_MODE, 0x0000 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendRequest_Diagnostics(0x01, CLEAR_COUNTERS_AND_DIAG_REGISTER, 0x0000 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendRequest_Diagnostics(0x01, RETURN_BUS_MESSAGE_COUNT, 0x0000 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendRequest_Diagnostics(0x01, RETURN_BUS_COMM_ERROR_COUNT, 0x0000 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendRequest_Diagnostics(0x01, RETURN_BUS_EXCEPTION_ERROR_COUNT, 0x0000 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendRequest_Diagnostics(0x01, RETURN_SLAVE_MESSAGE_COUNT, 0x0000 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendRequest_Diagnostics(0x01, RETURN_SLAVE_NO_RESPONSE_COUNT, 0x0000 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendRequest_Diagnostics(0x01, RETURN_SLAVE_NAK_COUNT, 0x0000 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendRequest_Diagnostics(0x01, RETURN_SLAVE_BUSY_COUNT, 0x0000 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendRequest_Diagnostics(0x01, RETURN_BUS_CHAR_OVERRUN_COUNT, 0x0000 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendRequest_Diagnostics(0x01, CLEAR_OVERRUN_COUNTER_AND_FLAG, 0x0000 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendGenDiagnosticRequest(0x01, 0x0B);
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendGenDiagnosticRequest(0x01, 0x0C);
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			U8 temp[] = {0xCD, 0x01};
			SendWriteMultipleCoilsRequest(0x01, 0x0013, 0x000A, 0x02, temp );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );
			
			U16 temp2[] = {0x000A, 0x0102};
			SendWriteMultipleRegistersRequest(0x01, 0x0001, 0x0002, 0x04, temp2 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendGenDiagnosticRequest(0x01, 0x11);
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );
			
			U8 RefTypes[] = {0x06, 0x06};
			U16 FileNumbers[] = {0x0004, 0x0003};
			U16 RecordNumbers[] = {0x0001, 0x0009};
			U16 RecordLengths[] = {0x0002, 0x0002};
			SendReadFileRecordRequest(0x01,0x0E, RefTypes, FileNumbers, RecordNumbers, RecordLengths );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			U8 RefTypes2[] = {0x06};
			U16 FileNumbers2[] = {0x0004};
			U16 RecordNumbers2[] = {0x0007};
			U16 RecordLengths2[] = {0x0003};
			U16 RecordData2[1][3] = {{0x06AF, 0x04BE ,0x100D} };
			SendWriteFileRecordRequest(0x01, 0x0D, RefTypes2, FileNumbers2, RecordNumbers2, RecordLengths2, RecordData2 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendMaskWriteRegisterRequest(0x01, 0x0004, 0x00F2, 0x0025);
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			U16 WriteMe[] = {0x00FF, 0x00FF, 0x00FF};
			SendReadWriteMultipleRegisters(0x01, 0x0003, 0x0006, 0x000E, 0x0003, 0x06, WriteMe );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendReadFIFOQueueRequest(0x01, 0x04DE);
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

		}
		else if (mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusRTUSlave || mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusASCIISlave)
		{
			//Simulate a Slave Device on a Modbus/RTU channel
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .075 ) );
			
			U8 bytes[3] = {0xCD, 0x6B, 0x05};
			SendGenericResponse(0x01, 0x01, 0x03, bytes);
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			U8 status[3] = {0xAC, 0xDB, 0x35};
			SendGenericResponse(0x01, 0x02,0x03, status );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			U16 values[3] = {0x022B, 0x0000, 0x0064};
			SendGeneric2Response(0x01, 0x03, 0x06, values);
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			U16 values2[1] = {0x000A};
			SendGeneric2Response(0x01, 0x04, 0x02, values2);
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			//response is an echo of request so lets just reuse the code
			SendGenericRequest(0x01, 0x05, 0x00AC, 0xFF00);
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			//response is an echo of request so lets just reuse the code
			SendGenericRequest(0x01, 0x06, 0x0001, 0x0003);
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendReadExceptionStatusResponse(0x01, 0x07, 0x6D );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			//all available subcodes used for Diagnostics command
			//responses are echoes (in most cases but sometimes filled with requested data)
			SendRequest_Diagnostics(0x01, RETURN_QUERY_DATA, 0xA537 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendRequest_Diagnostics(0x01, RESTART_COMMUNICATIONS_OPTION, 0x0000 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendRequest_Diagnostics(0x01, RESTART_COMMUNICATIONS_OPTION, 0xFF00 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendRequest_Diagnostics(0x01, RETURN_DIAGNOSTIC_REGISTER, 0x0000 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendRequest_Diagnostics(0x01, CHANGE_ASCII_INPUT_DELIM, 0x4300 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			//no response required
			//SendRequest_Diagnostics(0x01, FORCE_LISTEN_ONLY_MODE, 0x0000 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendRequest_Diagnostics(0x01, CLEAR_COUNTERS_AND_DIAG_REGISTER, 0x0000 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendRequest_Diagnostics(0x01, RETURN_BUS_MESSAGE_COUNT, 0x0005 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendRequest_Diagnostics(0x01, RETURN_BUS_COMM_ERROR_COUNT, 0x0004 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendRequest_Diagnostics(0x01, RETURN_BUS_EXCEPTION_ERROR_COUNT, 0x0003 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendRequest_Diagnostics(0x01, RETURN_SLAVE_MESSAGE_COUNT, 0x0002 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendRequest_Diagnostics(0x01, RETURN_SLAVE_NO_RESPONSE_COUNT, 0x0001 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendRequest_Diagnostics(0x01, RETURN_SLAVE_NAK_COUNT, 0x0002 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendRequest_Diagnostics(0x01, RETURN_SLAVE_BUSY_COUNT, 0x0003 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendRequest_Diagnostics(0x01, RETURN_BUS_CHAR_OVERRUN_COUNT, 0x0004 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			SendRequest_Diagnostics(0x01, CLEAR_OVERRUN_COUNTER_AND_FLAG, 0x0000 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			//response is an echo of request so lets just reuse the code
			SendGenericRequest(0x01, 0x0B, 0xFFFF, 0x0108);
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );
			
			U8 someevents[] = {0x20, 0x00};
			SendGetCommEventLogResponse(0x01, 0x08, 0x0000, 0x0108, 0x0121, someevents);
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			//use existing code to simulate this message also
			SendGenericRequest(0x01, 0x0F, 0x0013, 0x000A);
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			//use existing code to simulate this message also
			SendGenericRequest(0x01, 0x10, 0x0001, 0x0002);
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			//This one is device specific, but here's a sample implementation for decode purposes
			//SlaveID is "Saleae" followed by 0xFF (ON)
			U8 SlaveID[] = {0x53, 0x61, 0x6C, 0x65, 0x61, 0x65, 0xFF};
			SendReportSlaveIDResponse(0x01, 0x07, SlaveID);
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			//0x14 here
			U8 RefTypes1[] = {0x06, 0x06};
			U8 RecordLengths1[] = {0x05, 0x05};
			U16 RecordData1[2][2] = {{0x0DFE, 0x0020}, {0x33CD ,0x0040} };
			SendWriteFileRecordResponse(0x01, 0x0C, RecordLengths1, RefTypes1, RecordData1);
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			//0x15 response is just echo of request
			U8 RefTypes2[] = {0x06};
			U16 FileNumbers2[] = {0x0004};
			U16 RecordNumbers2[] = {0x0007};
			U16 RecordLengths2[] = {0x0003};
			U16 RecordData2[1][3] = {{0x06AF, 0x04BE ,0x100D} };
			SendWriteFileRecordRequest(0x01, 0x0D, RefTypes2, FileNumbers2, RecordNumbers2, RecordLengths2, RecordData2 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			//this message is an echo of the request
			SendMaskWriteRegisterRequest(0x01, 0x0004, 0x00F2, 0x0025);
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );
			
			U16 items[6] = {0x00FE, 0x0ACD, 0x0001, 0x0003, 0x000D,  0x00FF};
			SendGeneric2Response(0x01, 0x17, 0x0C, items);
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			//Read FIFO Queue
			U16 Qdata[] = {0x01B8, 0x1284};
			SendReadFIFOQueueResponse(0x01, 0x0006, 0x0002, Qdata);
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			//NAKs for 0x01
			ModbusSimulationDataGenerator::SendException(0x01, 0x01, 0x01 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x01, 0x02 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x01, 0x03 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x01, 0x04 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			//Naks for 0x02
			ModbusSimulationDataGenerator::SendException(0x01, 0x02, 0x01 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x02, 0x02 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x02, 0x03 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x02, 0x04 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			//Naks for 0x03
			ModbusSimulationDataGenerator::SendException(0x01, 0x03, 0x01 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x03, 0x02 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x03, 0x03 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x03, 0x04 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			//Naks for 0x04
			ModbusSimulationDataGenerator::SendException(0x01, 0x04, 0x01 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x04, 0x02 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x04, 0x03 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x04, 0x04 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			//Naks for 0x05
			ModbusSimulationDataGenerator::SendException(0x01, 0x05, 0x01 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x05, 0x02 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x05, 0x03 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x05, 0x04 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			//Naks for 0x06
			ModbusSimulationDataGenerator::SendException(0x01, 0x06, 0x01 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x06, 0x02 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x06, 0x03 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x06, 0x04 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			//Naks for 0x07
			ModbusSimulationDataGenerator::SendException(0x01, 0x07, 0x01 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x07, 0x04 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			//Naks for 0x08
			ModbusSimulationDataGenerator::SendException(0x01, 0x08, 0x01 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x08, 0x03 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x08, 0x04 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			//Naks for 0x0B
			ModbusSimulationDataGenerator::SendException(0x01, 0x0B, 0x01 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x0B, 0x04 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			//Naks for 0x0C
			ModbusSimulationDataGenerator::SendException(0x01, 0x0C, 0x01 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x0C, 0x04 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			//Naks for 0x0F
			ModbusSimulationDataGenerator::SendException(0x01, 0x0F, 0x01 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x0F, 0x02 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x0F, 0x03 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x0F, 0x04 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			//Naks for 0x10
			ModbusSimulationDataGenerator::SendException(0x01, 0x10, 0x01 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x10, 0x02 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x10, 0x03 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x10, 0x04 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			//Naks for 0x11
			ModbusSimulationDataGenerator::SendException(0x01, 0x11, 0x01 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x11, 0x04 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			//Naks for 0x14
			ModbusSimulationDataGenerator::SendException(0x01, 0x14, 0x01 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x14, 0x02 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x14, 0x03 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x14, 0x04 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );
			
			ModbusSimulationDataGenerator::SendException(0x01, 0x14, 0x08 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			//Naks for 0x15
			ModbusSimulationDataGenerator::SendException(0x01, 0x15, 0x01 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x15, 0x02 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x15, 0x03 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x15, 0x04 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );
			
			ModbusSimulationDataGenerator::SendException(0x01, 0x15, 0x08 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			//Naks for 0x16
			ModbusSimulationDataGenerator::SendException(0x01, 0x16, 0x01 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x16, 0x02 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x16, 0x03 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x16, 0x04 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			//Naks for 0x17
			ModbusSimulationDataGenerator::SendException(0x01, 0x17, 0x01 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x17, 0x02 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x17, 0x03 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x17, 0x04 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			//Naks for 0x18
			ModbusSimulationDataGenerator::SendException(0x01, 0x18, 0x01 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x18, 0x02 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x18, 0x03 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x18, 0x04 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			//Naks for 0x2B
			ModbusSimulationDataGenerator::SendException(0x01, 0x2B, 0x01 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x2B, 0x02 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x2B, 0x03 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x2B, 0x04 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			//Naks for unrecognized function
			ModbusSimulationDataGenerator::SendException(0x01, 0x71, 0x01 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x71, 0x02 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x71, 0x03 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );

			ModbusSimulationDataGenerator::SendException(0x01, 0x71, 0x04 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByTimeS( .125 ) );
		}
	}
	*simulation_channels = &mModbusSimulationData;

	return 1;  // we are retuning the size of the SimulationChannelDescriptor array.  In our case, the "array" is length 1.
}

void ModbusSimulationDataGenerator::CreateModbusByte( U64 value )
{
	//assume we start high
	mModbusSimulationData.Transition();  //low-going edge for start bit
	mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod() );  //add start bit time

	if( mSettings->mInverted == true )
		value = ~value;

	U32 num_bits = mSettings->mBitsPerTransfer;
	if( mSettings->mModbusMode == ModbusAnalyzerEnums::MpModeMsbOneMeansAddress || mSettings->mModbusMode == ModbusAnalyzerEnums::MpModeMsbZeroMeansAddress )
		num_bits++;

	BitExtractor bit_extractor( value, mSettings->mShiftOrder, num_bits );

	for( U32 i=0; i<num_bits; i++ )
	{
		mModbusSimulationData.TransitionIfNeeded( bit_extractor.GetNextBit() );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod() );
	}

	if( mSettings->mParity == AnalyzerEnums::Even )
	{

		if( AnalyzerHelpers::IsEven( AnalyzerHelpers::GetOnesCount( value ) ) == true )		
			mModbusSimulationData.TransitionIfNeeded( mBitLow ); //we want to add a zero bit
		else
			mModbusSimulationData.TransitionIfNeeded( mBitHigh ); //we want to add a one bit

		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod() );

	}else
	if( mSettings->mParity == AnalyzerEnums::Odd )
	{

		if( AnalyzerHelpers::IsOdd( AnalyzerHelpers::GetOnesCount( value ) ) == true )
			mModbusSimulationData.TransitionIfNeeded( mBitLow ); //we want to add a zero bit
		else
			mModbusSimulationData.TransitionIfNeeded( mBitHigh );

		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod() );

	}

		mModbusSimulationData.TransitionIfNeeded( mBitHigh ); //we need to end high

	//lets pad the end a bit for the stop bit:
	mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( mSettings->mStopBits ) );
}


void ModbusSimulationDataGenerator::SendGenericRequest(U8 DeviceID, U8 FuncCode, U16 StartingAddress, U16 Quantity )
{
	if(mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusRTUMaster || mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusRTUSlave)
	{
		
		CreateModbusByte( DeviceID );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	
		CreateModbusByte( FuncCode );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
			
		CreateModbusByte( StartingAddress&0x00FF );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		
		CreateModbusByte( (StartingAddress&0xFF00) >> 8 );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte( Quantity&0x00FF );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte( (Quantity&0xFF00) >> 8 );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		
		U16 CRCValue = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
		CRCValue = update_CRC( CRCValue, DeviceID );
		CRCValue = update_CRC( CRCValue, FuncCode );
		CRCValue = update_CRC( CRCValue, StartingAddress&0x00FF );
		CRCValue = update_CRC( CRCValue, (StartingAddress&0xFF00) >> 8 );
		CRCValue = update_CRC( CRCValue, Quantity&0x00FF );
		CRCValue = update_CRC( CRCValue, (Quantity&0xFF00) >> 8);

		CreateModbusByte((CRCValue&0x00FF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte(((CRCValue&0xFF00) >> 8));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	}
	else
	{
		//it's easier to compute the LRC before converting the data from binary to ASCII
		U16 LRCvalue = 0x00;   //Modbus/ASCII uses LRC, initialization to 0x00;

		LRCvalue = LRCvalue + DeviceID;
		LRCvalue = LRCvalue + FuncCode;
		LRCvalue = LRCvalue + (StartingAddress&0x00FF);
		LRCvalue = LRCvalue + ((StartingAddress&0xFF00) >> 8);
		LRCvalue = LRCvalue + (Quantity&0x00FF);
		LRCvalue = LRCvalue + ( (Quantity&0xFF00) >> 8 );

		LRCvalue =  ~LRCvalue +1 ;
		LRCvalue = LRCvalue &0x00FF;

		CreateModbusByte( ':' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID&0xF) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(FuncCode>>4));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(FuncCode&0xF) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((StartingAddress&0xF000)>>12) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((StartingAddress&0x0F00)>>8) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((StartingAddress&0x00F0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII(StartingAddress&0xF) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((Quantity&0xF000)>>12) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((Quantity&0x0F00)>>8) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((Quantity&0x00F0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII(Quantity&0xF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((LRCvalue&0xF0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((LRCvalue&0xF)) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( '\r' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( '\n' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );


	}
}


void ModbusSimulationDataGenerator::SendGenDiagnosticRequest(U8 DeviceID, U8 FuncCode )
{
	if(mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusRTUMaster)
	{
		
		CreateModbusByte( DeviceID );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	
		CreateModbusByte( FuncCode );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		
		U16 CRCValue = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
		CRCValue = update_CRC( CRCValue, DeviceID );
		CRCValue = update_CRC( CRCValue, FuncCode );

		CreateModbusByte((CRCValue&0x00FF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte(((CRCValue&0xFF00) >> 8));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	}
	else
	{
		//it's easier to compute the LRC before converting the data from binary to ASCII
		U16 LRCvalue = 0x00;   //Modbus/ASCII uses LRC, initialization to 0x00;

		LRCvalue = LRCvalue + DeviceID;
		LRCvalue = LRCvalue + FuncCode;

		LRCvalue =  ~LRCvalue +1 ;
		LRCvalue = LRCvalue &0x00FF;

		CreateModbusByte( ':' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID&0xF) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(FuncCode>>4));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(FuncCode&0xF) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((LRCvalue&0xF0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((LRCvalue&0xF)) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( '\r' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( '\n' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );


	}
}

void ModbusSimulationDataGenerator::SendMaskWriteRegisterRequest(U8 DeviceID, U16 ReferenceAddress, U16 And_Mask, U16 Or_Mask )
{
	if(mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusRTUMaster || mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusRTUSlave)
	{
		
		CreateModbusByte( DeviceID );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	
		CreateModbusByte( 0x16 );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
			
		CreateModbusByte( ReferenceAddress&0x00FF );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		
		CreateModbusByte( (ReferenceAddress&0xFF00) >> 8 );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte( And_Mask&0x00FF );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte( (And_Mask&0xFF00) >> 8 );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte( Or_Mask&0x00FF );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte( (Or_Mask&0xFF00) >> 8 );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		
		U16 CRCValue = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
		CRCValue = update_CRC( CRCValue, DeviceID );
		CRCValue = update_CRC( CRCValue, 0x16 );
		CRCValue = update_CRC( CRCValue, ReferenceAddress&0x00FF );
		CRCValue = update_CRC( CRCValue, (ReferenceAddress&0xFF00) >> 8 );
		CRCValue = update_CRC( CRCValue, And_Mask&0x00FF );
		CRCValue = update_CRC( CRCValue, (And_Mask&0xFF00) >> 8);
		CRCValue = update_CRC( CRCValue, Or_Mask&0x00FF );
		CRCValue = update_CRC( CRCValue, (Or_Mask&0xFF00) >> 8);

		CreateModbusByte((CRCValue&0x00FF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte(((CRCValue&0xFF00) >> 8));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	}
	else
	{
		//it's easier to compute the LRC before converting the data from binary to ASCII
		U16 LRCvalue = 0x00;   //Modbus/ASCII uses LRC, initialization to 0x00;

		LRCvalue = LRCvalue + DeviceID;
		LRCvalue = LRCvalue + 0x16;
		LRCvalue = LRCvalue + (ReferenceAddress&0x00FF);
		LRCvalue = LRCvalue + ((ReferenceAddress&0xFF00) >> 8);
		LRCvalue = LRCvalue + (And_Mask&0x00FF);
		LRCvalue = LRCvalue + ( (And_Mask&0xFF00) >> 8 );
		LRCvalue = LRCvalue + (Or_Mask&0x00FF);
		LRCvalue = LRCvalue + ( (Or_Mask&0xFF00) >> 8 );

		LRCvalue =  ~LRCvalue +1 ;
		LRCvalue = LRCvalue &0x00FF;

		CreateModbusByte( ':' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID&0xF) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( '1' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( '6' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((ReferenceAddress&0xF000)>>12) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((ReferenceAddress&0x0F00)>>8) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((ReferenceAddress&0x00F0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII(ReferenceAddress&0xF) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((And_Mask&0xF000)>>12) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((And_Mask&0x0F00)>>8) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((And_Mask&0x00F0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII(And_Mask&0xF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((Or_Mask&0xF000)>>12) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((Or_Mask&0x0F00)>>8) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((Or_Mask&0x00F0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII(Or_Mask&0xF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((LRCvalue&0xF0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((LRCvalue&0xF)) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( '\r' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( '\n' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

	}
}

void ModbusSimulationDataGenerator::SendReadFIFOQueueRequest(U8 DeviceID, U16 FIFOAddress)
{
	if(mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusRTUMaster)
	{
		
		CreateModbusByte( DeviceID );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	
		CreateModbusByte( 0x18 );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
			
		CreateModbusByte( FIFOAddress&0x00FF );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		
		CreateModbusByte( (FIFOAddress&0xFF00) >> 8 );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		
		U16 CRCValue = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
		CRCValue = update_CRC( CRCValue, DeviceID );
		CRCValue = update_CRC( CRCValue, 0x18 );
		CRCValue = update_CRC( CRCValue, FIFOAddress&0x00FF );
		CRCValue = update_CRC( CRCValue, (FIFOAddress&0xFF00) >> 8 );

		CreateModbusByte((CRCValue&0x00FF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte(((CRCValue&0xFF00) >> 8));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	}
	else
	{
		//it's easier to compute the LRC before converting the data from binary to ASCII
		U16 LRCvalue = 0x00;   //Modbus/ASCII uses LRC, initialization to 0x00;

		LRCvalue = LRCvalue + DeviceID;
		LRCvalue = LRCvalue + 0x18;
		LRCvalue = LRCvalue + (FIFOAddress&0x00FF);
		LRCvalue = LRCvalue + ((FIFOAddress&0xFF00) >> 8);

		LRCvalue =  ~LRCvalue +1 ;
		LRCvalue = LRCvalue &0x00FF;

		CreateModbusByte( ':' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID&0xF) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( '1' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( '8' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((FIFOAddress&0xF000)>>12) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((FIFOAddress&0x0F00)>>8) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((FIFOAddress&0x00F0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII(FIFOAddress&0xF) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((LRCvalue&0xF0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((LRCvalue&0xF)) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( '\r' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( '\n' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

	}
}


void ModbusSimulationDataGenerator::SendRequest_Diagnostics(U8 DeviceID, U16 SubFunction, U16 Data )
{
	if(mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusRTUMaster || mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusRTUSlave)
	{
		
		CreateModbusByte( DeviceID );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	
		CreateModbusByte( 0x08 );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
			
		CreateModbusByte( SubFunction&0x00FF );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		
		CreateModbusByte( (SubFunction&0xFF00) >> 8 );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte( Data&0x00FF );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte( (Data&0xFF00) >> 8 );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		
		U16 CRCValue = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
		CRCValue = update_CRC( CRCValue, DeviceID );
		CRCValue = update_CRC( CRCValue, 0x08 );
		CRCValue = update_CRC( CRCValue, SubFunction&0x00FF );
		CRCValue = update_CRC( CRCValue, (SubFunction&0xFF00) >> 8 );
		CRCValue = update_CRC( CRCValue, Data&0x00FF );
		CRCValue = update_CRC( CRCValue, (Data&0xFF00) >> 8);

		CreateModbusByte((CRCValue&0x00FF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte(((CRCValue&0xFF00) >> 8));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	}
	else
	{
		//it's easier to compute the LRC before converting the data from binary to ASCII
		U16 LRCvalue = 0x00;   //Modbus/ASCII uses LRC, initialization to 0x00;

		LRCvalue = LRCvalue + DeviceID;
		LRCvalue = LRCvalue + 0x08;
		LRCvalue = LRCvalue + (SubFunction&0x00FF);
		LRCvalue = LRCvalue + ((SubFunction&0xFF00) >> 8);
		LRCvalue = LRCvalue + (Data&0x00FF);
		LRCvalue = LRCvalue + ( (Data&0xFF00) >> 8 );

		LRCvalue =  ~LRCvalue +1 ;
		LRCvalue = LRCvalue &0x00FF;

		CreateModbusByte( ':' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID&0xF) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( '0' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( '8' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((SubFunction&0xF000)>>12) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((SubFunction&0x0F00)>>8) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((SubFunction&0x00F0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII(SubFunction&0xF) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((Data&0xF000)>>12) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((Data&0x0F00)>>8) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((Data&0x00F0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII(Data&0xF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((LRCvalue&0xF0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((LRCvalue&0xF)) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( '\r' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( '\n' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

	}
}

void ModbusSimulationDataGenerator::SendWriteMultipleCoilsRequest(U8 DeviceID, U16 StartingAddress, U16 Quantity, U8 ByteCount, U8 Values[] )
{
	if(mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusRTUMaster)
	{
		
		CreateModbusByte( DeviceID );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	
		CreateModbusByte( 0x0F );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
			
		CreateModbusByte( StartingAddress&0x00FF );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		
		CreateModbusByte( (StartingAddress&0xFF00) >> 8 );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte( Quantity&0x00FF );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte( (Quantity&0xFF00) >> 8 );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte( ByteCount );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		
		for(int i=0;i<ByteCount;i++)
		{
			CreateModbusByte( Values[i] );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		}
		
		U16 CRCValue = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
		CRCValue = update_CRC( CRCValue, DeviceID );
		CRCValue = update_CRC( CRCValue, 0x0F );
		CRCValue = update_CRC( CRCValue, StartingAddress&0x00FF );
		CRCValue = update_CRC( CRCValue, (StartingAddress&0xFF00) >> 8 );
		CRCValue = update_CRC( CRCValue, Quantity&0x00FF );
		CRCValue = update_CRC( CRCValue, (Quantity&0xFF00) >> 8);
		CRCValue = update_CRC( CRCValue, ByteCount );

		for(int i=0;i<ByteCount;i++)
		{
			CRCValue = update_CRC( CRCValue, Values[i] );
		}

		CreateModbusByte((CRCValue&0x00FF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte(((CRCValue&0xFF00) >> 8));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	}
	else
	{
		//it's easier to compute the LRC before converting the data from binary to ASCII
		U16 LRCvalue = 0x00;   //Modbus/ASCII uses LRC, initialization to 0x00;

		LRCvalue = LRCvalue + DeviceID;
		LRCvalue = LRCvalue + 0x0F;
		LRCvalue = LRCvalue + (StartingAddress&0x00FF);
		LRCvalue = LRCvalue + ((StartingAddress&0xFF00) >> 8);
		LRCvalue = LRCvalue + (Quantity&0x00FF);
		LRCvalue = LRCvalue + ( (Quantity&0xFF00) >> 8 );
		LRCvalue = LRCvalue + ByteCount;

		for(int i=0;i<ByteCount;i++)
		{
			LRCvalue = LRCvalue + Values[i];
		}

		LRCvalue =  ~LRCvalue +1 ;
		LRCvalue = LRCvalue &0x00FF;

		CreateModbusByte( ':' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID&0xF) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( '0' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( 'F' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((StartingAddress&0xF000)>>12) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((StartingAddress&0x0F00)>>8) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((StartingAddress&0x00F0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII(StartingAddress&0xF) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((Quantity&0xF000)>>12) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((Quantity&0x0F00)>>8) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((Quantity&0x00F0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII(Quantity&0xF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((ByteCount&0xF0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII(ByteCount&0xF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		
		for(int i=0;i<ByteCount;i++)
		{
			CreateModbusByte( BinToASCII((Values[i]&0xF0)>>4) );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII(Values[i]&0xF));
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		}

		CreateModbusByte( BinToASCII((LRCvalue&0xF0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((LRCvalue&0xF)) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( '\r' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( '\n' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
	}
}


void ModbusSimulationDataGenerator::SendWriteMultipleRegistersRequest(U8 DeviceID, U16 StartingAddress, U16 Quantity, U8 ByteCount, U16 Values[] )
{
	if(mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusRTUMaster)
	{
		
		CreateModbusByte( DeviceID );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	
		CreateModbusByte( 0x10 );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
			
		CreateModbusByte( StartingAddress&0x00FF );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		
		CreateModbusByte( (StartingAddress&0xFF00) >> 8 );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte( Quantity&0x00FF );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte( (Quantity&0xFF00) >> 8 );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte( ByteCount );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		
		for(int i=0;i<ByteCount/2;i++)
		{
			CreateModbusByte( Values[i]&0x00FF );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		
			CreateModbusByte( (Values[i]&0xFF00) >> 8 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		}
		
		U16 CRCValue = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
		CRCValue = update_CRC( CRCValue, DeviceID );
		CRCValue = update_CRC( CRCValue, 0x10 );
		CRCValue = update_CRC( CRCValue, StartingAddress&0x00FF );
		CRCValue = update_CRC( CRCValue, (StartingAddress&0xFF00) >> 8 );
		CRCValue = update_CRC( CRCValue, Quantity&0x00FF );
		CRCValue = update_CRC( CRCValue, (Quantity&0xFF00) >> 8);
		CRCValue = update_CRC( CRCValue, ByteCount );

		for(int i=0;i<ByteCount/2;i++)
		{
			CRCValue = update_CRC( CRCValue, Values[i]&0x00FF );
			CRCValue = update_CRC( CRCValue, (Values[i]&0xFF00) >> 8 );
		}

		CreateModbusByte((CRCValue&0x00FF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte(((CRCValue&0xFF00) >> 8));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	}
	else
	{
		//it's easier to compute the LRC before converting the data from binary to ASCII
		U16 LRCvalue = 0x00;   //Modbus/ASCII uses LRC, initialization to 0x00;

		LRCvalue = LRCvalue + DeviceID;
		LRCvalue = LRCvalue + 0x10;
		LRCvalue = LRCvalue + (StartingAddress&0x00FF);
		LRCvalue = LRCvalue + ((StartingAddress&0xFF00) >> 8);
		LRCvalue = LRCvalue + (Quantity&0x00FF);
		LRCvalue = LRCvalue + ( (Quantity&0xFF00) >> 8 );
		LRCvalue = LRCvalue + ByteCount;

		for(int i=0;i<ByteCount/2;i++)
		{
			LRCvalue = LRCvalue + (Values[i]&0x00FF);
			LRCvalue = LRCvalue + ((Values[i]&0xFF00) >> 8);
		}

		LRCvalue =  ~LRCvalue +1 ;
		LRCvalue = LRCvalue &0x00FF;

		CreateModbusByte( ':' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID&0xF) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( '1' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( '0' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((StartingAddress&0xF000)>>12) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((StartingAddress&0x0F00)>>8) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((StartingAddress&0x00F0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII(StartingAddress&0xF) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((Quantity&0xF000)>>12) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((Quantity&0x0F00)>>8) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((Quantity&0x00F0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII(Quantity&0xF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((ByteCount&0xF0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII(ByteCount&0xF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		
		for(int i=0;i<ByteCount/2;i++)
		{	
			CreateModbusByte( BinToASCII((Values[i]&0xF000)>>12) );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII((Values[i]&0x0F00)>>8));
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII((Values[i]&0xF0)>>4) );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII(Values[i]&0xF));
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		}

		CreateModbusByte( BinToASCII((LRCvalue&0xF0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((LRCvalue&0xF)) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( '\r' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( '\n' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
	}
}

void ModbusSimulationDataGenerator::SendReadFileRecordRequest(U8 DeviceID, U8 ByteCount, U8 SubReqReferenceTypes[], U16 SubReqFileNumbers[], U16 SubReqRecordNumbers[], U16 SubReqRecordLengths[] )
{
	if(mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusRTUMaster)
	{
		
		CreateModbusByte( DeviceID );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	
		CreateModbusByte( 0x14 );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte( ByteCount );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		
		for(int i=0;i<ByteCount/7;i++)
		{
			CreateModbusByte( SubReqReferenceTypes[i] );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

			CreateModbusByte( SubReqFileNumbers[i]&0x00FF );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		
			CreateModbusByte( (SubReqFileNumbers[i]&0xFF00) >> 8 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

			CreateModbusByte( SubReqRecordNumbers[i]&0x00FF );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		
			CreateModbusByte( (SubReqRecordNumbers[i]&0xFF00) >> 8 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

			CreateModbusByte( SubReqRecordLengths[i]&0x00FF );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		
			CreateModbusByte( (SubReqRecordLengths[i]&0xFF00) >> 8 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		}
		
		U16 CRCValue = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
		CRCValue = update_CRC( CRCValue, DeviceID );
		CRCValue = update_CRC( CRCValue, 0x14 );
		CRCValue = update_CRC( CRCValue, ByteCount );

		for(int i=0;i<ByteCount/7;i++)
		{
			CRCValue = update_CRC( CRCValue, SubReqReferenceTypes[i] );
			CRCValue = update_CRC( CRCValue, SubReqFileNumbers[i]&0x00FF );
			CRCValue = update_CRC( CRCValue, (SubReqFileNumbers[i]&0xFF00) >> 8 );
			CRCValue = update_CRC( CRCValue, SubReqRecordNumbers[i]&0x00FF );
			CRCValue = update_CRC( CRCValue, (SubReqRecordNumbers[i]&0xFF00) >> 8 );
			CRCValue = update_CRC( CRCValue, SubReqRecordLengths[i]&0x00FF );
			CRCValue = update_CRC( CRCValue, (SubReqRecordLengths[i]&0xFF00) >> 8 );
		}

		CreateModbusByte((CRCValue&0x00FF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte(((CRCValue&0xFF00) >> 8));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	}
	else
	{
		//it's easier to compute the LRC before converting the data from binary to ASCII
		U16 LRCvalue = 0x00;   //Modbus/ASCII uses LRC, initialization to 0x00;

		LRCvalue = LRCvalue + DeviceID;
		LRCvalue = LRCvalue + 0x14;
		LRCvalue = LRCvalue + ByteCount;

		for(int i=0;i<ByteCount/7;i++)
		{
			LRCvalue = LRCvalue + SubReqReferenceTypes[i];
			LRCvalue = LRCvalue + (SubReqFileNumbers[i]&0x00FF);
			LRCvalue = LRCvalue + ((SubReqFileNumbers[i]&0xFF00) >> 8);
			LRCvalue = LRCvalue + (SubReqRecordNumbers[i]&0x00FF);
			LRCvalue = LRCvalue + ((SubReqRecordNumbers[i]&0xFF00) >> 8);
			LRCvalue = LRCvalue + (SubReqRecordLengths[i]&0x00FF);
			LRCvalue = LRCvalue + ((SubReqRecordLengths[i]&0xFF00) >> 8);
		}

		LRCvalue =  ~LRCvalue +1 ;
		LRCvalue = LRCvalue &0x00FF;

		CreateModbusByte( ':' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID&0xF) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( '1' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( '4' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((ByteCount&0xF0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII(ByteCount&0xF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		
		for(int i=0;i<ByteCount/7;i++)
		{	
			CreateModbusByte( BinToASCII((SubReqReferenceTypes[i]&0xF0)>>4) );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII(SubReqReferenceTypes[i]&0xF));
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			
			CreateModbusByte( BinToASCII((SubReqFileNumbers[i]&0xF000)>>12) );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII((SubReqFileNumbers[i]&0x0F00)>>8));
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII((SubReqFileNumbers[i]&0xF0)>>4) );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII(SubReqFileNumbers[i]&0xF));
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

			CreateModbusByte( BinToASCII((SubReqRecordNumbers[i]&0xF000)>>12) );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII((SubReqRecordNumbers[i]&0x0F00)>>8));
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII((SubReqRecordNumbers[i]&0xF0)>>4) );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII(SubReqRecordNumbers[i]&0xF));
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

			CreateModbusByte( BinToASCII((SubReqRecordLengths[i]&0xF000)>>12) );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII((SubReqRecordLengths[i]&0x0F00)>>8));
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII((SubReqRecordLengths[i]&0xF0)>>4) );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII(SubReqRecordLengths[i]&0xF));
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		}

		CreateModbusByte( BinToASCII((LRCvalue&0xF0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((LRCvalue&0xF)) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( '\r' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( '\n' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
	}
}


void ModbusSimulationDataGenerator::SendWriteFileRecordRequest(U8 DeviceID, U8 ByteCount, U8 SubReqReferenceTypes[], U16 SubReqFileNumbers[], U16 SubReqRecordNumbers[], U16 SubReqRecordLengths[], U16 SubReqRecordData[1][3] )
{
	if(mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusRTUMaster || mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusRTUSlave)
	{
		
		CreateModbusByte( DeviceID );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	
		CreateModbusByte( 0x15 );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte( ByteCount );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		
		U16 totalbytes=0;
		int i = 0;
		while(totalbytes<ByteCount)
		{
			CreateModbusByte( SubReqReferenceTypes[i] );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

			CreateModbusByte( SubReqFileNumbers[i]&0x00FF );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		
			CreateModbusByte( (SubReqFileNumbers[i]&0xFF00) >> 8 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

			CreateModbusByte( SubReqRecordNumbers[i]&0x00FF );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		
			CreateModbusByte( (SubReqRecordNumbers[i]&0xFF00) >> 8 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

			CreateModbusByte( SubReqRecordLengths[i]&0x00FF );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		
			CreateModbusByte( (SubReqRecordLengths[i]&0xFF00) >> 8 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

			for(int j=0;j<SubReqRecordLengths[i];j++)
			{
				CreateModbusByte( SubReqRecordData[i][j]&0x00FF );
				mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		
				CreateModbusByte( (SubReqRecordData[i][j]&0xFF00) >> 8 );
				mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

			}

			totalbytes = totalbytes + (SubReqRecordLengths[i]*2) + 7;
			i++;
		}
		
		U16 CRCValue = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
		CRCValue = update_CRC( CRCValue, DeviceID );
		CRCValue = update_CRC( CRCValue, 0x15 );
		CRCValue = update_CRC( CRCValue, ByteCount );

		totalbytes=0;
		i = 0;
		while(totalbytes<ByteCount)
		{
			CRCValue = update_CRC( CRCValue, SubReqReferenceTypes[i] );
			CRCValue = update_CRC( CRCValue, SubReqFileNumbers[i]&0x00FF );
			CRCValue = update_CRC( CRCValue, (SubReqFileNumbers[i]&0xFF00) >> 8 );
			CRCValue = update_CRC( CRCValue, SubReqRecordNumbers[i]&0x00FF );
			CRCValue = update_CRC( CRCValue, (SubReqRecordNumbers[i]&0xFF00) >> 8 );
			CRCValue = update_CRC( CRCValue, SubReqRecordLengths[i]&0x00FF );
			CRCValue = update_CRC( CRCValue, (SubReqRecordLengths[i]&0xFF00) >> 8 );

			for(int j=0;j<SubReqRecordLengths[i];j++)
			{
				CRCValue = update_CRC( CRCValue, SubReqRecordData[i][j]&0x00FF );
				CRCValue = update_CRC( CRCValue, (SubReqRecordData[i][j]&0xFF00) >> 8 );
			}

			totalbytes = totalbytes + (SubReqRecordLengths[i]*2) + 7;
			i++;
		}

		CreateModbusByte((CRCValue&0x00FF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte(((CRCValue&0xFF00) >> 8));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	}
	else
	{
		//it's easier to compute the LRC before converting the data from binary to ASCII
		U16 LRCvalue = 0x00;   //Modbus/ASCII uses LRC, initialization to 0x00;

		LRCvalue = LRCvalue + DeviceID;
		LRCvalue = LRCvalue + 0x15;
		LRCvalue = LRCvalue + ByteCount;

		U16 totalbytes=0;
		int i = 0;
		while(totalbytes<ByteCount)
		{
			LRCvalue = LRCvalue + SubReqReferenceTypes[i];
			LRCvalue = LRCvalue + (SubReqFileNumbers[i]&0x00FF);
			LRCvalue = LRCvalue + ((SubReqFileNumbers[i]&0xFF00) >> 8);
			LRCvalue = LRCvalue + (SubReqRecordNumbers[i]&0x00FF);
			LRCvalue = LRCvalue + ((SubReqRecordNumbers[i]&0xFF00) >> 8);
			LRCvalue = LRCvalue + (SubReqRecordLengths[i]&0x00FF);
			LRCvalue = LRCvalue + ((SubReqRecordLengths[i]&0xFF00) >> 8);
			
			for(int j=0;j<SubReqRecordLengths[i];j++)
			{
				LRCvalue = LRCvalue + SubReqRecordData[i][j]&0x00FF;
				LRCvalue = LRCvalue +((SubReqRecordData[i][j]&0xFF00) >> 8 );
			}

			totalbytes = totalbytes + (SubReqRecordLengths[i]*2) + 7;
			i++;
		}

		LRCvalue =  ~LRCvalue +1 ;
		LRCvalue = LRCvalue &0x00FF;

		CreateModbusByte( ':' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID&0xF) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( '1' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( '5' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((ByteCount&0xF0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII(ByteCount&0xF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		
		totalbytes=0;
		i = 0;
		while(totalbytes<ByteCount)
		{
			CreateModbusByte( BinToASCII((SubReqReferenceTypes[i]&0xF0)>>4) );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII(SubReqReferenceTypes[i]&0xF));
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			
			CreateModbusByte( BinToASCII((SubReqFileNumbers[i]&0xF000)>>12) );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII((SubReqFileNumbers[i]&0x0F00)>>8));
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII((SubReqFileNumbers[i]&0xF0)>>4) );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII(SubReqFileNumbers[i]&0xF));
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

			CreateModbusByte( BinToASCII((SubReqRecordNumbers[i]&0xF000)>>12) );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII((SubReqRecordNumbers[i]&0x0F00)>>8));
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII((SubReqRecordNumbers[i]&0xF0)>>4) );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII(SubReqRecordNumbers[i]&0xF));
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

			CreateModbusByte( BinToASCII((SubReqRecordLengths[i]&0xF000)>>12) );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII((SubReqRecordLengths[i]&0x0F00)>>8));
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII((SubReqRecordLengths[i]&0xF0)>>4) );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII(SubReqRecordLengths[i]&0xF));
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

			for(int j=0;j<SubReqRecordLengths[i];j++)
			{
				CreateModbusByte( BinToASCII((SubReqRecordData[i][j]&0xF000)>>12) );
				mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
				CreateModbusByte( BinToASCII((SubReqRecordData[i][j]&0x0F00)>>8));
				mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
				CreateModbusByte( BinToASCII((SubReqRecordData[i][j]&0xF0)>>4) );
				mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
				CreateModbusByte( BinToASCII(SubReqRecordData[i][j]&0xF));
				mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

			}

			totalbytes = totalbytes + (SubReqRecordLengths[i]*2) + 7;
			i++;

		}

		CreateModbusByte( BinToASCII((LRCvalue&0xF0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((LRCvalue&0xF)) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( '\r' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( '\n' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
	}
}

void ModbusSimulationDataGenerator::SendReadWriteMultipleRegisters(U8 DeviceID, U16 ReadStartingAddress, U16 QuantityToRead, U16 WriteStartingAddress, U16 QuantityToWrite, U8 WriteByteCount, U16 Values[] )
{
	if(mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusRTUMaster)
	{
		
		CreateModbusByte( DeviceID );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	
		CreateModbusByte( 0x17 );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
			
		CreateModbusByte( ReadStartingAddress&0x00FF );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		
		CreateModbusByte( (ReadStartingAddress&0xFF00) >> 8 );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte( QuantityToRead&0x00FF );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte( (QuantityToRead&0xFF00) >> 8 );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte( WriteStartingAddress&0x00FF );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		
		CreateModbusByte( (WriteStartingAddress&0xFF00) >> 8 );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte( QuantityToWrite&0x00FF );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte( (QuantityToWrite&0xFF00) >> 8 );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte( WriteByteCount );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		
		for(int i=0;i<WriteByteCount/2;i++)
		{
			CreateModbusByte( Values[i]&0x00FF );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

			CreateModbusByte( (Values[i]&0xFF00) >> 8 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		}
		
		U16 CRCValue = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
		CRCValue = update_CRC( CRCValue, DeviceID );
		CRCValue = update_CRC( CRCValue, 0x17 );
		CRCValue = update_CRC( CRCValue, ReadStartingAddress&0x00FF );
		CRCValue = update_CRC( CRCValue, (ReadStartingAddress&0xFF00) >> 8 );
		CRCValue = update_CRC( CRCValue, QuantityToRead&0x00FF );
		CRCValue = update_CRC( CRCValue, (QuantityToRead&0xFF00) >> 8);
		CRCValue = update_CRC( CRCValue, WriteStartingAddress&0x00FF );
		CRCValue = update_CRC( CRCValue, (WriteStartingAddress&0xFF00) >> 8 );
		CRCValue = update_CRC( CRCValue, QuantityToWrite&0x00FF );
		CRCValue = update_CRC( CRCValue, (QuantityToWrite&0xFF00) >> 8);
		CRCValue = update_CRC( CRCValue, WriteByteCount );

		for(int i=0;i<WriteByteCount/2;i++)
		{
			CRCValue = update_CRC( CRCValue, Values[i]&0x00FF );
			CRCValue = update_CRC( CRCValue,(Values[i]&0xFF00) >> 8 );
		}

		CreateModbusByte((CRCValue&0x00FF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte(((CRCValue&0xFF00) >> 8));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	}
	else
	{
		//it's easier to compute the LRC before converting the data from binary to ASCII
		U16 LRCvalue = 0x00;   //Modbus/ASCII uses LRC, initialization to 0x00;

		LRCvalue = LRCvalue + DeviceID;
		LRCvalue = LRCvalue + 0x17;
		LRCvalue = LRCvalue + (ReadStartingAddress&0x00FF);
		LRCvalue = LRCvalue + ((ReadStartingAddress&0xFF00) >> 8);
		LRCvalue = LRCvalue + (QuantityToRead&0x00FF);
		LRCvalue = LRCvalue + ( (QuantityToRead&0xFF00) >> 8 );
		LRCvalue = LRCvalue + (WriteStartingAddress&0x00FF);
		LRCvalue = LRCvalue + ((WriteStartingAddress&0xFF00) >> 8);
		LRCvalue = LRCvalue + (QuantityToWrite&0x00FF);
		LRCvalue = LRCvalue + ( (QuantityToWrite&0xFF00) >> 8 );
		LRCvalue = LRCvalue + WriteByteCount;

		for(int i=0;i<WriteByteCount/2;i++)
		{
			LRCvalue = LRCvalue + Values[i]&0x00FF;
			LRCvalue = LRCvalue + ((Values[i]&0xFF00) >> 8);
		}

		LRCvalue =  ~LRCvalue +1 ;
		LRCvalue = LRCvalue &0x00FF;

		CreateModbusByte( ':' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID&0xF) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( '1' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( '7' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((ReadStartingAddress&0xF000)>>12) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((ReadStartingAddress&0x0F00)>>8) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((ReadStartingAddress&0x00F0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII(ReadStartingAddress&0xF) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((QuantityToRead&0xF000)>>12) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((QuantityToRead&0x0F00)>>8) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((QuantityToRead&0x00F0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII(QuantityToRead&0xF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((WriteStartingAddress&0xF000)>>12) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((WriteStartingAddress&0x0F00)>>8) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((WriteStartingAddress&0x00F0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII(WriteStartingAddress&0xF) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((QuantityToWrite&0xF000)>>12) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((QuantityToWrite&0x0F00)>>8) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((QuantityToWrite&0x00F0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII(QuantityToWrite&0xF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((WriteByteCount&0xF0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII(WriteByteCount&0xF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		
		for(int i=0;i<WriteByteCount/2;i++)
		{
			CreateModbusByte( BinToASCII((Values[i]&0xF000)>>12) );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII((Values[i]&0x0F00)>>8) );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII((Values[i]&0xF0)>>4) );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII(Values[i]&0xF));
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		}

		CreateModbusByte( BinToASCII((LRCvalue&0xF0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((LRCvalue&0xF)) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( '\r' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( '\n' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
	}
}
//Responses

void ModbusSimulationDataGenerator::SendGenericResponse(U8 DeviceID, U8 FuncCode, U8 ByteCount, U8 Status[])
{
	if(mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusRTUSlave)
	{
		
		CreateModbusByte( DeviceID );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	
		CreateModbusByte( FuncCode );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
			
		CreateModbusByte( ByteCount );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		
		for(int i=0;i<ByteCount;i++)
		{
			CreateModbusByte( Status[i] );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		}
		
		U16 CRCValue = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
		CRCValue = update_CRC( CRCValue, DeviceID );
		CRCValue = update_CRC( CRCValue, FuncCode );
		CRCValue = update_CRC( CRCValue, ByteCount );

		for(int i=0;i<ByteCount;i++)
		{
			CRCValue = update_CRC( CRCValue, Status[i] );
		}

		CreateModbusByte((CRCValue&0x00FF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte(((CRCValue&0xFF00) >> 8));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	}
	else
	{
		//it's easier to compute the LRC before converting the data from binary to ASCII
		U16 LRCvalue = 0x00;   //Modbus/ASCII uses LRC, initialization to 0x00;

		LRCvalue = LRCvalue + DeviceID;
		LRCvalue = LRCvalue + FuncCode;
		LRCvalue = LRCvalue + (ByteCount);

		for(int i=0;i<ByteCount;i++)
		{
			LRCvalue = LRCvalue + (Status[i]);
		}

		LRCvalue =  ~LRCvalue +1 ;
		LRCvalue = LRCvalue &0x00FF;

		CreateModbusByte( ':' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID&0xF) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(FuncCode>>4));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(FuncCode&0xF) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(ByteCount>>4));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(ByteCount&0xF) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		for(int i=0;i<ByteCount;i++)
		{
			CreateModbusByte( BinToASCII(Status[i]>>4));
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

			CreateModbusByte( BinToASCII(Status[i]&0xF) );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		}

		CreateModbusByte( BinToASCII((LRCvalue&0xF0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((LRCvalue&0xF)) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( '\r' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( '\n' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );


	}
}


void ModbusSimulationDataGenerator::SendGeneric2Response(U8 DeviceID, U8 FuncCode, U8 ByteCount, U16 Values[])
{
	if(mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusRTUSlave)
	{
		
		CreateModbusByte( DeviceID );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	
		CreateModbusByte( FuncCode );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
			
		CreateModbusByte( ByteCount );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		
		for(int i=0;i<ByteCount/2;i++)
		{
			CreateModbusByte( Values[i]&0x00FF );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

			CreateModbusByte( (Values[i]&0xFF00) >> 8 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		}
		
		U16 CRCValue = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
		CRCValue = update_CRC( CRCValue, DeviceID );
		CRCValue = update_CRC( CRCValue, FuncCode);
		CRCValue = update_CRC( CRCValue, ByteCount );

		for(int i=0;i<ByteCount/2;i++)
		{
			CRCValue = update_CRC( CRCValue, Values[i]&0x00FF );
			CRCValue = update_CRC( CRCValue,(Values[i]&0xFF00) >> 8 );
		}

		CreateModbusByte((CRCValue&0x00FF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte(((CRCValue&0xFF00) >> 8));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	}
	else
	{
		//it's easier to compute the LRC before converting the data from binary to ASCII
		U16 LRCvalue = 0x00;   //Modbus/ASCII uses LRC, initialization to 0x00;

		LRCvalue = LRCvalue + DeviceID;
		LRCvalue = LRCvalue + FuncCode;
		LRCvalue = LRCvalue + ByteCount;

		for(int i=0;i<ByteCount/2;i++)
		{
			LRCvalue = LRCvalue + Values[i]&0x00FF;
			LRCvalue = LRCvalue + ((Values[i]&0xFF00) >> 8);
		}

		LRCvalue =  ~LRCvalue +1 ;
		LRCvalue = LRCvalue &0x00FF;

		CreateModbusByte( ':' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID&0xF) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(FuncCode>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(FuncCode&0xF) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((ByteCount&0xF0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII(ByteCount&0xF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		
		for(int i=0;i<ByteCount/2;i++)
		{
			CreateModbusByte( BinToASCII((Values[i]&0xF000)>>12) );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII((Values[i]&0x0F00)>>8) );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII((Values[i]&0xF0)>>4) );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII(Values[i]&0xF));
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		}

		CreateModbusByte( BinToASCII((LRCvalue&0xF0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((LRCvalue&0xF)) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( '\r' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( '\n' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
	}
}

void ModbusSimulationDataGenerator::SendReadExceptionStatusResponse(U8 DeviceID, U8 FuncCode, U8 Data )
{
	if(mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusRTUSlave)
	{
		
		CreateModbusByte( DeviceID );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	
		CreateModbusByte( FuncCode );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte( Data );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		
		U16 CRCValue = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
		CRCValue = update_CRC( CRCValue, DeviceID );
		CRCValue = update_CRC( CRCValue, FuncCode );
		CRCValue = update_CRC( CRCValue, Data );

		CreateModbusByte((CRCValue&0x00FF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte(((CRCValue&0xFF00) >> 8));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	}
	else
	{
		//it's easier to compute the LRC before converting the data from binary to ASCII
		U16 LRCvalue = 0x00;   //Modbus/ASCII uses LRC, initialization to 0x00;

		LRCvalue = LRCvalue + DeviceID;
		LRCvalue = LRCvalue + FuncCode;
		LRCvalue = LRCvalue + Data;

		LRCvalue =  ~LRCvalue +1 ;
		LRCvalue = LRCvalue &0x00FF;

		CreateModbusByte( ':' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID&0xF) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(FuncCode>>4));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(FuncCode&0xF) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(Data>>4));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(Data&0xF) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((LRCvalue&0xF0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((LRCvalue&0xF)) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( '\r' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( '\n' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );


	}
}

void ModbusSimulationDataGenerator::SendReadFIFOQueueResponse(U8 DeviceID, U16 ByteCount, U16 FIFOCount, U16 Values[])
{
	if(mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusRTUSlave)
	{
		
		CreateModbusByte( DeviceID );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	
		CreateModbusByte( 0x18 );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
			
		CreateModbusByte( ByteCount&0x00FF );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte( (ByteCount&0xFF00) >> 8 );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte( FIFOCount&0x00FF );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte( (FIFOCount&0xFF00) >> 8 );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		
		for(int i=0;i<FIFOCount;i++)
		{
			CreateModbusByte( Values[i]&0x00FF );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

			CreateModbusByte( (Values[i]&0xFF00) >> 8 );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		}
		
		U16 CRCValue = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
		CRCValue = update_CRC( CRCValue, DeviceID );
		CRCValue = update_CRC( CRCValue, 0x18);
		CRCValue = update_CRC( CRCValue, ByteCount&0x00FF );
		CRCValue = update_CRC( CRCValue, (ByteCount&0xFF00) >> 8 );
		CRCValue = update_CRC( CRCValue, FIFOCount&0x00FF );
		CRCValue = update_CRC( CRCValue, (FIFOCount&0xFF00) >> 8 );

		for(int i=0;i<FIFOCount;i++)
		{
			CRCValue = update_CRC( CRCValue, Values[i]&0x00FF );
			CRCValue = update_CRC( CRCValue,(Values[i]&0xFF00) >> 8 );
		}

		CreateModbusByte((CRCValue&0x00FF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte(((CRCValue&0xFF00) >> 8));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	}
	else
	{
		//it's easier to compute the LRC before converting the data from binary to ASCII
		U16 LRCvalue = 0x00;   //Modbus/ASCII uses LRC, initialization to 0x00;

		LRCvalue = LRCvalue + DeviceID;
		LRCvalue = LRCvalue + 0x18;
		LRCvalue = LRCvalue + ByteCount&0x00FF;
		LRCvalue = LRCvalue + ((ByteCount&0xFF00) >> 8);
		LRCvalue = LRCvalue + FIFOCount&0x00FF;
		LRCvalue = LRCvalue + ((FIFOCount&0xFF00) >> 8);

		for(int i=0;i<FIFOCount;i++)
		{
			LRCvalue = LRCvalue + Values[i]&0x00FF;
			LRCvalue = LRCvalue + ((Values[i]&0xFF00) >> 8);
		}

		LRCvalue =  ~LRCvalue +1 ;
		LRCvalue = LRCvalue &0x00FF;

		CreateModbusByte( ':' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID&0xF) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(0x1) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(0x8) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((ByteCount&0xF000)>>12) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		
		CreateModbusByte( BinToASCII((ByteCount&0x0F00)>>8) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((ByteCount&0xF0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(ByteCount&0xF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((FIFOCount&0xF000)>>12) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		
		CreateModbusByte( BinToASCII((FIFOCount&0x0F00)>>8) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		
		CreateModbusByte( BinToASCII((FIFOCount&0xF0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		
		CreateModbusByte( BinToASCII(FIFOCount&0xF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		
		for(int i=0;i<FIFOCount;i++)
		{
			CreateModbusByte( BinToASCII((Values[i]&0xF000)>>12) );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII((Values[i]&0x0F00)>>8) );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII((Values[i]&0xF0)>>4) );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII(Values[i]&0xF));
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		}

		CreateModbusByte( BinToASCII((LRCvalue&0xF0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((LRCvalue&0xF)) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( '\r' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( '\n' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
	}
}

void ModbusSimulationDataGenerator::SendGetCommEventLogResponse(U8 DeviceID, U8 ByteCount, U16 Status, U16 EventCount, U16 MessageCount, U8 Events[])
{
	if(mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusRTUSlave)
	{
		
		CreateModbusByte( DeviceID );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	
		CreateModbusByte( 0x0C );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
			
		CreateModbusByte( ByteCount );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte( Status&0x00FF );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte( (Status&0xFF00) >> 8 );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte( EventCount&0x00FF );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte( (EventCount&0xFF00) >> 8 );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte( MessageCount&0x00FF );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte( (MessageCount&0xFF00) >> 8 );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		
		for(int i=0;i<(ByteCount-6);i++)
		{
			CreateModbusByte( Events[i] );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		}
		
		U16 CRCValue = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
		CRCValue = update_CRC( CRCValue, DeviceID );
		CRCValue = update_CRC( CRCValue, 0x0C);
		CRCValue = update_CRC( CRCValue, ByteCount);
		CRCValue = update_CRC( CRCValue, Status&0x00FF );
		CRCValue = update_CRC( CRCValue, (Status&0xFF00) >> 8 );
		CRCValue = update_CRC( CRCValue, EventCount&0x00FF );
		CRCValue = update_CRC( CRCValue, (EventCount&0xFF00) >> 8 );
		CRCValue = update_CRC( CRCValue, MessageCount&0x00FF );
		CRCValue = update_CRC( CRCValue, (MessageCount&0xFF00) >> 8 );

		for(int i=0;i<(ByteCount-6);i++)
		{
			CRCValue = update_CRC( CRCValue, Events[i]);
		}

		CreateModbusByte((CRCValue&0x00FF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte(((CRCValue&0xFF00) >> 8));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	}
	else
	{
		//it's easier to compute the LRC before converting the data from binary to ASCII
		U16 LRCvalue = 0x00;   //Modbus/ASCII uses LRC, initialization to 0x00;

		LRCvalue = LRCvalue + DeviceID;
		LRCvalue = LRCvalue + 0x0C;
		LRCvalue = LRCvalue + ByteCount;
		LRCvalue = LRCvalue + Status&0x00FF;
		LRCvalue = LRCvalue + ((Status&0xFF00) >> 8);
		LRCvalue = LRCvalue + EventCount&0x00FF;
		LRCvalue = LRCvalue + ((EventCount&0xFF00) >> 8);
		LRCvalue = LRCvalue + MessageCount&0x00FF;
		LRCvalue = LRCvalue + ((MessageCount&0xFF00) >> 8);


		for(int i=0;i<(ByteCount-6);i++)
		{
			LRCvalue = LRCvalue + Events[i]&0x00FF;
			LRCvalue = LRCvalue + ((Events[i]&0xFF00) >> 8);
		}

		LRCvalue =  ~LRCvalue +1 ;
		LRCvalue = LRCvalue &0x00FF;

		CreateModbusByte( ':' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID&0xF) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(0x0) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(0xC) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((ByteCount&0xF0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(ByteCount&0xF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((Status&0xF000)>>12) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		
		CreateModbusByte( BinToASCII((Status&0x0F00)>>8) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		
		CreateModbusByte( BinToASCII((Status&0xF0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		
		CreateModbusByte( BinToASCII(Status&0xF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((EventCount&0xF000)>>12) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		
		CreateModbusByte( BinToASCII((EventCount&0x0F00)>>8) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		
		CreateModbusByte( BinToASCII((EventCount&0xF0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		
		CreateModbusByte( BinToASCII(EventCount&0xF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((MessageCount&0xF000)>>12) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		
		CreateModbusByte( BinToASCII((MessageCount&0x0F00)>>8) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		
		CreateModbusByte( BinToASCII((MessageCount&0xF0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		
		CreateModbusByte( BinToASCII(MessageCount&0xF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		
		for(int i=0;i<(ByteCount-6);i++)
		{
			CreateModbusByte( BinToASCII((Events[i]&0xF0)>>4) );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII(Events[i]&0xF));
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		}

		CreateModbusByte( BinToASCII((LRCvalue&0xF0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((LRCvalue&0xF)) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( '\r' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( '\n' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
	}
}

void ModbusSimulationDataGenerator::SendReportSlaveIDResponse(U8 DeviceID, U8 ByteCount, U8 Data[])
{
	if(mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusRTUSlave)
	{
		
		CreateModbusByte( DeviceID );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	
		CreateModbusByte( 0x11 );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
			
		CreateModbusByte( ByteCount );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		
		for(int i=0;i<ByteCount;i++)
		{
			CreateModbusByte( Data[i] );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		}
		
		U16 CRCValue = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
		CRCValue = update_CRC( CRCValue, DeviceID );
		CRCValue = update_CRC( CRCValue, 0x11);
		CRCValue = update_CRC( CRCValue, ByteCount);

		for(int i=0;i<ByteCount;i++)
		{
			CRCValue = update_CRC( CRCValue, Data[i]);
		}

		CreateModbusByte((CRCValue&0x00FF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte(((CRCValue&0xFF00) >> 8));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	}
	else
	{
		//it's easier to compute the LRC before converting the data from binary to ASCII
		U16 LRCvalue = 0x00;   //Modbus/ASCII uses LRC, initialization to 0x00;

		LRCvalue = LRCvalue + DeviceID;
		LRCvalue = LRCvalue + 0x11;
		LRCvalue = LRCvalue + ByteCount;

		for(int i=0;i<ByteCount;i++)
		{
			LRCvalue = LRCvalue + Data[i]&0x00FF;
			LRCvalue = LRCvalue + ((Data[i]&0xFF00) >> 8);
		}

		LRCvalue =  ~LRCvalue +1 ;
		LRCvalue = LRCvalue &0x00FF;

		CreateModbusByte( ':' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID&0xF) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(0x1) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(0x1) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((ByteCount&0xF0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(ByteCount&0xF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		
		for(int i=0;i<ByteCount;i++)
		{
			CreateModbusByte( BinToASCII((Data[i]&0xF0)>>4) );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII(Data[i]&0xF));
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		}

		CreateModbusByte( BinToASCII((LRCvalue&0xF0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((LRCvalue&0xF)) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( '\r' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( '\n' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
	}
}

void ModbusSimulationDataGenerator::SendWriteFileRecordResponse(U8 DeviceID, U8 ByteCount, U8 SubReqRecordLengths[], U8 SubReqReferenceTypes[],  U16 SubReqRecordData[2][2] )
{
	if(mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusRTUMaster || mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusRTUSlave)
	{
		
		CreateModbusByte( DeviceID );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	
		CreateModbusByte( 0x14 );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte( ByteCount );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		
		U16 totalbytes=0;
		int i = 0;
		while(totalbytes<ByteCount)
		{
			CreateModbusByte( SubReqRecordLengths[i] );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
			
			CreateModbusByte( SubReqReferenceTypes[i] );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

			for(int j=0;j<(SubReqRecordLengths[i]-1)/2;j++)
			{
				CreateModbusByte( SubReqRecordData[i][j]&0x00FF );
				mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		
				CreateModbusByte( (SubReqRecordData[i][j]&0xFF00) >> 8 );
				mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

			}

			totalbytes = totalbytes + SubReqRecordLengths[i] +2;
			i++;
		}
		
		U16 CRCValue = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
		CRCValue = update_CRC( CRCValue, DeviceID );
		CRCValue = update_CRC( CRCValue, 0x14 );
		CRCValue = update_CRC( CRCValue, ByteCount );

		totalbytes=0;
		i = 0;
		while(totalbytes<ByteCount)
		{
			CRCValue = update_CRC( CRCValue, SubReqRecordLengths[i]);
			CRCValue = update_CRC( CRCValue, SubReqReferenceTypes[i] );


			for(int j=0;j<(SubReqRecordLengths[i]-1)/2;j++)
			{
				CRCValue = update_CRC( CRCValue, SubReqRecordData[i][j]&0x00FF );
				CRCValue = update_CRC( CRCValue, (SubReqRecordData[i][j]&0xFF00) >> 8 );
			}

			totalbytes = totalbytes + SubReqRecordLengths[i] + 2;
			i++;
		}

		CreateModbusByte((CRCValue&0x00FF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte(((CRCValue&0xFF00) >> 8));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	}
	else
	{
		//it's easier to compute the LRC before converting the data from binary to ASCII
		U16 LRCvalue = 0x00;   //Modbus/ASCII uses LRC, initialization to 0x00;

		LRCvalue = LRCvalue + DeviceID;
		LRCvalue = LRCvalue + 0x14;
		LRCvalue = LRCvalue + ByteCount;

		U16 totalbytes=0;
		int i = 0;
		while(totalbytes<ByteCount)
		{
			LRCvalue = LRCvalue + SubReqRecordLengths[i];
			LRCvalue = LRCvalue + SubReqReferenceTypes[i];
			
			for(int j=0;j<(SubReqRecordLengths[i]-1)/2;j++)
			{
				LRCvalue = LRCvalue + SubReqRecordData[i][j]&0x00FF;
				LRCvalue = LRCvalue +((SubReqRecordData[i][j]&0xFF00) >> 8 );
			}

			totalbytes = totalbytes + SubReqRecordLengths[i] + 2;
			i++;
		}

		LRCvalue =  ~LRCvalue +1 ;
		LRCvalue = LRCvalue &0x00FF;

		CreateModbusByte( ':' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID&0xF) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( '1' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( '4' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((ByteCount&0xF0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII(ByteCount&0xF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		
		totalbytes=0;
		i = 0;
		while(totalbytes<ByteCount)
		{
			CreateModbusByte( BinToASCII((SubReqRecordLengths[i]&0xF0)>>4) );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII(SubReqRecordLengths[i]&0xF));
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

			CreateModbusByte( BinToASCII((SubReqReferenceTypes[i]&0xF0)>>4) );
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			CreateModbusByte( BinToASCII(SubReqReferenceTypes[i]&0xF));
			mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

			for(int j=0;j<(SubReqRecordLengths[i]-1)/2;j++)
			{
				CreateModbusByte( BinToASCII((SubReqRecordData[i][j]&0xF000)>>12) );
				mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
				CreateModbusByte( BinToASCII((SubReqRecordData[i][j]&0x0F00)>>8));
				mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
				CreateModbusByte( BinToASCII((SubReqRecordData[i][j]&0xF0)>>4) );
				mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
				CreateModbusByte( BinToASCII(SubReqRecordData[i][j]&0xF));
				mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
			}

			totalbytes = totalbytes + SubReqRecordLengths[i] + 2;
			i++;

		}

		CreateModbusByte( BinToASCII((LRCvalue&0xF0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((LRCvalue&0xF)) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( '\r' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( '\n' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
	}
}

void ModbusSimulationDataGenerator::SendException(U8 DeviceID, U8 FuncCode, U8 ExceptionCode )
{
	if(mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusRTUMaster || mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusRTUSlave)
	{
		
		CreateModbusByte( DeviceID );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	
		CreateModbusByte( FuncCode+0x80 );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
			
		CreateModbusByte( ExceptionCode);
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
		
		U16 CRCValue = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
		CRCValue = update_CRC( CRCValue, DeviceID );
		CRCValue = update_CRC( CRCValue, (FuncCode+0x80) );
		CRCValue = update_CRC( CRCValue, ExceptionCode);

		CreateModbusByte((CRCValue&0x00FF));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle

		CreateModbusByte(((CRCValue&0xFF00) >> 8));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );  //insert 10 bit-periods of idle
	}
	else
	{
		//it's easier to compute the LRC before converting the data from binary to ASCII
		U16 LRCvalue = 0x00;   //Modbus/ASCII uses LRC, initialization to 0x00;

		LRCvalue = LRCvalue + DeviceID;
		LRCvalue = LRCvalue + (FuncCode+0x80);
		LRCvalue = LRCvalue + (ExceptionCode);

		LRCvalue =  ~LRCvalue +1 ;
		LRCvalue = LRCvalue &0x00FF;

		CreateModbusByte( ':' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(DeviceID&0xF) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((FuncCode+0x80)>>4));
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII(FuncCode&0xF) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((ExceptionCode&0x00F0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII(ExceptionCode&0xF) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( BinToASCII((LRCvalue&0xF0)>>4) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( BinToASCII((LRCvalue&0xF)) );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

		CreateModbusByte( '\r' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );
		CreateModbusByte( '\n' );
		mModbusSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );


	}
}

U16 ModbusSimulationDataGenerator::update_CRC( U16 crc, U8 c )
{

    U16 tmp, short_c;

    short_c = 0x00ff & (U16) c;

    //if ( ! crc_tab16_init ) init_crc16_tab();

    tmp =  crc       ^ short_c;
    crc = (crc >> 8) ^ crc_tab16[ tmp & 0xff ];

    return crc;

} 

void ModbusSimulationDataGenerator::init_crc16_tab( void ) 
{

    int i, j;
    U16 crc, c;

    for (i=0; i<256; i++) {

        crc = 0;
        c   = (U16) i;

        for (j=0; j<8; j++) {

            if ( (crc ^ c) & 0x0001 ) crc = ( crc >> 1 ) ^ 0xA001;
            else                      crc =   crc >> 1;

            c = c >> 1;
        }

        crc_tab16[i] = crc;
    }

} 

char ModbusSimulationDataGenerator::BinToASCII( U8 value )
{
	switch(value)
	{
	case 0:
		return '0';
	case 1:
		return '1';
	case 2:
		return '2';
	case 3:
		return '3';
	case 4:
		return '4';
	case 5:
		return '5';
	case 6:
		return '6';
	case 7:
		return '7';
	case 8:
		return '8';
	case 9:
		return '9';
	case 0xA:
		return 'A';
	case 0xB:
		return 'B';
	case 0xC:
		return 'C';
	case 0xD:
		return 'D';
	case 0xE:
		return 'E';
	case 0xF:
		return 'F';
	default:
		return '0';
	}
}

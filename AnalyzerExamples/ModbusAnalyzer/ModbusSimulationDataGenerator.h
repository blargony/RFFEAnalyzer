#ifndef MODBUS_SIMULATION_DATA_GENERATOR
#define MODBUS_SIMULATION_DATA_GENERATOR

#include <AnalyzerHelpers.h>

#include <stdio.h>
#include <string.h>

class ModbusAnalyzerSettings;

class ModbusSimulationDataGenerator
{
public:
	ModbusSimulationDataGenerator();
	~ModbusSimulationDataGenerator();

	void Initialize( U32 simulation_sample_rate, ModbusAnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );

protected:
	ModbusAnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;
	BitState mBitLow;
	BitState mBitHigh;
	U64 mValue;

	//U64 mMpModeAddressMask;
	//U64 mMpModeDataMask;
	U64 mNumBitsMask;

protected: //Modbus specific

	void CreateModbusByte( U64 value );
	ClockGenerator mClockGenerator;
	SimulationChannelDescriptor mModbusSimulationData;  //if we had more than one channel to simulate, they would need to be in an array
	

	/////////// BEGIN MODBUS SECTION //////////////////

	//used for doing CRC calculations
	U16   crc_tab16[256];
	void init_crc16_tab( void );
	U16 update_CRC( U16 crc, U8 c );
	
	//my own itoa function
	char BinToASCII( U8 value );

	//this function generates requests for function codes 0x01 - 0x06 for both RTU & ASCII
	// 0x01 Read Coils
	// 0x02 Read Descrete Inputs
	// 0x03 Read Holding Registers
	// 0x04 Read Input Registers
	// 0x05 Write Single Coil
	// 0x06 Write Single Register
	void SendGenericRequest(U8 DeviceID, U8 FuncCode, U16 StartingAddress, U16 Quantity );

	//this function generates request for some of the similar functions of the Diagnostics space
	// 0x07 Read Exception Status
	// 0x0B Get Comm Event Counter
	// 0x0C Get Comm Event Log
	// 0x11 Report Slave ID
	void SendGenDiagnosticRequest(U8 DeviceID, U8 FuncCode);

	//this function generates the request for the Mask Write Register (0x16) command
	void SendMaskWriteRegisterRequest(U8 DeviceID, U16 ReferenceAddress, U16 And_Mask, U16 Or_Mask );

	//this function generates the request for the Read FIFO Queue (0x18) command
	void SendReadFIFOQueueRequest(U8 DeviceID, U16 FIFOAddress);

	//this function generates the request for Diagnostics (0x08) command
	void SendRequest_Diagnostics(U8 DeviceID, U16 SubFunction, U16 Data );

	//this function generates the request for write multiple coils (0x0F) command
	void SendWriteMultipleCoilsRequest(U8 DeviceID, U16 StartingAddress, U16 Quantity, U8 ByteCount, U8 Values[] );

	//this function generates the request for write multiple registers (0x10) command
	void SendWriteMultipleRegistersRequest(U8 DeviceID, U16 StartingAddress, U16 Quantity, U8 ByteCount, U16 Values[] );

	//this function generates the request for the Read File Record (0x14) command
	void SendReadFileRecordRequest(U8 DeviceID, U8 ByteCount, U8 SubReqReferenceTypes[], U16 SubReqFileNumbers[], U16 SubReqRecordNumbers[], U16 SubReqRecordLengths[] );

	//this function generates the request for the Write File Record (0x15) command (note the forced size of the 2D array, no reason to implement points for this, overkill for simulation functions
	void SendWriteFileRecordRequest(U8 DeviceID, U8 ByteCount, U8 SubReqReferenceTypes[], U16 SubReqFileNumbers[], U16 SubReqRecordNumbers[], U16 SubReqRecordLengths[], U16 SubReqRecordData[1][3] );
	
	//this function generates the request for the Read/Write Multiple Registers (0x17) command
	void SendReadWriteMultipleRegisters(U8 DeviceID, U16 ReadStartingAddress, U16 QuantityToRead, U16 WriteStartingAddress, U16 QuantityToWrite, U8 WriteByteCount, U16 Values[] );

	//RESPONSES
	void SendGenericResponse(U8 DeviceID, U8 FuncCode, U8 ByteCount, U8 Status[]);
	void SendGeneric2Response(U8 DeviceID, U8 FuncCode, U8 ByteCount, U16 Values[]);
	void SendReadExceptionStatusResponse(U8 DeviceID, U8 FuncCode, U8 Data );
	void SendReadFIFOQueueResponse(U8 DeviceID, U16 ByteCount, U16 FIFOCount, U16 Values[]);
	void SendGetCommEventLogResponse(U8 DeviceID, U8 ByteCount, U16 Status, U16 EventCount, U16 MessageCount, U8 Events[]);
	void SendReportSlaveIDResponse(U8 DeviceID, U8 ByteCount, U8 Data[]);
	//forced a constant value for the array size, no reason to implement dynamic memory for this, overkill for purpose of simulation
	void SendWriteFileRecordResponse(U8 DeviceID, U8 ByteCount, U8 SubReqRecordLengths[], U8 SubReqReferenceTypes[],  U16 SubReqRecordData[2][2] );

	//Error/NAK 
	void SendException(U8 DeviceID, U8 FuncCode, U8 ExceptionCode );

};
#endif //UNIO_SIMULATION_DATA_GENERATOR

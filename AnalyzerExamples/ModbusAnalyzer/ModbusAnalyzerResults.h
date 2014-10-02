#ifndef MODBUS_ANALYZER_RESULTS
#define MODBUS_ANALYZER_RESULTS

#include <AnalyzerResults.h>

#include <stdio.h>
#include <string.h>

#define FRAMING_ERROR_FLAG ( 1 << 0 )
#define PARITY_ERROR_FLAG ( 1 << 1 )
#define MP_MODE_ADDRESS_FLAG ( 1 << 2 )

class ModbusAnalyzer;
class ModbusAnalyzerSettings;

class ModbusAnalyzerResults : public AnalyzerResults
{
public:
	ModbusAnalyzerResults( ModbusAnalyzer* analyzer, ModbusAnalyzerSettings* settings );
	virtual ~ModbusAnalyzerResults();

	virtual void GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base );
	virtual void GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id );

	virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base );
	virtual void GeneratePacketTabularText( U64 packet_id, DisplayBase display_base );
	virtual void GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base );

protected: //functions

protected:  //vars
	ModbusAnalyzerSettings* mSettings;
	ModbusAnalyzer* mAnalyzer;
};

#endif //MODBUS_ANALYZER_RESULTS

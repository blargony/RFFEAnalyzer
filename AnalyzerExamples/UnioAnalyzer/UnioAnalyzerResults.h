#ifndef UNIO_ANALYZER_RESULTS
#define UNIO_ANALYZER_RESULTS

#include <AnalyzerResults.h>


enum UnioFrameType { HeaderFrame, AddressFrame8, AddressFrame12, DataFrame, InvalidBit, ErrorMakRequired, ErrorNoSakRequired };
#define SLAVE_ACK ( 1 << 0 )
#define MASTER_ACK ( 1 << 1 )

class UnioAnalyzer;
class UnioAnalyzerSettings;

class UnioAnalyzerResults : public AnalyzerResults
{
public:
	UnioAnalyzerResults( UnioAnalyzer* analyzer, UnioAnalyzerSettings* settings );
	virtual ~UnioAnalyzerResults();

	virtual void GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base );
	virtual void GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id );

	virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base );
	virtual void GeneratePacketTabularText( U64 packet_id, DisplayBase display_base );
	virtual void GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base );

protected: //functions

protected:  //vars
	UnioAnalyzerSettings* mSettings;
	UnioAnalyzer* mAnalyzer;
};

#endif //UNIO_ANALYZER_RESULTS

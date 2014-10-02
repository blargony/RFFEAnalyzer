#ifndef LIN_ANALYZER_RESULTS
#define LIN_ANALYZER_RESULTS

#include <AnalyzerResults.h>

class LINAnalyzer;
class LINAnalyzerSettings;

class LINAnalyzerResults : public AnalyzerResults
{
public:

	typedef enum
	{
		NoFrame=0,				// no frame recognized.
		// LIN Header
		headerBreak,			// expecting break.
		headerSync,				// expecting sync.
		headerPID,				// expecting PID.
		// LIN Response
		responseDataZero,		// expecting first data byte.
		responseData,			// expecting response data.
		responseChecksum,		// expecting checksum.
	} tLINFrameState;

	typedef enum
	{
		Okay=0x00,
		byteFramingError=0x01,
		headerBreakExpected=0x02,
		headerSyncExpected=0x04,
		checksumMismatch=0x08,
	} tLINFrameFlags;

	LINAnalyzerResults( LINAnalyzer* analyzer, LINAnalyzerSettings* settings );
	virtual ~LINAnalyzerResults();

	virtual void GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base );
	virtual void GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id );

	virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base );
	virtual void GeneratePacketTabularText( U64 packet_id, DisplayBase display_base );
	virtual void GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base );

protected: //functions

protected:  //vars
	LINAnalyzerSettings* mSettings;
	LINAnalyzer* mAnalyzer;
};

#endif //LIN_ANALYZER_RESULTS

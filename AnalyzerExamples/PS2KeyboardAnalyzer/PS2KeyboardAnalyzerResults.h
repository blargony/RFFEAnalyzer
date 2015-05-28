#ifndef PS2KEYBOARD_ANALYZER_RESULTS
#define PS2KEYBOARD_ANALYZER_RESULTS

#include <AnalyzerResults.h>
#include "PS2KeyboardAnalyzerScanCodes.h"

class PS2KeyboardAnalyzer;
class PS2KeyboardAnalyzerSettings;

class PS2KeyboardAnalyzerResults : public AnalyzerResults
{
public:
	PS2KeyboardAnalyzerResults( PS2KeyboardAnalyzer* analyzer, PS2KeyboardAnalyzerSettings* settings );
	virtual ~PS2KeyboardAnalyzerResults();

	virtual void GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base );
	virtual void GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id );

	virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base );
	virtual void GeneratePacketTabularText( U64 packet_id, DisplayBase display_base );
	virtual void GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base );

protected: //functions
	void GetKeyName(char returnstr[], U64 code, int keyset);
	void GetCommandName(char returnstr[], U64 code, double DeviceType);
	void GetKey(char returnstr[], U64 code, int keyset);
protected:  //vars
	PS2KeyboardAnalyzerSettings* mSettings;
	PS2KeyboardAnalyzer* mAnalyzer;
};


#endif //PS2KEYBOARD_ANALYZER_RESULTS

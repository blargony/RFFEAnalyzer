#ifndef USB_ANALYZER_RESULTS_H
#define USB_ANALYZER_RESULTS_H

#include <AnalyzerResults.h>

#include "USBTypes.h"

class USBAnalyzer;
class USBAnalyzerSettings;

class USBAnalyzerResults: public AnalyzerResults
{
public:
	USBAnalyzerResults(USBAnalyzer* analyzer, USBAnalyzerSettings* settings);
	virtual ~USBAnalyzerResults();

	virtual void GenerateBubbleText(U64 frame_index, Channel& channel, DisplayBase display_base);
	virtual void GenerateExportFile(const char* file, DisplayBase display_base, U32 export_type_user_id);

	void GenerateExportFilePackets(const char* file, DisplayBase display_base);
	void GenerateExportFileBytes(const char* file, DisplayBase display_base);
	void GenerateExportFileSignals(const char* file, DisplayBase display_base);

	virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base);
	virtual void GeneratePacketTabularText(U64 packet_id, DisplayBase display_base);
	virtual void GenerateTransactionTabularText(U64 transaction_id, DisplayBase display_base);

protected:	// functions

protected:	// vars

	USBAnalyzerSettings*	mSettings;
	USBAnalyzer*			mAnalyzer;
};

#endif	// USB_ANALYZER_RESULTS_H

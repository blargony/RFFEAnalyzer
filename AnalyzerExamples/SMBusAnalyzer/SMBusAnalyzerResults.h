#ifndef SMBUS_ANALYZER_RESULTS_H
#define SMBUS_ANALYZER_RESULTS_H

#include <AnalyzerResults.h>

#include "SMBusTypes.h"

class SMBusAnalyzer;
class SMBusAnalyzerSettings;

class SMBusAnalyzerResults: public AnalyzerResults
{
public:
	SMBusAnalyzerResults(SMBusAnalyzer* analyzer, SMBusAnalyzerSettings* settings);
	virtual ~SMBusAnalyzerResults();

	virtual void GenerateBubbleText(U64 frame_index, Channel& channel, DisplayBase display_base);
	virtual void GenerateExportFile(const char* file, DisplayBase display_base, U32 export_type_user_id);

	virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base);
	virtual void GeneratePacketTabularText(U64 packet_id, DisplayBase display_base);
	virtual void GenerateTransactionTabularText(U64 transaction_id, DisplayBase display_base);

protected:	// functions

protected:	// vars

	SMBusAnalyzerSettings*	mSettings;
	SMBusAnalyzer*			mAnalyzer;

	void AddResultStringsFromVector(const std::string& prefix, const std::vector<std::string>& v, const std::string& postfix, std::vector<std::string>& results);
	void GetBubbleText(const Frame& f, const bool isClock, DisplayBase display_base, std::vector<std::string>& results);
};

#endif	// SMBUS_ANALYZER_RESULTS_H
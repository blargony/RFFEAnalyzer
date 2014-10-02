#ifndef ATMEL_SWI_ANALYZER_RESULTS_H
#define ATMEL_SWI_ANALYZER_RESULTS_H

#include <AnalyzerResults.h>

#include "AtmelSWITypes.h"

class AtmelSWIAnalyzer;
class AtmelSWIAnalyzerSettings;

class AtmelSWIAnalyzerResults: public AnalyzerResults
{
public:
	AtmelSWIAnalyzerResults(AtmelSWIAnalyzer* analyzer, AtmelSWIAnalyzerSettings* settings);
	virtual ~AtmelSWIAnalyzerResults();

	virtual void GenerateBubbleText(U64 frame_index, Channel& channel, DisplayBase display_base);
	virtual void GenerateExportFile(const char* file, DisplayBase display_base, U32 export_type_user_id);

	virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base);
	virtual void GeneratePacketTabularText(U64 packet_id, DisplayBase display_base);
	virtual void GenerateTransactionTabularText(U64 transaction_id, DisplayBase display_base);

	size_t AddBlock(const SWI_Block& block)
	{
		mBlocks.push_back(block);
		return mBlocks.size() - 1;
	}

	void GetTextsForPacketSegmentFrame(const Frame& f, DisplayBase display_base, std::vector<std::string>& texts);
	static void GetTextsForChecksumFrame(const Frame& f, DisplayBase display_base, std::vector<std::string>& texts);

protected:	// functions

protected:	// vars

	AtmelSWIAnalyzerSettings*	mSettings;
	AtmelSWIAnalyzer*			mAnalyzer;

	std::vector<SWI_Block>		mBlocks;
};

#endif	// ATMEL_SWI_ANALYZER_RESULTS_H

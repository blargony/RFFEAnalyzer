#ifndef MDIO_ANALYZER_RESULTS
#define MDIO_ANALYZER_RESULTS

#include <AnalyzerResults.h>

class MDIOAnalyzer;
class MDIOAnalyzerSettings;

enum MDIOFrameType { MDIO_C22_START = 0, MDIO_C45_START, 
					MDIO_OP_W, MDIO_OP_R, MDIO_C45_OP_ADDR, MDIO_C45_OP_READ_INC_ADDR,
					MDIO_PHYADDR, 
					MDIO_C22_REGADDR, 
					MDIO_C45_DEVTYPE_RESERVED, MDIO_C45_DEVTYPE_PMD_PMA, MDIO_C45_DEVTYPE_WIS, 
					MDIO_C45_DEVTYPE_PCS, MDIO_C45_DEVTYPE_PHY_XS, MDIO_C45_DEVTYPE_DTE_XS, MDIO_C45_DEVTYPE_OTHER,
                    MDIO_TA, 
					MDIO_C22_DATA, 
					MDIO_C45_ADDR,
					MDIO_C45_DATA,
					MDIO_UNKNOWN 
				   };

class MDIOAnalyzerResults : public AnalyzerResults
{
public:
	MDIOAnalyzerResults( MDIOAnalyzer* analyzer, MDIOAnalyzerSettings* settings );
	virtual ~MDIOAnalyzerResults();

	virtual void GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base );
	virtual void GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id );

	virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base );
	virtual void GeneratePacketTabularText( U64 packet_id, DisplayBase display_base );
	virtual void GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base );

protected: 
	void GenBubbleText(U64 frame_index, DisplayBase display_base, bool tabular); 

	// helper functions to generate text for each type of frame
	void GenStartString(const Frame & frame, const char* clause, bool tabular);
	void GenOpString(const Frame & frame, 
					 const char* opcode_str0, const char* opcode_str1, const char* opcode_str2, 
					 bool tabular); 
    void GenPhyAddrString(const Frame & frame, DisplayBase display_base, bool tabular); 
    void GenC22RegAddrString(const Frame & frame, DisplayBase display_base, bool tabular);
	void GenC45DevTypeString(const Frame & frame, DisplayBase display_base, 
							 const char* devtype, bool tabular);
	void GenC45DevType(const Frame & frame, DisplayBase display_base, bool tabular); 
    void GenTAString(const Frame & frame, DisplayBase display_base, bool tabular); 
    void GenC22DataString(const Frame & frame, DisplayBase display_base, bool tabular); 
    void GenC45AddrDataString(const Frame & frame, DisplayBase display_base, 
							  const char* str0, const char* str1, const char* str2,
							  bool tabular);
	void GenUnknownString(bool tabular);

protected:  
	MDIOAnalyzerSettings* mSettings;
	MDIOAnalyzer* mAnalyzer;
};

#endif //MDIO_ANALYZER_RESULTS

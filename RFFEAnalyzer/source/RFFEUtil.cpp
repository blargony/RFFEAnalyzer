#include "RFFEUtil.h"

RFFEAnalyzerResults::RffeTypeFieldType  RFFEUtil::decodeRFFECmdFrame(U8 cmd)
{
    if ( cmd  < 0x10 )
    {
        return RFFEAnalyzerResults::RffeTypeExtWrite;
    }
    else if ( (cmd >= 0x10) && (cmd < 0x20) )
    {
        return RFFEAnalyzerResults::RffeTypeReserved;
    }
    else if ( (cmd >= 0x20) && (cmd < 0x30) )
    {
        return RFFEAnalyzerResults::RffeTypeExtRead;
    }
    else if ( (cmd >= 0x30) && (cmd < 0x38) )
    {
        return RFFEAnalyzerResults::RffeTypeExtLongWrite;
    }
    else if ( (cmd >= 0x38) && (cmd < 0x40) )
    {
        return RFFEAnalyzerResults::RffeTypeExtLongRead;
    }
    else if ( (cmd >= 0x40) && (cmd < 0x60) )
    {
        return RFFEAnalyzerResults::RffeTypeNormalWrite;
    }
    else if ( (cmd >= 0x60) && (cmd < 0x80) )
    {
        return RFFEAnalyzerResults::RffeTypeNormalRead;
    }
    else
    {
        return RFFEAnalyzerResults::RffeTypeShortWrite;
    }
}

U8 RFFEUtil::byteCount(U8 cmd)
{
    if ( cmd < 0x30 )
    {
        return ( cmd & 0x0F );
    }
    else
    {
        return ( cmd & 0x07 );
    }
}

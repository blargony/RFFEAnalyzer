#ifndef RFFE_UTIL
#define RFFE_UTIL

#include <LogicPublicTypes.h>
#include <AnalyzerTypes.h>
#include "RFFEAnalyzerResults.h"

class RFFEUtil
{
public:
    static RFFEAnalyzerResults::RffeTypeFieldType decodeRFFECmdFrame(U8 cmd);
    static U8 byteCount(U8 cmd);
};

#endif //RFFE_UTIL

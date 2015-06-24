#ifndef RFFE_UTIL
#define RFFE_UTIL

#include <LogicPublicTypes.h>
#include <AnalyzerTypes.h>
#include "RFFEAnalyzerResults.h"

class RFFEUtil {
public:
  static RFFEAnalyzerResults::RffeCommandFieldType decodeRFFECmdFrame(U8 cmd);
  static U8 byteCount(U8 cmd);
  static bool CalcParity(U64 val);
};

#endif // RFFE_UTIL

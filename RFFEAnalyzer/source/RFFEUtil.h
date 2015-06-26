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

// Parity Lookup Table
static const U8 ParityTable256[256] = {
#define P2(n) n, n ^ 1, n ^ 1, n
#define P4(n) P2(n), P2(n ^ 1), P2(n ^ 1), P2(n)
#define P6(n) P4(n), P4(n ^ 1), P4(n ^ 1), P4(n)
    P6(1), P6(0), P6(0), P6(1)};

#endif // RFFE_UTIL

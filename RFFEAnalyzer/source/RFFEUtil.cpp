#include "RFFEUtil.h"

RFFEAnalyzerResults::RffeCommandFieldType RFFEUtil::decodeRFFECmdFrame(U8 cmd) {
  if (cmd < 0x10) {
    return RFFEAnalyzerResults::RffeTypeExtWrite;
  } else if ((cmd >= 0x10) && (cmd < 0x1c)) {
    return RFFEAnalyzerResults::RffeTypeReserved;
  } else if (cmd == 0x1c) {
    return RFFEAnalyzerResults::RffeTypeMasterRead;
  } else if (cmd == 0x1d) {
    return RFFEAnalyzerResults::RffeTypeMasterWrite;
  } else if (cmd == 0x1e) {
    return RFFEAnalyzerResults::RffeTypeMasterHandoff;
  } else if (cmd == 0x1f) {
    return RFFEAnalyzerResults::RffeTypeInterrupt;
  } else if ((cmd >= 0x20) && (cmd < 0x30)) {
    return RFFEAnalyzerResults::RffeTypeExtRead;
  } else if ((cmd >= 0x30) && (cmd < 0x38)) {
    return RFFEAnalyzerResults::RffeTypeExtLongWrite;
  } else if ((cmd >= 0x38) && (cmd < 0x40)) {
    return RFFEAnalyzerResults::RffeTypeExtLongRead;
  } else if ((cmd >= 0x40) && (cmd < 0x60)) {
    return RFFEAnalyzerResults::RffeTypeNormalWrite;
  } else if ((cmd >= 0x60) && (cmd < 0x80)) {
    return RFFEAnalyzerResults::RffeTypeNormalRead;
  } else {
    return RFFEAnalyzerResults::RffeTypeWrite0;
  }
}

// Note:  This returns the number of bytes expected MINUS one
// (This is due to how command lengths are encoded in the RFFE Commands
U8 RFFEUtil::byteCount(U8 cmd) {
  if (cmd < 0x10) { // ExtWr
    return (cmd & 0x0f);
  } else if ((cmd >= 0x10) && (cmd < 0x1C)) { // Rsvd
    return 0;
  } else if ((cmd >= 0x1c) && (cmd < 0x1e)) { // Master Rd/Wr
    return 1;   // 2 bytes
  } else if (cmd == 0x1e) { // Master Handoff
    return 0;   // 1 byte
  } else if (cmd == 0x1f) { // Interrupt - non-byte sized length, handle as special case
    return 0;
  } else if ((cmd >= 0x20) && (cmd < 0x30)) { // ExtRd
    return (cmd & 0x0F);
  } else if ((cmd >= 0x30) && (cmd < 0x40)) { // Long Rd/Wr
    return (cmd & 0x07);
  } else if ((cmd >= 0x40) && (cmd < 0x80)) { // Rd/Wr
    return 0;
  } else { // Write0
    return 0;
  }
}

// Take a 32 bit unsigned so we can handle SA+Cmd
bool RFFEUtil::CalcParity(U64 val) {
  U8 c;
  U64 v;

  c = 1;
  v = val;

  // Add one to the bit count until we have zeroed
  // out all the bits in the passed val.  Initialized
  // to '1' for odd parity
  while (v) {
    c += 1;
    v &= (v - 1);
  }
  return (c & 0x1);
}

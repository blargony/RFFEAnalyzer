#include "RFFEAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "RFFEAnalyzer.h"
#include "RFFEAnalyzerSettings.h"
#include <iostream>
#include <stdio.h>


RFFEAnalyzerResults::RFFEAnalyzerResults(RFFEAnalyzer *analyzer, RFFEAnalyzerSettings *settings)
    : AnalyzerResults(), mSettings(settings), mAnalyzer(analyzer) {}

RFFEAnalyzerResults::~RFFEAnalyzerResults() {}

//
void RFFEAnalyzerResults::GenerateBubbleText(U64 frame_index, Channel &channel, DisplayBase display_base) {
  char number_str[8];
  char results_str[16];

  channel = channel;

  ClearResultStrings();
  Frame frame = GetFrame(frame_index);

  switch (frame.mType) {
    case RffeSSCField: {
      AddResultString("SSC");
    } break;

    case RffeSAField: {
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 4, number_str, 8);
      snprintf(results_str, sizeof(results_str), "SA:%s", number_str);
      AddResultString("SA");
      AddResultString(results_str);
    } break;

    case RffeCommandField: {
      AddResultString(RffeCommandFieldStringShort[frame.mData1]);
      AddResultString(RffeCommandFieldStringMid[frame.mData1]);
    } break;

    case RffeExByteCountField: {
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 4, number_str, 8);
      snprintf(results_str, sizeof(results_str), "BC:%s", number_str);
      AddResultString("BC");
      AddResultString(results_str);
    } break;

    case RffeExLongByteCountField: {
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 3, number_str, 8);
      snprintf(results_str, sizeof(results_str), "BC:%s", number_str);
      AddResultString("BC");
      AddResultString(results_str);
    } break;

    case RffeAddressField: {
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, 8);
      snprintf(results_str, sizeof(results_str), "A:%s", number_str);
      AddResultString("A");
      AddResultString(results_str);
    } break;

    case RffeAddressHiField: {
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, 8);
      snprintf(results_str, sizeof(results_str), "AH:%s", number_str);
      AddResultString("A");
      AddResultString(results_str);

    } break;

    case RffeAddressLoField: {
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, 8);
      snprintf(results_str, sizeof(results_str), "AL:%s", number_str);
      AddResultString("A");
      AddResultString(results_str);
    } break;

    case RffeShortDataField: {
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 7, number_str, 8);
      snprintf(results_str, sizeof(results_str), "D:%s", number_str);
      AddResultString("D");
      AddResultString(results_str);
    } break;

    case RffeDataField: {
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, 8);
      snprintf(results_str, sizeof(results_str), "D:%s", number_str);
      AddResultString("D");
      AddResultString(results_str);
    } break;

    case RffeMasterHandoffAckField: {
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, 8);
      snprintf(results_str, sizeof(results_str), "MHAck:%s", number_str);
      AddResultString("MHA");
      AddResultString(results_str);
    } break;

    case RffeISIField: {
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 2, number_str, 4);
      AddResultString("ISI");
      AddResultString("ISI");
    } break;
      
    case RffeIntSlotField: {
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 1, number_str, 4);
      snprintf(results_str, sizeof(results_str), "INT%d", U8(frame.mData2));
      AddResultString("INT");
      AddResultString(results_str);
    } break;
      
    case RffeParityField: {
      AnalyzerHelpers::GetNumberString(frame.mData1, Decimal, 1, number_str, 4);
      snprintf(results_str, sizeof(results_str), "P:%s", number_str);
      AddResultString("P");
      AddResultString(results_str);
    } break;

    case RffeBusParkField: {
      AddResultString("B");
      AddResultString("BP");
    } break;

    case RffeErrorCaseField:
    default: {
      char number1_str[20];
      char number2_str[20];

      AnalyzerHelpers::GetNumberString(frame.mData1, Hexadecimal, 32, number1_str, 10);
      AnalyzerHelpers::GetNumberString(frame.mData2, Hexadecimal, 32, number2_str, 10);
      snprintf(results_str, sizeof(results_str), "E:%s - %s", number1_str, number2_str);
      AddResultString("E");
      AddResultString(results_str);
    } break;
  }
}

void RFFEAnalyzerResults::GenerateExportFile(const char *file, DisplayBase display_base, U32 export_type_user_id) {
  U64 first_frame_id;
  U64 last_frame_id;
  U64 address;
  char time_str[16];
  char packet_str[16];
  char sa_str[8];
  char type_str[16];
  char addr_str[16];
  char parity_str[8];
  char parityCmd_str[8];
  char bc_str[8];
  char data_str[8];
  bool show_parity = mSettings->mShowParityInReport;
  bool show_buspark = mSettings->mShowBusParkInReport;
  char payload_str[256];
  char results_str[256];
  Frame frame;
  void *f = AnalyzerHelpers::StartFile(file);

  export_type_user_id = export_type_user_id;

  U64 trigger_sample = mAnalyzer->GetTriggerSample();
  U32 sample_rate = mAnalyzer->GetSampleRate();

  if (show_parity) {
    snprintf(results_str, sizeof(results_str), "Time[s],Packet ID,SA,Type,Adr,BC,CmdP,Payload\n");
  }
  else {
    snprintf(results_str, sizeof(results_str), "Time[s],Packet ID,SA,Type,Adr,BC,Payload\n");
  }
  AnalyzerHelpers::AppendToFile((U8*)results_str, (U32)strlen(results_str), f);

  U64 num_packets = GetNumPackets();
  for (U32 i = 0; i < num_packets; i++) {
    // package id
    AnalyzerHelpers::GetNumberString(i, Decimal, 0, packet_str, 16);

    snprintf(sa_str, 8, "");
    snprintf(type_str, 8, "");
    snprintf(addr_str, 8, "");
    snprintf(parity_str, 8, "");
    snprintf(parityCmd_str, 8, "");
    snprintf(bc_str, 8, "");
    snprintf(data_str, 8, "");
    snprintf(payload_str, 256, "");
    snprintf(results_str, 256, "");
    address = 0x0;

    GetFramesContainedInPacket(i, &first_frame_id, &last_frame_id);
    for (U64 j = first_frame_id; j <= last_frame_id; j++) {
      frame = GetFrame(j);

      switch (frame.mType) {
        case RffeSSCField:
          // starting time using SSC as marker
          AnalyzerHelpers::GetTimeString(frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 16);
          break;

        case RffeSAField:
          AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 4, sa_str, 8);
          break;

        case RffeCommandField:
          snprintf(type_str, sizeof(type_str), "%s", RffeCommandFieldStringMid[frame.mData1]);
          if (frame.mData1 == RffeTypeNormalWrite || frame.mData1 == RffeTypeNormalRead) {
            snprintf(bc_str, sizeof(bc_str), "0x0");
          }
          else if (frame.mData1 == RffeTypeWrite0) {
            snprintf(bc_str, sizeof(bc_str), "0x0");
            snprintf(addr_str, sizeof(addr_str), "0x0");
          }
          break;

        case RffeExByteCountField:
          AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 4, bc_str, 8);
          break;

        case RffeExLongByteCountField:
          AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 3, bc_str, 8);
          break;

        case RffeAddressField:
          address = frame.mData1;
          AnalyzerHelpers::GetNumberString(address, display_base, sizeof(addr_str), addr_str, 16);
          break;

        case RffeAddressHiField:
          address = (frame.mData1 << 8);
          AnalyzerHelpers::GetNumberString(address, display_base, sizeof(addr_str), addr_str, 16);
          break;

        case RffeAddressLoField:
          address |= frame.mData1;
          AnalyzerHelpers::GetNumberString(address, display_base, sizeof(addr_str), addr_str, 16);
          break;

        case RffeShortDataField:
          AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 7, data_str, 8);
          snprintf(payload_str, sizeof(payload_str), "%s", data_str);
          break;

        case RffeDataField:
          AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, data_str, 8);
          // Gather up all the data bytes into the payload
          if (strlen(payload_str) == 0) {
            snprintf(payload_str, sizeof(payload_str), "%s", data_str);
          } else {
            snprintf(payload_str, sizeof(payload_str), "%s %s", payload_str, data_str);
          }
          break;

        case RffeParityField:
          if (show_parity) {
            AnalyzerHelpers::GetNumberString(frame.mData1, Decimal, 1, parity_str, 4);
            if (frame.mData2) {
              // Data Parity
              if (strlen(payload_str) == 0) {
                snprintf(payload_str, sizeof(payload_str), "P%s", parity_str);
              } else {
                snprintf(payload_str, sizeof(payload_str), "%s P%s", payload_str, parity_str);
              }
            }
            else {
              snprintf(parityCmd_str, sizeof(parityCmd_str), "P%s", parity_str);   // CmdParity
            }
            
          }
          break;

        case RffeBusParkField:
          if (show_buspark) {
            if (strlen(payload_str) == 0) {
              snprintf(payload_str, sizeof(payload_str), "BP");
            } else {
              snprintf(payload_str, sizeof(payload_str), "%s BP", payload_str);
            }
          }
          break;

        case RffeErrorCaseField:
        default:
          char number1_str[20];
          char number2_str[20];

          AnalyzerHelpers::GetNumberString(frame.mData1, Hexadecimal, 32, number1_str, 10);
          AnalyzerHelpers::GetNumberString(frame.mData2, Hexadecimal, 32, number2_str, 10);
          snprintf(payload_str, sizeof(payload_str), "E:%s - %s ", number1_str, number2_str);
          break;
      }
    }


    if (show_parity) {
      snprintf(results_str, sizeof(results_str), "%s,%s,%s,%s,%s,%s,%s,%s\n", time_str, packet_str, sa_str, type_str, addr_str, bc_str, parityCmd_str, payload_str);
    }
    else {
      snprintf(results_str, sizeof(results_str), "%s,%s,%s,%s,%s,%s,%s\n", time_str, packet_str, sa_str, type_str, addr_str, bc_str, payload_str);
    }
    AnalyzerHelpers::AppendToFile((U8*)results_str, (U32)strlen(results_str), f);

    if (UpdateExportProgressAndCheckForCancel(i, num_packets) == true) {
      AnalyzerHelpers::EndFile(f);
      return;
    }
  }
}

void RFFEAnalyzerResults::GenerateFrameTabularText(U64 frame_index, DisplayBase display_base) {
  char time_str[16];
  char sa_str[8];
  char type_str[16];
  char parity_str[8];
  char parityCmd_str[8];
  char addr_str[8];
  char bc_str[8];
  char data_str[8];
  char frame_str[32];
  Frame frame;

  U64 trigger_sample = mAnalyzer->GetTriggerSample();
  U32 sample_rate = mAnalyzer->GetSampleRate();

  ClearTabularText();

  frame = GetFrame(frame_index);

  switch (frame.mType) {
    case RffeSSCField:
      // starting time using SSC as marker
      AnalyzerHelpers::GetTimeString(frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 16);
      snprintf(frame_str, sizeof(frame_str), "SSC");
      break;

    case RffeSAField:
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 4, sa_str, 8);
      snprintf(frame_str, sizeof(frame_str), "SA: %s", sa_str);
      break;

    case RffeCommandField:
      snprintf(type_str, sizeof(type_str), "%s", RffeCommandFieldStringMid[frame.mData1]);
      snprintf(frame_str, sizeof(frame_str), "Type: %s", type_str);
      break;

    case RffeExByteCountField:
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 4, bc_str, 8);
      snprintf(frame_str, sizeof(frame_str), "ExByteCount: %s", bc_str);
      break;

    case RffeExLongByteCountField:
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 3, bc_str, 8);
      snprintf(frame_str, sizeof(frame_str), "ExLongByteCount: %s", bc_str);
      break;

    case RffeAddressField:
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, addr_str, 8);
      snprintf(frame_str, sizeof(frame_str), "Addr: %s", addr_str);
      break;

    case RffeAddressHiField:
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, addr_str, 8);
      snprintf(frame_str, sizeof(frame_str), "AddrHi: %s", addr_str);
      break;

    case RffeAddressLoField:
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, addr_str, 8);
      snprintf(frame_str, sizeof(frame_str), "AddrLo: %s", addr_str);
      break;

    case RffeShortDataField:
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 7, data_str, 8);
      snprintf(frame_str, sizeof(frame_str), "DataShort: %s", data_str);
      break;

    case RffeDataField:
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, data_str, 8);
      snprintf(frame_str, sizeof(frame_str), "Data: %s", data_str);
      break;

    case RffeParityField:
      if (frame.mData2 == 1) {
        AnalyzerHelpers::GetNumberString(frame.mData1, Decimal, 1, parity_str, 4);
        snprintf(frame_str, sizeof(frame_str), "DataParity: %s", parity_str);
      } else {
        AnalyzerHelpers::GetNumberString(frame.mData1, Decimal, 1, parityCmd_str, 4);
        snprintf(frame_str, sizeof(frame_str), "CmdParity: %s", parityCmd_str);
      }
      break;

    case RffeBusParkField:
      snprintf(frame_str, sizeof(frame_str), "BP");
      break;

    case RffeErrorCaseField:
    default:
      char number1_str[20];
      char number2_str[20];

      AnalyzerHelpers::GetNumberString(frame.mData1, Hexadecimal, 32, number1_str, 10);
      AnalyzerHelpers::GetNumberString(frame.mData2, Hexadecimal, 32, number2_str, 10);
      snprintf(frame_str, sizeof(frame_str), "Error: %s - %s", number1_str, number2_str);
      break;
  }
  
  AddTabularText(frame_str);
}

void RFFEAnalyzerResults::GeneratePacketTabularText(U64 packet_id, DisplayBase display_base) {
  packet_id = packet_id;
  display_base = display_base;

  ClearResultStrings();
  AddResultString("not supported yet");
}

void RFFEAnalyzerResults::GenerateTransactionTabularText(U64 transaction_id, DisplayBase display_base) {
  transaction_id = transaction_id;
  display_base = display_base;

  ClearResultStrings();
  AddResultString("not supported yet");
}

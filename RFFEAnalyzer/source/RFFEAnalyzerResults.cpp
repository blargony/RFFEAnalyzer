#include "RFFEAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "RFFEAnalyzer.h"
#include "RFFEAnalyzerSettings.h"
#include <iostream>
#include <sstream>
#include <stdio.h>


RFFEAnalyzerResults::RFFEAnalyzerResults(RFFEAnalyzer *analyzer, RFFEAnalyzerSettings *settings)
    : AnalyzerResults(), mSettings(settings), mAnalyzer(analyzer) {}

RFFEAnalyzerResults::~RFFEAnalyzerResults() {}

//
void RFFEAnalyzerResults::GenerateBubbleText(U64 frame_index, Channel &channel, DisplayBase display_base) {
  char number_str[8];
  char results_string[16];

  channel = channel;

  ClearResultStrings();
  Frame frame = GetFrame(frame_index);

  switch (frame.mType) {
    case RffeSSCField: {
      AddResultString("SSC");
    } break;

    case RffeSAField: {
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 4, number_str, 8);
      snprintf(results_string, 16, "SA:%s", number_str);
      AddResultString("SA");
      AddResultString(results_string);
    } break;

    case RffeCommandField: {
      AddResultString(RffeCommandFieldStringShort[frame.mData1]);
      AddResultString(RffeCommandFieldStringMid[frame.mData1]);
    } break;

    case RffeExByteCountField: {
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 4, number_str, 8);
      snprintf(results_string, 16, "BC:%s", number_str);
      AddResultString("BC");
      AddResultString(results_string);
    } break;

    case RffeExLongByteCountField: {
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 3, number_str, 8);
      snprintf(results_string, 16, "BC:%s", number_str);
      AddResultString("BC");
      AddResultString(results_string);
    } break;

    case RffeAddressField: {
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, 8);
      snprintf(results_string, 16, "A:%s", number_str);
      AddResultString("A");
      AddResultString(results_string);
    } break;

    case RffeAddressHiField: {
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, 8);
      snprintf(results_string, 16, "AH:%s", number_str);
      AddResultString("A");
      AddResultString(results_string);

    } break;

    case RffeAddressLoField: {
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, 8);
      snprintf(results_string, 16, "AL:%s", number_str);
      AddResultString("A");
      AddResultString(results_string);
    } break;

    case RffeShortDataField: {
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 7, number_str, 8);
      snprintf(results_string, 16, "D:%s", number_str);
      AddResultString("D");
      AddResultString(results_string);
    } break;

    case RffeDataField: {
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, 8);
      snprintf(results_string, 16, "D:%s", number_str);
      AddResultString("D");
      AddResultString(results_string);
    } break;

    case RffeMasterHandoffAckField: {
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, 8);
      snprintf(results_string, 16, "MHAck:%s", number_str);
      AddResultString("MHA");
      AddResultString(results_string);
    } break;

    case RffeISIField: {
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 2, number_str, 4);
      AddResultString("ISI");
      AddResultString("ISI");
    } break;
      
    case RffeIntSlotField: {
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 1, number_str, 4);
      snprintf(results_string, 16, "INT%d", U8(frame.mData2));
      AddResultString("INT");
      AddResultString(results_string);
    } break;
      
    case RffeParityField: {
      AnalyzerHelpers::GetNumberString(frame.mData1, Decimal, 1, number_str, 4);
      snprintf(results_string, 16, "P:%s", number_str);
      AddResultString("P");
      AddResultString(results_string);
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
      snprintf(results_string, 16, "E:%s - %s", number1_str, number2_str);
      AddResultString("E");
      AddResultString(results_string);
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
  std::stringstream payload;
  std::stringstream ss;
  Frame frame;
  void *f = AnalyzerHelpers::StartFile(file);

  export_type_user_id = export_type_user_id;

  U64 trigger_sample = mAnalyzer->GetTriggerSample();
  U32 sample_rate = mAnalyzer->GetSampleRate();

  ss << "Time [s],Packet ID,SSC,SA,Type,Adr,BC,Payload" << std::endl;

  U64 num_packets = GetNumPackets();
  for (U32 i = 0; i < num_packets; i++) {
    // package id
    AnalyzerHelpers::GetNumberString(i, Decimal, 0, packet_str, 16);

    payload.str(std::string());
    snprintf(sa_str, 8, "");
    snprintf(type_str, 8, "");
    snprintf(addr_str, 8, "");
    snprintf(parity_str, 8, "");
    snprintf(parityCmd_str, 8, "");
    snprintf(bc_str, 8, "");
    snprintf(data_str, 8, "");
    address = 0xFFFFFFFF;

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
          break;

        case RffeExByteCountField:
          AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 4, bc_str, 8);
          break;

        case RffeExLongByteCountField:
          AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 3, bc_str, 8);
          break;

        case RffeAddressField:
          address = frame.mData1;
          break;

        case RffeAddressHiField:
          address = (frame.mData1 << 8);
          break;

        case RffeAddressLoField:
          address |= frame.mData1;
          break;

        case RffeShortDataField:
          AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 7, data_str, 8);
          payload << "D:" << data_str << " ";
          break;

        case RffeDataField:
          AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, data_str, 8);
          payload << data_str << " ";
          break;

        case RffeParityField:
          if (!show_parity)
            break;
          if (frame.mData2 == 0) {
            AnalyzerHelpers::GetNumberString(frame.mData1, Decimal, 1, parity_str, 4);
            payload << "P" << parity_str << " ";
          } else {
            AnalyzerHelpers::GetNumberString(frame.mData1, Decimal, 1, parityCmd_str, 4);
          }
          break;

        case RffeBusParkField:
          if (!show_buspark)
            break;

          payload << "BP ";
          break;

        case RffeErrorCaseField:
        default:
          char number1_str[20];
          char number2_str[20];

          AnalyzerHelpers::GetNumberString(frame.mData1, Hexadecimal, 32, number1_str, 10);
          AnalyzerHelpers::GetNumberString(frame.mData2, Hexadecimal, 32, number2_str, 10);
          payload << "E:" << number1_str << " - " << number2_str << " ";
          break;
      }
    }

    ss << time_str << "," << packet_str << ",SSC," << sa_str << "," << type_str;

    if (address == 0xFFFFFFFF) {
      ss << ",,," << payload.str().c_str();
      if (show_parity) {
        ss << " P" << parityCmd_str;
      }
    } else {
      AnalyzerHelpers::GetNumberString(address, display_base, 16, addr_str, 16);
      ss << "," << addr_str;
      if (show_parity) {
        ss << " P" << parityCmd_str;
      }
      ss << "," << bc_str << "," << payload.str().c_str();
    }
    ss << std::endl;
    AnalyzerHelpers::AppendToFile((U8 *)ss.str().c_str(), (U32)ss.str().length(), f);
    ss.str(std::string());

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
  std::stringstream payload;
  std::stringstream ss;
  Frame frame;

  U64 trigger_sample = mAnalyzer->GetTriggerSample();
  U32 sample_rate = mAnalyzer->GetSampleRate();

  ClearTabularText();

  frame = GetFrame(frame_index);

  switch (frame.mType) {
    case RffeSSCField:
      // starting time using SSC as marker
      AnalyzerHelpers::GetTimeString(frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 16);
      ss << "SSC";
      AddTabularText(ss.str().c_str());
      break;

    case RffeSAField:
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 4, sa_str, 8);
      ss << "SA: " << sa_str;
      AddTabularText(ss.str().c_str());
      break;

    case RffeCommandField:
      snprintf(type_str, sizeof(type_str), "%s", RffeCommandFieldStringMid[frame.mData1]);
      ss << "Type: " << type_str;
      AddTabularText(ss.str().c_str());
      break;

    case RffeExByteCountField:
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 4, bc_str, 8);
      ss << "ExByteCount: " << bc_str;
      AddTabularText(ss.str().c_str());
      break;

    case RffeExLongByteCountField:
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 3, bc_str, 8);
      ss << "ExLongByteCount: " << bc_str;
      AddTabularText(ss.str().c_str());
      break;

    case RffeAddressField:
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, addr_str, 8);
      ss << "Addr: " << addr_str;
      AddTabularText(ss.str().c_str());
      break;

    case RffeAddressHiField:
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, addr_str, 8);
      ss << "AddrHi: " << addr_str;
      AddTabularText(ss.str().c_str());
      break;

    case RffeAddressLoField:
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, addr_str, 8);
      ss << "AddrLo: " << addr_str;
      AddTabularText(ss.str().c_str());
      break;

    case RffeShortDataField:
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 7, data_str, 8);
      ss << "DataShort: " << data_str;
      AddTabularText(ss.str().c_str());
      break;

    case RffeDataField:
      AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, data_str, 8);
      ss << "Data: " << data_str;
      AddTabularText(ss.str().c_str());
      break;

    case RffeParityField:
      if (frame.mData2 == 0) {
        AnalyzerHelpers::GetNumberString(frame.mData1, Decimal, 1, parity_str, 4);
        ss << "DataParity: " << parity_str;
        AddTabularText(ss.str().c_str());
      } else {
        AnalyzerHelpers::GetNumberString(frame.mData1, Decimal, 1, parityCmd_str, 4);
        ss << "CmdParity: " << parityCmd_str;
        AddTabularText(ss.str().c_str());
      }
      break;

    case RffeBusParkField:
      ss << "BP";
      AddTabularText(ss.str().c_str());
      break;

    case RffeErrorCaseField:
    default:
      char number1_str[20];
      char number2_str[20];

      AnalyzerHelpers::GetNumberString(frame.mData1, Hexadecimal, 32, number1_str, 10);
      AnalyzerHelpers::GetNumberString(frame.mData2, Hexadecimal, 32, number2_str, 10);
      ss << "ERROR:" << number1_str << " - " << number2_str << " ";
      AddTabularText(ss.str().c_str());
      break;
  }
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

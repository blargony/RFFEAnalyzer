#include "RFFEAnalyzer.h"
#include "RFFEAnalyzerSettings.h"
#include "RFFEUtil.h"
#include <AnalyzerChannelData.h>

// ==============================================================================
// Boilerplate for API
// ==============================================================================
RFFEAnalyzer::RFFEAnalyzer() : Analyzer2(), mSettings(new RFFEAnalyzerSettings()), mSimulationInitilized(false) {
  SetAnalyzerSettings(mSettings.get());

  // Setup the default state on the bus
  mSclkCurrent = BIT_LOW;
  mSclkPrevious = BIT_LOW;
  mSdataCurrent = BIT_LOW;
  mSdataPrevious = BIT_LOW;
  mUnexpectedSSC = false;
}

RFFEAnalyzer::~RFFEAnalyzer() {
  KillThread();
}

void RFFEAnalyzer::SetupResults() {
  mResults.reset(new RFFEAnalyzerResults(this, mSettings.get()));
  SetAnalyzerResults(mResults.get());
  mResults->AddChannelBubblesWillAppearOn(mSettings->mSdataChannel);
}

// ==============================================================================
// Main data parsing method
// ==============================================================================
void RFFEAnalyzer::WorkerThread() {
  U8 byte_count;

  mSampleRateHz = GetSampleRate();

  mSdata = GetAnalyzerChannelData(mSettings->mSdataChannel);
  mSclk = GetAnalyzerChannelData(mSettings->mSclkChannel);

  mResults->CancelPacketAndStartNewPacket();

  while (1) {
    try {
      // Look for an SSC
      // This method only returns false if there is no more data to be scanned
      // in which case, we call the Cancel and wait for new data method in the
      // API
      if (!FindStartSeqCondition()) {
        mResults->CancelPacketAndStartNewPacket();
        break;
      }

      // Find and parse the Slave Address and the RFFE Command
      // Return ByteCount field - depending on the command it may or may not be
      // relevent
      byte_count = FindCommandFrame();

      // We know what kind of packet we are handling now, and various parameters
      // including the number of data bytes.  Go ahead and handle the different
      // cases
      switch (mRffeCmdType) {
        case RFFEAnalyzerResults::RffeTypeExtWrite:
          FindByteFrame(RFFEAnalyzerResults::RffeAddressField);
          for (U32 i = 0; i <= byte_count; i += 1) {
            FindByteFrame(RFFEAnalyzerResults::RffeDataField);
          }
          break;
        case RFFEAnalyzerResults::RffeTypeReserved:
          break;
        case RFFEAnalyzerResults::RffeTypeMasterRead:
          FindByteFrame(RFFEAnalyzerResults::RffeAddressField);
          FindBusPark();
          for (U32 i = 0; i <= byte_count; i += 1) {
            FindByteFrame(RFFEAnalyzerResults::RffeDataField);
          }
          break;
        case RFFEAnalyzerResults::RffeTypeMasterWrite:
          FindByteFrame(RFFEAnalyzerResults::RffeAddressField);
          for (U32 i = 0; i <= byte_count; i += 1) {
            FindByteFrame(RFFEAnalyzerResults::RffeDataField);
          }
          break;
        case RFFEAnalyzerResults::RffeTypeMasterHandoff:
          FindBusPark();
          for (U32 i = 0; i <= byte_count; i += 1) {
            FindByteFrame(RFFEAnalyzerResults::RffeDataField);
          }
          break;
        case RFFEAnalyzerResults::RffeTypeInterrupt:
          FindBusPark();
          if (FindISI()) {
            FindInterruptSlots();
          }
          break;
        case RFFEAnalyzerResults::RffeTypeExtRead:
          FindByteFrame(RFFEAnalyzerResults::RffeAddressField);
          FindBusPark();
          for (U32 i = 0; i <= byte_count; i += 1) {
            FindByteFrame(RFFEAnalyzerResults::RffeDataField);
          }
          break;
        case RFFEAnalyzerResults::RffeTypeExtLongWrite:
          FindByteFrame(RFFEAnalyzerResults::RffeAddressHiField);
          FindByteFrame(RFFEAnalyzerResults::RffeAddressLoField);
          for (U32 i = 0; i <= byte_count; i += 1) {
            FindByteFrame(RFFEAnalyzerResults::RffeDataField);
          }
          break;
        case RFFEAnalyzerResults::RffeTypeExtLongRead:
          FindByteFrame(RFFEAnalyzerResults::RffeAddressHiField);
          FindByteFrame(RFFEAnalyzerResults::RffeAddressLoField);
          FindBusPark();
          for (U32 i = 0; i <= byte_count; i += 1) {
            FindByteFrame(RFFEAnalyzerResults::RffeDataField);
          }
          break;
        case RFFEAnalyzerResults::RffeTypeNormalWrite:
          FindByteFrame(RFFEAnalyzerResults::RffeDataField);
          break;
        case RFFEAnalyzerResults::RffeTypeNormalRead:
          FindBusPark();
          FindByteFrame(RFFEAnalyzerResults::RffeDataField);
          break;
        case RFFEAnalyzerResults::RffeTypeWrite0:
          break;
      }

      // Finish up with a Bus Park (except for interrupts)
      if (mRffeCmdType != RFFEAnalyzerResults::RffeTypeInterrupt) {
        FindBusPark();
      }

    } catch (int exception) {
      if (exception == UNEXPECTED_SSC_EXCEPTION) {
        // Do nothing, this can happen
      }
    }

    // Commit the Result and call the API Required finished? method.
    mResults->CommitPacketAndStartNewPacket();
    CheckIfThreadShouldExit();
  }
}

// ==============================================================================
// Worker thread support methods
// ==============================================================================
U8 RFFEAnalyzer::FindStartSeqCondition() {
  bool ssc_possible = 0;
  U8 flags = 0;

  // Check for no more data, and return error status if none remains
  if (!mSclk->DoMoreTransitionsExistInCurrentData())
    return 0;
  if (!mSdata->DoMoreTransitionsExistInCurrentData())
    return 0;

  // Scan for an SSC
  if (mUnexpectedSSC) {
    flags |= (DISPLAY_AS_ERROR_FLAG | DISPLAY_AS_WARNING_FLAG | RFFE_INCOMPLETE_PACKET_ERROR_FLAG);
    mUnexpectedSSC = false;                      // Reset our unexpected SSC state
    mSampleClkOffsets[0] = mUnexpectedSSCStart;  // The unexpected SSC logic
                                                 // saved the sdata rising edge
                                                 // position
    mSampleDataOffsets[0] = mUnexpectedSSCStart; // For the Marker
  } else {
    while (1) {
      GotoNextTransition();

      if (mSclkCurrent == BIT_HIGH) {
        ssc_possible = 0;
      } else if (mSclkCurrent == BIT_LOW && mSclkPrevious == BIT_LOW && mSdataCurrent == BIT_HIGH && mSdataPrevious == BIT_LOW) {
        ssc_possible = 1;
        mSampleClkOffsets[0] = mSamplePosition;  // For the Frame Boundaries
        mSampleDataOffsets[0] = mSamplePosition; // For the Marker
      } else if (ssc_possible && mSclkCurrent == BIT_LOW && mSdataCurrent == BIT_LOW) {
        break; // SSC!
      }
    }
    // TODO: Here is where we could check RFFE SSC Pulse width, if we were so
    // inclined
    // ssc_pulse_width_in_samples = sdata_fedge_sample - sdata_redge_sample;
  }

  // SSC is complete at the rising edge of the SCLK
  GotoSclkEdge(BIT_HIGH);
  mSampleClkOffsets[1] = mSamplePosition;

  // TODO: Here is where we could check SSC to SCLK rising edge width, again, if
  // we were so inclined
  // ssc_2_sclk_delay_in_samples = mSamplePosition - sdata_fedge_sample;

  // Setup the 'Start' Marker and send off the Frame to the AnalyzerResults
  mSampleMarker[0] = AnalyzerResults::Start;
  FillInFrame(RFFEAnalyzerResults::RffeSSCField, 0, 0, 0, 1, flags);

  return 1;
}

// ------------------------------------------------------------------------------
U8 RFFEAnalyzer::FindCommandFrame() {
  U8 byte_count = 0;
  U8 flags = 0;
  U64 RffeID;
  U64 RffeCmd;

  // Get RFFE SA+Command (4 + 8 bits) and decode the fields
  // Why grab all 12 bits?  So we can calculate parity across the SA+Cmd field
  // after we are done parsing the Command
  RffeCmd = GetBitStream(12);

  // Now look at the RFFE command and decode the various options.
  mRffeCmdType = RFFEUtil::decodeRFFECmdFrame((U8)(RffeCmd & 0xFF));
  byte_count = RFFEUtil::byteCount((U8)RffeCmd);
  RffeID = (RffeCmd & 0xF00) >> 8;

  // Check for an invalid Master address, if this is a Master Command
  if ((mRffeCmdType == RFFEAnalyzerResults::RffeTypeMasterRead || mRffeCmdType == RFFEAnalyzerResults::RffeTypeMasterWrite ||
       mRffeCmdType == RFFEAnalyzerResults::RffeTypeMasterHandoff) &&
      (RffeID > 0x3)) {
    flags = RFFE_INVALID_MASTER_ID;
  }
  // Log the Master or Slave Address
  FillInFrame(RFFEAnalyzerResults::RffeSAField, RffeID, 0, 0, 4, flags);
  flags = 0;

  switch (mRffeCmdType) {
    case RFFEAnalyzerResults::RffeTypeReserved:
      // 8 Bit Reserved Cmd Type
      flags |= (DISPLAY_AS_ERROR_FLAG | DISPLAY_AS_WARNING_FLAG | RFFE_INVALID_CMD_ERROR_FLAG);
      FillInFrame(RFFEAnalyzerResults::RffeCommandField, mRffeCmdType, 0, 4, 12, flags);
      break;
    case RFFEAnalyzerResults::RffeTypeMasterRead:
    case RFFEAnalyzerResults::RffeTypeMasterWrite:
    case RFFEAnalyzerResults::RffeTypeMasterHandoff:
    case RFFEAnalyzerResults::RffeTypeInterrupt:
      // Interrupt/Master commands - 8 Bit RFFE Command
      FillInFrame(RFFEAnalyzerResults::RffeCommandField, mRffeCmdType, 0, 4, 12, flags);
      break;
    case RFFEAnalyzerResults::RffeTypeExtWrite:
    case RFFEAnalyzerResults::RffeTypeExtRead:
      // 4 Bit Command w/ 4 bit Byte Count
      FillInFrame(RFFEAnalyzerResults::RffeCommandField, mRffeCmdType, 0, 4, 8, flags);
      FillInFrame(RFFEAnalyzerResults::RffeExByteCountField, (RffeCmd & 0x0F), 0, 8, 12, flags);
      break;
    case RFFEAnalyzerResults::RffeTypeExtLongWrite:
    case RFFEAnalyzerResults::RffeTypeExtLongRead:
      // 5 Bit Command w/ 3 bit Byte Count
      FillInFrame(RFFEAnalyzerResults::RffeCommandField, mRffeCmdType, 0, 4, 9, flags);
      FillInFrame(RFFEAnalyzerResults::RffeExLongByteCountField, (RffeCmd & 0x07), 0, 9, 12, flags);
      break;
    case RFFEAnalyzerResults::RffeTypeNormalWrite:
    case RFFEAnalyzerResults::RffeTypeNormalRead:
      // 3 Bit Command w/ 5 bit Addr
      FillInFrame(RFFEAnalyzerResults::RffeCommandField, mRffeCmdType, 0, 4, 7, flags);
      FillInFrame(RFFEAnalyzerResults::RffeAddressField, (RffeCmd & 0x1F), 0, 7, 12, flags);
      break;
    case RFFEAnalyzerResults::RffeTypeWrite0:
      // 1 Bit Command w/ 7 bit Write Data
      FillInFrame(RFFEAnalyzerResults::RffeCommandField, mRffeCmdType, 0, 4, 5, flags);
      FillInFrame(RFFEAnalyzerResults::RffeShortDataField, (RffeCmd & 0x7F), 0, 5, 12, flags);
      break;
  }

  // Check Parity - Parity bit covers the full SA/Command field (12 bits)
  FindParity(RFFEUtil::CalcParity(RffeCmd), 0);

  return byte_count;
}

// ------------------------------------------------------------------------------
U8 RFFEAnalyzer::FindISI() {
  U64 byte = GetBitStream(2);
  FillInFrame(RFFEAnalyzerResults::RffeISIField, byte, 0, 0, 2, 0);
  FindBusPark();
  return (byte & 0x2); // Return the ISI bit
}

// ------------------------------------------------------------------------------
void RFFEAnalyzer::FindInterruptSlots() {
  for (S8 i = 15; i >= 0; i -= 1) {
    U64 byte = GetBitStream(1);
    FillInFrame(RFFEAnalyzerResults::RffeIntSlotField, byte, i, 0, 1, 0); // Send int number as a note
    FindBusPark();
  }
}

// ------------------------------------------------------------------------------
void RFFEAnalyzer::FindByteFrame(RFFEAnalyzerResults::RffeFrameType type) {
  U64 byte = GetBitStream(8);
  FillInFrame(type, byte, 0, 0, 8, 0);
  FindParity(RFFEUtil::CalcParity(byte), 1);
}

// ------------------------------------------------------------------------------
void RFFEAnalyzer::FindParity(bool expParity, U64 extra_data) {
  BitState bitstate;
  bool data_parity;
  U64 parity_value;
  U8 flags = 0;

  // Get the Parity Bit on the next sample clock
  bitstate = GetNextBit(0);

  // Store away tailing sample position as the end of the Parity bit
  mSampleClkOffsets[1] = mSamplePosition;

  if (bitstate == BIT_HIGH) {
    parity_value = 1;
    data_parity = true;
    mSampleMarker[0] = AnalyzerResults::One;
  } else {
    parity_value = 0;
    data_parity = false;
    mSampleMarker[0] = AnalyzerResults::Zero;
  }

  if (data_parity != expParity) {
    flags |= (DISPLAY_AS_ERROR_FLAG | DISPLAY_AS_WARNING_FLAG | RFFE_PARITY_ERROR_FLAG);
    mSampleMarker[0] = AnalyzerResults::ErrorDot;
  }

  FillInFrame(RFFEAnalyzerResults::RffeParityField, parity_value, extra_data, 0, 1, flags);
}

// ------------------------------------------------------------------------------
void RFFEAnalyzer::FindBusPark() {
  U64 half_clk;

  // Mark this as a stop
  mSampleMarker[0] = AnalyzerResults::Stop;

  // Enter at the rising edge of SCLK
  mSampleClkOffsets[0] = mSamplePosition;

  // Goto Falling
  GotoSclkEdge(BIT_LOW);

  // Now at falling edge of clk
  mSampleDataOffsets[0] = mSamplePosition;

  // Now the bus park is offically complete, but for display
  // purposes it would look better if we had the Bus Park
  // 'Bubble' extend for what would be the full clock period.
  half_clk = mSampleDataOffsets[0] - mSampleClkOffsets[0];

  // If there is another clock edge less thank a 1/2 clock period out
  // (plus a little) then extend the Bus Park to that edge.
  // Otherwise edge the Bus Park for the 1/2 clock period from the
  // previous SClk Rising -> Falling
  if (mSclk->WouldAdvancingCauseTransition((U32)(half_clk + 2))) {
    GotoSclkEdge(BIT_HIGH);
    mSampleClkOffsets[1] = mSclk->GetSampleNumber();
  } else {
    mSampleClkOffsets[1] = mSampleDataOffsets[0] + half_clk;
  }

  FillInFrame(RFFEAnalyzerResults::RffeBusParkField, 0, 0, 0, 1, 0);
}

// ==============================================================================
// Advance to the next transition on the bus (SCLK or SDATA transition)
// ==============================================================================
void RFFEAnalyzer::GotoNextTransition() {
  U64 SclkEdgeSample;
  U64 SdataEdgeSample;

  // Store the previous Transition bus state
  mSclkPrevious = mSclkCurrent;
  mSdataPrevious = mSdataCurrent;

  // Look for a transition on SDATA without a clock transition
  SclkEdgeSample = mSclk->GetSampleOfNextEdge();
  SdataEdgeSample = mSdata->GetSampleOfNextEdge();

  if (SclkEdgeSample > SdataEdgeSample) {
    // Sclk is further out the sData
    mSamplePosition = SdataEdgeSample;
    mSclk->AdvanceToAbsPosition(SdataEdgeSample);
    mSdata->AdvanceToAbsPosition(SdataEdgeSample);
  } else {
    // Sdata transition is further out than Sclk
    mSamplePosition = SclkEdgeSample;
    mSclk->AdvanceToAbsPosition(SclkEdgeSample);
    mSdata->AdvanceToAbsPosition(SclkEdgeSample);
  }

  // Update the current transition bus state
  mSclkCurrent = mSclk->GetBitState();
  mSdataCurrent = mSdata->GetBitState();
}

// ==============================================================================
void RFFEAnalyzer::GotoSclkEdge(BitState edge_type) {
  bool ssc_possible = 0;

  // Scan for the next falling edge on SCLK while watching for SSCs
  while (1) {
    GotoNextTransition();
    if (mSclkCurrent == edge_type && mSclkPrevious != edge_type) {
      break;
    }

    // Unexpected SSC monitoring
    if (mSclkCurrent == BIT_HIGH) {
      ssc_possible = 0;
    } else if (mSclkCurrent == BIT_LOW && mSclkPrevious == BIT_LOW && mSdataCurrent == BIT_HIGH && mSdataPrevious == BIT_LOW) {
      mUnexpectedSSCStart = mSamplePosition;
      ssc_possible = 1;
    } else if (ssc_possible && mSclkCurrent == BIT_LOW && mSdataCurrent == BIT_LOW) {
      mUnexpectedSSC = true; // !!!
      throw UNEXPECTED_SSC_EXCEPTION;
    }
  }
}

// ==============================================================================
BitState RFFEAnalyzer::GetNextBit(U8 idx) {
  BitState data;

  // Previous FindSSC or GetNextBit left us at the rising edge of the SCLK
  // Grab this as the current sample point for delimiting the frame.
  mSampleClkOffsets[idx] = mSamplePosition;

  // Goto the next SCLK falling edge, sample SDATA and
  // put a marker to indicate that this is the sampled edge
  GotoSclkEdge(BIT_LOW);
  data = mSdataCurrent;

  mSampleDataOffsets[idx] = mSamplePosition;
  mResults->AddMarker(mSamplePosition, AnalyzerResults::DownArrow, mSettings->mSclkChannel);

  GotoSclkEdge(BIT_HIGH);

  // Return the sData Value sampled on the falling edge
  return data;
}

// --------------------------------------
U64 RFFEAnalyzer::GetBitStream(U8 len) {
  U64 data;
  U32 i;
  BitState state;
  DataBuilder data_builder;

  data_builder.Reset(&data, AnalyzerEnums::MsbFirst, len);

  // starting at rising edge of clk
  for (i = 0; i < len; i++) {
    state = GetNextBit(i);
    data_builder.AddBit(state);

    if (state == BIT_HIGH) {
      mSampleMarker[i] = AnalyzerResults::One;
    } else {
      mSampleMarker[i] = AnalyzerResults::Zero;
    }
  }
  mSampleClkOffsets[i] = mSamplePosition;

  return data;
}

// ==============================================================================
// Physical Layer checks, if we so choose to implement them.
// ==============================================================================
bool RFFEAnalyzer::CheckClockRate() {
  // TODO:  Compare the clock pulse width based on sample
  // rate against the spec.
  return true;
}

// ==============================================================================
// Results and Screen Markers
// ==============================================================================
void RFFEAnalyzer::FillInFrame(RFFEAnalyzerResults::RffeFrameType type, U64 frame_data, U64 extra_data, U32 idx_start, U32 idx_end, U8 flags) {
  Frame frame;

  frame.mType = (U8)type;
  frame.mData1 = frame_data;
  frame.mData2 = extra_data; // Additional Data (could be used for AnalyzerResults if required)
  frame.mStartingSampleInclusive = mSampleClkOffsets[idx_start];
  frame.mEndingSampleInclusive = mSampleClkOffsets[idx_end];
  frame.mFlags = flags;

  // Add Markers to the SDATA stream while we are creating the Frame
  // That is if the Frame is non-zero length and also merits a marker.
  for (U32 i = idx_start; i < idx_end; i += 1) {
    mResults->AddMarker(mSampleDataOffsets[i], mSampleMarker[i], mSettings->mSdataChannel);
  }

  mResults->AddFrame(frame);
  mResults->CommitResults();
  ReportProgress(frame.mEndingSampleInclusive);
}

// ==============================================================================
// Boilerplate for the API
// ==============================================================================
bool RFFEAnalyzer::NeedsRerun() {
  return false;
}

// --------------------------------------
U32 RFFEAnalyzer::GenerateSimulationData(U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor **simulation_channels) {
  if (mSimulationInitilized == false) {
    mSimulationDataGenerator.Initialize(GetSimulationSampleRate(), mSettings.get());
    mSimulationInitilized = true;
  }

  return mSimulationDataGenerator.GenerateSimulationData(minimum_sample_index, device_sample_rate, simulation_channels);
}

// --------------------------------------
U32 RFFEAnalyzer::GetMinimumSampleRateHz() {
  return 50000000;
}

// --------------------------------------
const char *RFFEAnalyzer::GetAnalyzerName() const {
  return "RFFEv2.0";
}

// --------------------------------------
const char *GetAnalyzerName() {
  return "RFFEv2.0";
}

// --------------------------------------
Analyzer *CreateAnalyzer() {
  return new RFFEAnalyzer();
}

// --------------------------------------
void DestroyAnalyzer(Analyzer *analyzer) {
  delete analyzer;
}

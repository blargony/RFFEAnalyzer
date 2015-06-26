#include "RFFESimulationDataGenerator.h"
#include "RFFEAnalyzerSettings.h"
#include "RFFEUtil.h"

#include <AnalyzerHelpers.h>

RFFESimulationDataGenerator::RFFESimulationDataGenerator() {
  mLSFRData = 0xE1; // Must be non-zero (or we get stuck)
}

RFFESimulationDataGenerator::~RFFESimulationDataGenerator() {
}

void RFFESimulationDataGenerator::Initialize(U32 simulation_sample_rate, RFFEAnalyzerSettings *settings) {
  mSimulationSampleRateHz = simulation_sample_rate;
  mSettings = settings;

  mClockGenerator.Init(simulation_sample_rate / 10, simulation_sample_rate);

  if (settings->mSclkChannel != UNDEFINED_CHANNEL)
    mSclk = mRffeSimulationChannels.Add(settings->mSclkChannel, mSimulationSampleRateHz, BIT_LOW);
  else
    mSclk = NULL;

  if (settings->mSdataChannel != UNDEFINED_CHANNEL)
    mSdata = mRffeSimulationChannels.Add(settings->mSdataChannel, mSimulationSampleRateHz, BIT_LOW);
  else
    mSdata = NULL;

  // insert 10 bit-periods of idle
  mRffeSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(10.0));
}

U32 RFFESimulationDataGenerator::GenerateSimulationData(U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor **simulation_channels) {
  U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample(largest_sample_requested, sample_rate, mSimulationSampleRateHz);

  while (mSclk->GetCurrentSampleNumber() < adjusted_largest_sample_requested) {
    CreateRffeTransaction();

    mRffeSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(80.0));
  }

  *simulation_channels = mRffeSimulationChannels.GetArray();
  return mRffeSimulationChannels.GetCount();
}

void RFFESimulationDataGenerator::CreateRffeTransaction() {
  U8 cmd;
  U8 sa_addrs[] = {0x5};

  for (U32 adr = 0; adr < sizeof(sa_addrs) / sizeof(sa_addrs[0]); adr++) {
    for (U32 i = 0; i < 256; i += 1) {
      CreateSSC();
      CreateSlaveAddress(sa_addrs[adr]);
      cmd = i & 0xff;
      CreateByteFrame(cmd);

      switch (RFFEUtil::decodeRFFECmdFrame(cmd)) {
        case RFFEAnalyzerResults::RffeTypeExtWrite:
          CreateByteFrame(CreateRandomData());
          for (U32 i = 0; i <= RFFEUtil::byteCount(cmd); i += 1) {
            CreateByteFrame(CreateRandomData());
          }
          CreateBusPark();
          break;
        case RFFEAnalyzerResults::RffeTypeReserved:
          CreateBusPark();
          break;

        case RFFEAnalyzerResults::RffeTypeMasterRead:
          CreateByteFrame(CreateRandomData());
          CreateBusPark();
          for (U32 i = 0; i <= RFFEUtil::byteCount(cmd); i += 1) {
            CreateByteFrame(CreateRandomData());
          }
          CreateBusPark();
          break;
        case RFFEAnalyzerResults::RffeTypeMasterWrite:
          CreateByteFrame(CreateRandomData());
          for (U32 i = 0; i <= RFFEUtil::byteCount(cmd); i += 1) {
            CreateByteFrame(CreateRandomData());
          }
          CreateBusPark();
          break;
        case RFFEAnalyzerResults::RffeTypeMasterHandoff:
          CreateBusPark();
          for (U32 i = 0; i <= RFFEUtil::byteCount(cmd); i += 1) {
            CreateByteFrame((CreateRandomData() & 0x18) | 0x80); // Mask Out bits that are always zero and always set the ACK bit
          }
          CreateBusPark();
          break;
        case RFFEAnalyzerResults::RffeTypeInterrupt:
          CreateBusPark();
          CreateBits(2, 0x3); // Always indicate interrupts
          CreateBusPark();
          for (U32 i = 0; i < 16; i += 1) {
            CreateBits(2, CreateRandomData() & 0x2); // Interrupts are 1 bit of interrupt followed by a 0/BP
          }
          break;
        case RFFEAnalyzerResults::RffeTypeExtRead:
          CreateByteFrame(CreateRandomData());
          CreateBusPark();
          for (U32 i = 0; i <= RFFEUtil::byteCount(cmd); i += 1) {
            CreateByteFrame(CreateRandomData());
          }
          CreateBusPark();
          break;
        case RFFEAnalyzerResults::RffeTypeExtLongWrite:
          CreateByteFrame(CreateRandomData());
          CreateByteFrame(CreateRandomData());
          for (U32 i = 0; i <= RFFEUtil::byteCount(cmd); i += 1) {
            CreateByteFrame(CreateRandomData());
          }
          CreateBusPark();
          break;
        case RFFEAnalyzerResults::RffeTypeExtLongRead:
          CreateByteFrame(CreateRandomData());
          CreateByteFrame(CreateRandomData());
          CreateBusPark();
          for (U32 i = 0; i <= RFFEUtil::byteCount(cmd); i += 1) {
            CreateByteFrame(CreateRandomData());
          }
          CreateBusPark();
          break;
        case RFFEAnalyzerResults::RffeTypeNormalWrite:
          CreateByteFrame(CreateRandomData());
          CreateBusPark();
          break;
        case RFFEAnalyzerResults::RffeTypeNormalRead:
          CreateBusPark();
          CreateByteFrame(CreateRandomData());
          CreateBusPark();
          break;
        case RFFEAnalyzerResults::RffeTypeWrite0:
          CreateBusPark();
          break;
      }
    }
  }
}

void RFFESimulationDataGenerator::CreateSSC() {
  if (mSclk->GetCurrentBitState() == BIT_HIGH) {
    mSclk->Transition();
  }

  if (mSdata->GetCurrentBitState() == BIT_HIGH) {
    mSdata->Transition();
  }

  mRffeSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(2.0));

  // sdata pulse for 1-clock cycle
  mSdata->Transition();
  mRffeSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(2.0));
  mSdata->Transition();
  // sdata and sclk state low for 1-clock cycle
  mRffeSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(2.0));
}

void RFFESimulationDataGenerator::CreateSlaveAddress(U8 addr) {
  CreateBits(4, addr & 0x0f);
}

void RFFESimulationDataGenerator::CreateByteFrame(U8 byte) {
  CreateByte(byte);
  CreateParity(byte);
}

void RFFESimulationDataGenerator::CreateByte(U8 cmd) {
  CreateBits(8, cmd);
}

void RFFESimulationDataGenerator::CreateParity(U8 byte) {
  CreateBits(1, ParityTable256[byte]);
}

void RFFESimulationDataGenerator::CreateBusPark() {
  CreateBits(1, 0);
}

void RFFESimulationDataGenerator::CreateBits(U8 bits, U8 cmd) {
  BitState bit;
  U8 idx = 0x1 << (bits - 1);

  for (U32 i = 0; i < bits; i += 1) {
    mSclk->Transition();
    mRffeSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(.5));

    if (cmd & idx) {
      bit = BIT_HIGH;
    } else {
      bit = BIT_LOW;
    }
    mSdata->TransitionIfNeeded(bit);

    mRffeSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(.5));
    mSclk->Transition();

    mRffeSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(1.0));

    idx = idx >> 0x1;
  }
}

U8 RFFESimulationDataGenerator::CreateRandomData() {
  U8 lsb;

  lsb = mLSFRData & 0x1;
  mLSFRData >>= 1;
  if (lsb) {
    mLSFRData ^= 0xB8; // Maximum period polynomial for Galois LFSR of 8 bits
  }
  return mLSFRData;
}

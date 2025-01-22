#pragma once

#include <mutex>
#include <string>

#include <aidl/android/hardware/ir/BnConsumerIr.h>

namespace aidl {
namespace android {
namespace hardware {
namespace ir {

class ConsumerIr : public BnConsumerIr {
  public:
    ConsumerIr();

    ::ndk::ScopedAStatus getCarrierFreqs(
            ::std::vector<::aidl::android::hardware::ir::ConsumerIrFreqRange>* _aidl_return) override;
    ::ndk::ScopedAStatus transmit(int32_t carrierFreqHz,
                                  const ::std::vector<int32_t>& pattern) override;

  private:
    std::string mDevicePath;
    std::once_flag mDeviceInitOnce;
};

}  // namespace ir
}  // namespace hardware
}  // namespace android
}  // namespace aidl

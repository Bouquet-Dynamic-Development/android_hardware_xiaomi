/*
 * SPDX-FileCopyrightText: 2017-2024 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "ConsumerIr"

#include "ConsumerIr.h"

#include <android-base/logging.h>
#include <fcntl.h>
#include <linux/lirc.h>
#include <mutex>
#include <string>
#include <vector>

using std::vector;

namespace aidl {
namespace android {
namespace hardware {
namespace ir {

static const vector<ConsumerIrFreqRange> kRangeVec{
        {.minHz = 30000, .maxHz = 60000},
};

ConsumerIr::ConsumerIr() = default;

::ndk::ScopedAStatus ConsumerIr::getCarrierFreqs(vector<ConsumerIrFreqRange>* _aidl_return) {
    *_aidl_return = kRangeVec;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus ConsumerIr::transmit(int32_t carrierFreqHz, const vector<int32_t>& pattern) {
    size_t entries = pattern.size();

    if (entries == 0) {
        return ::ndk::ScopedAStatus::ok();
    }

    // Probe for device path once
    std::call_once(mDeviceInitOnce, [this]() {
        static const std::vector<std::string> kPossibleDevices = {"/dev/lirc0", "/dev/spidev7.1"};
        for (const auto& device : kPossibleDevices) {
            int fd = open(device.c_str(), O_RDWR);
            if (fd >= 0) {
                close(fd);
                mDevicePath = device;
                LOG(INFO) << "Using IR device: " << mDevicePath;
                break;
            }
        }
        if (mDevicePath.empty()) {
            LOG(ERROR) << "No valid IR device found";
        }
    });

    if (mDevicePath.empty()) {
        return ::ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

    int fd = open(mDevicePath.c_str(), O_RDWR);
    if (fd < 0) {
        LOG(ERROR) << "Failed to open " << mDevicePath << ", error: " << errno;
        return ::ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

    // Only set carrier frequency for LIRC devices
    if (mDevicePath.find("lirc") != std::string::npos) {
        int rc = ioctl(fd, LIRC_SET_SEND_CARRIER, &carrierFreqHz);
        if (rc < 0) {
            LOG(ERROR) << "Failed to set carrier " << carrierFreqHz << ", error: " << errno;
            close(fd);
            return ::ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
        }
    }

    // Common transmission logic
    int rc;
    if ((entries & 1) != 0) {
        rc = write(fd, pattern.data(), entries * sizeof(int32_t));
    } else {
        rc = write(fd, pattern.data(), (entries - 1) * sizeof(int32_t));
        usleep(pattern[entries - 1]);
    }

    if (rc < 0) {
        LOG(ERROR) << "Failed to write pattern, " << entries << " entries, error: " << errno;
        close(fd);
        return ::ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

    close(fd);
    return ::ndk::ScopedAStatus::ok();
}

}  // namespace ir
}  // namespace hardware
}  // namespace android
}  // namespace aidl

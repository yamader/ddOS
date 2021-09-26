#include "cdc.hpp"

#include <algorithm>
#include <cstdlib>
#include <iterator>

#include "log.hpp"
#include "../device.hpp"

namespace usb::cdc {
  CDCDriver::CDCDriver(Device* dev, const InterfaceDescriptor* if_comm,
                       const InterfaceDescriptor* if_data) : ClassDriver{dev} {
  }

  kError CDCDriver::Initialize() {
    return KernelError(kError::kNotImplemented);
  }

  kError CDCDriver::SetEndpoint(const std::vector<EndpointConfig>& configs) {
    for (const auto& config : configs) {
      if (config.ep_type == EndpointType::kInterrupt && config.ep_id.IsIn()) {
        ep_interrupt_in_ = config.ep_id;
      } else if (config.ep_type == EndpointType::kBulk && config.ep_id.IsIn()) {
        ep_bulk_in_ = config.ep_id;
      } else if (config.ep_type == EndpointType::kBulk && !config.ep_id.IsIn()) {
        ep_bulk_out_ = config.ep_id;
      }
    }
    return KernelError(kError::kSuccess);
  }

  kError CDCDriver::OnEndpointsConfigured() {
    return KernelError(kError::kSuccess);
  }

  kError CDCDriver::OnControlCompleted(EndpointID ep_id, SetupData setup_data,
                           const void* buf, int len) {
    return KernelError(kError::kNotImplemented);
  }

  kError CDCDriver::OnNormalCompleted(EndpointID ep_id, const void* buf, int len) {
    PutLog(kLogDebug, "CDCDriver::OnNormalCompleted: buf='%.*s'\n", len, buf);
    auto buf8 = reinterpret_cast<const uint8_t*>(buf);
    if (ep_id == ep_bulk_in_) {
      std::copy_n(buf8, len, std::back_inserter(receive_buf_));
    } else if (ep_id == ep_bulk_out_) {
    } else {
      return KernelError(kError::kEndpointNotInCharge);
    }
    delete[] buf8;
    return KernelError(kError::kSuccess);
  }

  kError CDCDriver::SendSerial(const void* buf, int len) {
    uint8_t* buf_out = new uint8_t[len];
    memcpy(buf_out, buf, len);
    if (auto err = ParentDevice()->NormalOut(ep_bulk_out_, buf_out, len)) {
      PutLog(kLogError, "%s:%d: NormalOut failed: %s\n", err.File(), err.Line(), err.Name());
      return err;
    }

    uint8_t* buf_in = new uint8_t[8];
    if (auto err = ParentDevice()->NormalIn(ep_bulk_in_, buf_in, 8)) {
      PutLog(kLogError, "%s:%d: NormalIn failed: %s\n", err.File(), err.Line(), err.Name());
      return err;
    }
    return KernelError(kError::kSuccess);
  }

  int CDCDriver::ReceiveSerial(void* buf, int len) {
    const auto recv_len = std::min(len, static_cast<int>(receive_buf_.size()));
    auto buf8 = reinterpret_cast<uint8_t*>(buf);
    for (int i = 0; i < recv_len; ++i) {
      buf8[i] = receive_buf_.front();
      receive_buf_.pop_front();
    }
    return recv_len;
  }
}

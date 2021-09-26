/**
 * @file usb/xhci/xhci.hpp
 *
 * xHCI ホストコントローラ制御用クラス．
 */

#pragma once

#include <memory>
#include "error.hpp"
#include "registers.hpp"
#include "context.hpp"
#include "ring.hpp"
#include "port.hpp"
#include "devmgr.hpp"

namespace usb::xhci {
  class Controller {
   public:
    Controller(uintptr_t mmio_base);
    kError Initialize();
    kError Run();
    Ring* CommandRing() { return &cr_; }
    EventRing* PrimaryEventRing() { return &er_; }
    DoorbellRegister* DoorbellRegisterAt(uint8_t index);
    Port PortAt(uint8_t port_num) {
      return Port{port_num, PortRegisterSets()[port_num - 1]};
    }
    uint8_t MaxPorts() const { return max_ports_; }
    DeviceManager* DeviceManager() { return &devmgr_; }

   private:
    static const size_t kDeviceSize = 8;

    const uintptr_t mmio_base_;
    CapabilityRegisters* const cap_;
    OperationalRegisters* const op_;
    const uint8_t max_ports_;

    class DeviceManager devmgr_;
    Ring cr_;
    EventRing er_;

    InterrupterRegisterSetArray InterrupterRegisterSets() const {
      return {mmio_base_ + cap_->RTSOFF.Read().Offset() + 0x20u, 1024};
    }

    PortRegisterSetArray PortRegisterSets() const {
      return {reinterpret_cast<uintptr_t>(op_) + 0x400u, max_ports_};
    }

    DoorbellRegisterArray DoorbellRegisters() const {
      return {mmio_base_ + cap_->DBOFF.Read().Offset(), 256};
    }
  };

  kError ConfigurePort(Controller& xhc, Port& port);
  kError ConfigureEndpoints(Controller& xhc, Device& dev);

  /** @brief イベントリングに登録されたイベントを高々1つ処理する．
   *
   * xhc のプライマリイベントリングの先頭のイベントを処理する．
   * イベントが無ければ即座に kError::kSuccess を返す．
   *
   * @return イベントを正常に処理できたら kError::kSuccess
   */
  kError ProcessEvent(Controller& xhc);

  extern Controller* controller;
  void Initialize();
  void ProcessEvents();
}

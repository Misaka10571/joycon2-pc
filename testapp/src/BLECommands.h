#pragma once
// BLE Commands for Joy-Con / Pro Controller communication
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Devices.Bluetooth.GenericAttributeProfile.h>
#include <vector>
#include <thread>
#include <chrono>

using namespace winrt;
using namespace Windows::Devices::Bluetooth::GenericAttributeProfile;
using namespace Windows::Storage::Streams;

inline void SendGenericCommand(GattCharacteristic const& characteristic, uint8_t cmdId, uint8_t subCmdId, const std::vector<uint8_t>& data) {
    if (!characteristic) return;

    DataWriter writer;
    writer.WriteByte(cmdId);
    writer.WriteByte(0x91);
    writer.WriteByte(0x01);
    writer.WriteByte(subCmdId);
    writer.WriteByte(0x00);
    writer.WriteByte(static_cast<uint8_t>(data.size()));
    writer.WriteByte(0x00);
    writer.WriteByte(0x00);
    for (uint8_t b : data) writer.WriteByte(b);

    IBuffer buffer = writer.DetachBuffer();
    characteristic.WriteValueAsync(buffer, GattWriteOption::WriteWithoutResponse).get();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

inline void SendCustomCommands(GattCharacteristic const& characteristic) {
    std::vector<std::vector<uint8_t>> commands = {
        { 0x0c, 0x91, 0x01, 0x02, 0x00, 0x04, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00 },
        { 0x0c, 0x91, 0x01, 0x04, 0x00, 0x04, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00 }
    };

    for (const auto& cmd : commands) {
        auto writer = DataWriter();
        writer.WriteBytes(cmd);
        IBuffer buffer = writer.DetachBuffer();
        characteristic.WriteValueAsync(buffer, GattWriteOption::WriteWithoutResponse).get();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

inline void EmitSound(GattCharacteristic const& characteristic) {
    std::vector<uint8_t> data(8, 0x00);
    data[0] = 0x04;
    SendGenericCommand(characteristic, 0x0A, 0x02, data);
}

inline void SetPlayerLEDs(GattCharacteristic const& characteristic, uint8_t pattern) {
    std::vector<uint8_t> data(8, 0x00);
    data[0] = pattern;
    SendGenericCommand(characteristic, 0x09, 0x07, data);
}

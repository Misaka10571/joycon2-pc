#pragma once
// DeviceManager - Async BLE scanning replacing blocking WaitForJoyCon
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Devices.Bluetooth.h>
#include <winrt/Windows.Devices.Bluetooth.Advertisement.h>
#include <winrt/Windows.Devices.Bluetooth.GenericAttributeProfile.h>
#include <vector>
#include <mutex>
#include <atomic>
#include <functional>
#include <thread>

using namespace winrt;
using namespace Windows::Devices::Bluetooth;
using namespace Windows::Devices::Bluetooth::Advertisement;
using namespace Windows::Devices::Bluetooth::GenericAttributeProfile;
using namespace Windows::Storage::Streams;
using namespace Windows::Foundation;

constexpr uint16_t JOYCON_MANUFACTURER_ID = 1363;
inline const std::vector<uint8_t> JOYCON_MANUFACTURER_PREFIX = { 0x01, 0x00, 0x03, 0x7E };
inline const wchar_t* INPUT_REPORT_UUID_STR = L"ab7de9be-89fe-49ad-828f-118f09df7fd2";
inline const wchar_t* WRITE_COMMAND_UUID_STR = L"649d4ac9-8eb7-4e6c-af44-1ea54fe5f005";

struct ConnectedJoyCon {
    BluetoothLEDevice device = nullptr;
    GattCharacteristic inputChar = nullptr;
    GattCharacteristic writeChar = nullptr;
};

enum class ScanState { Idle, Scanning, Found, Error, Timeout };

class DeviceManager {
public:
    static DeviceManager& Instance() {
        static DeviceManager inst;
        return inst;
    }

    using ScanCallback = std::function<void(ConnectedJoyCon, ScanState)>;

    ScanState GetScanState() const { return state.load(); }

    void StartScan(ScanCallback callback) {
        if (state.load() == ScanState::Scanning) return;
        
        state.store(ScanState::Scanning);
        scanCallback = callback;

        // Run scanning in background thread so UI stays responsive
        if (scanThread.joinable()) scanThread.join();
        scanThread = std::thread([this]() {
            RunScan();
        });
    }

    void StopScan() {
        cancelScan.store(true);
        if (scanThread.joinable()) scanThread.join();
        state.store(ScanState::Idle);
    }

    ~DeviceManager() {
        StopScan();
    }

private:
    DeviceManager() = default;

    void RunScan() {
        cancelScan.store(false);
        ConnectedJoyCon cj{};
        BluetoothLEDevice device = nullptr;
        std::atomic<bool> connected{ false };

        BluetoothLEAdvertisementWatcher watcher;
        std::mutex mtx;
        std::condition_variable cv;

        watcher.Received([&](auto const&, auto const& args) {
            if (connected.load(std::memory_order_acquire)) return;
            if (cancelScan.load()) return;

            auto mfg = args.Advertisement().ManufacturerData();
            for (uint32_t i = 0; i < mfg.Size(); i++) {
                auto section = mfg.GetAt(i);
                if (section.CompanyId() != JOYCON_MANUFACTURER_ID) continue;
                auto reader = DataReader::FromBuffer(section.Data());
                std::vector<uint8_t> data(reader.UnconsumedBufferLength());
                reader.ReadBytes(data);
                if (data.size() >= JOYCON_MANUFACTURER_PREFIX.size() &&
                    std::equal(JOYCON_MANUFACTURER_PREFIX.begin(), JOYCON_MANUFACTURER_PREFIX.end(), data.begin())) {
                    
                    bool expected = false;
                    if (!connected.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
                        return;

                    BluetoothLEDevice dev = BluetoothLEDevice::FromBluetoothAddressAsync(args.BluetoothAddress()).get();
                    if (!dev) {
                        connected.store(false, std::memory_order_release);
                        return;
                    }

                    {
                        std::lock_guard<std::mutex> lock(mtx);
                        device = dev;
                    }
                    cv.notify_one();
                    return;
                }
            }
        });

        watcher.ScanningMode(BluetoothLEScanningMode::Active);
        watcher.Start();

        {
            std::unique_lock<std::mutex> lock(mtx);
            if (!cv.wait_for(lock, std::chrono::seconds(30), [&]() { 
                return connected.load(std::memory_order_acquire) || cancelScan.load(); 
            })) {
                watcher.Stop();
                state.store(ScanState::Timeout);
                if (scanCallback) scanCallback(ConnectedJoyCon{}, ScanState::Timeout);
                return;
            }
        }
        watcher.Stop();

        if (cancelScan.load()) {
            state.store(ScanState::Idle);
            return;
        }

        cj.device = device;

        // Discover GATT services
        auto servicesResult = device.GetGattServicesAsync().get();
        if (servicesResult.Status() != GattCommunicationStatus::Success) {
            state.store(ScanState::Error);
            if (scanCallback) scanCallback(ConnectedJoyCon{}, ScanState::Error);
            return;
        }

        for (auto service : servicesResult.Services()) {
            auto charsResult = service.GetCharacteristicsAsync().get();
            if (charsResult.Status() != GattCommunicationStatus::Success) continue;
            for (auto characteristic : charsResult.Characteristics()) {
                if (characteristic.Uuid() == guid(INPUT_REPORT_UUID_STR))
                    cj.inputChar = characteristic;
                else if (characteristic.Uuid() == guid(WRITE_COMMAND_UUID_STR))
                    cj.writeChar = characteristic;
            }
        }

        // Request lowest connection interval (7.5ms) for minimal input lag
        try {
            auto connectionParams = BluetoothLEPreferredConnectionParameters::ThroughputOptimized();
            cj.device.RequestPreferredConnectionParameters(connectionParams);
        } catch (...) {}

        state.store(ScanState::Found);
        if (scanCallback) scanCallback(cj, ScanState::Found);
    }

    std::atomic<ScanState> state{ ScanState::Idle };
    std::atomic<bool> cancelScan{ false };
    ScanCallback scanCallback;
    std::thread scanThread;
};

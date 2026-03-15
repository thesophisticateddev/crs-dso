#include "UsbTransport.h"
#include <cstring>

UsbTransport::UsbTransport() {
    int ret = libusb_init(&m_ctx);
    if (ret != LIBUSB_SUCCESS) {
        m_lastError = "Failed to initialize libusb: "
                    + std::string(libusb_strerror(static_cast<libusb_error>(ret)));
        m_state = TransportState::Error;
    }
}

UsbTransport::~UsbTransport() {
    close();
    if (m_ctx) {
        libusb_exit(m_ctx);
        m_ctx = nullptr;
    }
}

bool UsbTransport::open(const DeviceInfo& device) {
    if (m_state == TransportState::Connected) {
        close();
    }

    m_state = TransportState::Connecting;

    // Get device list
    libusb_device** devList = nullptr;
    ssize_t cnt = libusb_get_device_list(m_ctx, &devList);
    if (cnt < 0) {
        m_lastError = "Failed to get device list";
        m_state = TransportState::Error;
        return false;
    }

    libusb_device* targetDev = nullptr;
    for (ssize_t i = 0; i < cnt; i++) {
        libusb_device_descriptor desc;
        if (libusb_get_device_descriptor(devList[i], &desc) != 0)
            continue;

        if (desc.idVendor == device.vendorId && desc.idProduct == device.productId) {
            // If serial is specified, check it
            if (!device.serial.empty()) {
                libusb_device_handle* tmpHandle = nullptr;
                if (libusb_open(devList[i], &tmpHandle) == LIBUSB_SUCCESS) {
                    unsigned char serialBuf[256] = {0};
                    if (desc.iSerialNumber > 0) {
                        libusb_get_string_descriptor_ascii(
                            tmpHandle, desc.iSerialNumber, serialBuf, sizeof(serialBuf));
                    }
                    libusb_close(tmpHandle);
                    if (device.serial == reinterpret_cast<const char*>(serialBuf)) {
                        targetDev = devList[i];
                        break;
                    }
                }
            } else {
                targetDev = devList[i];
                break;
            }
        }
    }

    if (!targetDev) {
        libusb_free_device_list(devList, 1);
        m_lastError = "Device not found (VID=0x"
                    + std::to_string(device.vendorId)
                    + " PID=0x" + std::to_string(device.productId) + ")";
        m_state = TransportState::Disconnected;
        return false;
    }

    int ret = libusb_open(targetDev, &m_handle);
    libusb_free_device_list(devList, 1);

    if (ret != LIBUSB_SUCCESS) {
        m_lastError = "Failed to open device: "
                    + std::string(libusb_strerror(static_cast<libusb_error>(ret)));
        m_state = TransportState::Error;
        return false;
    }

    if (!claimInterface()) {
        return false;
    }

    m_state = TransportState::Connected;
    return true;
}

bool UsbTransport::claimInterface() {
    // Cross-platform: auto-detach kernel drivers
    // Works on Linux, silently no-ops on Windows/macOS
    libusb_set_auto_detach_kernel_driver(m_handle, 1);

    int ret = libusb_claim_interface(m_handle, 0);
    if (ret != LIBUSB_SUCCESS) {
#ifdef _WIN32
        if (ret == LIBUSB_ERROR_NOT_SUPPORTED) {
            m_lastError = "WinUSB driver not installed. Run the driver "
                          "installer from the application directory, or "
                          "use Zadig to install the WinUSB driver.";
        } else
#endif
#ifdef __linux__
        if (ret == LIBUSB_ERROR_ACCESS) {
            m_lastError = "USB permission denied. Install the udev rule:\n"
                          "  sudo cp 99-crs-dso.rules /etc/udev/rules.d/\n"
                          "  sudo udevadm control --reload-rules\n"
                          "Then unplug and replug the device.";
        } else
#endif
#ifdef __APPLE__
        if (ret == LIBUSB_ERROR_ACCESS) {
            m_lastError = "USB access denied. This may be caused by a "
                          "kernel driver conflict. Ensure the device uses "
                          "vendor-specific USB class (0xFF), or try running "
                          "with sudo.";
        } else
#endif
        {
            m_lastError = "Failed to claim USB interface: "
                        + std::string(libusb_strerror(static_cast<libusb_error>(ret)));
        }
        libusb_close(m_handle);
        m_handle = nullptr;
        m_state = TransportState::Error;
        return false;
    }

    return true;
}

void UsbTransport::close() {
    if (m_handle) {
        libusb_release_interface(m_handle, 0);
        libusb_close(m_handle);
        m_handle = nullptr;
    }
    m_state = TransportState::Disconnected;
}

TransportState UsbTransport::state() const {
    return m_state;
}

int UsbTransport::write(const uint8_t* data, size_t length, unsigned int timeoutMs) {
    if (!m_handle || m_state != TransportState::Connected) {
        m_lastError = "Not connected";
        return -1;
    }

    int transferred = 0;
    int ret = libusb_bulk_transfer(m_handle, EP_BULK_OUT,
                                   const_cast<uint8_t*>(data),
                                   static_cast<int>(length),
                                   &transferred, timeoutMs);

    if (ret != LIBUSB_SUCCESS && ret != LIBUSB_ERROR_TIMEOUT) {
        m_lastError = "Write failed: "
                    + std::string(libusb_strerror(static_cast<libusb_error>(ret)));
        return -1;
    }

    return transferred;
}

int UsbTransport::read(uint8_t* buffer, size_t maxLength, unsigned int timeoutMs) {
    if (!m_handle || m_state != TransportState::Connected) {
        m_lastError = "Not connected";
        return -1;
    }

    int transferred = 0;
    int ret = libusb_bulk_transfer(m_handle, EP_BULK_IN,
                                   buffer,
                                   static_cast<int>(maxLength),
                                   &transferred, timeoutMs);

    if (ret != LIBUSB_SUCCESS && ret != LIBUSB_ERROR_TIMEOUT) {
        m_lastError = "Read failed: "
                    + std::string(libusb_strerror(static_cast<libusb_error>(ret)));
        return -1;
    }

    return transferred;
}

std::vector<DeviceInfo> UsbTransport::enumerate() {
    std::vector<DeviceInfo> devices;

    if (!m_ctx) return devices;

    libusb_device** devList = nullptr;
    ssize_t cnt = libusb_get_device_list(m_ctx, &devList);
    if (cnt < 0) return devices;

    for (ssize_t i = 0; i < cnt; i++) {
        libusb_device_descriptor desc;
        if (libusb_get_device_descriptor(devList[i], &desc) != 0)
            continue;

        // Only list vendor-specific devices (class 0xFF)
        // Adjust this filter based on your hardware's VID/PID
        if (desc.bDeviceClass != 0xFF && desc.bDeviceClass != 0x00)
            continue;

        DeviceInfo info;
        info.vendorId = desc.idVendor;
        info.productId = desc.idProduct;

        libusb_device_handle* tmpHandle = nullptr;
        if (libusb_open(devList[i], &tmpHandle) == LIBUSB_SUCCESS) {
            unsigned char buf[256] = {0};

            if (desc.iSerialNumber > 0) {
                libusb_get_string_descriptor_ascii(tmpHandle, desc.iSerialNumber, buf, sizeof(buf));
                info.serial = reinterpret_cast<const char*>(buf);
            }
            if (desc.iManufacturer > 0) {
                libusb_get_string_descriptor_ascii(tmpHandle, desc.iManufacturer, buf, sizeof(buf));
                info.manufacturer = reinterpret_cast<const char*>(buf);
            }
            if (desc.iProduct > 0) {
                libusb_get_string_descriptor_ascii(tmpHandle, desc.iProduct, buf, sizeof(buf));
                info.product = reinterpret_cast<const char*>(buf);
            }

            libusb_close(tmpHandle);
        }

        devices.push_back(std::move(info));
    }

    libusb_free_device_list(devList, 1);
    return devices;
}

std::string UsbTransport::lastError() const {
    return m_lastError;
}

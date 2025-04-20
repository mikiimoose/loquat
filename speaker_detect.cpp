#include "speaker_detect.h"

// Example vendor and product IDs - you'll need to replace these with your specific speaker values
#define SPEAKER_VENDOR_ID  0x4c4a   // Replace with your speaker's vendor ID
#define SPEAKER_PRODUCT_ID 0x4155   // Replace with your speaker's product ID

int speaker_init() {
    libusb_device **devices;
    libusb_device *device;
    libusb_context *context = NULL;
    struct libusb_device_descriptor desc;
    ssize_t device_count;
    int found = 0;
    int result;

    // Initialize libusb
    result = libusb_init(&context);
    if (result < 0) {
        log_message(LOG_ERR, "Failed to initialize libusb: %s\n", libusb_error_name(result));
        return 0;
    }

    // Get list of USB devices
    device_count = libusb_get_device_list(context, &devices);
    if (device_count < 0) {
        log_message(LOG_ERR, "Failed to get device list: %s\n", libusb_error_name(device_count));
        libusb_exit(context);
        return 0;
    }

    // Iterate through devices
    int i = 0;
    while ((device = devices[i++]) != NULL) {
        result = libusb_get_device_descriptor(device, &desc);
        if (result < 0) {
            log_message(LOG_ERR, "Failed to get device descriptor: %s\n", libusb_error_name(result));
            continue;
        }

        // Check if this device matches our speaker's vendor and product IDs
        if (desc.idVendor == SPEAKER_VENDOR_ID && desc.idProduct == SPEAKER_PRODUCT_ID) {
            found = 1;
            log_message(LOG_INFO, "Speaker found!\n");
            log_message(LOG_INFO, "Bus: %d, Device: %d\n",
                   libusb_get_bus_number(device),
                   libusb_get_device_address(device));
            
            // Get more detailed information
            libusb_device_handle *handle;
            result = libusb_open(device, &handle);
            if (result >= 0) {
                unsigned char string[256];
                
                // Try to get product string
                if (desc.iProduct) {
                    result = libusb_get_string_descriptor_ascii(handle, desc.iProduct, string, sizeof(string));
                    if (result > 0) {
                        log_message(LOG_INFO, "Product: %s\n", string);
                    }
                }
                
                // Try to get manufacturer string
                if (desc.iManufacturer) {
                    result = libusb_get_string_descriptor_ascii(handle, desc.iManufacturer, string, sizeof(string));
                    if (result > 0) {
                        log_message(LOG_INFO, "Manufacturer: %s\n", string);
                    }
                }
                
                libusb_close(handle);
            }
        }
    }

    // Clean up
    libusb_free_device_list(devices, 1);
    libusb_exit(context);

    if (!found) {
        log_message(LOG_ERR, "Speaker not found\n");
    }

    return found;
} 
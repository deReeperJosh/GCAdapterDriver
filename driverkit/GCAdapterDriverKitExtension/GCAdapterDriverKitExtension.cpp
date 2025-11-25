//
//  GCAdapterDriverKitExtension.cpp
//  GCAdapterDriverKitExtension
//
//  Created by Ryan McGrath on 6/15/21.
//

#include <os/log.h>

#include <DriverKit/IOUserServer.h>
#include <DriverKit/IOLib.h>

#include <USBDriverKit/USBDriverKit.h>

#include "GCAdapterDriverKitExtension.h"

#define Log(fmt, ...) os_log(OS_LOG_DEFAULT, "GCAdapterDriver - " fmt "\n", ##__VA_ARGS__)

#define __Require(assertion, exceptionLabel)                    \
do {                                                            \
    if ( __builtin_expect(!(assertion), 0) ) {                  \
        goto exceptionLabel;                                    \
    }                                                           \
} while (0)

#define __Require_Action(assertion, exceptionLabel, action)     \
do {                                                            \
    if ( __builtin_expect(!(assertion), 0) ) {                  \
        {                                                       \
            action;                                             \
        }                                                       \
        goto exceptionLabel;                                    \
    }                                                           \
} while (0)

kern_return_t
IMPL(GCAdapterDriverKitExtension, Start)
{
    kern_return_t ret;
    Log("Starting");
    ret = Start(provider, SUPERDISPATCH);

    if (ret != kIOReturnSuccess) {
        Stop(provider, SUPERDISPATCH);
        Log("Failed to start");
        return ret;
    }


    // Perform startup tasks...
    IOUSBHostInterface *interface = OSDynamicCast(IOUSBHostInterface, provider);
    IOUSBHostDevice *device;
    ret = interface->CopyDevice(&device);
    if (ret != kIOReturnSuccess) {
        Stop(provider, SUPERDISPATCH);
        Log("Failed to copy device");
        return ret;
    }
    
    const IOUSBDeviceDescriptor *deviceDesc;
    deviceDesc = device->CopyDeviceDescriptor();
    if (deviceDesc == NULL) {
        device->release();
        Stop(provider, SUPERDISPATCH);
        Log("Failed to copy device descriptor");
        return kIOReturnError;
    }

    if (deviceDesc->idVendor == 0x1430 && deviceDesc->idProduct == 0x0150) {
        Log("Found Skylander Portal for use");
    }
    
    device->release();

    // Register the service with the system.
    RegisterService();
    
    return ret;
}

kern_return_t
IMPL(GCAdapterDriverKitExtension, Stop)
{
    kern_return_t ret;
    ret = Stop(provider, SUPERDISPATCH);
    Log("GCAdapterDriver shutting down");
    return ret;
}

// Given a USB interface, walks the endpoints and attempts to update them to poll at a more
// preferable rate.
//
// Also accepts a desired interval to try and set the device (adapter) to.
// The general math is: divide 1000 by bInterval. Want 1000hz? 1. Want 500hz? 2. Etc.
//void updateInterval(IOUSBHostInterface *interface, uint8_t desired_interval)
//{
//    const IOUSBConfigurationDescriptor *configDescriptor = interface->CopyConfigurationDescriptor();
//    const IOUSBInterfaceDescriptor *interfaceDescriptor = interface->GetInterfaceDescriptor(configDescriptor);
//
//    const IOUSBEndpointDescriptor *endpointDescriptor = NULL;
//
//    while((
//        endpointDescriptor = IOUSBGetNextEndpointDescriptor(
//            configDescriptor,
//            interfaceDescriptor,
//            (IOUSBDescriptorHeader*)endpointDescriptor
//        )
//    ) != NULL) {
//        if(endpointDescriptor->bEndpointAddress == 0x81 || endpointDescriptor->bEndpointAddress == 0x82) {
//            IOUSBHostPipe *pipe = NULL;
//            interface->CopyPipe(endpointDescriptor->bEndpointAddress, &pipe);
//
//            if(pipe) {
//                os_log(OS_LOG_DEFAULT, "Acquired pipe, will attempt to update bInterval");
//
//                IOUSBStandardEndpointDescriptors *descriptors = NULL;
//                pipe->GetDescriptors(descriptors, kIOUSBGetEndpointDescriptorOriginal);
//
//                if(descriptors) {
//                    descriptors->descriptor.bInterval = desired_interval;
//                    IOReturn result = pipe->AdjustPipe(descriptors);
//                    os_log(OS_LOG_DEFAULT, "Status: %d", result);
//                }
//
//                pipe->release();
//            } else {
//                os_log(OS_LOG_DEFAULT, "Unable to acquire pipe to modify interval!");
//            }
//        }
//    }
//}

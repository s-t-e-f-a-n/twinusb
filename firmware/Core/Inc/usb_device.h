/*
 * usb_device.h
 *
 *  Created on: 22.05.2026
 *      Author: s-t-e-f-a-n
 */

#ifndef __USB_DEVICE_H__
#define __USB_DEVICE_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DEBUG
  #define MX_USB_Device_Init() (void)0
#else
  void MX_USB_Device_Init(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* INC_USB_DEVICE_H_ */

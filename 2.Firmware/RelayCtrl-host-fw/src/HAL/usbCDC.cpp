#include <CircularBuffer.h>
#include <Arduino.h>
#include <assert.h>
#include "esp_system.h"
#include "esp_log.h"

extern "C"
{
#include "esp_usbh_cdc.h"
}

/* Apply for a fifo buffer */
CircularBuffer<uint8_t, 2048> usb_buff;

static const char *TAG = "cdc_basic_demo";
/* USB PIN fixed in esp32-s2/s3, can not use io matrix */
#define BOARD_USB_DP_PIN 20
#define BOARD_USB_DN_PIN 19

/* ringbuffer size */
#define IN_RINGBUF_SIZE (1024)
#define OUT_RINGBUF_SIZE (1024 + 128)

/* bulk endpoint address */
#define EXAMPLE_BULK_IN_EP_ADDR 0x81
#define EXAMPLE_BULK_OUT_EP_ADDR 0x01
/* bulk endpoint max package size */
#define EXAMPLE_BULK_EP_MPS 64
/* bulk endpoint transfer interval */
#define EXAMPLE_BULK_EP_INTERVAL 0

/* choose if use user endpoint descriptors */
// #define EXAMPLE_CONFIG_USER_EP_DESC

#ifdef EXAMPLE_CONFIG_USER_EP_DESC
/*
the basic demo skip the standred get descriptors process,
users need to get params from cdc device descriptors from PC side,
eg. run `lsusb -v` in linux, then hardcode the related params below
*/

static usb_ep_desc_t bulk_out_ep_desc = {
    .bLength = sizeof(usb_ep_desc_t),
    .bDescriptorType = USB_B_DESCRIPTOR_TYPE_ENDPOINT,
    .bEndpointAddress = EXAMPLE_BULK_OUT_EP_ADDR,
    .bmAttributes = USB_BM_ATTRIBUTES_XFER_BULK,
    .wMaxPacketSize = EXAMPLE_BULK_EP_MPS,
    .bInterval = EXAMPLE_BULK_EP_INTERVAL,
};

static usb_ep_desc_t bulk_in_ep_desc = {
    .bLength = sizeof(usb_ep_desc_t),
    .bDescriptorType = USB_B_DESCRIPTOR_TYPE_ENDPOINT,
    .bEndpointAddress = EXAMPLE_BULK_IN_EP_ADDR,
    .bmAttributes = USB_BM_ATTRIBUTES_XFER_BULK,
    .wMaxPacketSize = EXAMPLE_BULK_EP_MPS,
    .bInterval = EXAMPLE_BULK_EP_INTERVAL,
};

#endif

static void usb_receive_task(void *param)
{
    size_t data_len = 0;
    uint8_t buf[IN_RINGBUF_SIZE];

    while (true)
    {
        /* Polling USB receive buffer to get data */
        usbh_cdc_get_buffered_data_len(&data_len);

        if (data_len != 0)
        {
            usbh_cdc_read_bytes(buf, data_len, 10);

            for (size_t i = 0; i < data_len; i++)
            {
                usb_buff.push(buf[i]);
            }

            // Serial.printf("Receive len=%d: %.*s\r\n", data_len, data_len, buf);
        }
        vTaskDelay(10);
    }
}

static void usb_connect_callback(void *arg)
{
    log_i("Device Connected!");
}

static void usb_disconnect_callback(void *arg)
{
    log_w("Device Disconnected!");
}

void usb_init()
{
    /* @brief install usbh cdc driver with bulk endpoint configs
    and size of internal ringbuffer*/
#ifdef EXAMPLE_CONFIG_USER_EP_DESC
    ESP_LOGI(TAG, "using user bulk endpoint descriptor");
    static usbh_cdc_config_t config = {
        /* use user endpoint descriptor */
        .bulk_in_ep = &bulk_in_ep_desc,
        .bulk_out_ep = &bulk_out_ep_desc,
        .rx_buffer_size = IN_RINGBUF_SIZE,
        .tx_buffer_size = OUT_RINGBUF_SIZE,
        .conn_callback = usb_connect_callback,
        .disconn_callback = usb_disconnect_callback,
    };
#else
    ESP_LOGI(TAG, "using default bulk endpoint descriptor");
    static usbh_cdc_config_t config = {
        /* use default endpoint descriptor with user address */
        .bulk_in_ep_addr = EXAMPLE_BULK_IN_EP_ADDR,
        .bulk_out_ep_addr = EXAMPLE_BULK_OUT_EP_ADDR,
        .rx_buffer_size = IN_RINGBUF_SIZE,
        .tx_buffer_size = OUT_RINGBUF_SIZE,
        .conn_callback = usb_connect_callback,
        .disconn_callback = usb_disconnect_callback,
    };
#endif
    /* install USB host CDC driver */
    esp_err_t ret = usbh_cdc_driver_install(&config);
    if (ret != ESP_OK)
    {
        log_e("cdc install failed");
        esp_restart();
    }
    /* Waitting for USB device connected */
    ret = usbh_cdc_wait_connect(portMAX_DELAY);
    if (ret != ESP_OK)
    {
        log_e("cdc wait connect failed");
        esp_restart();
    }
    /* Create a task for USB data processing */
    xTaskCreate(usb_receive_task, "usb_rx", 4096, NULL, 2, NULL);
}

int usb_send_data(uint8_t *buf, uint16_t len)
{
    return usbh_cdc_write_bytes(buf, len);
}

int usb_read_data(uint8_t *buf, uint16_t len)
{
    uint16_t read_len = 0;

    if (usb_buff.isEmpty())
    {
        return 0;
    }

    len > usb_buff.size() ? read_len = usb_buff.size() : read_len = len;

    for (uint16_t i = 0; i < read_len; i++)
    {
        buf[i] = usb_buff.shift();
    }

    return read_len;
}

int usb_avaulable()
{
    if (usb_buff.isEmpty())
    {
        return 0;
    }

    return usb_buff.size();
}

void usb_cache_clear()
{
    usb_buff.clear();
}
// Copyright (c) 2011, XMOS Ltd, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include <syscall.h>
#include <platform.h>
#include <xs1.h>
#include <xclib.h>
#include <print.h>
#include <stdio.h>

#include "xud.h"
#include "usb.h"

/* Endpoint type tables */
XUD_EpType epTypeTableOut[2] =   {XUD_EPTYPE_CTL, 
                                  XUD_EPTYPE_BUL, 
}; 

XUD_EpType epTypeTableIn[3] = {   XUD_EPTYPE_CTL,
                                  XUD_EPTYPE_BUL, 
                                  XUD_EPTYPE_BUL,
};

/* USB Port declarations */
out port p_usb_rst       = XS1_PORT_1I; // XDK PORT_1B, XTR: 1k
clock    clk             = XS1_CLKBLK_3;
clock    clkPins         = XS1_CLKBLK_4;

void Endpoint0(chanend c_ep0_out, chanend c_ep0_in);

#define BUF_WORDS 512  // Buffer length
#define NUM_BUFFERS 16

#define USB_HOST_BUF_WORDS 128

extern unsigned int data_buffer[NUM_BUFFERS][BUF_WORDS/4];
extern unsigned char data_buffer_[NUM_BUFFERS][BUF_WORDS];

#pragma unsafe arrays
void buffer_thread(chanend from_usb, streaming chanend xlink_data) {
    unsigned int xlink_value;
    unsigned usb_signal;
    unsigned write_ptr = 0;
    int buf_num = 0;
    int read_num = 0;
    int asked = 0;
    
    while (1) {
        select {
        case inuint_byref(from_usb, usb_signal):
            if (!usb_signal) {
                return;
            }
            if (buf_num == read_num) {
                asked = 1;
            } else {
                outuchar(from_usb, read_num);
                read_num = read_num + 1;
                if (read_num == NUM_BUFFERS)
                    read_num = 0;
            }
            break;
            
        case xlink_data :> xlink_value:
            data_buffer[buf_num][write_ptr] = xlink_value;
            write_ptr++;
            if (write_ptr >= BUF_WORDS/4) {
                if (asked) {
                    asked = 0;
                    outuchar(from_usb, buf_num);
                } else {
                    buf_num = buf_num + 1;
                    if (buf_num == NUM_BUFFERS)
                        buf_num = 0;
                }
                write_ptr = 0;
            }
            break;
        }
    }
}

int from_host_buf_uart[USB_HOST_BUF_WORDS];

void endpoint2(chanend to_host, chanend to_uart) {
    unsigned char buf_num;
    int datalength;

    XUD_ep ep_to_host = XUD_Init_Ep(to_host);
    
    outuint(to_uart, 1);
    while (1) {
        inuchar_byref(to_uart, buf_num);
        datalength = XUD_SetBuffer(ep_to_host, data_buffer_[(int)buf_num], USB_HOST_BUF_WORDS*4);
        outuint(to_uart, 1);

        if (datalength < 0) {
            XUD_ResetEndpoint(ep_to_host, null);
        }
    }
}

void endpoint1_configuration(chanend from_host, chanend to_host, clock clk) {
    unsigned char buffer[512];
    int div;

    XUD_ep ep_from_host = XUD_Init_Ep(from_host);
    XUD_ep ep_to_host = XUD_Init_Ep(to_host);
    
    while (1) {
        int datalength = XUD_GetBuffer(ep_from_host, buffer);
        if (datalength > 0) {
            switch(buffer[0]) {
            case 0:         // set divider
                div = buffer[1] | buffer[2]<<8;
                stop_clock(clk);
                configure_clock_ref(clk, div);
                start_clock(clk);
                datalength = 0;
                break;
            default:        // unknown command, ignore without ack
                datalength = 0;
                break;
            }
            if (datalength) {
                datalength = XUD_SetBuffer(ep_to_host, buffer, datalength);
            }
        }

        if (datalength < 0) {
            XUD_ResetEndpoint(ep_from_host, ep_to_host);
        }
    }
}


static int lookuplow[256];
static int lookuphigh[256];

/*
 * These port declarations are specific to the XTAG2, the pins on the XK-1
 * are pins 0-7 in order, starting at pin 3 on the connector, follow the
 * top row all the way to pin 17. Use any of pins 4, 8, 12, 16, or 20 as
 * ground.
 */
buffered in port:8 oh = XS1_PORT_1L; // Pin 3 on the connector
buffered in port:8 og = XS1_PORT_1A; // Pin 5 on the connector
buffered in port:8 of = XS1_PORT_1C; // Pin 7 on the connector
buffered in port:8 oe = XS1_PORT_1D; // Pin 9 on the connector

buffered in port:8 od = XS1_PORT_1K; // Pin 11 on the connector
buffered in port:8 oc = XS1_PORT_1B; // Pin 13 on the connector
buffered in port:8 ob = XS1_PORT_1M; // Pin 15 on the connector
buffered in port:8 oa = XS1_PORT_1J; // Pin 17 on the connector

extern void sampler(buffered in port:8 a, buffered in port:8 b, buffered in port:8 c, buffered in port:8 d, buffered in port:8 e, buffered in port:8 f, buffered in port:8 g, buffered in port:8 h, const int lookuplow[256], const int lookuphigh[256], streaming chanend k);

int main() {
    chan c_ep_out[2];
    chan c_ep_in[3];
    chan usb_to_uart;
    streaming chan c;
    configure_clock_rate(clkPins, 100, 4);
    set_port_pull_down(oa);
    set_port_pull_down(ob);
    set_port_pull_down(oc);
    set_port_pull_down(od);
    set_port_pull_down(oe);
    set_port_pull_down(of);
    set_port_pull_down(og);
    set_port_pull_down(oh);
    configure_in_port(oa, clkPins);
    configure_in_port(ob, clkPins);
    configure_in_port(oc, clkPins);
    configure_in_port(od, clkPins);
    configure_in_port(oe, clkPins);
    configure_in_port(of, clkPins);
    configure_in_port(og, clkPins);
    configure_in_port(oh, clkPins);
    start_clock(clkPins);
    for(int i = 0; i < 256; i++) {
        int mask = 0, m;
        for(int k = 1, m = 1 ; k < 16; k <<= 1, m <<= 8) {
            if (k & i) mask |= m;
        }
        lookuplow[i] = mask;
        for(int k = 16, m = 1 ; k < 256; k <<= 1, m <<= 8) {
            if (k & i) mask |= m;
        }
        lookuphigh[i] = mask;
    }
    par {
        XUD_Manager( c_ep_out, 2, c_ep_in, 3,
                     null, epTypeTableOut, epTypeTableIn,
                     p_usb_rst, clk, -1, XUD_SPEED_HS, null);  
        Endpoint0( c_ep_out[0], c_ep_in[0]);
        sampler(oa,ob,oc,od,oe,of,og,oh,lookuplow, lookuphigh, c);
        buffer_thread(usb_to_uart, c);
        endpoint2(c_ep_in[2], usb_to_uart);
        endpoint1_configuration(c_ep_out[1], c_ep_in[1], clkPins);
    }

    return 0;
}









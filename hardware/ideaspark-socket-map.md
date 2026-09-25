# Ideaspark socket mapping - A1

Source: original ideaspark-branded product pinout graphic, visually inspected. Local copy: [pinout](references/ideaspark-pinout-source.jpg).

Original image: https://m.media-amazon.com/images/I/71j0W0VM6CL._AC_SL1500_.jpg

View: screen facing viewer, USB connector at bottom. J1 is the left row; J8 is the right row. Pin 1 is nearest USB on both rows. These are separate 15-position sockets, not a consecutively numbered 30-pin connector. Pin numbering in the drawing runs bottom-to-top. Carrier footprint orientation must preserve this view and must not mirror the underside accidentally.

| Pin | J1 left | J8 right |
| --- | --- | --- |
| 1 | VIN | 3V3 |
| 2 | GND | GND |
| 3 | GPIO13 | GPIO15 (LCD CS) |
| 4 | GPIO12 | GPIO2 (LCD DC) |
| 5 | GPIO14 | GPIO4 (LCD reset) |
| 6 | GPIO27 | GPIO16 |
| 7 | GPIO26 | GPIO17 |
| 8 | GPIO25 | GPIO5 |
| 9 | GPIO33 | GPIO18 (LCD clock) |
| 10 | GPIO32 (LCD backlight) | GPIO19 |
| 11 | GPIO35 | GPIO21 |
| 12 | GPIO34 | GPIO3 (USB serial RX) |
| 13 | GPIO39 | GPIO1 (USB serial TX) |
| 14 | GPIO36 | GPIO22 |
| 15 | EN | GPIO23 (LCD MOSI) |

Unused socket contacts carry no-connect markers on the carrier only. This does not disconnect the display or USB bridge already wired on the ideaspark board. Both ground contacts are connected.

The product graphic establishes pin count, order and GPIO labels, but does not specify header-row spacing or pin pitch numerically. No socket footprint or row-spacing value is frozen. VIN is a label, not proof that it provides USB 5 V output: the USB-to-VIN power path and current capacity still require verification before powering the IR emitter from it.

The source webpage transcription incorrectly associated GPIO21 with backlight; the original product graphic explicitly places LCD backlight on GPIO32. The schematic follows the product graphic.

A1 supersedes the A0 logical 14-pin connector. The complete GPIO-to-socket mapping is now recorded in the native schematic and model.

# STM32-digital-logic-analyzer

STM32F411RE firmware for capturing digital inputs and streaming samples to
PulseView/sigrok using the SUMP/OpenBench Logic Sniffer serial protocol.

## Capture hardware

TIM4 update events request DMA1 Stream 6, Channel 2 transfers from `GPIOA->IDR`
to a circular byte buffer. Each byte is one sample of GPIOA pins PA0-PA7.
PA2/PA3 are also used by USART2 for the ST-LINK virtual COM port, so their
captured values reflect serial traffic.

The maximum reported capture is 4096 samples. DMA uses a larger circular
buffer to retain the requested samples while software detects a trigger and
stops the timer. The SUMP divider sets the TIM4 sampling interval based on the
active APB1 timer clock; with the default 16 MHz clock, divider 15 produces a
1 MHz sample rate.

## SUMP serial protocol

USART2 uses 115200 baud, 8 data bits, no parity, and 1 stop bit. Its RX/TX
pins are PA3/PA2. The firmware responds to reset, ID, and metadata requests,
accepts divider and capture-size settings, supports up to four basic
mask/value trigger stages, and streams captured sample bytes in chronological
order. Trigger comparisons are level/value matches; edge and advanced trigger
modes are not implemented. XON/XOFF flow control is supported during capture
transmission.

Do not send debug log text over USART2 while connected to PulseView; the
protocol is binary. `Debug_Log()` remains available for use on a separate
debug transport.

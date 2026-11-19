# STM32-digital-logic-analyzer
STM32-based logic analyzer firmware for digital signal acquisition, event-triggered capture and data streaming to PC applications such as PulseView (sigrok).

## TIMER4 output

`configure_timer4_output_compare()` configures TIM4_CH1 on PB6 to output a 1 MHz square wave in output-compare toggle mode. It derives the timer clock from the active APB1 clock configuration. With the default 16 MHz clock, ARR is 1 and the prescaler is 3:

`f_out = f_TIM4 / (2 * (PSC + 1) * (ARR + 1))`

The compare event enables a TIM4_CH1 DMA request on every transition (2 MHz). The DMA stream/channel itself must be configured separately by the application.

## UART debug logger

The debug logger uses USART2 at 115200 baud, 8 data bits, no parity, and 1 stop bit. On the NUCLEO-F411RE, USART2 is routed to the ST-LINK virtual COM port using PA2 (TX) and PA3 (RX). Call `Debug_Log("message")` to transmit a line ending in CRLF; transmission is blocking.

# STM32-digital-logic-analyzer
STM32-based logic analyzer firmware for digital signal acquisition, event-triggered capture and data streaming to PC applications such as PulseView (sigrok).

## TIMER4 output

`configure_timer4_output_compare()` configures TIM4_CH1 on PB6 to output a 1 MHz square wave in output-compare toggle mode. It derives the timer clock from the active APB1 clock configuration. With the default 16 MHz clock, ARR is 1 and the prescaler is 3:

`f_out = f_TIM4 / (2 * (PSC + 1) * (ARR + 1))`

The compare event enables a TIM4_CH1 DMA request on every transition (2 MHz). The DMA stream/channel itself must be configured separately by the application.

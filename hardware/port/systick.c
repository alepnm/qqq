
#include "systick.h"


uint32_t timestamp = 0;


/*  */
void systick_setup(void)
{
	/* 72MHz / 8 => 9000000 counts per second */
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);

	/* 9000000/9000 = 1000 overflows per second - every 1ms one interrupt */
	/* SysTick interrupt every N clock pulses: set reload to N-1 */
	systick_set_reload(8999);

	systick_interrupt_enable();

	systick_counter_enable();
}


/*  */
void systick_handler(void)
{
    if( systick_get_countflag() ){
        timestamp++;
    }
}

/*  */
void systick_isr_handler(void)
{
    timestamp++;
}


#include "globals.h"
#include "iic.h"

/*  */
extern volatile uint32_t systick;

/*  */
static uint8_t i2c_hw_init_flag = 0;


/*  */
int iic_check( i2c_dev_t *dev )
{
    uint32_t delay = 0, tmp1, tmp2;
    uint8_t trials = 1, state = RESET;

    dev->alive = 0;

    delay = systick + 10;

    while( READ_BIT(I2C_SR2(dev->handle), I2C_SR2_BUSY) != RESET ){

        if(delay < systick) return ERR_TIMEOUT;

        wait();
    }

    CLEAR_BIT(I2C_CR1(dev->handle), I2C_CR1_POS);

    do{

        delay = systick + 10;

        i2c_send_start(dev->handle);

        while( READ_BIT(I2C_SR1(dev->handle), I2C_SR1_SB) == RESET ){

            if(delay < systick) return ERR_TIMEOUT;
            wait();
        }

        delay = systick + 10;

        i2c_send_7bit_address(dev->handle, dev->i2c_addr, I2C_WRITE);

        tmp1 = READ_BIT(I2C_SR1(dev->handle), I2C_SR1_ADDR);
        tmp2 = READ_BIT(I2C_SR1(dev->handle), I2C_SR1_AF);

        while( (tmp1 == RESET) && (tmp2 == RESET) && (state == RESET) )
        {
            if(delay < systick) state = SET;

            tmp1 = READ_BIT(I2C_SR1(dev->handle), I2C_SR1_ADDR);
            tmp2 = READ_BIT(I2C_SR1(dev->handle), I2C_SR1_AF);
            wait();
        }

        delay = systick + 10;

        if( READ_BIT(I2C_SR1(dev->handle), I2C_SR1_ADDR) != RESET ){

            i2c_send_stop(dev->handle);

            /* clear addr flag */
            (void)I2C_SR1(dev->handle);
            (void)I2C_SR2(dev->handle);

            while(READ_BIT(I2C_SR2(dev->handle), I2C_SR2_BUSY) != RESET){

                if(delay < systick) return ERR_ERROR;
                wait();
            }

            dev->alive = 1;

            return ERR_NONE;

        }else{

            i2c_send_stop(dev->handle);

            CLEAR_BIT(I2C_SR1(dev->handle), I2C_SR1_AF);

            while(READ_BIT(I2C_SR2(dev->handle), I2C_SR2_BUSY) != RESET){

                if(delay < systick) return ERR_ERROR;
                wait();
            }

            dev->alive = 0;

            return ERR_ERROR;
        }

    }while(trials--);

    return ERR_NONE;
}


/*
resetina hardwar'a net ir atveju, kai jis pakibo BUSY steite
manualas en.CD00260217.pdf, punktas 2.10.7
*/
static void iic_reset(uint32_t i2c)
{
    uint32_t i2c_port;
    uint16_t i2c_sda_pin, i2c_scl_pin;

    switch(i2c)
    {
    default:
        i2c_port = GPIOB;
        i2c_sda_pin = GPIO_I2C1_SDA;
        i2c_scl_pin = GPIO_I2C1_SCL;
        break;
    case I2C2:
        i2c_port = GPIOB;
        i2c_sda_pin = GPIO_I2C2_SDA;
        i2c_scl_pin = GPIO_I2C2_SCL;
        break;
    }

    i2c_peripheral_disable(i2c);

    gpio_set_mode(i2c_port, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, i2c_sda_pin | i2c_scl_pin);
    gpio_set(i2c_port, i2c_sda_pin | i2c_scl_pin);

    (void)gpio_get(i2c_port, i2c_sda_pin);
    (void)gpio_get(i2c_port, i2c_scl_pin);

    gpio_set_mode(i2c_port, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, i2c_sda_pin);
    gpio_clear(i2c_port, i2c_sda_pin);
    (void)gpio_get(i2c_port, i2c_sda_pin);

    gpio_set_mode(i2c_port, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, i2c_scl_pin);
    gpio_clear(i2c_port, i2c_scl_pin);
    (void)gpio_get(i2c_port, i2c_scl_pin);

    gpio_set_mode(i2c_port, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, i2c_scl_pin);
    gpio_set(i2c_port, i2c_scl_pin);
    (void)gpio_get(i2c_port, i2c_scl_pin);

    gpio_set_mode(i2c_port, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, i2c_sda_pin);
    gpio_set(i2c_port, i2c_sda_pin);
    (void)gpio_get(i2c_port, i2c_sda_pin);

    gpio_set_mode(i2c_port, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN, i2c_sda_pin | i2c_scl_pin);

    i2c_reset(i2c);

    i2c_peripheral_enable(i2c);
}


/*  */
static int iic_send_control_byte( i2c_dev_t *dev, uint16_t reg )
{
    while(I2C_SR2(dev->handle) & I2C_SR2_BUSY) wait();

    i2c_send_start(dev->handle);
    while (!(I2C_SR1(dev->handle) & I2C_SR1_SB)) wait();

    i2c_send_7bit_address(dev->handle, dev->i2c_addr, I2C_WRITE);
    while (!(I2C_SR1(dev->handle) & I2C_SR1_ADDR)) wait();

    (void)I2C_SR1(dev->handle);
    (void)I2C_SR2(dev->handle);

    if(dev->mem_addr_width == MEM_ADDR_WIDTH_16BIT)
    {
        i2c_send_data(dev->handle, (uint8_t)((reg>>8)&0x00FF));
        while (!(I2C_SR1(dev->handle) & (I2C_SR1_BTF))) wait();
    }

    i2c_send_data(dev->handle, (uint8_t)(reg&0x00FF));
    while (!(I2C_SR1(dev->handle) & (I2C_SR1_BTF | I2C_SR1_TxE))) wait();

    return 0;
}

/*  */
int iic_read( i2c_dev_t *dev, uint16_t reg, uint8_t *data, uint16_t len )
{
    iic_send_control_byte(dev, reg);

    i2c_enable_ack(dev->handle);

    i2c_send_start(dev->handle);
    while (!(I2C_SR1(dev->handle) & I2C_SR1_SB)) wait();

    i2c_send_7bit_address(dev->handle, dev->i2c_addr, I2C_READ);
    while (!(I2C_SR1(dev->handle) & I2C_SR1_ADDR)) wait();

    (void)I2C_SR1(dev->handle);
    (void)I2C_SR2(dev->handle);

    while(len-- > 1)
    {
        while (!(I2C_SR1(dev->handle) & I2C_SR1_RxNE)) wait();
        *data++ = i2c_get_data(dev->handle);
    }

    i2c_disable_ack(dev->handle);

    i2c_send_stop(dev->handle);

    while (!(I2C_SR1(dev->handle) & I2C_SR1_RxNE)) wait();
    *data = i2c_get_data(dev->handle);

    return 0;
}

/*  */
int iic_write( i2c_dev_t *dev, uint16_t reg, uint8_t *data, uint16_t len )
{
    iic_send_control_byte(dev, reg);

    i2c_disable_ack(dev->handle);

    while(len--)
    {
        i2c_send_data(dev->handle, *data++);
        while (!(I2C_SR1(dev->handle) & (I2C_SR1_BTF))) wait();
    }

    while (!(I2C_SR1(dev->handle) & (I2C_SR1_BTF | I2C_SR1_TxE))) wait();

    i2c_send_stop(dev->handle);

    return 0;
}




/*  */
int iic_simple_rw( i2c_dev_t *dev, uint8_t *data, uint16_t len, uint8_t rw ){

    while(I2C_SR2(dev->handle) & I2C_SR2_BUSY) wait();

    i2c_send_start(dev->handle);
    while (!(I2C_SR1(dev->handle) & I2C_SR1_SB)) wait();

    i2c_send_7bit_address(dev->handle, dev->i2c_addr, I2C_WRITE);
    while (!(I2C_SR1(dev->handle) & I2C_SR1_ADDR)) wait();

    (void)I2C_SR1(dev->handle);
    (void)I2C_SR2(dev->handle);

    i2c_enable_ack(dev->handle);

    if(rw == I2C_WRITE){

        while(len--)
        {
            i2c_send_data(dev->handle, *data++);
            while (!(I2C_SR1(dev->handle) & (I2C_SR1_BTF))) wait();
        }

        while (!(I2C_SR1(dev->handle) & (I2C_SR1_BTF | I2C_SR1_TxE))) wait();

    }else{

        while(len--)
        {
            while (!(I2C_SR1(dev->handle) & I2C_SR1_RxNE)) wait();
            *data++ = i2c_get_data(dev->handle);
        }
    }

    i2c_send_stop(dev->handle);

    return 0;
}







/*  */
uint8_t iic_get_init_flag(uint32_t i2c)
{
    switch(i2c)
    {
    default:
        return i2c_hw_init_flag;
        break;
    case I2C2:
        return i2c_hw_init_flag;
        break;
    }
}


/*  */
void i2c_hw_init(uint32_t i2c)
{
    if(i2c_hw_init_flag) return;    // i2c hw inicializuotas - iseinam

    iic_reset(i2c);

    i2c_peripheral_disable(i2c);

    i2c_clear_stop(i2c);

    rcc_periph_clock_enable(RCC_AFIO);

    switch(i2c)
    {

    default:
        /* Enable clocks for I2C1 */
        rcc_periph_clock_enable(RCC_I2C1);
        rcc_periph_clock_enable(RCC_GPIOB);

        /* Set alternate functions for the SCL and SDA pins of I2C1. */
        gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN, GPIO_I2C1_SCL | GPIO_I2C1_SDA);
        gpio_set(GPIOB, GPIO_I2C1_SDA | GPIO_I2C1_SCL);

        break;
    case I2C2:
        /* Enable clocks for I2C2 */
        rcc_periph_clock_enable(RCC_I2C2);
        rcc_periph_clock_enable(RCC_GPIOB);

        /* Set alternate functions for the SCL and SDA pins of I2C2. */
        gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN, GPIO_I2C2_SCL | GPIO_I2C2_SDA);
        gpio_set(GPIOB, GPIO_I2C2_SCL | GPIO_I2C2_SDA);

        break;
    }

    i2c_set_speed(i2c, i2c_speed_sm_100k, rcc_apb1_frequency / 1000000);

    i2c_set_dutycycle(i2c, I2C_CCR_DUTY_DIV2);

    i2c_peripheral_enable(i2c);

    i2c_hw_init_flag = 1;
}



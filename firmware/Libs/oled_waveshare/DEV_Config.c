#include "DEV_Config.h"
#include "main.h"

/* Pointer to the external hardware-specific transfer function */
static I2C_Transfer_Callback transfer_cb = NULL;

/*******************************************************************************
function:
    Register the external I2C transfer function
*******************************************************************************/
void DEV_I2C_RegisterCallback(I2C_Transfer_Callback cb)
{
    transfer_cb = cb;
}

/*******************************************************************************
function:
    System initialization
*******************************************************************************/
UBYTE System_Init(void)
{
    return 0;
}

/*******************************************************************************
function:
    Exit system
*******************************************************************************/
void System_Exit(void)
{
    /* No cleanup required */
}

/*******************************************************************************
function:
    Write a byte to the I2C bus using the registered callback
*******************************************************************************/
void I2C_Write_Byte(UBYTE value, UBYTE Cmd)
{
    /* Execute the registered transfer function if it exists */
    if (transfer_cb != NULL) {
        transfer_cb(value, Cmd);
    }
}

/*******************************************************************************
function:
    Delay in milliseconds
*******************************************************************************/
void Driver_Delay_ms(uint32_t xms)
{
    /* Assuming standard HAL_Delay is available globally */
    HAL_Delay(xms);
}

/*******************************************************************************
function:
    Delay in microseconds
*******************************************************************************/
void Driver_Delay_us(uint32_t xus)
{
    /* Simple busy-wait delay */
    uint32_t count = xus * 10;
    while(count--);
}

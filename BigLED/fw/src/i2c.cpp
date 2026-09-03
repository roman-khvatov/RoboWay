#pragma once

/*

I2C communication flow:

1. I2C Slave find its address:
    Wait bit ADDR to 1                                                                                                   LL_I2C_IsActiveFlag_ADDR(I2C1)
    Check bit TRA bit indicates that it is currently in receiver mode or transmitter mode (0 - recieve, 1 - transmit)    LL_I2C_GetTransferDirection(I2C1) -> LL_I2C_DIRECTION_WRITE/LL_I2C_DIRECTION_READ

Transmiting:

1. Wait for ReadyToTransmit
    Wait for bit TxE is set to 1 or BTF is set to 1 -> goto 2          LL_I2C_IsActiveFlag_TXE(I2C1) & LL_I2C_IsActiveFlag_BTF(I2C1)
   or
    If AF is 1:                                                        LL_I2C_IsActiveFlag_AF(I2C1)
        Write 0 to AF to clear                                         LL_I2C_ClearFlag_AF(I2C1)
        Done transfer
   or
    if OVR is 1 or BERR is 1                                           LL_I2C_IsActiveFlag_OVR(I2C1) & LL_I2C_IsActiveFlag_BERR(I2C1)
        This is ERROR - reinit I2C and Done transfer

3. Next byte transit:
    Write byte to DR register                                          LL_I2C_TransmitData8(I2C1)

3. Goto 1

Recieving:

1. Wait for ReadyToRecieve:
    Wait for bit RxNE is set to 1 or BTF is set to 1 -> goto 2         LL_I2C_IsActiveFlag_RXNE(I2C1) & LL_I2C_IsActiveFlag_BTF(I2C1)
   or
    If STOPF is 1:                                                     LL_I2C_IsActiveFlag_STOP(I2C1)
        Write CR1                                                      LL_I2C_ClearFlag_STOP(I2C1)
        Done transfer
   or
    if OVR is 1 or AF is 1 or BERR is 1                                LL_I2C_IsActiveFlag_OVR(I2C1) & LL_I2C_IsActiveFlag_AF(I2C1) & LL_I2C_IsActiveFlag_BERR(I2C1)
        This is ERROR - reinit I2C and Done transfer

2. Recieve
    Return byte from DR register                                       LL_I2C_ReceiveData8(I2C1)

3. Goto 1

*/

static bool isCurentlyI2CReading;

bool I2CAcceptedAddress()
{
    //
    //===============================
    //recieves requests from the host
    //===============================
    //
    //ADDR bit read in the SR1 register and read from the SR2 register to clear the bit
    //
    if (!LL_I2C_IsActiveFlag_ADDR(I2C1)) return false;
    isCurentlyI2CReading = (LL_I2C_GetTransferDirection(I2C1) == LL_I2C_DIRECTION_READ);
    return true;
}

bool isI2CRead()
{
    //request recieved is a read request 
    return isCurentlyI2CReading;
}

bool I2CWriteReady()
{
    //ready to send data for a write request sent by host
    if() return true;
    else return false;
}

bool I2CReadReady()
{
    //ready to send data for a read request sent by host
    if() return true;
    else return false;
}

bool I2CAborted()
{
    //connection with host has stopped or an error occurred
    if() return true;
    else return false;
}

uint8_t I2CReadData()
{
    //returns read data if I2CReadReady is true
    if(I2CReadReady)
    {
        
    }
    else
    {

    }
}

void I2CWriteData(uint8_t data)
{
    //returns write data if I2CWriteReady is true
    if(I2CWriteReady)
    {
        
    }
    else
    {

    }
}

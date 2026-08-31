#pragma once

bool I2CAcceptedAddress()
{
    //
    //===============================
    //recieves requests from the host
    //===============================
    //
    //ADDR bit read in the SR1 register and read from the SR2 register to clear the bit
    //
    return LL_I2C_IsActiveFlag_ADDR(I2C1);
}

bool isI2CRead()
{
    //request recieved is a read request 
    if() return true;
    else return false;
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

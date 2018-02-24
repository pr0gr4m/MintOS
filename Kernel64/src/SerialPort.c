#include "SerialPort.h"
#include "Utility.h"
#include "AssemblyUtility.h"

static SERIALMANAGER gs_stSerialManager;

void kInitializeSerialPort(void)
{
	WORD wPortBaseAddress;

	kInitializeMutex(&(gs_stSerialManager.stMutex));
	wPortBaseAddress = SERIAL_PORT_COM1;
	// Disable Interrupt
	kOutPortByte(wPortBaseAddress + SERIAL_PORT_INDEX_INTERRUPTENABLE, 0);
	// Approach to latch register to set DLAB 1
	kOutPortByte(wPortBaseAddress + SERIAL_PORT_INDEX_LINECONTROL,
			SERIAL_LINECONTROL_DLAB);
	kOutPortByte(wPortBaseAddress + SERIAL_PORT_INDEX_DIVISORLATCHLSB,
			SERIAL_DIVISORLATCH_115200);
	kOutPortByte(wPortBaseAddress + SERIAL_PORT_INDEX_DIVISORLATCHMSB,
			SERIAL_DIVISORLATCH_115200 >> 8);

	// 8 bit, No parity, 1 Stop bit
	kOutPortByte(wPortBaseAddress + SERIAL_PORT_INDEX_LINECONTROL,
			SERIAL_LINECONTROL_8BIT | SERIAL_LINECONTROL_NOPARITY |
			SERIAL_LINECONTROL_1BITSTOP);

	// set FIFO interrupt by 14 byte
	kOutPortByte(wPortBaseAddress + SERIAL_PORT_INDEX_FIFOCONTROL,
			SERIAL_FIFOCONTROL_FIFOENABLE | SERIAL_FIFOCONTROL_14BYTEFIFO);
}

static BOOL kIsSerialTransmitterBufferEmpty(void)
{
	BYTE bData;

	// check TBE bit
	bData = kInPortByte(SERIAL_PORT_COM1 + SERIAL_PORT_INDEX_LINESTATUS);
	if ((bData & SERIAL_LINESTATUS_TRANSMITBUFFEREMPTY) ==
			SERIAL_LINESTATUS_TRANSMITBUFFEREMPTY)
		return TRUE;
	return FALSE;
}

void kSendSerialData(BYTE* pbBuffer, int iSize)
{
	int iSentByte, iTempSize, j;

	kLock(&(gs_stSerialManager.stMutex));

	iSentByte = 0;
	while (iSentByte < iSize)
	{
		while (kIsSerialTransmitterBufferEmpty() == FALSE)
			kSleep(1);

		iTempSize = MIN(iSize - iSentByte, SERIAL_FIFOMAXSIZE);
		for (j = 0; j < iTempSize; j++)
		{
			kOutPortByte(SERIAL_PORT_COM1 + SERIAL_PORT_INDEX_TRANSMITBUFFER,
					pbBuffer[iSentByte + j]);
		}
		iSentByte += iTempSize;
	}
	kUnlock(&(gs_stSerialManager.stMutex));
}

static BOOL kIsSerialReceiveBufferFull(void)
{
	BYTE bData;

	// check RxRD bit
	bData = kInPortByte(SERIAL_PORT_COM1 + SERIAL_PORT_INDEX_LINESTATUS);
	if ((bData & SERIAL_LINESTATUS_RECEIVEDDATAREADY) ==
			SERIAL_LINESTATUS_RECEIVEDDATAREADY)
		return TRUE;
	return FALSE;
}

int kReceiveSerialData(BYTE* pbBuffer, int iSize)
{
	int i;

	kLock(&(gs_stSerialManager.stMutex));
	for (i = 0; i < iSize; i++)
	{
		if (kIsSerialReceiveBufferFull() == FALSE)
			break;
		pbBuffer[i] = kInPortByte(SERIAL_PORT_COM1 +
				SERIAL_PORT_INDEX_RECEIVEBUFFER);
	}
	kUnlock(&(gs_stSerialManager.stMutex));
	return i;
}

void kClearSerialFIFO(void)
{
	kLock(&(gs_stSerialManager.stMutex));

	kOutPortByte(SERIAL_PORT_COM1 + SERIAL_PORT_INDEX_FIFOCONTROL,
			SERIAL_FIFOCONTROL_FIFOENABLE | SERIAL_FIFOCONTROL_14BYTEFIFO |
			SERIAL_FIFOCONTROL_CLEARRECEIVEFIFO | SERIAL_FIFOCONTROL_CLEARTRANSMITFIFO);
	
	kUnlock(&(gs_stSerialManager.stMutex));
}

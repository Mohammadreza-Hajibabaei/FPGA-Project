/* ------------------------------------------------------------ */
/*				Include File Definitions						*/
/* ------------------------------------------------------------ */

#include "adc_dma_ctrl.h"
#include "wave/wave.h"
#include "xgpiops.h"

#define MIO_0_ID           XPAR_PS7_GPIO_0_DEVICE_ID

#define GPIO_INPUT         0
#define KEY_INTR_ID        XPAR_XGPIOPS_0_INTR
XScuGic INST ;
/*
 * DMA s2mm receiver buffer
 */
u8 DmaRxBuffer[MAX_DMA_LEN]  __attribute__ ((aligned(64)));
u8 myFFtSignal[MAX_DMA_LEN]  __attribute__ ((aligned(64)));

u8 DmaTxBuffer[MAX_DMA_LEN] __attribute__ ((aligned(64)));
u8 WaveBuffer[MAX_DMA_LEN] __attribute__ ((aligned(64)));
XGpioPs GPIO_PTR ;
/*
 * Canvas buffer for drawing grid and wave
 */
u8 CanvasBuffer[CANVAS_LEN] ;
/*
 * DMA struct
 */
XAxiDma AxiDma;
/*
 * s2mm interrupt flag
 */
volatile int s2mm_flag ;
volatile int key_flag ;
/*
 * Function declaration
 */
int KeySetup(XScuGic *InstancePtr, u16 IntrID, XGpioPs *GpioInstancePtr) ;
int XAxiDma_Initial(u16 DeviceId, u16 IntrID, XAxiDma *XAxiDma, XScuGic *InstancePtr) ;
void Dma_Interrupt_Handler(void *CallBackRef);
void frame_copy(u32 width, u32 height, u32 stride, int hor_x, int ver_y, u8 *frame, u8 *CanvasBufferPtr) ;
void GpioHandler(void *CallbackRef);
// void ad9280_sample(u32 adc_addr, u32 adc_len) ;
/*
 *Initial DMA,Draw grid and wave,Start ADC sample
 *
 *@param width is frame width
 *@param frame is display frame pointer
 *@param stride is frame stride
 *@param InstancePtr is GIC pointer
 */

#define MAX_AMP_VAL        256	/* 2^8, do not change */
#define AMP_VAL            256	/* must be less than 2^8 */
int XAxiDma_Adc_Wave(u32 width, u8 *frame, u32 stride, XScuGic *InstancePtr)
{
	int Status;
	u32 wave_width = width ;
	GetSinWave(MAX_DMA_LEN, MAX_AMP_VAL, AMP_VAL, WaveBuffer) ;
	
	KeySetup(&INST, KEY_INTR_ID, &GPIO_PTR) ;

	s2mm_flag = 1 ;
	key_flag = 0;
	int wave_sel = 0;
	XAxiDma_Initial(DMA_DEV_ID, S2MM_INTR_ID, &AxiDma, InstancePtr) ;
	memcpy(DmaTxBuffer, WaveBuffer, MAX_DMA_LEN) ;
	Xil_DCacheFlushRange((UINTPTR)DmaTxBuffer, MAX_DMA_LEN);
	// AD9708_SEND_mWriteReg(AD9708_BASE, AD9708_START, 1) ;
	Status = XAxiDma_SimpleTransfer(&AxiDma,(UINTPTR) DmaRxBuffer,
					ADC_CAPTURELEN, XAXIDMA_DEVICE_TO_DMA);

	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	Status = XAxiDma_SimpleTransfer(&AxiDma,(UINTPTR) DmaTxBuffer,
				ADC_CAPTURELEN, XAXIDMA_DMA_TO_DEVICE);

	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	while(1) {

		if (s2mm_flag)
		{
			if (key_flag == 1) {
				key_flag =0;
				if (wave_sel == 4)
					wave_sel = 0 ;
				else 
				{
					wave_sel++ ;
				}
			}
			
			switch(wave_sel)
			{
			case 0 : GetSquareWave(MAX_DMA_LEN, MAX_AMP_VAL, AMP_VAL, WaveBuffer) ; break ;
			case 1 : GetTriangleWave(MAX_DMA_LEN, MAX_AMP_VAL, AMP_VAL, WaveBuffer) ; break ;
			case 2 : GetSawtoothWave(MAX_DMA_LEN, MAX_AMP_VAL, AMP_VAL, WaveBuffer) ; break ;
			case 3 : GetSubSawtoothWave(MAX_DMA_LEN, MAX_AMP_VAL, AMP_VAL, WaveBuffer) ; break ;
			case 4 : GetSinWave(MAX_DMA_LEN, MAX_AMP_VAL, AMP_VAL, WaveBuffer) ; break ;
			default: GetSinWave(MAX_DMA_LEN, MAX_AMP_VAL, AMP_VAL, WaveBuffer) ; break ;
			}

			memcpy(DmaTxBuffer, WaveBuffer, MAX_DMA_LEN) ;
			Xil_DCacheFlushRange((UINTPTR)DmaTxBuffer, MAX_DMA_LEN);

			/* Clear flag */
			Status = XAxiDma_SimpleTransfer(&AxiDma,(UINTPTR) DmaRxBuffer,
					ADC_CAPTURELEN, XAXIDMA_DEVICE_TO_DMA);

			if (Status != XST_SUCCESS) {
				return XST_FAILURE;
			}
			/* clear s2mm_flag */
			s2mm_flag = 0 ;
			/* Grid Overlay */
			draw_grid(wave_width, WAVE_HEIGHT,CanvasBuffer) ;
			/* wave Overlay */
			// should be changed
			int num = 2;
			draw_wave(wave_width, WAVE_HEIGHT, (void *)myFFtSignal, CanvasBuffer, UNSIGNEDCHAR, ADC_BITS, YELLOW, ADC_COE , num , 0) ;
			draw_wave(wave_width, WAVE_HEIGHT, (void *)WaveBuffer ,CanvasBuffer, UNSIGNEDCHAR, ADC_BITS, YELLOW, ADC_COE , num , 1) ;
			/* Copy Canvas to frame buffer */
			frame_copy(wave_width, WAVE_HEIGHT, stride, WAVE_START_COLUMN, WAVE_START_ROW, frame, CanvasBuffer) ;
			/* delay 100ms */
			usleep(100000) ;

			
		}
	}
}

/*
 *This is ADC sample function, use it and start sample adc data
 *
 *@param adc_addr ADC base address
 *@param adc_len is sample length in ADC data width
 */
// void ad9280_sample(u32 adc_addr, u32 adc_len)
// {
// 	/* provide length to AD9280 module */
// 	AD9280_SAMPLE_mWriteReg(adc_addr, AD9280_LENGTH, adc_len)  ;
// 	/* start sample AD9280 */
// 	AD9280_SAMPLE_mWriteReg(adc_addr, AD9280_START, 1) ;
// }

/*
 *Copy canvas buffer data to special position in frame
 *
 *@param hor_x  start horizontal position for copy
 *@param ver_y  start vertical position for copy
 *@param width  width for copy
 *@param height height for copy
 *
 *@note  hor_x+width should be less than frame width, ver_y+height should be less than frame height
 */
void frame_copy(u32 width, u32 height, u32 stride, int hor_x, int ver_y, u8 *frame, u8 *CanvasBufferPtr)
{
	int i ;
	u32 FrameOffset ;
	u32 CanvasOffset ;
	u32 CopyLen = width*BYTES_PIXEL ;

	for(i = 0 ; i < height;  i++)
	{
		FrameOffset = (ver_y+i)*stride + hor_x*BYTES_PIXEL ;
		CanvasOffset = i*width*BYTES_PIXEL ;
		memcpy(frame+FrameOffset, CanvasBufferPtr+CanvasOffset, CopyLen) ;
	}

	FrameOffset = ver_y*stride ;

	Xil_DCacheFlushRange((INTPTR) frame+FrameOffset, height*stride) ;
}


/*
 *Initial DMA and connect interrupt to handler, open s2mm interrupt
 *
 *@param DeviceId    DMA device id
 *@param IntrID      DMA interrupt id
 *@param XAxiDma     DMA pointer
 *@param InstancePtr GIC pointer
 *
 *@note  none
 */
int XAxiDma_Initial(u16 DeviceId, u16 IntrID, XAxiDma *XAxiDma, XScuGic *InstancePtr)
{
	XAxiDma_Config *CfgPtr;
	int Status;
	/* Initialize the XAxiDma device. */
	CfgPtr = XAxiDma_LookupConfig(DeviceId);
	if (!CfgPtr) {
		xil_printf("No config found for %d\r\n", DeviceId);
		return XST_FAILURE;
	}

	Status = XAxiDma_CfgInitialize(XAxiDma, CfgPtr);
	if (Status != XST_SUCCESS) {
		xil_printf("Initialization failed %d\r\n", Status);
		return XST_FAILURE;
	}

	Status = XScuGic_Connect(InstancePtr, IntrID,
			(Xil_ExceptionHandler)Dma_Interrupt_Handler,
			(void *)XAxiDma) ;

	if (Status != XST_SUCCESS) {
		return Status;
	}

	XScuGic_Enable(InstancePtr, IntrID);


	/* Disable MM2S interrupt, Enable S2MM interrupt */
	XAxiDma_IntrEnable(XAxiDma, XAXIDMA_IRQ_IOC_MASK,
			XAXIDMA_DEVICE_TO_DMA);
	XAxiDma_IntrDisable(XAxiDma, XAXIDMA_IRQ_ALL_MASK,
			XAXIDMA_DMA_TO_DEVICE);

	return XST_SUCCESS ;
}


/*
 *callback function
 *Check interrupt status and assert s2mm flag
 */
void Dma_Interrupt_Handler(void *CallBackRef)
{
	XAxiDma *XAxiDmaPtr ;
	XAxiDmaPtr = (XAxiDma *) CallBackRef ;

	int s2mm_sr ;

	s2mm_sr = XAxiDma_IntrGetIrq(XAxiDmaPtr, XAXIDMA_DEVICE_TO_DMA) ;

	if (s2mm_sr & XAXIDMA_IRQ_IOC_MASK)
	{
		/* Clear interrupt */
		XAxiDma_IntrAckIrq(XAxiDmaPtr, XAXIDMA_IRQ_IOC_MASK,
				XAXIDMA_DEVICE_TO_DMA) ;
		/* Invalidate the Data cache for the given address range */
		Xil_DCacheInvalidateRange((INTPTR)DmaRxBuffer, ADC_CAPTURELEN);
		 // Copy DMA values to myFFTsignal buffer
        for (int i = 0; i < MAX_DMA_LEN; i++)
        {

            myFFtSignal[i] = DmaRxBuffer[i];
        }
		s2mm_flag = 1 ;
	}

}

int KeySetup(XScuGic *InstancePtr, u16 IntrID, XGpioPs *GpioInstancePtr)
{
	XGpioPs_Config *GPIO_CONFIG ;
	int Status ;
	key_flag = 0 ;


	GPIO_CONFIG = XGpioPs_LookupConfig(MIO_0_ID) ;
	Status = XGpioPs_CfgInitialize(GpioInstancePtr, GPIO_CONFIG, GPIO_CONFIG->BaseAddr) ;
	if (Status != XST_SUCCESS)
	{
		return XST_FAILURE ;
	}
	//set MIO 50 as input
	XGpioPs_SetDirectionPin(GpioInstancePtr, 50, GPIO_INPUT) ;
	//set interrupt type
	XGpioPs_SetIntrTypePin(GpioInstancePtr, 50, XGPIOPS_IRQ_TYPE_EDGE_RISING) ;


	//set priority and trigger type
	XScuGic_SetPriorityTriggerType(InstancePtr, IntrID,
			0xA0, 0x3);
	Status = XScuGic_Connect(InstancePtr, IntrID,
			(Xil_ExceptionHandler)GpioHandler,
			(void *)GpioInstancePtr) ;

	XScuGic_Enable(InstancePtr, IntrID) ;

	if (Status != XST_SUCCESS) {
		return Status;
	}


	XGpioPs_IntrEnablePin(GpioInstancePtr, 50) ;

	return XST_SUCCESS ;
}

void GpioHandler(void *CallbackRef)
{
	XGpioPs *GpioInstancePtr = (XGpioPs *)CallbackRef ;
	int Int_val ;

	Int_val = XGpioPs_IntrGetStatusPin(GpioInstancePtr, 50) ;
	/* clear key interrupt */
	XGpioPs_IntrClearPin(GpioInstancePtr, 50) ;
	if (Int_val == TRUE)
		key_flag = 1 ;

}

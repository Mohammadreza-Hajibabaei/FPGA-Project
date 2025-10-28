/* ------------------------------------------------------------ */
/*				Include File Definitions						*/
/* ------------------------------------------------------------ */

#include "adc_dma_ctrl.h"
#include "wave/wave.h"
#include "xgpiops.h"

#define MIO_0_ID           XPAR_PS7_GPIO_0_DEVICE_ID

#define GPIO_INPUT         0
#define KEY_INTR_ID        XPAR_XGPIOPS_0_INTR
#define SampleRate 1024
#define GPIO_INPUT         0
#define GPIO_OUTPUT		   1
XScuGic INST ;
/*
 * DMA s2mm receiver buffer
 */
s32 DmaRxBuffer[SampleRate]  __attribute__ ((aligned(64)));
int myFFtSignal[SampleRate]  __attribute__ ((aligned(64)));

u32 DmaTxBuffer[SampleRate] __attribute__ ((aligned(64)));
u32 WaveBuffer[SampleRate] __attribute__ ((aligned(64)));

u32 Wave1[1280]__attribute__ ((aligned(64)));
u32 Wave2[1280]__attribute__ ((aligned(64)));
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
int KeySetup(XScuGic *InstancePtr, u16 DeviceId, u16 IntrID, XGpioPs *GpioInstancePtr) ;
int XAxiDma_Initial(u16 DeviceId, u16 IntrID, XAxiDma *XAxiDma, XScuGic *InstancePtr) ;
void Dma_Interrupt_Handler(void *CallBackRef);
void frame_copy(u32 width, u32 height, u32 stride, int hor_x, int ver_y, u8 *frame, u8 *CanvasBufferPtr) ;
void GpioHandler(void *CallbackRef);
void map_array(const u32 input[1024], u32 output[1280]) ;
// void ad9280_sample(u32 adc_addr, u32 adc_len) ;


#define MAX_AMP_VAL        256	/* 2^8, do not change */
#define AMP_VAL            256	/* must be less than 2^8 */
int XAxiDma_Adc_Wave(u32 width, u8 *frame, u32 stride, XScuGic *InstancePtr)
{
	Scrambler scrambler;
	CCS_DS_Header header = generate_header(0x01, 0x1001, 0x2001, 0x01, FRAME_LENGTH);
	int Status;
	u32 wave_width = width ;
	GetSinWave(MAX_DMA_LEN, MAX_AMP_VAL, AMP_VAL, WaveBuffer) ;
	xil_printf("Sine\n");
	
	KeySetup(&INST, MIO_0_ID, KEY_INTR_ID, &GPIO_PTR) ;
	xil_printf("Key\n");

	s2mm_flag = 1 ;
	key_flag = 0;
	int wave_sel = 0;
	XAxiDma_Initial(DMA_DEV_ID, S2MM_INTR_ID, &AxiDma, InstancePtr) ;
	xil_printf("dma initial\n");
	memcpy(DmaTxBuffer, WaveBuffer, MAX_DMA_LEN) ;
	Xil_DCacheFlushRange((UINTPTR)DmaTxBuffer, MAX_DMA_LEN);
	xil_printf("flush\n");
	// AD9708_SEND_mWriteReg(AD9708_BASE, AD9708_START, 1) ;
	Status = XAxiDma_SimpleTransfer(&AxiDma,(UINTPTR) DmaRxBuffer,
					ADC_CAPTURELEN, XAXIDMA_DEVICE_TO_DMA);
	xil_printf("s transfer\n");
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	Status = XAxiDma_SimpleTransfer(&AxiDma,(UINTPTR) DmaTxBuffer,
				ADC_CAPTURELEN, XAXIDMA_DMA_TO_DEVICE);

	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	while(1) {
		xil_printf("Loop\n");
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
			case 0 : GetSquareWave(SampleRate, MAX_AMP_VAL, AMP_VAL, WaveBuffer) ; break ;
			case 1 : GetTriangleWave(SampleRate, MAX_AMP_VAL, AMP_VAL, WaveBuffer) ; break ;
			case 2 : GetSawtoothWave(SampleRate, MAX_AMP_VAL, AMP_VAL, WaveBuffer) ; break ;
			case 3 : GetSubSawtoothWave(SampleRate, MAX_AMP_VAL, AMP_VAL, WaveBuffer) ; break ;
			case 4 : GetSinWave(SampleRate, MAX_AMP_VAL, AMP_VAL, WaveBuffer) ; break ;
			default: GetSinWave(SampleRate, MAX_AMP_VAL, AMP_VAL, WaveBuffer) ; break ;
			}
			printf("Wave sel %d\n" , wave_sel);
			Scrambler scrambler;
			CCS_DS_Header header = generate_header(0x01, 0x1001, 0x2001, 0x01, FRAME_LENGTH);
			 for (int i = 0; i < frame_count; i++) {
			        init_scrambler(&scrambler);
			        uint8_t WaveBuffer[BYTES_PER_FRAME] = {0};

			        // Fill frame data with wave samples
			        int sample_start = i * DATA_LENGTH;
			        int sample_end = sample_start + DATA_LENGTH;
			        for (int j = sample_start, k = HEADER_LENGTH; j < sample_end && j < TOTAL_SAMPLES; j++, k++) {
			        	WaveBuffer[k] = data[j];
			        }

			        insert_header_and_scramble(&scrambler, WaveBuffer, header);

					// Print scrambled data
					for (int j = 0; j < BYTES_PER_FRAME; j++) {
						printf("%02X ", WaveBuffer[j]);
						if ((i + 1) % 16 == 0) {
							printf("\n");
						}
					}
			    }
			memcpy(DmaTxBuffer, WaveBuffer, MAX_DMA_LEN) ;
			Xil_DCacheFlushRange((UINTPTR)DmaTxBuffer, MAX_DMA_LEN);
			printf("Wave buffer sent\n");

			/* Clear flag */
			Status = XAxiDma_SimpleTransfer(&AxiDma,(UINTPTR) DmaRxBuffer,
					ADC_CAPTURELEN, XAXIDMA_DEVICE_TO_DMA);
			printf("FFt sent\n");
			if (Status != XST_SUCCESS) {
				return XST_FAILURE;
			}
			/* clear s2mm_flag */
			s2mm_flag = 0 ;
			/* Grid Overlay */
			draw_grid(wave_width, WAVE_HEIGHT,CanvasBuffer) ;
			/* wave Overlay */
			int num = 2;
			map_array(myFFtSignal , Wave1);
			map_array(WaveBuffer , Wave2);
			draw_wave(wave_width, WAVE_HEIGHT, (void *)Wave1, CanvasBuffer, UNSIGNEDCHAR, ADC_BITS, YELLOW, ADC_COE , num , 0) ;
			draw_wave(wave_width, WAVE_HEIGHT, (void *)Wave2 ,CanvasBuffer, UNSIGNEDCHAR, ADC_BITS, YELLOW, ADC_COE , num , 1) ;
			/* Copy Canvas to frame buffer */
			frame_copy(wave_width, WAVE_HEIGHT, stride, WAVE_START_COLUMN, WAVE_START_ROW, frame, CanvasBuffer) ;
			/* delay 100ms */
			usleep(100000) ;

			
		}
	}
}


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

	xil_printf("1\n");

	Status = XAxiDma_CfgInitialize(XAxiDma, CfgPtr);
	if (Status != XST_SUCCESS) {
		xil_printf("Initialization failed %d\r\n", Status);
		return XST_FAILURE;
	}

	xil_printf("2\n");

	Status = XScuGic_Connect(InstancePtr, IntrID,
			(Xil_ExceptionHandler)Dma_Interrupt_Handler,
			(void *)XAxiDma) ;

	if (Status != XST_SUCCESS) {
		xil_printf("2 not\n");
		return Status;
	}

	xil_printf("3\n");

	XScuGic_Enable(InstancePtr, IntrID);

	xil_printf("4\n");
	/* Disable MM2S interrupt, Enable S2MM interrupt */
	XAxiDma_IntrEnable(XAxiDma, XAXIDMA_IRQ_IOC_MASK,
			XAXIDMA_DEVICE_TO_DMA);
	XAxiDma_IntrDisable(XAxiDma, XAXIDMA_IRQ_ALL_MASK,
			XAXIDMA_DMA_TO_DEVICE);
	xil_printf("5\n");
	return XST_SUCCESS ;
}


/*
 *callback function
 *Check interrupt status and assert s2mm flag
 */
void Dma_Interrupt_Handler(void *CallBackRef)
{
	printf("FFt interrupt\n");
//	XAxiDma *XAxiDmaPtr ;
//	XAxiDmaPtr = (XAxiDma *) CallBackRef ;
//
//	int s2mm_sr ;
//
//	s2mm_sr = XAxiDma_IntrGetIrq(XAxiDmaPtr, XAXIDMA_DEVICE_TO_DMA) ;
//
//	if (s2mm_sr & XAXIDMA_IRQ_IOC_MASK)
//	{
//		/* Clear interrupt */
//		XAxiDma_IntrAckIrq(XAxiDmaPtr, XAXIDMA_IRQ_IOC_MASK,
//				XAXIDMA_DEVICE_TO_DMA) ;
//		/* Invalidate the Data cache for the given address range */
//		Xil_DCacheInvalidateRange((INTPTR)DmaRxBuffer, ADC_CAPTURELEN);
//		 // Copy DMA values to myFFTsignal buffer
//        for (int i = 0; i < SampleRate; i++)
//        {
//
//            myFFtSignal[i] = (abs((DmaRxBuffer[i] && 0xffff0000) >> 16)) + abs((DmaRxBuffer[i] && 0x0000ffff));
//        }
//		s2mm_flag = 1 ;
//	}

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

			s2mm_flag = 1 ;
		}

	}



int KeySetup(XScuGic *InstancePtr, u16 DeviceId, u16 IntrID, XGpioPs *GpioInstancePtr)
{
	XGpioPs_Config *GpioCfg ;
	int Status ;

	GpioCfg = XGpioPs_LookupConfig(DeviceId) ;
	Status = XGpioPs_CfgInitialize(GpioInstancePtr, GpioCfg, GpioCfg->BaseAddr) ;
	if (Status != XST_SUCCESS)
	{
		return XST_FAILURE ;
	}
	/* set MIO 50 as input */
	XGpioPs_SetDirectionPin(GpioInstancePtr, 50, GPIO_INPUT) ;
	/* set interrupt type */
	XGpioPs_SetIntrTypePin(GpioInstancePtr, 50, XGPIOPS_IRQ_TYPE_EDGE_RISING) ;

	/* set MIO 0 as output */
	XGpioPs_SetDirectionPin(&GPIO_PTR, 0, GPIO_OUTPUT) ;
	/* enable MIO 0 output */
	XGpioPs_SetOutputEnablePin(&GPIO_PTR, 0, GPIO_OUTPUT) ;
	/* set priority and trigger type */
	XScuGic_SetPriorityTriggerType(InstancePtr, IntrID,
			0xA0, 0x3);
	Status = XScuGic_Connect(InstancePtr, IntrID,
			(Xil_ExceptionHandler)GpioHandler,
			(void *)GpioInstancePtr) ;

	XScuGic_Enable(InstancePtr, IntrID) ;

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
	printf("Key flag\n");
	if (Int_val == TRUE)
		key_flag = 1 ;

}
void map_array(const u32 input[1024], u32 output[1280]) {
    int j = 0;
    for (int i = 0; i < 1024; i++) {
        output[j++] = input[i];
        if ((i + 1) % 4 == 0 && j < 1280) {
            output[j++] = input[i]; // Add zero after every 4 elements
        }
    }

    // If there are remaining spaces in the output array, pad them with zeros
    while (j < 1280) {
        output[j++] = 0;
    }
}

/*
 ******************************************************************************
 * @file    MX25L_QSPI.c
 * @author  DNK-043
 * @date    28-Dec-2022
 *******************************************************************************
*/

#include "MX25L_QSPI_Flash.h"
#include "main.h"


QSPI_HandleTypeDef QSPIHandle;


/* Private functions ---------------------------------------------------------*/

static uint8_t QSPI_ResetMemory(QSPI_HandleTypeDef *hqspi);
static uint8_t QSPI_EnterFourBytesAddress(QSPI_HandleTypeDef *hqspi);
static uint8_t QSPI_DummyCyclesCfg(QSPI_HandleTypeDef *hqspi);
static uint8_t QSPI_EnterMemory_QPI(QSPI_HandleTypeDef *hqspi);
static uint8_t QSPI_ExitMemory_QPI(QSPI_HandleTypeDef *hqspi);
static uint8_t QSPI_WriteEnable(QSPI_HandleTypeDef *hqspi);
static uint8_t QSPI_AutoPollingMemReady(QSPI_HandleTypeDef *hqspi,
		uint32_t Timeout);

/**
 * @brief  Initializes the QSPI interface.
 * @retval QSPI memory status
 */
uint8_t BSP_QSPI_Init(void) {
	QSPIHandle.Instance = QUADSPI;
	/* Call the DeInit function to reset the driver */
	if (HAL_QSPI_DeInit(&QSPIHandle) != HAL_OK) {
		return QSPI_ERROR;
	}

	/* System level initialization */
//	BSP_QSPI_MspInit(&QSPIHandle, NULL);

	/* QSPI initialization */
	QSPIHandle.Init.ClockPrescaler = 0;
	QSPIHandle.Init.FifoThreshold = 4;
	QSPIHandle.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_NONE;
	QSPIHandle.Init.FlashSize = POSITION_VAL(MX25L512_FLASH_SIZE) - 1;
	QSPIHandle.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_1_CYCLE;
	QSPIHandle.Init.ClockMode = QSPI_CLOCK_MODE_0;
	QSPIHandle.Init.FlashID = QSPI_FLASH_ID_1;
	QSPIHandle.Init.DualFlash = QSPI_DUALFLASH_DISABLE;

	if (HAL_QSPI_Init(&QSPIHandle) != HAL_OK) {
		return QSPI_ERROR;
	}

	/* QSPI memory reset */
	if (QSPI_ResetMemory(&QSPIHandle) != QSPI_OK) {
		return QSPI_NOT_SUPPORTED;
	}

	/* Put QSPI memory in QPI mode */
	if (QSPI_EnterMemory_QPI(&QSPIHandle) != QSPI_OK) {
		return QSPI_NOT_SUPPORTED;
	}

	/* Set the QSPI memory in 4-bytes address mode */
	if (QSPI_EnterFourBytesAddress(&QSPIHandle) != QSPI_OK) {
		return QSPI_NOT_SUPPORTED;
	}

	/* Configuration of the dummy cycles on QSPI memory side */
	if (QSPI_DummyCyclesCfg(&QSPIHandle) != QSPI_OK) {
		return QSPI_NOT_SUPPORTED;
	}

	return QSPI_OK;
}

/**
 * @brief  De-Initializes the QSPI interface.
 * @retval QSPI memory status
 */
uint8_t BSP_QSPI_DeInit(void) {
	QSPIHandle.Instance = QUADSPI;

	/* Put QSPI memory in SPI mode */
	if (QSPI_ExitMemory_QPI(&QSPIHandle) != QSPI_OK) {
		return QSPI_NOT_SUPPORTED;
	}

	/* Call the DeInit function to reset the driver */
	if (HAL_QSPI_DeInit(&QSPIHandle) != HAL_OK) {
		return QSPI_ERROR;
	}

	/* System level De-initialization */
	BSP_QSPI_MspDeInit(&QSPIHandle, NULL);

	return QSPI_OK;
}
/**
 * @brief  Reads device id from the QSPI memory.
 * @param  pData: Pointer to data to be read
 * @retval QSPI memory status
 */
uint8_t BSP_QSPI_ReadID(uint8_t *pData) {
	QSPI_CommandTypeDef s_command;
	uint8_t ret = 0;

	s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_command.Instruction = READ_ID_CMD;
	s_command.AddressMode = QSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DataMode = QSPI_DATA_1_LINE;
	s_command.DummyCycles = 0;
	s_command.NbData = 3;
	s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
	s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
	s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

	/* Configure the command */
	if ((ret = HAL_QSPI_Command(&QSPIHandle, &s_command, 5000)) != HAL_OK) {
		printf("%d HAL_QSPI_Command\r\n", ret);
		return QSPI_ERROR;
	}

	/* Reception of the data */
	if ((ret = HAL_QSPI_Receive(&QSPIHandle, pData, 5000)) != HAL_OK) {
		printf("%d HAL_QSPI_Receive\r\n", ret);
		return QSPI_ERROR;
	}

	if (ret == 0) {
		printf("data : %x %x %x\r\n", pData[0], pData[1], pData[2]);
	}

	return QSPI_OK;
}
/**
 * @brief  Reads an amount of data from the QSPI memory.
 * @param  pData: Pointer to data to be read
 * @param  ReadAddr: Read start address
 * @param  Size: Size of data to read
 * @retval QSPI memory status
 */
uint8_t BSP_QSPI_Read(uint8_t *pData, uint32_t ReadAddr, uint32_t Size) {
	QSPI_CommandTypeDef s_command;
	uint8_t ret = 0;

	/* Initialize the read command */
	s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_command.Instruction = READ_CMD;
	s_command.AddressMode = QSPI_ADDRESS_1_LINE;
	s_command.AddressSize = QSPI_ADDRESS_24_BITS;
	s_command.Address = ReadAddr;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DataMode = QSPI_DATA_1_LINE;
	s_command.DummyCycles = 0;
	s_command.NbData = Size;
	s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
	s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
	s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

	/* Configure the command */
	if ((ret = HAL_QSPI_Command(&QSPIHandle, &s_command,
			HAL_QPSI_TIMEOUT_DEFAULT_VALUE)) != HAL_OK) {
		return QSPI_ERROR;
	}

	/* Reception of the data */
	if ((ret = HAL_QSPI_Receive(&QSPIHandle, pData,
			HAL_QPSI_TIMEOUT_DEFAULT_VALUE)) != HAL_OK) {
		return QSPI_ERROR;
	}

	return QSPI_OK;
}

/**
 * @brief  Writes an amount of data to the QSPI memory.
 * @param  pData: Pointer to data to be written
 * @param  WriteAddr: Write start address
 * @param  Size: Size of data to write
 * @retval QSPI memory status
 */

uint8_t BSP_QSPI_Write(uint8_t *pData, uint32_t WriteAddr, uint32_t Size) {
	QSPI_CommandTypeDef s_command;
	uint32_t end_addr, current_size, current_addr;

	/* Calculation of the size between the write address and the end of the page */
	current_size = MX25L512_PAGE_SIZE - (WriteAddr % MX25L512_PAGE_SIZE);

	/* Check if the size of the data is less than the remaining place in the page */
	if (current_size > Size) {
		current_size = Size;
	}

	/* Initialize the adress variables */
	current_addr = WriteAddr;
	end_addr = WriteAddr + Size;

	/* Initialize the program command */
	s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_command.Instruction = PAGE_PROG_CMD;
	s_command.AddressMode = QSPI_ADDRESS_1_LINE;
	s_command.AddressSize = QSPI_ADDRESS_24_BITS;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DataMode          = QSPI_DATA_1_LINE;
	s_command.DummyCycles = 0;
	s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
	s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
	s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

	/* Perform the write page by page */
	do {
		s_command.Address = current_addr;
		s_command.NbData = current_size;

		/* Enable write operations */
		if (QSPI_WriteEnable(&QSPIHandle) != QSPI_OK) {
			return QSPI_ERROR;
		}

		/* Configure the command */
		if (HAL_QSPI_Command(&QSPIHandle, &s_command,
				HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
			return QSPI_ERROR;
		}

		/* Transmission of the data */
		if (HAL_QSPI_Transmit(&QSPIHandle, pData,
				HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
			return QSPI_ERROR;
		}

		/* Configure automatic polling mode to wait for end of program */
		if (QSPI_AutoPollingMemReady(&QSPIHandle,
				HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != QSPI_OK) {
			return QSPI_ERROR;
		}
		/* Update the address and size variables for next page programming */
		current_addr += current_size;
		pData += current_size;
		current_size =
				((current_addr + MX25L512_PAGE_SIZE) > end_addr) ?
						(end_addr - current_addr) : MX25L512_PAGE_SIZE;
	} while (current_addr < end_addr);

	return QSPI_OK;
}

/**
 * @brief  Erases the specified sector of the QSPI memory.
 * @param  SectorAddress: Sector address to erase
 * @retval QSPI memory status
 */
uint8_t BSP_QSPI_Erase_Sector(uint32_t SectorAddress) {
	QSPI_CommandTypeDef s_command;

	/* Initialize the erase command */
	s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_command.Instruction = SECTOR_ERASE_CMD;
	s_command.AddressMode = QSPI_ADDRESS_1_LINE;
	s_command.AddressSize = QSPI_ADDRESS_24_BITS;
	s_command.Address = SectorAddress;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DataMode = QSPI_DATA_NONE;
	s_command.DummyCycles = 0;
	s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
	s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
	s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

	/* Enable write operations */
	if (QSPI_WriteEnable(&QSPIHandle) != QSPI_OK) {
		return QSPI_ERROR;
	}

	/* Send the command */
	if (HAL_QSPI_Command(&QSPIHandle, &s_command,
			HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		return QSPI_ERROR;
	}

	/* Configure automatic polling mode to wait for end of erase */
	if (QSPI_AutoPollingMemReady(&QSPIHandle,
			MX25L512_SECTOR_ERASE_MAX_TIME) != QSPI_OK) {
		return QSPI_ERROR;
	}

	return QSPI_OK;
}


/**
 * @brief  Erases the specified block of the QSPI memory.
 * @param  BlockAddress: Block address to erase
 * @retval QSPI memory status
 */
uint8_t BSP_QSPI_Erase_Block(uint32_t BlockAddress) {
	QSPI_CommandTypeDef s_command;

	/* Initialize the erase command */
	s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_command.Instruction = BLOCK_ERASE_64KB;
	s_command.AddressMode = QSPI_ADDRESS_1_LINE;
	s_command.AddressSize = QSPI_ADDRESS_24_BITS;
	s_command.Address = BlockAddress;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DataMode = QSPI_DATA_NONE;
	s_command.DummyCycles = 0;
	s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
	s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
	s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

	/* Enable write operations */
	if (QSPI_WriteEnable(&QSPIHandle) != QSPI_OK) {
		return QSPI_ERROR;
	}

	/* Send the command */
	if (HAL_QSPI_Command(&QSPIHandle, &s_command,
			HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		return QSPI_ERROR;
	}

	/* Configure automatic polling mode to wait for end of erase */
	if (QSPI_AutoPollingMemReady(&QSPIHandle,
			MX25L512_SECTOR_ERASE_MAX_TIME) != QSPI_OK) {
		return QSPI_ERROR;
	}

	return QSPI_OK;
}

/**
 * @brief  Erases the entire QSPI memory.
 * @retval QSPI memory status
 */
uint8_t BSP_QSPI_Erase_Chip(void) {
	QSPI_CommandTypeDef s_command;

	/* Initialize the erase command */
	s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_command.Instruction = BULK_ERASE_CMD;
	s_command.AddressMode = QSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DataMode = QSPI_DATA_NONE;
	s_command.DummyCycles = 0;
	s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
	s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
	s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

	/* Enable write operations */
	if (QSPI_WriteEnable(&QSPIHandle) != QSPI_OK) {
		return QSPI_ERROR;
	}

	/* Send the command */
	if (HAL_QSPI_Command(&QSPIHandle, &s_command,
			HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		return QSPI_ERROR;
	}

//	/* Configure automatic polling mode to wait for end of erase */
//	if (QSPI_AutoPollingMemReady(&QSPIHandle,
//			MX25L512_BULK_ERASE_MAX_TIME) != QSPI_OK) {
//		return QSPI_ERROR;
//	}
	return QSPI_OK;
}

/**
 * @brief  Reads current status of the QSPI memory.
 * @retval QSPI memory status
 */
uint8_t BSP_QSPI_GetStatus(void) {
	QSPI_CommandTypeDef s_command;
	uint8_t reg;

	/* Initialize the read flag status register command */
	s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_command.Instruction = READ_STATUS_REG_CMD;
	s_command.AddressMode = QSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DataMode = QSPI_DATA_1_LINE;
	s_command.DummyCycles = 0;
	s_command.NbData = 1;
	s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
	s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
	s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

	/* Configure the command */
	if (HAL_QSPI_Command(&QSPIHandle, &s_command,
			HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		return QSPI_ERROR;
	}

	/* Reception of the data */
	if (HAL_QSPI_Receive(&QSPIHandle, &reg, HAL_QPSI_TIMEOUT_DEFAULT_VALUE)
			!= HAL_OK) {
		return QSPI_ERROR;
	}

	/* Check the value of the register*/
	if ((reg & MX25L512_SR_WIP) == 0) {
		return QSPI_OK;
	} else {
		return QSPI_BUSY;
	}
}

/**
 * @brief  Return the configuration of the QSPI memory.
 * @param  pInfo: pointer on the configuration structure
 * @retval QSPI memory status
 */
uint8_t BSP_QSPI_GetInfo(QSPI_Info *pInfo) {
	/* Configure the structure with the memory configuration */
	pInfo->FlashSize = MX25L512_FLASH_SIZE;
	pInfo->EraseSectorSize = MX25L512_SUBSECTOR_SIZE;
	pInfo->EraseSectorsNumber = (MX25L512_FLASH_SIZE / MX25L512_SUBSECTOR_SIZE);
	pInfo->ProgPageSize = MX25L512_PAGE_SIZE;
	pInfo->ProgPagesNumber = (MX25L512_FLASH_SIZE / MX25L512_PAGE_SIZE);

	return QSPI_OK;
}

/**
 * @brief  Configure the QSPI in memory-mapped mode
 * @retval QSPI memory status
 */
uint8_t BSP_QSPI_EnableMemoryMappedMode(void) {
	QSPI_CommandTypeDef s_command;
	QSPI_MemoryMappedTypeDef s_mem_mapped_cfg;

	/* Configure the command for the read instruction */
	s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_command.Instruction = QPI_READ_4_BYTE_ADDR_CMD;
	s_command.AddressMode = QSPI_ADDRESS_4_LINES;
	s_command.AddressSize = QSPI_ADDRESS_32_BITS;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DataMode = QSPI_DATA_4_LINES;
	s_command.DummyCycles = MX25L512_DUMMY_CYCLES_READ_QUAD_IO;
	s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
	s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
	s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

	/* Configure the memory mapped mode */
	s_mem_mapped_cfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;
	s_mem_mapped_cfg.TimeOutPeriod = 0;

	if (HAL_QSPI_MemoryMapped(&QSPIHandle, &s_command, &s_mem_mapped_cfg)
			!= HAL_OK) {
		return QSPI_ERROR;
	}

	return QSPI_OK;
}
/**
 * @brief QSPI MSP Initialization
 *        This function configures the hardware resources used in this example:
 *           - Peripheral's clock enable
 *           - Peripheral's GPIO Configuration
 *           - NVIC configuration for QSPI interrupt
 * @retval None
 */
//__weak void BSP_QSPI_MspInit(QSPI_HandleTypeDef *hqspi, void *Params) {
//	/* Need to take a reference from STM32L4 QSPI lib */
//
//	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
//	/* USER CODE BEGIN QUADSPI_MspInit 0 */
//
//	/* USER CODE END QUADSPI_MspInit 0 */
//	/* Peripheral clock enable */
//	__HAL_RCC_QSPI_CLK_ENABLE();
//
//	__HAL_RCC_GPIOB_CLK_ENABLE();
//	__HAL_RCC_GPIOA_CLK_ENABLE();
//	/**QUADSPI GPIO Configuration
//	 PB0     ------> QUADSPI_BK1_IO1
//	 PB1     ------> QUADSPI_BK1_IO0
//	 PB11     ------> QUADSPI_BK1_NCS
//	 PB10     ------> QUADSPI_CLK
//	 PA6     ------> QUADSPI_BK1_IO3
//	 PA7     ------> QUADSPI_BK1_IO2
//	 */
//	GPIO_InitStruct.Pin =
//			QSPI_CS_PIN | QSPI_CLK_PIN | QSPI_D1_PIN | QSPI_D0_PIN;
//	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
//	GPIO_InitStruct.Pull = GPIO_NOPULL;
//	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
//	GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;
//	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
//
//	GPIO_InitStruct.Pin = QSPI_D3_PIN | QSPI_D2_PIN;
//	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
//	GPIO_InitStruct.Pull = GPIO_NOPULL;
//	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
//	GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;
//	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
//
//	/*	    __HAL_SYSCFG_FASTMODEPLUS_ENABLE(SYSCFG_FASTMODEPLUS_PB8);
//
//	 __HAL_SYSCFG_FASTMODEPLUS_ENABLE(SYSCFG_FASTMODEPLUS_PB9);*/
//
//	/* USER CODE BEGIN QUADSPI_MspInit 1 */
//
//	/* USER CODE END QUADSPI_MspInit 1 */
//}

/**
 * @brief QSPI MSP De-Initialization
 *        This function frees the hardware resources used in this example:
 *          - Disable the Peripheral's clock
 *          - Revert GPIO and NVIC configuration to their default state
 * @retval None
 */
__weak void BSP_QSPI_MspDeInit(QSPI_HandleTypeDef *hqspi, void *Params) {
	/*##-1- Disable the NVIC for QSPI ###########################################*/
	HAL_NVIC_DisableIRQ(QUADSPI_IRQn);

	/*##-2- Disable peripherals and GPIO Clocks ################################*/
	/* De-Configure QSPI pins */
	HAL_GPIO_DeInit(QSPI_CS_GPIO_PORT, QSPI_CS_PIN);
	HAL_GPIO_DeInit(QSPI_CLK_GPIO_PORT, QSPI_CLK_PIN);
	HAL_GPIO_DeInit(QSPI_D0_GPIO_PORT, QSPI_D0_PIN);
	HAL_GPIO_DeInit(QSPI_D1_GPIO_PORT, QSPI_D1_PIN);
	HAL_GPIO_DeInit(QSPI_D2_GPIO_PORT, QSPI_D2_PIN);
	HAL_GPIO_DeInit(QSPI_D3_GPIO_PORT, QSPI_D3_PIN);

	/*##-3- Reset peripherals ##################################################*/
	/* Reset the QuadSPI memory interface */
	QSPI_FORCE_RESET();
	QSPI_RELEASE_RESET();

	/* Disable the QuadSPI memory interface clock */
	QSPI_CLK_DISABLE();
}

/**
 * @brief  This function reset the QSPI memory.
 * @param  hqspi: QSPI handle
 * @retval None
 */
static uint8_t QSPI_ResetMemory(QSPI_HandleTypeDef *hqspi) {
	QSPI_CommandTypeDef s_command;
	QSPI_AutoPollingTypeDef s_config;
	uint8_t reg;

	/* Send command RESET command in QPI mode (QUAD I/Os) */
	/* Initialize the reset enable command */
	s_command.InstructionMode = QSPI_INSTRUCTION_4_LINES;
	s_command.Instruction = RESET_ENABLE_CMD;
	s_command.AddressMode = QSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DataMode = QSPI_DATA_NONE;
	s_command.DummyCycles = 0;
	s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
	s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
	s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
	/* Send the command */
	if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE)
			!= HAL_OK) {
		return QSPI_ERROR;
	}
	/* Send the reset memory command */
	s_command.Instruction = RESET_MEMORY_CMD;
	if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE)
			!= HAL_OK) {
		return QSPI_ERROR;
	}

	/* Send command RESET command in SPI mode */
	/* Initialize the reset enable command */
	s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_command.Instruction = RESET_ENABLE_CMD;
	/* Send the command */
	if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE)
			!= HAL_OK) {
		return QSPI_ERROR;
	}
	/* Send the reset memory command */
	s_command.Instruction = RESET_MEMORY_CMD;
	if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE)
			!= HAL_OK) {
		return QSPI_ERROR;
	}

	/* After reset CMD, 1000ms requested if QSPI memory SWReset occured during full chip erase operation */
	HAL_Delay(1000);

	/* Configure automatic polling mode to wait the WIP bit=0 */
	s_config.Match = 0;
	s_config.Mask = MX25L512_SR_WIP;
	s_config.MatchMode = QSPI_MATCH_MODE_AND;
	s_config.StatusBytesSize = 1;
	s_config.Interval = 0x10;
	s_config.AutomaticStop = QSPI_AUTOMATIC_STOP_ENABLE;

	s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_command.Instruction = READ_STATUS_REG_CMD;
	s_command.DataMode = QSPI_DATA_1_LINE;

	if (HAL_QSPI_AutoPolling(hqspi, &s_command, &s_config,
			HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		return QSPI_ERROR;
	}

	/* Initialize the reading of status register */
	s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_command.Instruction = READ_STATUS_REG_CMD;
	s_command.AddressMode = QSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DataMode = QSPI_DATA_1_LINE;
	s_command.DummyCycles = 0;
	s_command.NbData = 1;
	s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
	s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
	s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

	/* Configure the command */
	if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE)
			!= HAL_OK) {
		return QSPI_ERROR;
	}

	/* Reception of the data */
	if (HAL_QSPI_Receive(hqspi, &reg, HAL_QPSI_TIMEOUT_DEFAULT_VALUE)
			!= HAL_OK) {
		return QSPI_ERROR;
	}
#ifdef DEBUG
//  DEBUG_PRINTF("Status Reg : %02x\r\n",reg);
#endif
	/* Enable write operations, command in 1 bit */
	/* Enable write operations */
	s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_command.Instruction = WRITE_ENABLE_CMD;
	s_command.AddressMode = QSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DataMode = QSPI_DATA_NONE;
	s_command.DummyCycles = 0;
	s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
	s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
	s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

	if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE)
			!= HAL_OK) {
		return QSPI_ERROR;
	}

	/* Configure automatic polling mode to wait for write enabling */
	s_config.Match = MX25L512_SR_WREN;
	s_config.Mask = MX25L512_SR_WREN;
	s_config.MatchMode = QSPI_MATCH_MODE_AND;
	s_config.StatusBytesSize = 1;
	s_config.Interval = 0x10;
	s_config.AutomaticStop = QSPI_AUTOMATIC_STOP_ENABLE;

	s_command.Instruction = READ_STATUS_REG_CMD;
	s_command.DataMode = QSPI_DATA_1_LINE;

	if (HAL_QSPI_AutoPolling(hqspi, &s_command, &s_config,
			HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		return QSPI_ERROR;
	}

	/* Update the configuration register with new dummy cycles */
	s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_command.Instruction = WRITE_STATUS_CFG_REG_CMD;
	s_command.AddressMode = QSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DataMode = QSPI_DATA_1_LINE;
	s_command.DummyCycles = 0;
	s_command.NbData = 1;
	s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
	s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
	s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

	/* Enable the Quad IO on the QSPI memory (Non-volatile bit) */
	reg |= MX25L512_SR_QUADEN;

	/* Configure the command */
	if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE)
			!= HAL_OK) {
		return QSPI_ERROR;
	}

	/* Transmission of the data */
	if (HAL_QSPI_Transmit(hqspi, &reg, HAL_QPSI_TIMEOUT_DEFAULT_VALUE)
			!= HAL_OK) {
		return QSPI_ERROR;
	}

	/* 40ms  Write Status/Configuration Register Cycle Time */
	HAL_Delay(40);

	return QSPI_OK;
}

/**
 * @brief  This function set the QSPI memory in 4-byte address mode
 * @param  hqspi: QSPI handle
 * @retval None
 */
static uint8_t QSPI_EnterFourBytesAddress(QSPI_HandleTypeDef *hqspi) {
	QSPI_CommandTypeDef s_command;

	/* Initialize the command */
	s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_command.Instruction = ENTER_4_BYTE_ADDR_MODE_CMD;
	s_command.AddressMode = QSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DataMode = QSPI_DATA_NONE;
	s_command.DummyCycles = 0;
	s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
	s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
	s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

	/* Enable write operations */
	if (QSPI_WriteEnable(hqspi) != QSPI_OK) {
		return QSPI_ERROR;
	}

	/* Send the command */
	if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE)
			!= HAL_OK) {
		return QSPI_ERROR;
	}

	/* Configure automatic polling mode to wait the memory is ready */
	if (QSPI_AutoPollingMemReady(hqspi,
			HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != QSPI_OK) {
		return QSPI_ERROR;
	}

	return QSPI_OK;
}

/**
 * @brief  This function configure the dummy cycles on memory side.
 * @param  hqspi: QSPI handle
 * @retval None
 */
static uint8_t QSPI_DummyCyclesCfg(QSPI_HandleTypeDef *hqspi) {
	QSPI_CommandTypeDef s_command;
	uint8_t reg[2];

	/* Initialize the reading of status register */
	s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_command.Instruction = READ_STATUS_REG_CMD;
	s_command.AddressMode = QSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DataMode = QSPI_DATA_1_LINE;
	s_command.DummyCycles = 0;
	s_command.NbData = 1;
	s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
	s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
	s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

	/* Configure the command */
	if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE)
			!= HAL_OK) {
		return QSPI_ERROR;
	}

	/* Reception of the data */
	if (HAL_QSPI_Receive(hqspi, &(reg[0]), HAL_QPSI_TIMEOUT_DEFAULT_VALUE)
			!= HAL_OK) {
		return QSPI_ERROR;
	}

	/* Initialize the reading of configuration register */
	s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_command.Instruction = READ_CFG_REG_CMD;
	s_command.AddressMode = QSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DataMode = QSPI_DATA_1_LINE;
	s_command.DummyCycles = 0;
	s_command.NbData = 1;
	s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
	s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
	s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

	/* Configure the command */
	if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE)
			!= HAL_OK) {
		return QSPI_ERROR;
	}

	/* Reception of the data */
	if (HAL_QSPI_Receive(hqspi, &(reg[1]), HAL_QPSI_TIMEOUT_DEFAULT_VALUE)
			!= HAL_OK) {
		return QSPI_ERROR;
	}

	/* Enable write operations */
	if (QSPI_WriteEnable(hqspi) != QSPI_OK) {
		return QSPI_ERROR;
	}

	/* Update the configuration register with new dummy cycles */
	s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_command.Instruction = WRITE_STATUS_CFG_REG_CMD;
	s_command.AddressMode = QSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DataMode = QSPI_DATA_1_LINE;
	s_command.DummyCycles = 0;
	s_command.NbData = 2;
	s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
	s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
	s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

	/* MX25L512_DUMMY_CYCLES_READ_QUAD = 3 for 10 cycles in QPI mode */
	MODIFY_REG(reg[1], MX25L512_CR_NB_DUMMY,
			(MX25L512_DUMMY_CYCLES_READ_QUAD << POSITION_VAL(MX25L512_CR_NB_DUMMY)));

	/* Configure the write volatile configuration register command */
	if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE)
			!= HAL_OK) {
		return QSPI_ERROR;
	}

	/* Transmission of the data */
	if (HAL_QSPI_Transmit(hqspi, &(reg[0]), HAL_QPSI_TIMEOUT_DEFAULT_VALUE)
			!= HAL_OK) {
		return QSPI_ERROR;
	}

	/* 40ms  Write Status/Configuration Register Cycle Time */
	HAL_Delay(40);

	return QSPI_OK;
}

/**
 * @brief  This function put QSPI memory in QPI mode (quad I/O).
 * @param  hqspi: QSPI handle
 * @retval None
 */
static uint8_t QSPI_EnterMemory_QPI(QSPI_HandleTypeDef *hqspi) {
	QSPI_CommandTypeDef s_command;
	QSPI_AutoPollingTypeDef s_config;

	/* Initialize the QPI enable command */
	/* QSPI memory is supported to be in SPI mode, so CMD on 1 LINE */
	s_command.InstructionMode = QSPI_INSTRUCTION_4_LINES;
	s_command.Instruction = ENTER_QUAD_CMD;
	s_command.AddressMode = QSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DataMode = QSPI_DATA_NONE;
	s_command.DummyCycles = 0;
	s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
	s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
	s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

	/* Send the command */
	if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE)
			!= HAL_OK) {
		return QSPI_ERROR;
	}

	/* Configure automatic polling mode to wait the QUADEN bit=1 and WIP bit=0 */
	s_config.Match = MX25L512_SR_QUADEN;
	s_config.Mask = MX25L512_SR_QUADEN | MX25L512_SR_WIP;
	s_config.MatchMode = QSPI_MATCH_MODE_AND;
	s_config.StatusBytesSize = 1;
	s_config.Interval = 0x10;
	s_config.AutomaticStop = QSPI_AUTOMATIC_STOP_ENABLE;

	s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_command.Instruction = READ_STATUS_REG_CMD;
	s_command.DataMode = QSPI_DATA_1_LINE;

	if (HAL_QSPI_AutoPolling(hqspi, &s_command, &s_config,
			HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		return QSPI_ERROR;
	}

	return QSPI_OK;
}

/**
 * @brief  This function put QSPI memory in SPI mode.
 * @param  hqspi: QSPI handle
 * @retval None
 */
static uint8_t QSPI_ExitMemory_QPI(QSPI_HandleTypeDef *hqspi) {
	QSPI_CommandTypeDef s_command;

	/* Initialize the QPI enable command */
	/* QSPI memory is supported to be in QPI mode, so CMD on 4 LINES */
	s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_command.Instruction = EXIT_QUAD_CMD;
	s_command.AddressMode = QSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DataMode = QSPI_DATA_NONE;
	s_command.DummyCycles = 0;
	s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
	s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
	s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

	/* Send the command */
	if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE)
			!= HAL_OK) {
		return QSPI_ERROR;
	}

	return QSPI_OK;
}

///**
//  * @brief  This function configure the Output driver strength on memory side.
//  * @param  hqspi: QSPI handle
//  * @retval None
//  */
//static uint8_t QSPI_OutDrvStrengthCfg( QSPI_HandleTypeDef *hqspi )
//{
//  QSPI_CommandTypeDef s_command;
//  uint8_t reg[2];
//
//  /* Initialize the reading of status register */
//  s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
//  s_command.Instruction       = READ_STATUS_REG_CMD;
//  s_command.AddressMode       = QSPI_ADDRESS_NONE;
//  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
//  s_command.DataMode          = QSPI_DATA_1_LINE;
//  s_command.DummyCycles       = 0;
//  s_command.NbData            = 1;
//  s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
//  s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
//  s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
//
//  /* Configure the command */
//  if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
//  {
//    return QSPI_ERROR;
//  }
//
//  /* Reception of the data */
//  if (HAL_QSPI_Receive(hqspi, &(reg[0]), HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
//  {
//    return QSPI_ERROR;
//  }
//
//  /* Initialize the reading of configuration register */
//  s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
//  s_command.Instruction       = READ_CFG_REG_CMD;
//  s_command.AddressMode       = QSPI_ADDRESS_NONE;
//  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
//  s_command.DataMode          = QSPI_DATA_1_LINE;
//  s_command.DummyCycles       = 0;
//  s_command.NbData            = 1;
//  s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
//  s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
//  s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
//
//  /* Configure the command */
//  if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
//  {
//    return QSPI_ERROR;
//  }
//
//  /* Reception of the data */
//  if (HAL_QSPI_Receive(hqspi, &(reg[1]), HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
//  {
//    return QSPI_ERROR;
//  }
//
//  /* Enable write operations */
//  if (QSPI_WriteEnable(&QSPIHandle) != QSPI_OK)
//  {
//    return QSPI_ERROR;
//  }
//
//  /* Update the configuration register with new output driver strength */
//  s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
//  s_command.Instruction       = WRITE_STATUS_CFG_REG_CMD;
//  s_command.AddressMode       = QSPI_ADDRESS_NONE;
//  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
//  s_command.DataMode          = QSPI_DATA_1_LINE;
//  s_command.DummyCycles       = 0;
//  s_command.NbData            = 2;
//  s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
//  s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
//  s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
//
//  /* Set Output Strength of the QSPI memory 15 ohms */
//  MODIFY_REG( reg[1], MX25L512_CR_ODS, (MX25L512_CR_ODS_30 << POSITION_VAL(MX25L512_CR_ODS)));
//
//  /* Configure the write volatile configuration register command */
//  if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
//  {
//    return QSPI_ERROR;
//  }
//
//  /* Transmission of the data */
//  if (HAL_QSPI_Transmit(hqspi, &(reg[0]), HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
//  {
//    return QSPI_ERROR;
//  }
//
//  return QSPI_OK;
//}

/**
 * @brief  This function send a Write Enable and wait it is effective.
 * @param  hqspi: QSPI handle
 * @retval None
 */
static uint8_t QSPI_WriteEnable(QSPI_HandleTypeDef *hqspi) {
	QSPI_CommandTypeDef s_command;
	QSPI_AutoPollingTypeDef s_config;
	uint8_t ret = 0;

	/* Enable write operations */
	s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_command.Instruction = WRITE_ENABLE_CMD;
	s_command.AddressMode = QSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DataMode = QSPI_DATA_NONE;
	s_command.DummyCycles = 0;
	s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
	s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
	s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

	if ((ret = HAL_QSPI_Command(hqspi, &s_command,
			HAL_QPSI_TIMEOUT_DEFAULT_VALUE)) != HAL_OK) {
		return QSPI_ERROR;
	}

	/* Configure automatic polling mode to wait for write enabling */
	s_config.Match = 0x02;
	s_config.Mask = 0x02;
	s_config.MatchMode = QSPI_MATCH_MODE_AND;
	s_config.StatusBytesSize = 1;
	s_config.Interval = 0x10;
	s_config.AutomaticStop = QSPI_AUTOMATIC_STOP_ENABLE;

	s_command.Instruction = READ_STATUS_REG_CMD;
	s_command.DataMode = QSPI_DATA_1_LINE;

	if ((ret = HAL_QSPI_AutoPolling(hqspi, &s_command, &s_config,
			HAL_QPSI_TIMEOUT_DEFAULT_VALUE)) != HAL_OK) {
		return QSPI_ERROR;
	}

	return QSPI_OK;
}

/**
 * @brief  This function read the SR of the memory and wait the EOP.
 * @param  hqspi: QSPI handle
 * @param  Timeout
 * @retval None
 */
static uint8_t QSPI_AutoPollingMemReady(QSPI_HandleTypeDef *hqspi,
		uint32_t Timeout) {
	QSPI_CommandTypeDef s_command;
	QSPI_AutoPollingTypeDef s_config;

	/* Configure automatic polling mode to wait for memory ready */
	s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_command.Instruction = READ_STATUS_REG_CMD;
	s_command.AddressMode = QSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DataMode = QSPI_DATA_1_LINE;
	s_command.DummyCycles = 0;
	s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
	s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
	s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

	s_config.Match = 0;
	s_config.Mask = MX25L512_SR_WIP;
	s_config.MatchMode = QSPI_MATCH_MODE_AND;
	s_config.StatusBytesSize = 1;
	s_config.Interval = 0x10;
	s_config.AutomaticStop = QSPI_AUTOMATIC_STOP_ENABLE;

	if (HAL_QSPI_AutoPolling(hqspi, &s_command, &s_config, Timeout) != HAL_OK) {
		return QSPI_ERROR;
	}

	return QSPI_OK;
}

/**
 * @brief  this function chack the BUSY flash of flash.
 * @retval None
 */
uint8_t waitForFlashbusy(void) {
	uint32_t Tickstart = HAL_GetTick();
	while (BSP_QSPI_GetStatus() == QSPI_BUSY) {
		/* Check for the Timeout */
		if (FLASH_STATUSE_TIMEOUT != HAL_MAX_DELAY) {
			if (((HAL_GetTick() - Tickstart) > FLASH_STATUSE_TIMEOUT)
					|| (FLASH_STATUSE_TIMEOUT == 0U)) {

				return (QSPI_BUSY);
			}
		}
	}
	return (QSPI_OK);
}


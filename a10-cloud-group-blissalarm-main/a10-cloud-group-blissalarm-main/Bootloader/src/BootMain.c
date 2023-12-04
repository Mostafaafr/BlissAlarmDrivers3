/**************************************************************************//**
* @file      BootMain.c
* @brief     Main file for the ESE516 bootloader. Handles updating the main application
* @details   Main file for the ESE516 bootloader. Handles updating the main application
* @author    Eduardo Garcia
* @author 	 Abir Hossain
* @author 	 Mostafa Afr
* @date      2020-02-15
* @version   2.0
* @copyright Copyright University of Pennsylvania
******************************************************************************/


/******************************************************************************
* Includes
******************************************************************************/
#include <asf.h>                             /*Include the advanced software framework that provides a rich set of 
                                             proven drivers and code modules for SAM Devices*/
#include "conf_example.h"                    /*Include the configuration file*/
#include <string.h>                          /*Include the standard string library*/
#include "sd_mmc_spi.h"                      /*Include the Secure Digital (SD) library using SPI*/

#include "SD Card/SdCard.h"                  /*Include the Sdcard library*/
#include "Systick/Systick.h"                 /*Include the ARM Cortex system timer to configure
                                                the system real time tick*/
#include "SerialConsole/SerialConsole.h"     /*Include Serial console library for debugging
                                                and unit testing*/
#include "ASF/sam0/drivers/dsu/crc32/crc32.h" /*Include cyclic redundancy check library
                                                for the MCU to check handshaking*/





/******************************************************************************
* Defines
******************************************************************************/
#define APP_START_ADDRESS  ((uint32_t)0x12000) /* The start address of the main application*/
#define APP_START_RESET_VEC_ADDRESS (APP_START_ADDRESS+(uint32_t)0x04) /* Defines the reset vector address for the main
                                                                            application*/
#define BUFFER_SIZE 64                                                  /* Defines the buffer size for the NVM*/
#define ROW_SIZE 256                                                   /* Defines the NVM Row size*/



/*Global Structures ------------------------------------------------------------------------------------------------------*/

struct usart_module cdc_uart_module; /*Declare a usart module for CDC to help in E-Debug*/

/******************************************************************************
* Local Function Declaration
******************************************************************************/
static void jumpToApplication(void);
static bool filesystem_test(void);
static void configure_nvm(void);
static bool NVM_erase_Pages(void);
static bool NVM_write_Pages(char* filename);
static void Start_Bootloader(void);
static void Peripheral_Init(void);
static void HW_Deinit(void);

/******************************************************************************
* Global Variables
******************************************************************************/
//INITIALIZE VARIABLES
char test_file_name[] = "0:sd_mmc_test.txt";	/*Defines the test file name with a pointer to char*/
char test_bin_file[] = "0:sd_binary.bin";	/*Defines the binary file name for test*/

char testA_filename[] = "0:FlagA.txt";  /*Defines the binary file name for test2*/
char testA_bin_file[] = "0:TestA.bin"; /*Defines the test1 file name with a pointer to char*/

char testB_filename[] = "0:FlagB.txt"; /*Defines the test2 file name with a pointer to char*/
char testB_bin_file[] = "0:TestB.bin";  /*Defines the binary file name for test2*/
char helpStr[64];                       /*Defines the debugging string*/

bool isTaskA = true;
bool FlagA = false;                      /* Task1 flag*/
Ctrl_status status;                     /*Defines the system initialization status*/
FRESULT FATFS_Result;                   /*Stores the result of the FATFS functions
                                        done on the SD CARD TEST*/
FATFS fs;                               /*Holds the File System of the SD CARD*/
FIL file_object;                        /*FILE OBJECT used on main for the SD Card Test*/


/******************************************************************************
* Global Functions
******************************************************************************/

/**
 *  @fn int main(void) 
 *  @brief The main application entry point for ESE516 bootloader application
 *  @retval int
 *  @note The bootloader code is initiated here.
*/

int main(void)
{

    /*
     * Initialize the system peripherals
     * Initialize the Nested Interrupt controller (NVIC)
     * Initialize the system timer (SYSTICK) 
    */
    Peripheral_Init();
    /*
    * SD CARD Mounting and testing!!
    */
    /* Initiate the communication and Add a string to Rx buffer*/

	SerialConsoleWriteString("ESE516 - ENTER BOOTLOADER");	//Order to add string to TX Buffer

	/*END SYSTEM PERIPHERALS INITIALIZATION*/

    /* Mount the SD CARD and Try to write to a file*/
	SerialConsoleWriteString("\x0C\n\r-- SD/MMC Card Example on FatFs --\n\r");
     /* Test the SD CARD mounting and connection*/
	if(filesystem_test() == false)
	{
		SerialConsoleWriteString("SD CARD failed! Check your connections. System will restart in 5 seconds...\r\n");
		delay_cycles_ms(5000);      /* Delay for 5 seconds*/
		system_reset();     /*Reset the system configurations and peripherals*/
	}
	else
	{
		SerialConsoleWriteString("SD CARD mount success! Filesystem also mounted. \r\n");
	}

	/*END SIMPLE SD CARD MOUNTING AND TEST!*/


	/*3.) STARTS BOOTLOADER HERE!*/

	//PERFORM BOOTLOADER HERE!
    Start_Bootloader();
	/*END BOOTLOADER HERE!*/

	/*DEINITIALIZE HW AND JUMP TO MAIN APPLICATION!*/
	SerialConsoleWriteString("ESE516 - EXIT BOOTLOADER \r\n");	//Order to add string to TX Buffer
	delay_cycles_ms(100); //Delay to allow print

	
    HW_Deinit();

	/*Jump to application*/
	SerialConsoleWriteString("Jumping to Application\r\n");
	jumpToApplication();

	}



/******************************************************************************
* Static Functions
******************************************************************************/

/**************************************************************************//**
* function      static void NVM_erase_Pages()
* @brief        Erase all pages in the NVM
* @details      Erase all pages in the non-volatile memory NVM
* @return       Returns true in case of no NVM error while erasing
******************************************************************************/
static bool NVM_erase_Pages(void)
{
	uint32_t current_addr = APP_START_ADDRESS;
	int  row = (0x40000 - APP_START_ADDRESS) / 256;
	for (int i = 0; i < row; i++){
		enum status_code nvmError = nvm_erase_row(current_addr);
		if(nvmError != STATUS_OK)
	{
	SerialConsoleWriteString("Error: Erase error occured. \r\n");
	return false;
}
	// Make sure it got erased. Erasure in NVM is an 0xFF
	for(int iter = 0; iter < 256; iter++)
	{
		char *a = (char *)(APP_START_ADDRESS + iter); //Pointer pointing to address
		if(*a != 0xFF)
		{
			SerialConsoleWriteString("Error: Page was not erased! \r\n");
			break;
		}
	}
	current_addr += 256;
	}
	SerialConsoleWriteString("Pages are erased! \r\n");
	return true;
}



/**************************************************************************//**
* function      static void NVM_write_Pages()
* @brief        write all pages in the NVM
* @return       Returns true in case of no NVM error while writing
******************************************************************************/
static bool NVM_write_Pages(char* filename)
{
	// Create storage variables
	uint32_t resultCrcSd = 0;
	uint32_t resultCrcNVM = 0;
	uint8_t readBuffer[256];
	FIL file_object_bin;
	struct nvm_parameters parameters;
	nvm_get_parameters (&parameters); //Get NVM parameters

	// Setup reading files and creating FS
	int index = 0;
	int addr_offset = 256;
	int numBytesRead = 0;
	int numberBytesTotal = 0;
	FATFS_Result = f_open(&file_object_bin, filename, FA_READ);
	int numBytesLeft = f_size(&file_object_bin);
	if(FATFS_Result != FR_OK){
		SerialConsoleWriteString("Error: Can NOT find corresponding binary file.\r\n");
		return false;
	}

	//read all file
	while(numBytesLeft > 0){
		FATFS_Result = f_read(&file_object_bin, &readBuffer[0],  ROW_SIZE, &numBytesRead);//read a row = 256 Bytes
		numBytesLeft -= numBytesRead;
		numberBytesTotal += numBytesRead;
		if(FATFS_Result != FR_OK){
			SerialConsoleWriteString("Error: Could not read file. \r\n");
			f_close(&file_object_bin);
			return false;
		}

	int32_t cur_address = APP_START_ADDRESS + (addr_offset * index);

	//write 256 Bytes = 4 times 64 Bytes
	FATFS_Result = nvm_write_buffer(cur_address, &readBuffer[0], BUFFER_SIZE);
	FATFS_Result = nvm_write_buffer(cur_address + 64, &readBuffer[64], BUFFER_SIZE);
	FATFS_Result = nvm_write_buffer(cur_address + 128, &readBuffer[128], BUFFER_SIZE);
	FATFS_Result = nvm_write_buffer(cur_address + 192, &readBuffer[192], BUFFER_SIZE);

	// ERRATA Part 1 - To be done before RAM CRC
	uint32_t resultCrcSd = 0;
	*((volatile unsigned int*) 0x41007058) &= ~0x30000UL;

	// CRC of SD Card
	enum status_code crcres = dsu_crc32_cal	(readBuffer	,256, &resultCrcSd); //Instructor note: Was it the third parameter used for? Please check how you can use the third parameter to do the CRC of a long data stream in chunks - you will need it!

	// Errata Part 2 - To be done after RAM CRC
	*((volatile unsigned int*) 0x41007058) |= 0x20000UL;

	//CRC of memory (NVM)
	uint32_t resultCrcNvm = 0;
	crcres |= dsu_crc32_cal	(APP_START_ADDRESS, 256, &resultCrcNvm);

	if (crcres != STATUS_OK)
	{
		SerialConsoleWriteString("Error: Could not calculate CRC! \r\n");
	}
	else
	{
		snprintf(helpStr, 63,"CRC SD CARD: %d  CRC NVM: %d \r\n", resultCrcSd, resultCrcNvm);
		SerialConsoleWriteString(helpStr);
	}
	index++;
}

	if(resultCrcSd != resultCrcNVM){
		SerialConsoleWriteString("Error: CRC ERROR \r\n");
		snprintf(helpStr, 63,"CRC SD CARD: %d  CRC NVM: %d \r\n", resultCrcSd, resultCrcNVM);
		SerialConsoleWriteString(helpStr);
		f_close(&file_object_bin);
		return false;
	}

	f_close(&file_object_bin);
	return true;

}


/**************************************************************************//**
* function      static void filesystem_test()
* @brief        Starts the filesystem and tests it. Sets the filesystem to the global variable fs
* @details      Jumps to the main application. Please turn off ALL PERIPHERALS that were turned on by the bootloader
*				before performing the jump!
* @return       Returns true is SD card and file system test passed. False otherwise.
******************************************************************************/
static bool filesystem_test(void)
{
	bool sdCardPass = true;
	uint8_t binbuff[256];

	//Before we begin - fill buffer for binary write test
	//Fill binbuff with values 0x00 - 0xFF
	for(int i = 0; i < 256; i++)
	{
		binbuff[i] = i;
	}

	//MOUNT SD CARD
	Ctrl_status sdStatus= SdCard_Initiate();
	if(sdStatus == CTRL_GOOD) //If the SD card is good we continue mounting the system!
	{
		SerialConsoleWriteString("SD Card initiated correctly!\n\r");

		//Attempt to mount a FAT file system on the SD Card using FATFS
		SerialConsoleWriteString("Mount disk (f_mount)...\r\n");
		memset(&fs, 0, sizeof(FATFS));
		FATFS_Result = f_mount(LUN_ID_SD_MMC_0_MEM, &fs); //Order FATFS Mount
	if (FR_INVALID_DRIVE == FATFS_Result)
	{
		LogMessage(LOG_INFO_LVL ,"[FAIL] res %d\r\n", FATFS_Result);
		sdCardPass = false;
		goto main_end_of_test;
	}
	SerialConsoleWriteString("[OK]\r\n");

	//Create and open a file
	SerialConsoleWriteString("Create a file (f_open)...\r\n");

	test_file_name[0] = LUN_ID_SD_MMC_0_MEM + '0';
	FATFS_Result = f_open(&file_object,
	(char const *)test_file_name,
	FA_CREATE_ALWAYS | FA_WRITE);

	if (FATFS_Result != FR_OK)
	{
		LogMessage(LOG_INFO_LVL ,"[FAIL] res %d\r\n", FATFS_Result);
		sdCardPass = false;
		goto main_end_of_test;
	}

	SerialConsoleWriteString("[OK]\r\n");

	//Write to a file
	SerialConsoleWriteString("Write to test file (f_puts)...\r\n");

	if (0 == f_puts("Test SD/MMC stack\n", &file_object))
	{
		f_close(&file_object);
		LogMessage(LOG_INFO_LVL ,"[FAIL]\r\n");
		sdCardPass = false;
		goto main_end_of_test;
	}

	SerialConsoleWriteString("[OK]\r\n");
	f_close(&file_object); //Close file
	SerialConsoleWriteString("Test is successful.\n\r");

	//Read SD Card File
	test_bin_file[0] = LUN_ID_SD_MMC_0_MEM + '0';
	FATFS_Result = f_open(&file_object, (char const *)test_bin_file, FA_WRITE | FA_CREATE_ALWAYS);

	if (FATFS_Result != FR_OK)
	{
		SerialConsoleWriteString("Error: Could not open binary file!\r\n");
		LogMessage(LOG_INFO_LVL ,"[FAIL] res %d\r\n", FATFS_Result);
		sdCardPass = false;
		goto main_end_of_test;
	}

	// Write to a binaryfile
	SerialConsoleWriteString("Write to test file (f_write)...\r\n");
	uint32_t varWrite = 0;
	if (0 != f_write(&file_object, binbuff,256, (UINT*) &varWrite))
	{
		f_close(&file_object);
		LogMessage(LOG_INFO_LVL ,"[FAIL]\r\n");
		sdCardPass = false;
		goto main_end_of_test;
	}

	SerialConsoleWriteString("[OK]\r\n");
	f_close(&file_object); //Close file
	SerialConsoleWriteString("Test is successful.\n\r");

	main_end_of_test:
	SerialConsoleWriteString("End of Test.\n\r");

	}
	else
	{
		SerialConsoleWriteString("SD Card failed initiation! Check connections!\n\r");
		sdCardPass = false;
	}

	return sdCardPass;
	}



/**************************************************************************//**
* function      static void jumpToApplication(void)
* @brief        Jumps to main application
* @details      Jumps to the main application. Please turn off ALL PERIPHERALS that were turned on by the bootloader
*				before performing the jump!
* @return
******************************************************************************/
static void jumpToApplication(void)
{
	SerialConsoleWriteString("Jumping to Application!\r\n");
	// Function pointer to application section
	void (*applicationCodeEntry)(void);

	// Rebase stack pointer
	__set_MSP(*(uint32_t *) APP_START_ADDRESS);

	// Rebase vector table
	SCB->VTOR = ((uint32_t) APP_START_ADDRESS & SCB_VTOR_TBLOFF_Msk);

	// Set pointer to application section
	applicationCodeEntry =
	(void (*)(void))(unsigned *)(*(unsigned *)(APP_START_RESET_VEC_ADDRESS));

	// Jump to application. By calling applicationCodeEntry() as a function we move the PC to the point in memory pointed by applicationCodeEntry,
	//which should be the start of the main FW.
	applicationCodeEntry();
}



/**************************************************************************//**
* function      static void configure_nvm(void)
* @brief        Configures the NVM driver
* @details
* @return
******************************************************************************/
static void configure_nvm(void)
{
	struct nvm_config config_nvm;
	nvm_get_config_defaults(&config_nvm);
	config_nvm.manual_page_write = false;
	nvm_set_config(&config_nvm);
}

/**
 * function     static void Start_Bootloader(void)
 * @brief 		Starts the bootload function.
 * @details
 * @return 
 */
static void Start_Bootloader(void)
{
    #define MEM_EXAMPLE
	#ifdef MEM_EXAMPLE
	testA_filename[0] = LUN_ID_SD_MMC_0_MEM + '0';
	testA_bin_file[0] = LUN_ID_SD_MMC_0_MEM + '0';
	testB_filename[0] = LUN_ID_SD_MMC_0_MEM + '0';
	testB_bin_file[0] = LUN_ID_SD_MMC_0_MEM + '0';
	FlagA = true;
	FATFS_Result = f_open(&file_object, (char const *)testA_filename, FA_READ);
	f_close(&file_object);
	if (FATFS_Result == FR_OK)
	{
		FlagA = true;
	}
// 	else
// 	{
// 		FATFS_Result = f_open(&file_object, (char const *)testB_filename, FA_READ);
// 		f_close(&file_object);
// 		if (FATFS_Result != FR_OK)
// 		{
// 			FlagA = true;
// 		}
// 	}


	if(FlagA == true){
		NVM_erase_Pages();
		NVM_write_Pages(testA_bin_file);
		f_unlink(testA_filename);
	}
// 	else
// 	{
// 		NVM_erase_Pages();
// 		NVM_write_Pages(testB_bin_file);
// 		f_unlink(testB_filename);
// 	}
	#endif
}

/**
 * function		static void Peripheral_Init(void)
 * @brief 		Initializes the peripherals of the HW.
 * @details
 * @return 		void
 */
static void Peripheral_Init(void)
{
    
	system_init();
    /* Initialize the delay driver to provide simple delay loops */
	delay_init();
    /* Initialize the serial console for application tracing and debugging */
	InitializeSerialConsole();
    /* Enable the system interrupts */
	system_interrupt_enable_global();
	/* Initialize SD MMC stack */
	sd_mmc_init();

	/* Configure the Non-volatile memory driver (NVM)*/
	configure_nvm();
    /* Initialize the interrupt vector table and enable CPU interrupts*/
	irq_initialize_vectors();
	cpu_irq_enable();

	/* Configure the cyclic redundancy check for string CRC32*/
	dsu_crc32_init();
}

/**
 * function 	static void HW_Deinit(void)
 * @brief 		Deinitializes the hardware
 * @details
 * @return 		void
 */
static void HW_Deinit(void)
{
    /*
    Deinitialize HW - deinitialize started HW here!
    */
	DeinitializeSerialConsole(); /*Deinitializes UART*/
	sd_mmc_deinit(); /*Deinitialize SD CARD*/
}
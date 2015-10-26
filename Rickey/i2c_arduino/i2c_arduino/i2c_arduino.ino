#include "i2c_arduino.h"
#include <Wire.h>
#define ADUC7122_LOADER_ADDR (0x04 >> 1)

// Global Vars
char inputBuffer[10] = "", serial_in, oneHexValue[5] = "0x";
byte _i2caddr = ADUC7122_LOADER_ADDR;
unsigned char szTxData[255 + 2 + 1 + 1];  // 255 Data Bytes, 2 Start ID bytes, 1 Num of Bytes, and 1 Checksum byte
unsigned char TxDataBytes[255];		// Data bytes to send, include command and memory address. Minimum 5 bytes max 255 bytes
unsigned char TxDataBytesOriginal[255];     //Remembers the original data bytes set (verify operation will not overwrite this one)
unsigned char TxPayloadBytesLen = 0; //Length of the payload (data bytes number 6 to 255). Could be zero for no payload (i.e. The 'Run' command)
unsigned char ucTxCount = 0;        // Array index variable for szTxData[]
unsigned char szRxData[32];       // Array for reading Data from Slave
unsigned char ucRxCount = 0;        // Array index variable for szRxData[]
unsigned char ucWaitingForXIRQ0 = 1;  // Flag to begin reading from Slave - controlled by UART message
unsigned int ucTxCountMax = 1; //Must be int. The send message could be longer than 255
unsigned char ucRxCountMax = 1;
unsigned long szAddr = 0x0;

/*************************
**   Custom functions   **
*************************/

void clcCmd() {
    Serial.flush();
    inputBuffer[0] = '\0'; //Clears command for next round
}


void request_slave_read(int num_bytes) {
    Wire.requestFrom(0x05 >> 1, num_bytes);    // request x bytes from slave device #
}

void run_memory(){
    Serial.print('RUN_MEMORY');
    TxDataBytes[0] = 'R';
    TxDataBytes[1] = 0x00;
    TxDataBytes[2] = 0x00;
    TxDataBytes[3] = 0x00;
    TxDataBytes[4] = 0x01;
    i2c_send_data_bytes(5);
	Serial.print(' OK \n');
}

void set_address() {
	//Collect four bytes to form the address
	Serial.print(" a|0x");
	unsigned int i;
	for(i=1; i<=4; i++){
		Serial.flush();
		while (Serial.available() == 0);
		TxDataBytes[i] = Serial.read();
		Serial.print(' ');
		Serial.print(TxDataBytes[i], HEX);
	}
	Serial.print(" OK \n");
}

// Set the data bytes. From 1 to 250. 
void set_data(){
	unsigned int i;
	Serial.print(" d|0x");
	Serial.flush();
	while (Serial.available() == 0);
	TxPayloadBytesLen = Serial.read();
	if(TxPayloadBytesLen > 250) {
		Serial.print('TOO BIG');
		return;
	}
	Serial.print(TxPayloadBytesLen, HEX);
	Serial.print(" BEGIN ");
	//Put Data Bytes into buffer
	for(i=0; i<TxPayloadBytesLen; i++){
		Serial.flush();
		while (Serial.available() == 0);
		TxDataBytesOriginal[i+5] = Serial.read();
		Serial.print(">");
	}
	Serial.print(" END OK \n");
}

// Write Flash Contents
void write_flash(){
	unsigned int i;
	TxDataBytes[0]='W';
	//copy to buffer
	for(i=0; i<TxPayloadBytesLen; i++){
		TxDataBytes[i+5] = TxDataBytesOriginal[i+5];
	}
	//send it out
	i2c_send_data_bytes(TxPayloadBytesLen + 5);
}

// Verify Flash Contents
// Computes complemented data bytes using existing set_data, then send out
void verify_flash(){
	unsigned int i;
	TxDataBytes[0]='V';
	//Serial.print(" VERIFY-COMPLEMENT 0x");
	//Complemented Data Bytes (ARMv7)
	/* Not the most efficient. 
		 Rotate Left by three bits for each byte (not each char!)
		 But we want to just use i2c_send_data_bytes function for all sending activities
	*/
	for(i=0; i<TxPayloadBytesLen; i++){
		TxDataBytes[i+5] = rotl8(TxDataBytesOriginal[i+5], 3);
		/*Serial.print(rotl8(TxDataBytesOriginal[i+5], 3), HEX);
		Serial.print(rotl8(TxDataBytesOriginal[i+5], 2), HEX);
		Serial.print(rotl8(TxDataBytesOriginal[i+5], 1), HEX);
		Serial.print(rotl8(TxDataBytesOriginal[i+5], 0), HEX);
		Serial.print(" ");*/
	}
	
	i2c_send_data_bytes(TxPayloadBytesLen + 5);
}

unsigned char rotl8 (unsigned char value, unsigned int count) {
    const unsigned int mask = (8*sizeof(value)-1);
    count &= mask;
    return (value<<count) | (value>>( (-count)&mask ));
}

// Erase number of pages of memory starting at memory specified
void erase_memory(int pages){
    TxDataBytes[0] = 'E';
    TxDataBytes[5] = pages;
    i2c_send_data_bytes(6);
}

// Package the data bytes and send out.
// num_bytes should be between 5 and 255.
// The maximum number of data bytes allowed is 255:
// command function, 4-byte address, and 250 bytes of data.
void i2c_send_data_bytes(int num_bytes){
    unsigned int checksum, i;
    checksum = 0x00;
    
    //Construct the szTxData[] array
    //Start Byte
    szTxData[0] = 0x07;
    szTxData[1] = 0x0E;
    //Number of Data Bytes
    szTxData[2] = num_bytes;
    //Data Bytes and compute checksum
    for(i=0; i<num_bytes; i++){
        checksum = TxDataBytes[i] + checksum;
        szTxData[i + 3] = TxDataBytes[i];
    }
    //Compute Checksum
    //Subtract num of data bytes, then two's complement
    checksum = 0x00 - (num_bytes + checksum);
    szTxData[num_bytes + 3] = checksum;
    
    // Begin Master Transmit sequence
    ucTxCountMax = num_bytes + 2 + 1 + 1; //Start ID, Num Bytes, and Checksum
	
	Serial.print(" W|Bytes");
	Serial.print(ucTxCountMax);
	Serial.print("|numbytes");
	Serial.print(num_bytes);
    
    Wire.beginTransmission(_i2caddr); // transmit to device
    Serial.print(" | 0x");
    for(i=0; i<ucTxCountMax; i++){
        Wire.write(szTxData[i]); 
        Serial.print(" ");
        Serial.print(szTxData[i], HEX);
		delay(1); //small delay 
    }
    Wire.endTransmission();    // stop transmitting
	Serial.print(" TXDONE \n");
    // Don't forget to request acknowledge from loader
}

void setup()
{
    Wire.begin(); // join i2c bus (address optional for master)
    Serial.begin(115200);  // start serial for output
    
    Serial.print("Bonjour!");
}

void loop()
{
    //Wait to do something until a character is received on the serial port.
    if (Serial.available()){
        //Copy the incoming character to a variable and fold case
        serial_in = Serial.read();
        //Depending on the incoming character, decide what to do.
        switch (serial_in){
            case 'i': //Init the test
				//Collect Commands
				clcCmd();
				Serial.flush();
				Wire.beginTransmission(_i2caddr); // transmit to device #8
				Wire.write(0x08);              // sends one byte
				Wire.endTransmission();    // stop transmitting
				Serial.print(" Sent ");
				Serial.print(0x08);
				clcCmd();
				break;
			case 'a':
				set_address();
				clcCmd();
				break;
			case 'd':
				Serial.flush();
				set_data();
				clcCmd();
				break;
            case 'e':
				Serial.flush();
				while (Serial.available() == 0);
				serial_in = Serial.read();
				erase_memory(serial_in);
				clcCmd();
				break;
            case 'r':
				//request_slave_read(1);
				Wire.requestFrom(0x05 >> 1, 24);
				clcCmd();
				break;
            case 't':
            //request_slave_read(1);
				Wire.requestFrom(0x05 >> 1, 1);
				clcCmd();
				break;
			//Go run
			case 'g':
				run_memory();
				clcCmd();
				break;
			//Write
			case 'w':
				write_flash();
				clcCmd();
				break;
			case 'v':
				verify_flash();
				clcCmd();
				break;
            default:
				/*Serial.print("serial_in=");
				Serial.print(serial_in);
				Serial.print("n");
				*/
				strncat(inputBuffer, &serial_in, 1);
				//Echo the command string as it is composed
				// Serial.print(serial_in);
				// Serial.flush();
            break;
        }
    }
    
    while (Wire.available())   // slave may send less than requested
    {
        char c = Wire.read(); // receive a byte as character
        Serial.print(" ");
        Serial.print(c);         // print the character
        Serial.print(" 0x");
        Serial.print(c, HEX);
		Serial.print("\n");
    }
    
}
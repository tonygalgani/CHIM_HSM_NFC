#include <EEPROM.h>  // Include the EEPROM library to enable reading from and writing to the EEPROM. Useful for storing data between reboots on the ATmega32U4.

#include <Adafruit_PN532.h>  // Include the Adafruit PN532 library for interfacing with the NFC controller. This library provides functions for NFC tag reading and writing.

#define DEBUG  // Define the DEBUG preprocessor directive to enable debugging features/output in the code.

// Define constants related to the structure of Mifare Classic NFC tags.
#define NR_SHORTSECTOR (32)          // Number of short sectors in Mifare 1K or the first part of Mifare 4K.
#define NR_LONGSECTOR (8)            // Number of long sectors available only in Mifare 4K.
#define NR_BLOCK_OF_SHORTSECTOR (4)  // Blocks per short sector in Mifare tags.
#define NR_BLOCK_OF_LONGSECTOR (16)  // Blocks per long sector in Mifare 4K tags.

// Macro to calculate the block number of the sector trailer (last block of a sector) for a given sector number.
// Sector trailer blocks contain access control bits and keys for each sector.
#define BLOCK_NUMBER_OF_SECTOR_TRAILER(sector) (((sector) < NR_SHORTSECTOR) ? ((sector)*NR_BLOCK_OF_SHORTSECTOR + NR_BLOCK_OF_SHORTSECTOR - 1) : (NR_SHORTSECTOR * NR_BLOCK_OF_SHORTSECTOR + (sector - NR_SHORTSECTOR) * NR_BLOCK_OF_LONGSECTOR + NR_BLOCK_OF_LONGSECTOR - 1))

// Macro to calculate the first block number of a given sector, differentiating between short and long sectors.
#define BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(sector) (((sector) < NR_SHORTSECTOR) ? ((sector)*NR_BLOCK_OF_SHORTSECTOR) : (NR_SHORTSECTOR * NR_BLOCK_OF_SHORTSECTOR + (sector - NR_SHORTSECTOR) * NR_BLOCK_OF_LONGSECTOR))

// Define a limit for user input length to prevent buffer overflow in user-input handling routines.
#define MAX_INPUT 100

// Define the total number of sectors in a Mifare Classic 4K card for reference in the code.
#define sector_number 39

//
#define SEGMENT_SIZE 32
#define LOWEST_SEGMENT_ADDRESS 32
#define HIGHEST_SEGMENT_ADDRESS 992

// Define a blank data block template with 16 bytes set to zero, used for initializing or clearing data blocks.
#define BLANK_DATA_BLOCK \
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }

// Define an array of predefined key sets for MIFARE sector authentication.
// Stored in program memory (PROGMEM) to save RAM on the ATmega32U4.
// Each key set contains 6 bytes, and is used for securing sectors on a Mifare card.
const uint8_t keys[3][6] PROGMEM = {
  { 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5 },  // Key set 1: Sector 0's key A for NDEF configuration.
  { 0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7 },  // Key set 2: Data sectors key A for NDEF configuration.
  { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }   // Key set 3: Default factory key (all bytes are 0xFF).
};

// Define access condition bits for MIFARE sectors, detailing read/write permissions.
// These are also stored in PROGMEM. Each set configures permissions for one sector.
const uint8_t accessBits[3][4] PROGMEM = {
  { 0xFF, 0x07, 0x80, 0x69 },  // Access bits for default configuration.
  { 0x7F, 0x07, 0x88, 0x40 },  // Access bits for NDEF data sector configuration.
  { 0x78, 0x77, 0x88, 0xC1 }   // Access bits for configuration 3.
};

// Define the Memory Access Data (MAD) for the first sector of a Mifare card.
// MAD specifies the card's data layout. Each byte represents a specific type of data storage.
// This is useful for systems that require structured data organization on the NFC tags.
const uint8_t MAD1[2][16] PROGMEM = {
  { 0x14, 0x01, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1 },  // Row 1 of MAD configuration.
  { 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1 }   // Row 2 of MAD configuration.
};


// // Define an enumeration for vCard data fields. This enum helps in managing the sequence of vCard elements.
// typedef enum {
//   START,            // Marker for the start of a vCard entry.
//   NAME,             // Represents the name field in vCard.
//   EMAIL,            // Represents the email address field in vCard.
//   WORKPHONE,        // Represents the work phone number field in vCard.
//   HOMEPHONE,        // Represents the home phone number field in vCard.
//   ADDRESS,          // Represents the address field in vCard.
//   ORGANISATION,     // Represents the organisation field in vCard.
//   TITLE,            // Represents the title field in vCard.
//   URL,              // Represents the URL field in vCard.
//   PHOTO,            // Represents the photo field in vCard, using JPEG format and BASE64 encoding.
//   END,              // Marker for the end of a vCard entry.
//   vCardPrefixCount  // A utility value to know the total number of elements in the enum.
// } vCardPrefix;

// // Define constants for each vCard prefix, ensuring they are stored in PROGMEM to save RAM.
// const char Start[] PROGMEM = "BEGIN:VCARD\nVERSION:3.0\n";        // Starting template for a vCard entry, specifies vCard version 3.0.
// const char Name[] PROGMEM = "N:";                                 // Prefix for the name field in a vCard.
// const char Email[] PROGMEM = "EMAIL:";                            // Prefix for the email field in a vCard.
// const char WorkPhone[] PROGMEM = "TEL;WORK:";                     // Prefix for the work phone number field in a vCard.
// const char HomePhone[] PROGMEM = "TEL;CELL:";                     // Prefix for the cell phone number field in a vCard.
// const char Address[] PROGMEM = "ADR;WORK:";                       // Prefix for the work address field in a vCard.
// const char Organisation[] PROGMEM = "ORG:";                       // Prefix for the organisation field in a vCard.
// const char Title[] PROGMEM = "TITLE:";                            // Prefix for the title field in a vCard.
// const char Url[] PROGMEM = "URL:";                                // Prefix for the URL field in a vCard.
// const char Photo[] PROGMEM = "PHOTO;TYPE=JPEG;ENCODING=BASE64:";  // Prefix for the photo field in a vCard, specifies JPEG format and BASE64 encoding.
// const char End[] PROGMEM = "END:VCARD";                           // Ending template for a vCard entry.

// // Array of pointers to each vCard prefix string, stored in PROGMEM.
// // This array facilitates indexed access to each prefix based on the `vCardPrefix` enum, optimizing data handling.
// const char* const vCard_prefix[vCardPrefixCount] PROGMEM = {
//   Start,
//   Name,
//   Email,
//   WorkPhone,
//   HomePhone,
//   Address,
//   Organisation,
//   Title,
//   Url,
//   Photo,
//   End
// };

void terminate_current_serial(void);

/**
 * @brief Encrypts a block of data using AES-256 ECB mode.
 * 
 * This function takes an input block of plaintext and encrypts it using the AES-256 ECB mode of encryption.
 * It assumes that the input data length is a multiple of the AES block size (16 bytes). It uses provided key
 * to perform the encryption and places the result in the output buffer.
 *
 * @param output Pointer to the buffer where encrypted data should be stored.
 * @param input Pointer to the data to be encrypted.
 * @param len Length of the input data in bytes. Must be a multiple of CIPHER_BLOCK_SIZE.
 * @param key Pointer to the encryption key.
 * @return false if encryption was successful, true if the key was invalid.
 */
bool encrypt(uint8_t *output, const uint8_t *input, uint8_t len, const uint8_t *key);


/**
 * @brief Decrypts a block of data using AES-256 ECB mode.
 * 
 * This function takes an input block of ciphertext and decrypts it using the AES-256 ECB mode of decryption.
 * It assumes that the input data length is a multiple of the AES block size (16 bytes). It uses the provided key
 * to perform the decryption and places the result in the output buffer.
 *
 * @param output Pointer to the buffer where decrypted data should be stored.
 * @param input Pointer to the data to be decrypted.
 * @param len Length of the input data in bytes. Must be a multiple of CIPHER_BLOCK_SIZE.
 * @param key Pointer to the decryption key.
 * @return false if decryption was successful, true if the key was invalid.
 */
bool decrypt(uint8_t* output, const uint8_t* input, uint8_t len, const uint8_t* key);
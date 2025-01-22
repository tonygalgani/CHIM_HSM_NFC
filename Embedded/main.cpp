#include "main.h"

// Initialize an instance of the Adafruit PN532 class for NFC communication using SPI pins.
// Pins 15, 14, 16, and 10 correspond to SCK, MISO, MOSI, and SS respectively on the Arduino pro micro.
Adafruit_PN532 nfc(15, 14, 16, 10);


uint8_t uidLength = 0;  // Global variable to store the length of the UID (Unique Identifier) of the NFC card.
                        // The length can be either 4 or 7 bytes, depending on the card's compliance with ISO14443A standard.

uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Global array to hold the UID retrieved from an NFC card.
                                          // Initialized to zero and has a maximum length to accommodate both 4 and 7 byte UIDs.

uint8_t nb_data_blocks = 0;  // Global variable used for indexing and iterating over the data blocks of an NFC card.
                             // It helps in managing the read/write operations to the card's memory blocks.


bool nfc_begin(void) {
  return nfc.begin();
}

bool nfc_readPassiveTargetID() {
  return nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
}
#ifdef DEBUG
/**
 * Prints an array of bytes in hexadecimal format to the Serial interface, aiding in debugging.
 * The output is formatted in rows of 16 bytes to enhance readability.
 *
 * @param array Pointer to the array of bytes to be printed.
 * @param nbBytes The number of bytes in the array to be printed.
 */
void printDebugHex(uint8_t* array, size_t nbBytes) {
  // Print a header line to clearly separate debug output
  Serial.println(F("--------------------------------------DEBUG--------------------------------------"));

  // Loop through each byte of the array
  for (uint8_t i = 0; i < nbBytes; i++) {
    // Print an additional leading zero for bytes less than 0x10 to maintain consistent formatting
    if (array[i] < 0x10)
      Serial.print(F("0"));
    Serial.print(array[i], HEX);  // Print the byte in hexadecimal
    Serial.print(F(" "));         // Print a space for separation

    // Every 16 bytes, print a newline to format the output into blocks of 16 bytes each
    if ((i + 1) % 16 == 0) {
      Serial.println();
    }
  }

  // Print a newline to end the last line of data if it doesn't end exactly at a 16-byte boundary
  Serial.println();
  // Print a footer line to clearly end the debug section
  Serial.println(F("---------------------------------------------------------------------------------"));
}
#endif

/**
 * Initializes the NFC chip connection and checks for its presence by retrieving the firmware version.
 * If the NFC chip is found, the function prints the chip type and firmware version to the Serial monitor.
 * If the chip cannot be found, it prints an error message and halts the program.
 */
void nfc_chip_connect(void) {
  uint32_t versiondata = nfc.getFirmwareVersion();  // Attempt to retrieve the firmware version of the NFC chip.

  // Check if the firmware data was successfully retrieved.
  if (!versiondata) {
    Serial.print(F("Didn't find PN53x board"));  // Inform on the Serial monitor that no NFC board was detected.
    while (1)
      ;  // Halt execution indefinitely if no NFC chip is found.
  }

  // If the firmware data is valid, proceed to print details.
  Serial.print(F("Found chip PN5"));                // Print the chip model prefix.
  Serial.println((versiondata >> 24) & 0xFF, HEX);  // Extract and print the major chip version number in hexadecimal.

  Serial.print(F("Firmware ver. "));               // Notify that firmware version will be printed next.
  Serial.print((versiondata >> 16) & 0xFF, DEC);   // Extract and print the major version number.
  Serial.print('.');                               // Dot to separate major and minor firmware version numbers.
  Serial.println((versiondata >> 8) & 0xFF, DEC);  // Extract and print the minor version number.
}


#ifdef DEBUG
/**
 * Prints information about the detected NFC/RFID card to the Serial monitor.
 * This function identifies the type of card based on the UID length and displays the UID in hexadecimal format.
 */
void print_card_info(void) {

  // Determine the type of card based on the UID length.
  if (uidLength == 4)
    Serial.println(F("Card type: MIFARE Classic"));  // If the UID is 4 bytes long, it's likely a MIFARE Classic.
  else
    Serial.println(F("Card type: MIFARE Ultralight ou autre"));  // If not, assume MIFARE Ultralight or other types.

  // Display the length of the UID.
  Serial.print(F("UID Length: "));
  Serial.print(uidLength, DEC);  // Print the length of the UID in decimal format.
  Serial.println(F(" bytes"));

  // Display the UID itself.
  Serial.print(F("UID = "));
  for (uint8_t i = 0; i < uidLength; i++) {  // Loop through each byte of the UID.
    if (uid[i] < 0x10)                       // Add a leading zero for single digit hex values for consistency.
      Serial.print(F("0"));
    Serial.print(uid[i], HEX);  // Print each byte of the UID in hexadecimal.
    Serial.print(F(" "));
  }
  Serial.println();  // Move to a new line after finishing the UID output.
}
#endif


/**
 * Reads and prints the memory blocks of a MIFARE RFID card.
 * It uses a default key for authentication and attempts to read each sector's data blocks and the trailer block.
 * It displays the data in hexadecimal format and handles possible reading and authentication errors.
 */
void read_memory(void) {
  uint8_t default_key[6];  // Temporary storage for the MIFARE authentication key.

  // Retrieve the default key from program memory.
  for (uint8_t i = 0; i < 6; i++) {
    default_key[i] = pgm_read_byte_near(&(keys[2][i]));  // Load each byte of the key using PROGMEM access.
  }

  // If compiled with DEBUG defined, print the key for debugging purposes.
#ifdef DEBUG
  printDebugHex(default_key, 6);
#endif

  // Iterate over all sectors of the card.
  for (uint8_t sector_index = 0; sector_index <= sector_number; sector_index++) {
    uint8_t data_read[16];  // Buffer to store the data read from each block.

    // Authenticate using the default key before attempting to read blocks.
    if (nfc.mifareclassic_AuthenticateBlock(uid, uidLength, BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(sector_index), 1, default_key)) {
      // Determine the number of data blocks in the current sector (short or long sector).
      if (sector_index < 32)
        nb_data_blocks = 3;  // Short sectors have 3 data blocks.
      else
        nb_data_blocks = 15;  // Long sectors have 15 data blocks.

      // Read and print each data block in the current sector.
      for (uint8_t i = 0; i < nb_data_blocks; i++) {
        if (nfc.mifareclassic_ReadDataBlock(BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(sector_index) + i, data_read)) {
          Serial.print(F("Block: "));
          Serial.print(BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(sector_index) + i);  // Print block number.
          Serial.print(F("  "));
          nfc.PrintHexChar(data_read, 16);  // Print data in hex and readable format.
        } else {
          Serial.print(F("Unable to read block: "));
          Serial.print(BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(sector_index) + i);
          Serial.println();
          return;  // Exit if any block read fails.
        }
      }

      // Read and print the sector trailer block.
      if (nfc.mifareclassic_ReadDataBlock(BLOCK_NUMBER_OF_SECTOR_TRAILER(sector_index), data_read)) {
        Serial.println();
        Serial.print(F("Block: "));
        Serial.print(BLOCK_NUMBER_OF_SECTOR_TRAILER(sector_index));  // Print block number.
        Serial.print(F("  "));
        nfc.PrintHexChar(data_read, 16);  // Print data in hex and readable format.
        Serial.println();
      } else {
        Serial.print("Unable to read block ");
        Serial.println(BLOCK_NUMBER_OF_SECTOR_TRAILER(sector_index));
      }
    } else {
      Serial.print(F("Sector "));
      Serial.print(sector_index);
      Serial.println(F(" authentication failed, this could be a mifare 1k, try again."));
      return;  // Exit if authentication fails.
    }
  }

  // Clear the serial buffer and introduce a small delay to stabilize any subsequent operations.
  Serial.flush();
  while (Serial.available()) Serial.read();
  delay(1000);
}


/**
 * Gathers text input from the user via the Serial interface and stores it in a buffer.
 * The function waits for user input, reads it from the serial buffer, and ensures that
 * the input does not exceed the maximum allowed length defined by MAX_INPUT. It also
 * handles the input termination and confirms the received text by echoing it back to the user.
 *
 * @param input A character array where the received text will be stored.
 */
void get_ndef_text(char* input) {
  uint8_t i = 0;  // Initialize index to keep track of the input length.

  memset(input, 0, MAX_INPUT);              // Initialize the input buffer with zeros to clean previous data.
  Serial.println(F("Enter your text..."));  // Prompt the user to enter text.

  while (!Serial.available())  // Wait for the user to start typing.
    ;                          // Loop does nothing, just waits.

  // Read the input from the Serial buffer as long as data is available.
  while (Serial.available()) {
    char c = Serial.read();  // Read a single character from the Serial buffer.

    // Check if the character is not a newline, which signifies the end of input,
    // and ensure we do not exceed the buffer limit.
    if (c != '\n' && i < MAX_INPUT - 1) {  // MAX_INPUT - 1 to leave space for null terminator.
      input[i] = c;                        // Store the character in the buffer.
      i++;                                 // Increment the index.
    } else {
      input[i] = '\0';  // Null-terminate the string.
      i = 0;            // Reset the index for possible future use.

      // Output the received input back to the Serial to confirm correct reception.
      Serial.print(F("You typed: "));
      Serial.println(input);
      break;  // Exit the loop after processing the complete line of input.
    }
  }
}

/**
 * Writes an NDEF message to an NFC card, formatted for text records.
 * This function prompts the user to input text, constructs an NDEF record, and writes it to the card.
 * It handles the entire process from user input, through NDEF record formatting, to writing the data blocks.
 */
// void write_ndef(void) {
//   Serial.println(F("Updating card's ndef..."));  // Inform the user that the NDEF update is starting.

//   // Allocate and initialize the default MIFARE authentication key from stored keys.
//   uint8_t default_key[6];
//   for (uint8_t i = 0; i < 6; i++) {
//     default_key[i] = pgm_read_byte_near(&(keys[2][i]));  // Retrieve key from PROGMEM.
//   }

// #ifdef DEBUG
//   printDebugHex(default_key, 6);  // Debug print the key if DEBUG is defined.
// #endif

//   char input[MAX_INPUT];  // Buffer to store user input, currently limited to MAX_INPUT characters.

//   get_ndef_text(input);  // Function to get the text that will be written to the card as NDEF data.

//   uint8_t size = strlen(input);  // Calculate the length of the user input.

// #ifdef DEBUG
//   Serial.print(F("input size :"));  // Debug print the input size.
//   Serial.println(size);
// #endif

//   // Setup the NDEF record header and payload based on the user's input.
//   unsigned char ndef_record[14 + size + 1]{
//     0x03,                // NDEF message start marker.
//     0xFF,                // Indicates the use of a 3-byte length field.
//     (size + 10) >> 8,    // MSB of the length of the NDEF message.
//     (size + 10) & 0xFF,  // LSB of the length of the NDEF message.
//     0xC1,                // NDEF record header: Message Begin and End flags set, TNF=0x2 indicating MIME media.
//     0x01,                // TYPE LENGTH: Length of the 'T' type field (Text).
//     size + 3 >> 24,      // MSB of payload length.
//     size + 3 >> 16,      // Payload length.
//     size + 3 >> 8,       // Payload length.
//     size + 3 & 0xFF,     // LSB of payload length.
//     'T',                 // Type field: 'T' for Text.
//     0x02, 'e', 'n'       // Payload: UTF-8, language code 'en'.
//   };

//   memcpy(ndef_record + 14, input, size);  // Copy user input into the record.
//   ndef_record[14 + size] = 0xFE;          // NDEF record end marker.

//   size = size + 14 + 1;  // Total size of the NDEF record including headers and end marker.

//   uint8_t nb_sectors = size / 48;  // Calculate the number of sectors needed for the data.
//   if (size % 48)
//     nb_sectors++;

// #ifdef DEBUG
//   Serial.print(F("nb_sectors:"));  // Debug print the number of sectors required.
//   Serial.println(nb_sectors);
// #endif

//   // Write the NDEF message to the NFC card by iterating over the necessary sectors and blocks.
//   for (uint8_t current_sector = 1; current_sector <= nb_sectors; current_sector++) {
//     if (!nfc.mifareclassic_AuthenticateBlock(uid, uidLength, BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(current_sector), 1, default_key)) {
//       Serial.println(F("Authentication failed... is this card NDEF formatted? NDEF Record creation failed!"));
//       return;  // Exit the function if authentication fails.
//     }

//     for (uint8_t current_block = 0; current_block < 3; current_block++) {
//       uint8_t temp[16] = { 0 };  // Temporary buffer for the data to write.

//       // Prepare the data block by copying the relevant section of the NDEF record.
//       memcpy(temp, ndef_record + ((current_sector - 1) * 48) + (current_block * 16), 16);

// #ifdef DEBUG
//       printDebugHex(temp, 16);  // Debug print the data block to be written.
// #endif

//       // Attempt to write the block to the card.
//       if (!nfc.mifareclassic_WriteDataBlock(BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(current_sector) + current_block, temp)) {
//         Serial.print(F("Writing block "));
//         Serial.print(BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(current_sector) + current_block);
//         Serial.println(F(" failed, try again."));
//         return;  // Exit the function if block writing fails.
//       }
//     }
//   }

//   Serial.println(F("NDEF text written!"));   // Confirm successful writing.
//   Serial.flush();                            // Flush the serial buffer to ensure all output has been sent.
//   while (Serial.available()) Serial.read();  // Clear any remaining input from serial buffer.
//   delay(1000);                               // Short delay to ensure stability after operations.
// }


/**
 * Constructs and writes a vCard to an NFC card.
 * It gathers user input for each field of the vCard, builds the vCard data in memory, and writes it block by block to the NFC card.
 * The function handles NFC authentication, data preparation, and writing, ensuring that the vCard data is formatted correctly and stored persistently.
 */
// void write_vCard(void) {
//   // Notify start of vCard creation.
//   Serial.println(F("Updating card's vCard..."));

//   // Prepare the variables for assembling the vCard.
//   size_t decalage = 20;              // Offset for where vCard data starts in the array, after the NDEF message header.
//   uint8_t prefix_length = 0;         // Length of the current vCard field prefix.
//   unsigned char vCard[618] = { 0 };  // Large enough buffer to hold the entire vCard data.

//   // Loop through all vCard field types.
//   for (uint8_t type_info = 0; type_info < vCardPrefixCount; type_info++) {
//     // Debug print current field type index.
// #ifdef DEBUG
//     Serial.print(F("type_info: "));
//     Serial.println(type_info);
// #endif

//     // Calculate the prefix length for the current field.
//     prefix_length = strlen_P((const char*)pgm_read_word(&(vCard_prefix[type_info])));

//     // Debug print the length of the current prefix.
// #ifdef DEBUG
//     Serial.print(F("prefix_length: "));
//     Serial.println(prefix_length);
// #endif

//     // Copy the prefix from program memory to the vCard buffer at the current position.
//     strcpy_P(vCard + decalage, (char*)pgm_read_word(&(vCard_prefix[type_info])));
//     decalage += prefix_length;  // Update the offset for the next data.

//     // Handle user input for fields that are not the photo or the end field.
//     if (type_info > 0 && type_info < (vCardPrefixCount - 2)) {
//       uint8_t eeprom_cell = 0;  // Index for EEPROM data storage.
//       Serial.print(F("Enter the following information: "));
//       Serial.println(type_info);
//       // Wait for user input to become available.
//       while (!Serial.available())
//         ;
//       // Read user input from Serial.
//       while (Serial.available()) {
//         char c = Serial.read();
//         // Store user input in EEPROM until a newline character is encountered or the maximum size for field input has been reached(to be implemented)
//         if (c != '\n' /*&& i < MAX_INPUT_FIELD - 1*/) {
//           EEPROM.update(eeprom_cell, c);
//           eeprom_cell++;
//         } else {
//           EEPROM.update(eeprom_cell, '\n');
//           // Optionally, print the typed information in debug mode.
// #ifdef DEBUG
//           Serial.print(F("You typed: "));
//           for (uint8_t i = 0; i <= eeprom_cell; i++) {
//             if (EEPROM[i] < 0x10)
//               Serial.print(F("0"));
//             Serial.print(EEPROM[i], HEX);
//             Serial.print(F(" "));
//           }
//           Serial.println();
// #endif
//           // Copy user input from EEPROM to the vCard buffer.
//           for (uint8_t i = 0; i <= eeprom_cell; i++) {
//             vCard[decalage] = EEPROM[i];
//             decalage++;
//           }
//         }
//       }
//     }
//     // Add a newline to the vCard buffer after the photo field.
//     if (type_info == (vCardPrefixCount - 2)) {
//       vCard[decalage] = '\n';
//       decalage++;
//     }
//   }

//   // Append vCard format data at the start.
//   unsigned char vCardFormat[20] = {
//     // NDEF message header and vCard MIME type.
//     0x03, 0xFF, (decalage + 16) >> 8, (decalage + 16) & 0xFF, 0xC2, 0x0A,
//     decalage >> 24, decalage >> 16, decalage >> 8, decalage & 0xFF,
//     't', 'e', 'x', 't', '/', 'v', 'c', 'a', 'r', 'd'
//   };
//   memcpy(vCard, vCardFormat, 20);  // Place the header at the beginning of the vCard buffer.

//   // Calculate the number of sectors needed to store the vCard.
//   uint8_t nb_sectors = decalage / 48;
//   if (decalage % 48) nb_sectors++;

//   // Prepare the default key for NFC authentication.
//   uint8_t default_key[6];
//   for (uint8_t i = 0; i < 6; i++) {
//     default_key[i] = pgm_read_byte_near(&(keys[2][i]));
//   }

//   // Loop through each sector that needs to be written to store the full vCard.
//   for (uint8_t current_sector = 1; current_sector <= nb_sectors; current_sector++) {
//     // Attempt to authenticate the current sector with the default key.
//     if (!nfc.mifareclassic_AuthenticateBlock(uid, uidLength, BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(current_sector), 1, default_key)) {
//       Serial.print(F("Sector: "));
//       Serial.print(BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(current_sector));  // Print which sector failed to authenticate.
//       Serial.println(F(" authentication failed!"));
//       return;  // Exit the function if authentication fails, preventing further write attempts.
//     }

//     // Loop through the first three blocks of the current sector (typical for MIFARE Classic 1K, where each sector contains three data blocks and one trailer block).
//     for (uint8_t current_block = 0; current_block < 3; current_block++) {
//       uint8_t temp[16];     // Temporary buffer to hold the data to be written.
//       memset(temp, 0, 16);  // Initialize the buffer with zeros.

//       // Copy data from the vCard array into the temporary buffer, adjusting for sector and block offsets.
//       memcpy(temp, vCard + ((current_sector - 1) * 48) + (current_block * 16), 16);

//       // Write the prepared data block to the NFC card.
//       if (!nfc.mifareclassic_WriteDataBlock(BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(current_sector) + current_block, temp)) {
//         Serial.println(F("Write failed!"));  // Notify on serial if writing the block fails.
//         return;                              // Exit the function if the write operation fails, preventing partial writes and data corruption.
//       }
//     }
//   }

//   // Print completion message once all intended data blocks have been successfully written.
//   Serial.println(F("vCard creation done."));
//   // Flush any remaining output to the serial.
//   Serial.flush();
//   // Clear the serial buffer to ensure there are no remaining input characters.
//   while (Serial.available()) Serial.read();
//   // Delay to ensure all serial communications are complete and to stabilize the system after the write operations.
//   delay(1000);
// }



/**
 * Formats an NFC card's initial sector (sector 0) with MAD1 configuration and sets up the sector trailer blocks
 * across the card to a predefined ndef configuration for NFC data storage.
 * The function handles key loading, data preparation, authentication, and block writing.
 */
void format_MAD1(void) {
  // Load the default MIFARE authentication key into memory.
  uint8_t default_key[6];
  for (uint8_t i = 0; i < 6; i++) {
    default_key[i] = pgm_read_byte_near(&(keys[2][i]));
  }

#ifdef DEBUG
  printDebugHex(default_key, 6);  // Debug print the default key.
#endif

  // Prepare the data for sector 0, based on MAD1 specifications.
  uint8_t sector0[48];
  for (uint8_t i = 0; i < 48; i++) {
    if (i < 16)  // First 16 bytes are the MAD1 data for the first block.
      sector0[i] = pgm_read_byte_near(&(MAD1[0][i]));
    if (i >= 16 && i < 32)  // Repeat MAD1 data for the second block.
      sector0[i] = pgm_read_byte_near(&(MAD1[0][i - 16]));
    if (i >= 32 && i < 38)  // Key A for the sector trailer.
      sector0[i] = pgm_read_byte_near(&(keys[0][i - 32]));
    if (i >= 38 && i < 42)  // Access bits for the sector trailer.
      sector0[i] = pgm_read_byte_near(&(accessBits[2][i - 38]));
    if (i >= 42 && i < 48)  // Key B for the sector trailer.
      sector0[i] = pgm_read_byte_near(&(keys[2][i - 42]));
  }

#ifdef DEBUG
  printDebugHex(sector0, 48);  // Debug print the prepared sector 0 data.
#endif

  // Prepare the sector trailer block data with specific access bits and keys.
  uint8_t ndef_trailer_block[16];
  for (uint8_t i = 0; i < 16; i++) {
    if (i < 6)  // Key A for the trailer block.
      ndef_trailer_block[i] = pgm_read_byte_near(&(keys[1][i]));
    if (i >= 6 && i < 10)  // Access bits for the trailer block.
      ndef_trailer_block[i] = pgm_read_byte_near(&(accessBits[0][i - 6]));
    if (i >= 10 && i < 16)  // Key B for the trailer block.
      ndef_trailer_block[i] = pgm_read_byte_near(&(keys[2][i - 10]));
  }

#ifdef DEBUG
  printDebugHex(ndef_trailer_block, 16);  // Debug print the trailer block data.
#endif

  // Authenticate with the default key to format sector 0.
  if (!nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 0, 0, default_key)) {
    Serial.println(F("Unable to authenticate block 0 to enable card formatting! Maybe your card is already ndef formatted. If not, format it to default before trying again."));
    return;
  }

  // Write the prepared data to sector 0's blocks.
  if (!nfc.mifareclassic_WriteDataBlock(1, sector0)) {
    Serial.println(F("Unable to format block 1 into MAD1"));
    return;
  }
  if (!nfc.mifareclassic_WriteDataBlock(2, sector0 + 16)) {
    Serial.println(F("Unable to format block 2 into MAD1"));
    return;
  }
  if (!nfc.mifareclassic_WriteDataBlock(3, sector0 + 32)) {
    Serial.println(F("Unable to format block 3 into MAD1"));
    return;
  }
  Serial.println(F("MAD1 correctly formatted."));

  // Format all other sector trailers with the predefined ndef configuration.
  for (uint8_t sector_index = 1; sector_index <= sector_number; sector_index++) {
    if (nfc.mifareclassic_AuthenticateBlock(uid, uidLength, BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(sector_index), 0, default_key)) {
      if (!nfc.mifareclassic_WriteDataBlock(BLOCK_NUMBER_OF_SECTOR_TRAILER(sector_index), ndef_trailer_block)) {
        Serial.print(F("Unable to write trailer block "));
        Serial.print(BLOCK_NUMBER_OF_SECTOR_TRAILER(sector_index));
        Serial.println(F(", Try again."));
        return;
      }
    } else {
      Serial.print(F("Sector "));
      Serial.print(sector_index);
      Serial.println(F(" authentication failed! Verify your access Key. Or this could be a MIFARE 1K."));
      return;
    }
  }
  Serial.println(F("Keys correctly formatted into ndef values."));
  Serial.flush();                            // Ensure all serial data has been transmitted.
  while (Serial.available()) Serial.read();  // Clear any lingering data in the serial buffer.
  delay(1000);                               // Pause to stabilize system after formatting.
}



/**
 * Resets all the sectors of an NFC card to default settings. This includes writing zero values to all
 * data blocks and setting sector trailers to default access conditions using a predefined key.
 * The function iterates over all sectors, authenticates each one, and performs the write operations.
 */
void format_to_default(void) {
  // Load the default MIFARE authentication key into memory.
  uint8_t default_key[6];
  for (uint8_t i = 0; i < 6; i++) {
    default_key[i] = pgm_read_byte_near(&(keys[2][i]));
  }

#ifdef DEBUG
  printDebugHex(default_key, 6);  // Debug print the default key if DEBUG is defined.
#endif

  // Prepare the default sector trailer block with default keys and access bits.
  uint8_t default_trailer_block[16];
  for (uint8_t i = 0; i < 16; i++) {
    if (i < 6)  // First 6 bytes are the Key A.
      default_trailer_block[i] = pgm_read_byte_near(&(keys[2][i]));
    if (i >= 6 && i < 10)  // Next 4 bytes are the Access Bits.
      default_trailer_block[i] = pgm_read_byte_near(&(accessBits[0][i - 6]));
    if (i >= 10)  // Last 6 bytes are the Key B.
      default_trailer_block[i] = pgm_read_byte_near(&(keys[2][i - 10]));
  }

#ifdef DEBUG
  printDebugHex(default_trailer_block, 16);  // Debug print the prepared trailer block.
#endif

  // Define an array to be used to overwrite existing data with zeros.
  uint8_t blank_data_block[16] = BLANK_DATA_BLOCK;

  // Iterate over all sectors on the card to reset their content.
  for (uint8_t sector_index = 0; sector_index <= sector_number; sector_index++) {
    // Authenticate each sector before attempting to write.
    if (nfc.mifareclassic_AuthenticateBlock(uid, uidLength, BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(sector_index), 1, default_key)) {
      // Determine the number of data blocks to clear based on the sector index.
      nb_data_blocks = (sector_index < 32) ? 3 : 15;  // Short sectors have 3 data blocks, long sectors have 15.

      // Write zeros to all data blocks in the current sector, skipping block 0 (sector 0's first block).
      for (uint8_t i = 0; i < nb_data_blocks; i++) {
        if (BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(sector_index) + i != 0) {  // Skip sector 0 block 0 (reserved for manufacturer).
          if (!nfc.mifareclassic_WriteDataBlock(BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(sector_index) + i, blank_data_block)) {
            Serial.print(F("Unable to write data block "));
            Serial.print(BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(sector_index) + i);
            Serial.println(F(", Try again."));
            return;  // Exit if a write operation fails.
          }
        }
      }

      // Update the sector trailer block with default configuration.
      if (!nfc.mifareclassic_WriteDataBlock(BLOCK_NUMBER_OF_SECTOR_TRAILER(sector_index), default_trailer_block)) {
        Serial.print(F("Unable to write trailer block "));
        Serial.print(BLOCK_NUMBER_OF_SECTOR_TRAILER(sector_index));
        Serial.println(F(", Try again."));
        return;  // Exit if writing the trailer block fails.
      }
    } else {
      Serial.print(F("Sector "));
      Serial.print(sector_index);
      Serial.println(F(" authentication failed! Verify your access Key. Or this could be a MIFARE 1K."));
      return;  // Exit if authentication fails.
    }
  }

  // Notify completion of formatting operation.
  Serial.println(F("Data blocks correctly formatted to default values."));
  terminate_current_serial();  // Ends serial communication for this function.
}



void recover_segments(void) {

  // Load the default MIFARE authentication key into memory for NFC authentication.
  uint8_t default_key[6];
  for (uint8_t i = 0; i < 6; i++) {
    default_key[i] = pgm_read_byte_near(&(keys[2][i]));
  }

#ifdef DEBUG
  printDebugHex(default_key, 6);  // Debug print the default key if DEBUG is defined.
#endif

  char key_segment1[32] = { 0 };   // Buffer to store the first key segment retrieved from NFC.
  char key_segment2[32] = { 0 };   // Buffer to store the second key segment retrieved from NFC.
  uint8_t read_block[48] = { 0 };  // Buffer to hold data read from NFC.

  if (nfc.mifareclassic_AuthenticateBlock(uid, uidLength, BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(1), 1, default_key)) {
    // Read and concatenate data from the first three blocks of the sector into the read_block buffer.
    for (uint8_t i = 0; i < 3; i++) {
      if (!nfc.mifareclassic_ReadDataBlock(BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(1) + i, read_block + (i * 16))) {
#ifdef DEBUG
        Serial.print(F("Unable to read block: "));
        Serial.print(BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(1) + i);
        Serial.println();
#endif
        return;  // Exit if any block read fails.
      }
    }
    bool dualCard = read_block[46] & 0b01000000;
    if (dualCard) {  // Check the 47th byte of the first sector to know if it is a dual card.
      Serial.println(F("DualCards=true"));
      bool i = (read_block[46] & 0b00100000);
      if (i) {
        memcpy(key_segment2, read_block + 14, 32);
        while (!Serial.available()) {};            // Wait for any user input.
        while (Serial.available()) Serial.read();  // Clear the Serial buffer to ensure no residual inputs affect the process.
        if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
          if (nfc.mifareclassic_AuthenticateBlock(uid, uidLength, BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(1), 1, default_key)) {

            // Read and concatenate data from the first three blocks of the sector into the read_block buffer.
            for (uint8_t i = 0; i < 3; i++) {
              if (!nfc.mifareclassic_ReadDataBlock(BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(1) + i, read_block + (i * 16))) {
#ifdef DEBUG
                Serial.print(F("Unable to read block: "));
                Serial.print(BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(1) + i);
                Serial.println();
#endif
                return;  // Exit if any block read fails.
              }
            }
            memcpy(key_segment1, read_block + 14, 32);
          } else
            Serial.println(F("Failed to authenticate second card"));

        } else
          Serial.println(F("Failed to read second card"));

      } else {
#ifdef DEBUG
        printDebugHex(read_block, 48);
#endif
        memcpy(key_segment1, read_block + 14, 32);
        Serial.println(F("Read second card"));
        while (!Serial.available()) {};            // Wait for any user input.
        while (Serial.available()) Serial.read();  // Clear the Serial buffer to ensure no residual inputs affect the process.
        if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
          if (nfc.mifareclassic_AuthenticateBlock(uid, uidLength, BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(1), 1, default_key)) {
            read_block[48] = { 0 };  // Buffer to hold data read from NFC.

            // Read and concatenate data from the first three blocks of the sector into the read_block buffer.
            for (uint8_t i = 0; i < 3; i++) {
              if (!nfc.mifareclassic_ReadDataBlock(BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(1) + i, read_block + (i * 16))) {
#ifdef DEBUG
                Serial.print(F("Unable to read block: "));
                Serial.print(BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(1) + i);
                Serial.println();
#endif
                return;  // Exit if any block read fails.
              }
            }
#ifdef DEBUG
            printDebugHex(read_block, 48);
#endif
            memcpy(key_segment2, read_block + 14, 32);
          } else
            Serial.println(F("Failed to authenticate second card"));

        } else
          Serial.println(F("Failed to read second card"));
      }


    } else {  // This is not a dual card
      Serial.println(F("DualCards=false"));
      while (!Serial.available()) {};            // Wait for any user input.
      while (Serial.available()) Serial.read();  // Clear the Serial buffer to ensure no residual inputs affect the process.
      // Copy the key segment from the read_block buffer with an offset to not reader the ndef wrapper and header.
      memcpy(key_segment1, read_block + 14, 32);
      uint8_t i = (read_block[46] & 0b00111111);
      size_t idx = 32 * i;
      for (uint8_t j = 0; j < 32; j++)
        key_segment2[j] = EEPROM.read(idx++);  // Read each byte of the key segment directly from EEPROM.
    }
    decrypt(key_segment1, key_segment1, 32, "tony\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0");
    decrypt(key_segment2, key_segment2, 32, "tony\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0");
    // Transmit both key segments via serial.
    Serial.print(F("Key segments: "));
    while (Serial.availableForWrite() < 32)
      ;
    Serial.write(key_segment1, 32);
    Serial.write(key_segment2, 32);
    Serial.println();

  } else {
    Serial.println(F("Sector 1 authentication failed! Unable to recover ndef key. Try again."));
    return;  // Exit if authentication fails.
  }

  terminate_current_serial();  // Ends serial communication for this function.
}




bool write_keys(void) {

  // Load the default MIFARE authentication key into memory for NFC authentication.
  uint8_t default_key[6];
  for (uint8_t i = 0; i < 6; i++) {
    default_key[i] = pgm_read_byte_near(&(keys[2][i]));
  }

#ifdef DEBUG
  printDebugHex(default_key, 6);  // Debug print the default key if DEBUG is defined.
#endif

  // Setup the NDEF record header and payload based on the user's input.
  unsigned char ndef_record[48]{
    0x03,           // NDEF message start marker.
    0xFF,           // Indicates the use of a 3-byte length field.
    0x00,           // MSB of the length of the NDEF message.
    0x2B,           // LSB of the length of the NDEF message.
    0xC1,           // NDEF record header: Message Begin and End flags set, TNF=0x2 indicating MIME media.
    0x01,           // TYPE LENGTH: Length of the 'T' type field (Text).
    0x00,           // MSB of payload length.
    0x00,           // Payload length.
    0x00,           // Payload length.
    0x24,           // LSB of payload length.
    'T',            // Type field: 'T' for Text.
    0x02, 'e', 'n'  // Payload: UTF-8, language code 'en'.
  };
  ndef_record[47] = 0xFE;  // NDEF message tlv wrapper terminator.

  // Array to hold dualcard flag + keys
  char key_segments[65] = { 0 };

  while (!Serial.available()) {};  // Wait for any user input.

  uint8_t SerialIndex = 0;
  while (SerialIndex < 65) {
    if (Serial.available()) {
      char c = Serial.read();
      key_segments[SerialIndex++] = c;
    } else {
      delay(10);  // Delay to allow buffer to fill.
    }
  }

  encrypt(key_segments + 1, key_segments + 1, 64, "tony\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0");


#ifdef DEBUG
  Serial.print(F("dualCards: "));
  Serial.println(key_segments[0]);

  Serial.print(F("Key segments:"));
  for (uint8_t i = 1; i < 65; i++) {
    Serial.print(key_segments[i]);
  }
  Serial.println();
#endif

  uint8_t indexSegmentVide = 0;

  if (key_segments[0] == '1')  // Dual cards
    ndef_record[46] = 0x40;
  else {
    // Go throught every Key Address from EEPROM
    for (uint16_t EEPROMidx = LOWEST_SEGMENT_ADDRESS; EEPROMidx < HIGHEST_SEGMENT_ADDRESS; EEPROMidx = EEPROMidx + SEGMENT_SIZE) {
      bool NonNullKey = false;
#ifdef DEBUG
      Serial.print(F("EEPROMidx: "));
      Serial.println(EEPROMidx);
#endif
      uint8_t tmp[32];
      for (uint8_t j = 0; j < SEGMENT_SIZE; j++) {
        tmp[j] = EEPROM.read(EEPROMidx + j);
      }

      if (memcmp(key_segments + 33, tmp, 32) == 0) {
        indexSegmentVide = EEPROMidx;
        break;
      }
      bool isEmpty = true;
      for (uint8_t j = 0; j < SEGMENT_SIZE; j++) {
        if (tmp[j] != 0x00) {
          isEmpty = false;
          break;
        }
      }
      if (isEmpty) {
        indexSegmentVide = EEPROMidx;
        break;
      }
      if (EEPROMidx == HIGHEST_SEGMENT_ADDRESS - 1) {
        Serial.println(F("Key storage full."));
        return 1;
      }
    }
#ifdef DEBUG
    Serial.print(F("idxSegmVide:"));
    Serial.println(indexSegmentVide / 32);
#endif

    ndef_record[46] = indexSegmentVide / 32;  // Divided by 32 so it gives us an index from 1 to 31
  }

  memcpy(ndef_record + 14, key_segments + 1, 32);  // Copy the first key into the record

  /*
Read the card first to check if at the idex present there is a  (the same) key  and erase it if there is*/

  // Authenticate sector 1 of the first card
  if (nfc.mifareclassic_AuthenticateBlock(uid, uidLength, BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(1), 1, default_key)) {
    // write the ndef record containing the ey segment into the sector1 of the first card

    for (uint8_t i = 0; i < 3; i++) {
      if (!nfc.mifareclassic_WriteDataBlock(BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(1) + i, ndef_record + (i * 16))) {
#ifdef DEBUG
        Serial.print(F("Unable to write block: "));
        Serial.print(BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(1) + i);
        Serial.println();
#endif
        return;  // Exit if any block read fails.
      }
    }

  }
#ifdef DEBUG
  else {
    Serial.println(F("Failed to authenticate first sector card"));
  }
#endif

  Serial.println(F("First card written."));
  /// Writing second key

  if (key_segments[0] == '1') {                       // Dual cards
    memcpy(ndef_record + 14, key_segments + 33, 32);  // Copy the second key into the record
    ndef_record[46] = 0x60;
#ifdef DEBUG
    Serial.println(F("Ndef record:"));
    printDebugHex(ndef_record, 48);
#endif

    while (!Serial.available()) {};            // Wait for any user input.
    while (Serial.available()) Serial.read();  // Clear the Serial buffer to ensure no residual inputs affect the process.

    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {

      if (nfc.mifareclassic_AuthenticateBlock(uid, uidLength, BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(1), 1, default_key)) {
        // write the ndef record containing the ey segment into the sector1 of the second card
        for (uint8_t i = 0; i < 3; i++) {
          if (!nfc.mifareclassic_WriteDataBlock(BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(1) + i, ndef_record + (i * 16))) {
#ifdef DEBUG
            Serial.print(F("Unable to Write block: "));
            Serial.print(BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(1) + i);
            Serial.println();
#endif
            return;  // Exit if any block read fails.
          }
        }
      }

#ifdef DEBUG
      else {
        Serial.println(F("Failed to authenticate first sector of second card"));
      }
#endif
    }
#ifdef DEBUG
    else {
      Serial.println(F("Failed to read second card"));
    }
#endif
    Serial.println(F("Second card written."));

  } else {
    Serial.print(F("index:"));
    Serial.println(indexSegmentVide);
    for (uint8_t i = 0; i < SEGMENT_SIZE; i++) {
      EEPROM.update(indexSegmentVide + i, key_segments[33 + i]);
    }

    char write_verification[32];

    EEPROM.get(indexSegmentVide, write_verification);

    for (uint8_t i = 0; i < SEGMENT_SIZE; i++) {
      if (write_verification[i] != key_segments[33 + i]) {
        Serial.println(F("Failed writing 2nd key, try again."));
        Serial.println(F("Kthxbye."));
        for (uint8_t i = 0; i < SEGMENT_SIZE; i++) {
          EEPROM.update(indexSegmentVide + i, 0);
        }
        return true;
      }
    }
    Serial.println(F("EEPROM written."));
  }
  return false;
}

/**
 * Checks if the device is password protected by examining a specific section of the EEPROM.
 * This function assumes that the admin password, if set, is stored within the first 32 blocks of the EEPROM.
 * It prints the password protection status to the serial monitor.
 */
bool is_password_protected(void) {
  // Loop through the first 32 blocks from EEPROM where the admin password is potentially stored.
  for (uint8_t i = 0; i < 32; i++) {
    // If any block contains a value other than 0, a password is set.
    if (EEPROM[i] != 0) {
      Serial.println(F("passwordProtected=true"));
      terminate_current_serial();  // Ends serial communication for this function.
      return true;                 // Device is password protected
    }
    // If we reach the last block and it is still 0, then no password is set.
    if (i == 31 && EEPROM[i] == 0) {
      Serial.println(F("passwordProtected=false"));
      terminate_current_serial();  // Ends serial communication for this function.
      return false;                // Device is notpassword protected
    }
  }
}


/**
 * Creates an admin password by reading characters from Serial and storing them in EEPROM.
 * The password can be up to 32 characters long, it would be padded with zeros if less.
 * The line sent by the password should be terminated by '\n' and followed by a byte 
 * corresponding to the actual length of the password. If the Serial buffer runs out of 
 * characters prematurely, it resets any partial password to ensure security.
 */
bool create_admin_password(void) {
  char password[32];              // Buffer to store the password
  bool passwordCreation = false;  // Flag for succesful operation
  uint8_t i = 0;

  // Wait for any user input.
  while (!Serial.available()) {
    delay(20);
  }

  // Read up to 32 characters or until newline is found
  while (i < 32 && Serial.available()) {
    char c = Serial.read();  // Read a character
    password[i++] = c;       // Store character in the buffer
  }

  // Password received MUST be 32 characters. If the user sets one less than 32 characters long,
  // then it should have been padded with zeros before being sent by the app.
  if (i == 32) {
    Serial.println(F("passwordCreation=true"));  // Inform the app of successful operation
    for (uint8_t j = 0; j < 32; j++)
      EEPROM.update(j, password[j]);  // Store each character in EEPROM

    terminate_current_serial();                   // Ends serial communication for this function.
    return true;                                  // Returns positive password creation
  } else {                                        // If less or more than 32 characters were read
    Serial.println(F("passwordCreation=false"));  // Inform the app of failed operation

    for (uint8_t j = 0; j < 32; j++)
      EEPROM.update(j, 0);  // Reset the entire password area to be sure no data were written.

    terminate_current_serial();  // Ends serial communication for this function.
    return false;                // Returns negative password creation value
  }
}


/**
 * Authenticates the user by comparing the input from the Serial monitor to the password stored in EEPROM.
 * The function reads each character from Serial and checks it against the corresponding EEPROM value.
 */
bool authentication(void) {
  bool isCorrect = true;  // Flag to keep track of password correctness.

  // Loop through each character of the stored password.
  for (uint8_t i = 0; i < 32; i++) {
    while (!Serial.available()) {
      delay(20);  // Wait for characters to be available in the Serial buffer.
    }
    if (EEPROM[i] != Serial.read()) {  // Read each character from Serial and compare it to EEPROM.
      isCorrect = false;               // Set flag to false if any character does not match.
      break;                           // Exit the loop as there's no need to check further if a mismatch is found.
    }
  }

  // Check if the password was correct.
  if (isCorrect) {
    // Inform the app of the successful authentication
    Serial.println(F("passwordCorrect=true"));
    terminate_current_serial();  // Ends serial communication for this function.
    return true;                 // Return authentication value
  } else {
    // Inform the app of the failed authentication
    Serial.println(F("passwordCorrect=false"));
    terminate_current_serial();  // Ends serial communication for this function.
    return false;                // Return authentication value
  }
}

/**
 * @brief Terminates the current serial communication cleanly.
 *
 * This function ensures that all outgoing serial data is transmitted completely before
 * it clears any remaining data from the serial buffer. It also includes a delay to stabilize
 * the transition between communication sessions, which is necessary for some operations.
 */
void terminate_current_serial(void) {
  Serial.flush();  // Waits for the transmission of outgoing serial data to complete.

  // Continuously read from serial buffer until it's empty.
  while (Serial.available())
    Serial.read();  // Reads and discards any remaining characters in the serial buffer.

  delay(200);  // Introduces a 200 millisecond delay to ensure any transitions are stabilized.
}

/////////////////////////////DevFunctions//////////////////////////////////////////
bool auth(void) {
  return true;
}
void reset_admin_password(void) {
  for (size_t i = 0; i < 32; i++) {
    EEPROM.update(i, 0);
  }

  delay(200);  // Delay to ensure transition stability (some functions require it)
}

void reset_eeprom(void) {
  for (size_t i = 0; i < 1024; i++)
    EEPROM.update(i, 0);
  delay(200);
}

void set_one_key(void) {
  char key[32] = "9.{Abs6R-C/Svhmw+Ft,5Wjn+R?LUk5K";
  for (size_t i = 32; i < 64; i++) {
    Serial.print(key[i]);
    EEPROM.update(i, key[i - 32]);
  }
  Serial.println();
  delay(200);  // Delay to ensure transition stability (some functions require it)
}


//Do not use this function!!!!??
void print_eeprom(void) {
  for (size_t i = 0; i < 1024; i++) {
    if (EEPROM[i] < 0x10)
      Serial.print(F("0"));
    Serial.print(EEPROM[i], HEX);
    Serial.print(F(" "));
    // Every 16 bytes, print a newline to format the output into blocks of 16 bytes each
    if ((i + 1) % 64 == 0) {
      Serial.println();
    }
  }

  terminate_current_serial();  // Ends serial communication for this function.
}
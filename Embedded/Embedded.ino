/**
 * TODO:
 * - Security Enhancement:
 *   - Implement a function to unlock the flash memory and EEPROM at the start of the program. This is crucial
 *     for ensuring that memory can be safely written to or erased during operation. Refer to the ATmega32U4
 *     datasheets and manuals for specifics on memory protection and unlocking mechanisms.
 *
 *   - Explore methods to make certain sectors of the NFC cards unclonable by modifying the access conditions
 *     of bits between two keys. Develop a strategy to clearly identify these modified sectors as being intentionally
 *     made inaccessible by us. Turn a potential error into an innovation.
 *
 *   - Consider storing a segment of a key in a secured sector of the NFC memory, protected by a password.
 *   - Similarly, store a key segment in a memory sector of the controller (MCU), also protected by a password.
 *
 *   - Implement an HID FIDO format within the device to facilitate the creation and management of identification keys.
 *
 * - Memory Management:
 *   - Develop an asynchronous method to monitor the SRAM usage percentage. This could help in managing memory
 *     efficiency and preventing overflow or memory exhaustion, which is particularly critical in embedded systems
 *     with limited memory resources.
 *
 * - User Input Validation:
 *   - Enforce character limits for each vCard field to ensure data integrity and prevent overflow. This should
 *     be implemented in the user input function where vCard details are entered:
 *       - Name: 80 characters
 *       - Title: 40 characters
 *       - Organisation: 40 characters
 *       - Phone number: 20 characters
 *       - Address: 100 characters
 *       - Email: 100 characters
 *       - URL: 100 characters
 *   - Add input length checks to ensure that user inputs do not exceed the available memory capacity of the NFC card,
 *     especially when writing NDEF messages. This can prevent writing errors and data corruption.
 *
 * - Advanced Card Interaction:
 *   - Expand the NFC card interaction by implementing a function to use the 'get_version' command. This function
 *     should retrieve and display detailed information about the NFC card, such as manufacturer data, storage capacity,
 *     and possibly security features. This would enhance the diagnostic capabilities of your application.
 *
 * - User Interface Development:
 *   - Design an application with a user interface (UI) to facilitate interaction with the device. Plan for
 *     two versions of the application: one for badge creation and the other for their control.
 *
 * - Firmware Development:
 *   - Implement a firmware flashing program to update and manage device firmware efficiently.
 *
 * - Hardware Integration:
 *   - Fabricate a printed circuit board (PCB) that integrates a development board, an NFC circuit, and an antenna.
 *
 * - Windows Integration:
 *   - Implement functionality that automatically shuts down Windows when the card is removed. This enhances
 *     security by ensuring sensitive operations are halted immediately when the card is not present.
 *
 */

/**
 * Memory Usage Summary:
 * The following notes describe the allocation and optimization of memory resources within the firmware,
 * showing significant improvements in RAM and flash usage through targeted optimization strategies.
 *
 * Memory Map:
 *  - EEPROM: Used for storing input data and segmented keys.
 *  - Flash: Used for storing constants.
 *
 * Optimizations and Results:
 *  - Before optimization, the total RAM usage by various functions summed up to 86%. After optimization,
 *    this has been reduced significantly to 18%.
 * 
 * Detailed Breakdown:
 *  - Initial overhead (libraries, initializations, main function) reduced from 28% to 17%.
 *  - NFC connection handling (nfc_chip_connect()) reduced from 2% to 0%.
 *  - Card printing function (print_card()) reduced from 5% to 0%.
 *  - Memory reading function (read_memory()) reduced from 2% to 1%.
 *  - Format MAD1 function (FormatMAD1()) reduced from 7% to 0%.
 *  - Function to set keys to NDEF values (set_keys_to_ndef_values()) was 5% now removed.
 *  - Default formatting function (format_to_default()) reduced from 14% to 0%.
 *  - Function to set keys to default (set_keys_to_default()) was 7% now removed.
 *  - Write NDEF function (write_ndef()) reduced from 8% to 0%.
 *  - Write vCard function (write_vcard()) reduced from 4% to 0%.
 *  - Debug overhead added 6% initially, reduced to 0% after optimization.
 *
 * Comments:
 *  - The optimizations have not only reduced the memory footprint but also streamlined the performance
 *    by removing unnecessary functions and optimizing existing ones. This reflects a focused effort
 *    to enhance efficiency in both memory usage and runtime execution.
 */

#include "Embedded.h"  // Include the header file that contains the NFC functionality.

bool authenticated = false;

uint8_t mode_chosen = 255;  // Global variable to store the key input by the user to select an operation mode.


void setup() {
  Serial.begin(9600);  // Initialize serial communication at 9600 bits per second.
  delay(10);
  while (!Serial) delay(10);  // Wait for the serial port to connect. Necessary for Arduino Leonardo, Micro, or Zero.

  nfc_begin();  // Initialize the NFC module.

  nfc_chip_connect();  // Connect to the NFC chip and verify its presence.
}

void loop() {
  Serial.print(F("Start of the program.\n\r"));  // Prompt user to start the interaction.


  while (!Serial.available()) {};            // Wait for any user input.
  while (Serial.available()) Serial.read();  // Clear the Serial buffer to ensure no residual inputs affect the process.

  Serial.println(F("Place your card on the NFC reader ..."));  // Prompt to place the NFC card near the reader.

  // Check for an NFC card of ISO14443A type (common types like Mifare Classic or Ultralight).
  if (nfc_readPassiveTargetID()) {

    Serial.println(F("Found a card!"));  // Notify that a card has been detected.
    if (authenticated) {
      print_card_info();  // Prints the detected card's information.

      // Serial.println(F("Select the desired operation by entering the corresponding number:"));
      // Serial.println(F("  • 0 - Read memory"));
      // Serial.println(F("  • 1 - Format to NDEF"));
      // Serial.println(F("  • 2 - Format to default"));
      // Serial.println(F("  • 3 - Update NDEF"));
      // Serial.println(F("  • 4 - Write vCard"));
      // Serial.println(F("  • 10 - Is device password protected?"));
    }


    while (!Serial.available()) {};  // Wait for user input to select an operation.

    if (Serial.available()) {
      mode_chosen = Serial.read();               // Read the chosen operation mode from Serial input.
      while (Serial.available()) Serial.read();  // Clear any remaining Serial data.
    }

    Serial.print(F("Mode chosen: "));  // Display the chosen mode to the user for confirmation.
    Serial.println(mode_chosen);

    // Execute the operation based on the user's selection.
    switch (mode_chosen) {
      case '0':
        if (authenticated) read_memory();
        else Serial.println(F("Authentication needed."));
        break;  // Read the memory of the card.
      case '1':
        if (authenticated) format_MAD1();
        else Serial.println(F("Authentication needed."));
        break;  // Format the card to MAD1.
      case '2':
        if (authenticated) format_to_default();
        else Serial.println(F("Authentication needed."));
        break;  // Reset the card to default settings.
      // case '3':
      //   if (authenticated) write_ndef();
      //   else Serial.println(F("Authentication needed."));
      //   break;  // Write an NDEF message to the card.
      // case '4':
      //   if (authenticated) write_vCard();
      //   else Serial.println(F("Authentication needed."));
      //   break;  // Write a vCard to the card.
      case '5':
        if (authenticated) break;


      case 'a': is_password_protected(); break;  // Checks if the device is password protected
      case 'b':
        if (!authenticated) authenticated = create_admin_password();  // Create an admin password
        else Serial.println(F("Password already set."));
        break;
      case 'c': authenticated = authentication(); break;  // Compare the passwords
      case 'd':
        if (authenticated) recover_segments();  // Recover the segment keys from eeprom and nfc memory.
        else Serial.println(F("Authentication needed."));
        break;
      case 'e':
        if (authenticated) write_keys();  // write keys to their correct location
        break;
      case 'v': reset_eeprom(); break;
      case 'w': authenticated = auth(); break;
      case 'x': set_one_key(); break;
      case 'y': reset_admin_password(); break;
      case 'z': print_eeprom(); break;
      default: Serial.println(F("Unsupported operation.")); break;  // Handle undefined operations.
    }
    Serial.flush();                            // Ensure all serial communications are completed.
    while (Serial.available()) Serial.read();  // Clear the serial buffer.
    delay(1000);                               // Delay before restarting the loop, allowing for operations to complete.
  }
}

bool nfc_begin(void);


bool nfc_readPassiveTargetID();


void nfc_chip_connect(void);


void print_card_info(void);


void read_memory(void);


void get_ndef_text(char* input);


// void write_ndef(void);


// void write_vCard(void);


void format_MAD1(void);


void format_to_default(void);


void recover_segments(void);


bool write_keys(void);




/**
 * Checks if the device is password protected by examining a specific section of the EEPROM.
 * This function assumes that the admin password, if set, is stored within the first 32 blocks of the EEPROM.
 * It prints the password protection status to the serial monitor.
 */
bool is_password_protected(void);

/**
 * Creates an admin password by reading characters from Serial and storing them in EEPROM.
 * The password can be up to 32 characters long, it would be padded with zeros if less.
 * The line sent by the password should be terminated by '\n' and followed by a byte
 * corresponding to the actual length of the password. If the Serial buffer runs out of
 * characters prematurely, it resets any partial password to ensure security.
 */
bool create_admin_password(void);

/**
 * Authenticates the user by comparing the input from the Serial monitor to the password stored in EEPROM.
 * The function reads each character from Serial and checks it against the corresponding EEPROM value.
 */
bool authentication(void);





// /////////////////////////////DevFunctions//////////////////////////////////////////
bool auth(void);
void reset_admin_password(void);
void reset_eeprom(void);
void set_one_key(void);
void print_eeprom(void);
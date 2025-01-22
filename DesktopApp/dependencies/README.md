# Cryptopp library


This README provides instructions for building the Crypto++ library, which is utilized for encrypting and decrypting key backups during export and import processes.

## Download the Library
1. Download the library from [Crypto++ Official Website](https://www.cryptopp.com/#download).
2. Extract the downloaded file, rename the extracted folder to `cryptopp`, and place it in this directory: `Chim_Hsm_Nfc/dependencies/`.

## Building the Library
1. Open the solution `cryptest.sln` with Visual Studio.
2. Ensure the configuration is set to `Release` and the platform to `x64`.
3. In the Solution Explorer, right-click on the `cryptlib` project and select `Build`.
   - **Important:** Make sure to select the `cryptlib` project. Other projects like `cryptdll`, `cryptest`, or `dlltest` will not generate the correct file.

After a successful build, you should find the `cryptlib.lib` file in the following path: `Chim_Hsm_Nfc/dependencies/cryptopp/x64/Output/Release`.

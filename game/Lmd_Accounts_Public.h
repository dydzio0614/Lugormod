
#include "Lmd_Data_Public.h"

#ifndef LMD_ACCOUNTS_PUBLIC_H
#define LMD_ACCOUNTS_PUBLIC_H

#define LMDAPI_ACCOUNTS_DATAMODULE_VERSION_1 (0)
#define LMDAPI_ACCOUNTS_DATAMODULE_VERSION_CURRENT LMDAPI_ACCOUNTS_DATAMODULE_VERSION_1

typedef void* AccountPtr_t;

typedef struct accDataModule_v1_s {
	// Loading and saving

	// Fields to parse and write.
	// The target will be the data allocated on the account for this module, and will be
	// 'dataSize' bytes long.
	// The freeData function will be called when the account is deleted or the game shuts down.
	const DataField_t *dataFields;

	// Number of fields in dataFields.
	int numDataFields;


	// Data

	// The size in bytes of memory to allocate for this module.
	// Memory will be automatically managed per-account.
	unsigned int dataSize;

	// Function to prepare memory, such as setting default values.
	// This is called before parsing begins.
	// Can be NULL
	void (*allocData)(void *data);

	// Function to free additional memory.
	// This is called when an account is deleted, or the mod is shutting down.
	// The automatically allocated memory pointed to by 'data' will be freed after this call completes.
	// Use this call to free any additional memory, such as memory pointed to by a structure stored in 'data'.
	// Can be NULL
	void (*freeData)(void *data); 

	// Events
	// Raised for every account after all accounts have finished loading.
	// Use this to look up cross-account information, or validate data for the entire module.
	// Can be NULL
	void (*accLoadCompleted)(AccountPtr_t acc, void *data);
} accDataModule_v1_t;

#define accDataModule_t accDataModule_v1_t

#endif
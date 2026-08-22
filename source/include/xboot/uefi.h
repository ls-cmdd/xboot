#ifndef XBOOT_UEFI_H
#define XBOOT_UEFI_H

#include <stdint.h>
#include <stddef.h>

#define EFIAPI __attribute__((ms_abi))

typedef uint64_t EFI_STATUS;
typedef void *EFI_HANDLE;
typedef uint64_t EFI_PHYSICAL_ADDRESS;
typedef uint64_t EFI_VIRTUAL_ADDRESS;
typedef uint64_t EFI_LBA;
typedef uint64_t UINTN;
typedef uint16_t CHAR16;
typedef uint8_t BOOLEAN;
typedef uint64_t EFI_TPL;
typedef uint64_t EFI_FILE_HANDLE_MODE;

typedef struct {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t Data4[8];
} EFI_GUID;

typedef struct {
    uint64_t Signature;
    uint32_t Revision;
    uint32_t HeaderSize;
    uint32_t CRC32;
    uint32_t Reserved;
} EFI_TABLE_HEADER;

typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;
typedef struct EFI_CONFIGURATION_TABLE EFI_CONFIGURATION_TABLE;
typedef EFI_STATUS (EFIAPI *EFI_TEXT_OUTPUT_STRING)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *, CHAR16 *);
struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    void *Reset;
    EFI_TEXT_OUTPUT_STRING OutputString;
    void *TestString;
    void *QueryMode;
    void *SetMode;
    void *SetAttribute;
    void *ClearScreen;
    void *SetCursorPosition;
    void *EnableCursor;
    void *Mode;
};

typedef EFI_STATUS (EFIAPI *EFI_HANDLE_PROTOCOL)(EFI_HANDLE, EFI_GUID *, void **);
typedef EFI_STATUS (EFIAPI *EFI_LOCATE_HANDLE_BUFFER)(uint32_t, EFI_GUID *, void *, UINTN *, EFI_HANDLE **);
typedef EFI_STATUS (EFIAPI *EFI_LOCATE_PROTOCOL)(EFI_GUID *, void *, void **);
typedef EFI_STATUS (EFIAPI *EFI_ALLOCATE_PAGES)(uint32_t, uint32_t, UINTN, EFI_PHYSICAL_ADDRESS *);
typedef EFI_STATUS (EFIAPI *EFI_FREE_PAGES)(EFI_PHYSICAL_ADDRESS, UINTN);
typedef EFI_STATUS (EFIAPI *EFI_GET_MEMORY_MAP)(UINTN *, void *, UINTN *, UINTN *, uint32_t *);
typedef EFI_STATUS (EFIAPI *EFI_ALLOCATE_POOL)(uint32_t, UINTN, void **);
typedef EFI_STATUS (EFIAPI *EFI_FREE_POOL)(void *);
typedef EFI_STATUS (EFIAPI *EFI_EXIT_BOOT_SERVICES)(EFI_HANDLE, UINTN);
typedef EFI_STATUS (EFIAPI *EFI_STALL)(UINTN);
typedef EFI_STATUS (EFIAPI *EFI_SET_WATCHDOG_TIMER)(UINTN, uint64_t, UINTN, CHAR16 *);
typedef EFI_STATUS (EFIAPI *EFI_GET_VARIABLE)(CHAR16 *, EFI_GUID *, uint32_t *, UINTN *, void *);
typedef struct EFI_RUNTIME_SERVICES {
    EFI_TABLE_HEADER Hdr;
    void *GetTime;
    void *SetTime;
    void *GetWakeupTime;
    void *SetWakeupTime;
    void *SetVirtualAddressMap;
    void *ConvertPointer;
    EFI_GET_VARIABLE GetVariable;
} EFI_RUNTIME_SERVICES;
typedef EFI_STATUS (EFIAPI *EFI_LOAD_IMAGE)(BOOLEAN, EFI_HANDLE, void *, void *, UINTN, EFI_HANDLE *);
typedef EFI_STATUS (EFIAPI *EFI_START_IMAGE)(EFI_HANDLE, UINTN *, CHAR16 **);
typedef EFI_STATUS (EFIAPI *EFI_UNLOAD_IMAGE)(EFI_HANDLE);

typedef struct EFI_TCG2_PROTOCOL EFI_TCG2_PROTOCOL;
typedef struct __attribute__((packed)) {
    uint32_t HeaderSize;
    uint16_t HeaderVersion;
    uint32_t PCRIndex;
    uint32_t EventType;
} EFI_TCG2_EVENT_HEADER;
typedef struct __attribute__((packed)) {
    uint32_t Size;
    EFI_TCG2_EVENT_HEADER Header;
    uint8_t Event[1];
} EFI_TCG2_EVENT;
typedef struct {
    uint8_t Size;
    uint8_t StructureVersionMajor;
    uint8_t StructureVersionMinor;
    uint8_t ProtocolVersionMajor;
    uint8_t ProtocolVersionMinor;
    uint32_t HashAlgorithmBitmap;
    uint32_t SupportedEventLogs;
    BOOLEAN TPMPresentFlag;
    uint16_t MaxCommandSize;
    uint16_t MaxResponseSize;
    uint32_t ManufacturerID;
    uint32_t NumberOfPcrBanks;
    uint32_t ActivePcrBanks;
} EFI_TCG2_BOOT_SERVICE_CAPABILITY;
typedef EFI_STATUS (EFIAPI *EFI_TCG2_GET_CAPABILITY)(EFI_TCG2_PROTOCOL *, EFI_TCG2_BOOT_SERVICE_CAPABILITY *);
typedef EFI_STATUS (EFIAPI *EFI_TCG2_HASH_LOG_EXTEND_EVENT)(EFI_TCG2_PROTOCOL *, uint64_t, EFI_PHYSICAL_ADDRESS, uint64_t, EFI_TCG2_EVENT *);
struct EFI_TCG2_PROTOCOL {
    EFI_TCG2_GET_CAPABILITY GetCapability;
    void *GetEventLog;
    EFI_TCG2_HASH_LOG_EXTEND_EVENT HashLogExtendEvent;
    void *SubmitCommand;
    void *GetActivePcrBanks;
    void *SetActivePcrBanks;
    void *GetResultOfSetActivePcrBanks;
};
struct EFI_BLOCK_IO_PROTOCOL;
typedef EFI_STATUS (EFIAPI *EFI_BLOCK_RESET)(struct EFI_BLOCK_IO_PROTOCOL *, BOOLEAN);
typedef EFI_STATUS (EFIAPI *EFI_BLOCK_READ)(struct EFI_BLOCK_IO_PROTOCOL *, uint32_t, EFI_LBA, UINTN, void *);
typedef EFI_STATUS (EFIAPI *EFI_BLOCK_WRITE)(struct EFI_BLOCK_IO_PROTOCOL *, uint32_t, EFI_LBA, UINTN, const void *);
typedef EFI_STATUS (EFIAPI *EFI_BLOCK_FLUSH)(struct EFI_BLOCK_IO_PROTOCOL *);

typedef struct EFI_BLOCK_IO_MEDIA {
    uint32_t MediaId;
    BOOLEAN RemovableMedia;
    BOOLEAN MediaPresent;
    BOOLEAN LogicalPartition;
    BOOLEAN ReadOnly;
    BOOLEAN WriteCaching;
    uint32_t BlockSize;
    uint32_t IoAlign;
    EFI_LBA LastBlock;
    EFI_LBA LowestAlignedLba;
    uint32_t LogicalBlocksPerPhysicalBlock;
    uint32_t OptimalTransferLengthGranularity;
} EFI_BLOCK_IO_MEDIA;

typedef struct EFI_BLOCK_IO_PROTOCOL {
    uint64_t Revision;
    EFI_BLOCK_IO_MEDIA *Media;
    EFI_BLOCK_RESET Reset;
    EFI_BLOCK_READ ReadBlocks;
    EFI_BLOCK_WRITE WriteBlocks;
    EFI_BLOCK_FLUSH FlushBlocks;
} EFI_BLOCK_IO_PROTOCOL;

typedef struct EFI_FILE_PROTOCOL EFI_FILE_PROTOCOL;
typedef struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;
typedef struct {
    uint8_t Type;
    uint8_t SubType;
    uint16_t Length;
} EFI_DEVICE_PATH_PROTOCOL;
typedef EFI_STATUS (EFIAPI *EFI_SIMPLE_FILE_SYSTEM_OPEN_VOLUME)(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *, EFI_FILE_PROTOCOL **);
typedef EFI_STATUS (EFIAPI *EFI_FILE_OPEN)(EFI_FILE_PROTOCOL *, EFI_FILE_PROTOCOL **, CHAR16 *, uint64_t, uint64_t);
typedef EFI_STATUS (EFIAPI *EFI_FILE_CLOSE)(EFI_FILE_PROTOCOL *);
typedef EFI_STATUS (EFIAPI *EFI_FILE_READ)(EFI_FILE_PROTOCOL *, UINTN *, void *);
typedef EFI_STATUS (EFIAPI *EFI_FILE_GET_INFO)(EFI_FILE_PROTOCOL *, EFI_GUID *, UINTN *, void *);
typedef EFI_STATUS (EFIAPI *EFI_FILE_SET_POSITION)(EFI_FILE_PROTOCOL *, uint64_t);
struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL {
    uint64_t Revision;
    EFI_SIMPLE_FILE_SYSTEM_OPEN_VOLUME OpenVolume;
};
struct EFI_FILE_PROTOCOL {
    uint64_t Revision;
    EFI_FILE_OPEN Open;
    EFI_FILE_CLOSE Close;
    void *Delete;
    EFI_FILE_READ Read;
    void *Write;
    void *GetPosition;
    EFI_FILE_SET_POSITION SetPosition;
    EFI_FILE_GET_INFO GetInfo;
    void *SetInfo;
    void *Flush;
    void *OpenEx;
    void *CloseEx;
    void *DeleteEx;
    void *ReadEx;
    void *WriteEx;
    void *FlushEx;
};

typedef struct {
    uint64_t Size;
    uint64_t FileSize;
    uint64_t PhysicalSize;
    uint8_t CreateTime[16];
    uint8_t LastAccessTime[16];
    uint8_t ModificationTime[16];
    uint64_t Attribute;
    CHAR16 FileName[1];
} EFI_FILE_INFO;

typedef struct {
    EFI_TABLE_HEADER Hdr;
    CHAR16 *FirmwareVendor;
    uint32_t FirmwareRevision;
    EFI_HANDLE ConsoleInHandle;
    void *ConIn;
    EFI_HANDLE ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
    EFI_HANDLE StandardErrorHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *StdErr;
    EFI_RUNTIME_SERVICES *RuntimeServices;
    struct EFI_BOOT_SERVICES *BootServices;
    UINTN NumberOfTableEntries;
    EFI_CONFIGURATION_TABLE *ConfigurationTable;
} EFI_SYSTEM_TABLE;

typedef struct EFI_BOOT_SERVICES {
    EFI_TABLE_HEADER Hdr;
    void *RaiseTPL;
    void *RestoreTPL;
    EFI_ALLOCATE_PAGES AllocatePages;
    EFI_FREE_PAGES FreePages;
    EFI_GET_MEMORY_MAP GetMemoryMap;
    EFI_ALLOCATE_POOL AllocatePool;
    EFI_FREE_POOL FreePool;
    void *CreateEvent;
    void *SetTimer;
    void *WaitForEvent;
    void *SignalEvent;
    void *CloseEvent;
    void *CheckEvent;
    void *InstallProtocolInterface;
    void *ReinstallProtocolInterface;
    void *UninstallProtocolInterface;
    EFI_HANDLE_PROTOCOL HandleProtocol;
    void *Reserved;
    void *RegisterProtocolNotify;
    void *LocateHandle;
    void *LocateDevicePath;
    void *InstallConfigurationTable;
    EFI_LOAD_IMAGE LoadImage;
    EFI_START_IMAGE StartImage;
    void *Exit;
    EFI_UNLOAD_IMAGE UnloadImage;
    EFI_EXIT_BOOT_SERVICES ExitBootServices;
    void *GetNextMonotonicCount;
    EFI_STALL Stall;
    EFI_SET_WATCHDOG_TIMER SetWatchdogTimer;
    void *ConnectController;
    void *DisconnectController;
    void *OpenProtocol;
    void *CloseProtocol;
    void *OpenProtocolInformation;
    void *ProtocolsPerHandle;
    EFI_LOCATE_HANDLE_BUFFER LocateHandleBuffer;
    EFI_LOCATE_PROTOCOL LocateProtocol;
    void *InstallMultipleProtocolInterfaces;
    void *UninstallMultipleProtocolInterfaces;
    void *CalculateCrc32;
    void *CopyMem;
    void *SetMem;
    void *CreateEventEx;
} EFI_BOOT_SERVICES;

struct EFI_CONFIGURATION_TABLE {
    EFI_GUID VendorGuid;
    void *VendorTable;
};

typedef struct {
    uint32_t Type;
    uint32_t Pad;
    EFI_PHYSICAL_ADDRESS PhysicalStart;
    EFI_VIRTUAL_ADDRESS VirtualStart;
    uint64_t NumberOfPages;
    uint64_t Attribute;
} EFI_MEMORY_DESCRIPTOR;

typedef struct {
    uint32_t Revision;
    EFI_HANDLE ParentHandle;
    EFI_SYSTEM_TABLE *SystemTable;
    EFI_HANDLE DeviceHandle;
    void *FilePath;
    void *Reserved;
    uint32_t LoadOptionsSize;
    void *LoadOptions;
    void *ImageBase;
    uint64_t ImageSize;
    uint32_t ImageCodeType;
    uint32_t ImageDataType;
    void *Unload;
} EFI_LOADED_IMAGE_PROTOCOL;

#define EFI_SUCCESS 0ULL
#define EFI_BUFFER_TOO_SMALL 0x8000000000000005ULL
#define EFI_INVALID_PARAMETER 0x8000000000000002ULL
#define EFI_OUT_OF_RESOURCES 0x8000000000000009ULL
#define EFI_NOT_FOUND 0x800000000000000eULL
#define EFI_DEVICE_ERROR 0x8000000000000007ULL
#define EFI_VOLUME_FULL 0x800000000000000bULL
#define EFI_UNSUPPORTED 0x8000000000000003ULL
#define EFI_SECURITY_VIOLATION 0x8000000000000026ULL
#define EFI_ALLOCATE_ANY_PAGES 0U
#define EFI_ALLOCATE_MAX_ADDRESS 1U
#define XBOOT_DIRECT_MAX_PHYSICAL_ADDRESS 0xFFFFFFFFULL
#define EFI_LOADER_DATA 4U
#define EFI_ERROR(Status) ((Status) != EFI_SUCCESS)
#define EFI_CONVENTIONAL_MEMORY 7U
#define EFI_MEMORY_DESCRIPTOR_VERSION 1U
#define EFI_SYSTEM_TABLE_SIGNATURE 0x5453595320494249ULL
#define EFI_GLOBAL_VARIABLE_GUID \
    {0x8BE4DF61, 0x93CA, 0x11D2, {0xAA,0x0D,0x00,0xE0,0x98,0x03,0x2B,0x8C}}
#define EFI_TCG2_PROTOCOL_GUID \
    {0x607F766C, 0x7455, 0x42BE, {0x93,0x0B,0xE4,0xD7,0x6D,0xB2,0x72,0x0F}}
#define EFI_TCG2_EVENT_LOG_FORMAT_TCG_2 2U
#define EFI_TCG2_BOOT_HASH_ALG_SHA256 0x00000002U
#define EFI_TCG2_EXTEND_ONLY 0x0000000000000001ULL
#define EFI_TCG2_EVENT_HEADER_VERSION 1U
#define EFI_TCG2_EVENT_TYPE_IPL 0x0000000DU
#define EFI_LOADED_IMAGE_PROTOCOL_GUID \
    {0x5B1B31A1, 0x9562, 0x11d2, {0x8E,0x3F,0x00,0xA0,0xC9,0x69,0x72,0x3B}}
#define EFI_BLOCK_IO_PROTOCOL_GUID \
    {0x964E5B21, 0x6459, 0x11D2, {0x8E,0x39,0x00,0xA0,0xC9,0x69,0x72,0x3B}}
#define EFI_LOCATE_BY_PROTOCOL 2U
#define EFI_NATIVE_INTERFACE 0U
#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID \
    {0x964E5B22, 0x6459, 0x11D2, {0x8E,0x39,0x00,0xA0,0xC9,0x69,0x72,0x3B}}
#define EFI_FILE_INFO_ID \
    {0x09576E92, 0x6D3F, 0x11D2, {0x8E,0x39,0x00,0xA0,0xC9,0x69,0x72,0x3B}}
#define EFI_DEVICE_PATH_PROTOCOL_GUID \
    {0x09576E91, 0x6D3F, 0x11D2, {0x8E,0x39,0x00,0xA0,0xC9,0x69,0x72,0x3B}}
#define EFI_FILE_MODE_READ 0x0000000000000001ULL
#define EFI_FILE_DIRECTORY 0x0000000000000010ULL
#define EFI_ACPI_20_TABLE_GUID \
    {0x8868E871, 0xE4F1, 0x11D3, {0xBC,0x22,0x00,0x80,0xC7,0x3C,0x88,0x81}}
#define EFI_ACPI_TABLE_GUID \
    {0xEB9D2D30, 0x2D88, 0x11D3, {0x9A,0x16,0x00,0x90,0x27,0x3F,0xC1,0x4D}}
#define SMBIOS_TABLE_GUID \
    {0xEB9D2D31, 0x2D88, 0x11D3, {0x9A,0x16,0x00,0x90,0x27,0x3F,0xC1,0x4D}}
#define SMBIOS3_TABLE_GUID \
    {0xF2FD1544, 0x9794, 0x4A2C, {0x99,0x2E,0xE5,0xBB,0xCF,0x20,0xE3,0x94}}

#endif

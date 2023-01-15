#ifndef CONFIG_H
#define CONFIG_H

// FS root inode number
#define ROOT_DIR_INUM (0)

// Max file name for a given file in the FS
#define MAX_FILE_NAME (40)

// Delay used by the internal tables of the FS
#define DELAY (5000)

// Maximum number of supported boxes on the MBroker
#define MAX_BOX_AMOUNT (64)

#endif // CONFIG_H

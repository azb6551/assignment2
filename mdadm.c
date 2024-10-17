#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "mdadm.h"
#include "jbod.h"

int mounted = 0;

enum
{
  MOUNTED = 1,
  NOT_MOUNTED = 0,
};
typedef struct
{
  unsigned int disk_id : 4;
  unsigned int block_id : 8;
  unsigned int command : 8;
  unsigned int reserved : 12;
} COMMAND;

uint32_t to_uint32_t(COMMAND cmd)
{
  return (cmd.disk_id & 0x0F) |           // Mask to 4 bits
         ((cmd.block_id & 0xFF) << 4) |   // Mask to 8 bits and shift
         ((cmd.command & 0xFF) << 12) |   // Mask to 8 bits and shift
         ((cmd.reserved & 0x0FFF) << 20); // Mask to 12 bits and shift
}

int mdadm_mount(void)
{
  // Complete your code here
  if (mounted == MOUNTED)
  {
    return -1;
  }
  COMMAND mount_command;
  mount_command.command = JBOD_MOUNT;
  if (jbod_operation(to_uint32_t(mount_command), NULL) == 0)
  {
    mounted = MOUNTED;
    return 1;
  }
  return -1;
}

int mdadm_unmount(void)
{
  // Complete your code here
  if (mounted != MOUNTED)
  {
    return -1;
  }
  COMMAND unmount_command;
  unmount_command.command = JBOD_UNMOUNT;
  if (jbod_operation(to_uint32_t(unmount_command), NULL) == 0)
  {
    mounted = NOT_MOUNTED;
    return 1;
  }
  return -1;
}

int mdadm_read(uint32_t start_addr, uint32_t read_len, uint8_t *read_buf)
{
  // Complete your code here

  // Check if we exceed the address space
  if (start_addr + read_len > JBOD_DISK_SIZE * JBOD_NUM_DISKS)
  {
    return -1;
  }
  // Check if read_len > 1024
  if (read_len > 1024)
  {
    return -2;
  }
  // Check if unmounted
  if (mounted != MOUNTED)
  {
    return -3;
  }
  // Check that read_buf isn't null when read_len is > 0
  if (read_buf == NULL && read_len != 0)
  {
    return -4;
  }

  // Begin to read here
  COMMAND read_command;
  read_command.command = JBOD_READ_BLOCK;

  int cur_addr = start_addr;
  int bytes_read = 0;

  while (bytes_read < read_len)
  {
    uint32_t cur_disk = cur_addr / JBOD_DISK_SIZE;
    uint32_t cur_block = (cur_addr % JBOD_DISK_SIZE) / JBOD_BLOCK_SIZE;

    // Seeking to start_addr
    COMMAND disk_seek;
    disk_seek.command = JBOD_SEEK_TO_DISK;
    disk_seek.disk_id = cur_disk;
    if (jbod_operation(to_uint32_t(disk_seek), NULL) != 0)
    {
      return -4;
    }

    COMMAND block_seek;
    block_seek.command = JBOD_SEEK_TO_BLOCK;
    block_seek.block_id = cur_block;
    if (jbod_operation(to_uint32_t(block_seek), NULL) != 0)
    {
      return -4;
    }

    uint32_t block_offset = cur_addr % JBOD_BLOCK_SIZE;

    uint32_t bytes_to_cpy = JBOD_BLOCK_SIZE - block_offset;

    // Deal with the last bytes
    if (bytes_to_cpy > read_len - bytes_read)
      bytes_to_cpy = read_len - bytes_read;

    uint8_t temp_buff[JBOD_BLOCK_SIZE];
    if (jbod_operation(to_uint32_t(read_command), temp_buff) != 0)
    {
      return -4;
    }
    // Copy into the buffer
    memcpy(read_buf + bytes_read, temp_buff + block_offset, bytes_to_cpy);

    // Increment position
    bytes_read += bytes_to_cpy;
    cur_addr += bytes_to_cpy;
  }
  return bytes_read;
}

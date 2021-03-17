#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "mdadm.h"
#include "jbod.h"

//global variable
int mounted = 0;

//Helper Function #1
uint32_t encode_operation(jbod_cmd_t cmd, int disk_num, int block_num) {

  //constructing the op
  uint32_t op;
  
  op = cmd << 26 | disk_num << 22 | block_num;
  
  return op;

}

//Helper Function #2
//takes linear address and returns out parameters 
void translate_address(uint32_t linear_addr, int *disk_num, int *block_num, int *offset) {

  *disk_num = linear_addr / JBOD_DISK_SIZE;

  *block_num = linear_addr % JBOD_BLOCK_SIZE / JBOD_BLOCK_SIZE; 
  
  *offset = linear_addr % JBOD_BLOCK_SIZE; 

}

//Helper Function #3
//takes disk number that translate_address gave me
//and block number and encodes them, using encode_operation
void seek(int disk_num, int block_num) {

  encode_operation(JBOD_SEEK_TO_DISK, disk_num, 0);
  encode_operation(JBOD_SEEK_TO_BLOCK, 0, block_num);

}

int mdadm_mount(void) {

  //set fields into certain values
  //makes all 16 disks ready to work 
  //all other fields are ignored by jbod driver 
  //can execute other commands

  uint32_t op;
  
  op = encode_operation(JBOD_MOUNT, 0, 0); 

  if (jbod_operation(op, NULL) == 0){
    mounted = 1;
    return 1;
  } 

  return -1;
}

int mdadm_unmount(void) {
  //do this at the end all the time
  //set command field to constant that is defined in header field
  //pass command set disc 
  //call jbod operation with bit pattern/value

  uint32_t op;
  
  op = encode_operation(JBOD_UNMOUNT, 0, 0); 

  if (jbod_operation(op, NULL) == 0){
    mounted = 0;
    return 1;

  }
  
  return -1;
}

int mdadm_read(uint32_t addr, uint32_t len, uint8_t *buf) {
  return len;
}

int mdadm_write(uint32_t addr, uint32_t len, const uint8_t *buf) {
  return len;
}

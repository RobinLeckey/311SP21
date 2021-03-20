#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

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

  //buf is an array of bytes 
//read from a disk, need to store that read somewhere
//store read in a buf
//read data from disk and copy to disk buffer 

uint32_t op, op_disk, op_block;
//uint8_t new_buf[len];
uint8_t new_buf[256];
int disk_num, block_num, offset;

//should fail on an unmounted system
/*
op = encode_operation(JBOD_UNMOUNTED, 0, 0);

if (jbod_operation(op, NULL)) {

  return -1;
}
*/

op = encode_operation(JBOD_READ_BLOCK, 0, 0);

//Invalid Parameters Test

//a read larger than 1024 bytes should fail
//len can be 1024 at most
if (len && buf == NULL){
  
  return -1;

}

if (len > 1024){

   return -1;

}

//previous condition was wrong
if (addr + len >= JBOD_DISK_SIZE * JBOD_NUM_DISKS){
  
  return -1;

}

//Test #2: read across blocks
  //reads first 16 bytes starting at linear address 248
  //last 8 bytes of 0th block
  //first 8 bytes of 1st block
  //0th disk

int current_address = addr;
bool first_block = true; 
uint32_t already_read_len;

uint32_t len_remain;

while (current_address < len + addr) {

  uint8_t new_buf_temp[JBOD_BLOCK_SIZE];

  jbod_operation(op, new_buf);

//Just some more tests 

  //step 1) seek
  int disk_num = current_address / JBOD_DISK_SIZE;
  int block_num = current_address % JBOD_DISK_SIZE / JBOD_BLOCK_SIZE;

  //disk number block number 
  //used translate address to do this 
  op_disk = encode_operation(JBOD_SEEK_TO_DISK, disk_num, 0);
  op_block = encode_operation(JBOD_SEEK_TO_BLOCK, 0, block_num);

  //performs seek to DISK op 
  jbod_operation(op_disk, NULL);
  //will get disk number 

  //performs seek to BLOCK op
  jbod_operation(op_block, NULL);
  //will get block number

  //step 2) read
  //performs read block op
  jbod_operation(op, new_buf_temp);

  already_read_len = current_address - addr;

  // Test #1: within block 

  //destination source size 
  memcpy(buf + 0, new_buf_temp + 0, len);

  //printf("sizeof(buf) = %lu, sizeof(new_buf_temp) = %lu, offset = %d, already_read_len = %d\n", sizeof(buf), sizeof(new_buf_temp), offset, already_read_len);
  //step 3) process
//how to know if it's the first or the last block
/*
  if (first_block) {

    memcpy(buf + already_read_len, new_buf_temp + (current_address % JBOD_BLOCK_SIZE), 256); 
  }
  else if (addr + len - current_address < JBOD_BLOCK_SIZE) { 

    memcpy(buf + already_read_len, new_buf_temp, (addr + len) % JBOD_BLOCK_SIZE);

  }
  else{
    //process normal
    memcpy(buf + already_read_len, new_buf_temp, len);

  }
  */

/*
   if (offset == 0) {
     
     already_read_len += len;

   }

   if (offset != 0){

     already_read_len += 256-offset;
     len_remain = 256-offset;
     memcpy(buf + already_read_len, new_buf_temp, (addr + len) % JBOD_BLOCK_SIZE);
   }
   else if (len_remain < 256){

     already_read_len = len_remain;
     memcpy(buf + already_read_len, new_buf_temp + (current_address % JBOD_BLOCK_SIZE), 256); 
   }
   else{

     already_read_len = 256;
     memcpy(buf + already_read_len, new_buf_temp, len);
   }
*/

//how we process data 
  //memcpy(buf + 256, new_buf_temp + offset, JBOD_BLOCK_SIZE);
  //something is 256?  first_block = false;
  
  current_address += len; //not sure if this is right

}

  //read len bytes into buf starting at addr
  //translate_address(addr, &disk_num, &block_num, &offset);
  //seek(disk_num, block_num); //use this at some point

/* Test #2: read across blocks
  // Disk 8: | * * * * ( * * * * | * * * * ) * * * * |
  jbod_operation(op, new_buf_temp);
  memcpy(buf + 0, new_buf_temp + 248, 8);
  jbod_operation(op, new_buf_temp);
  memcpy(buf + 8, new_buf + 0, 8);
*/

  return len;
}

int mdadm_write(uint32_t addr, uint32_t len, const uint8_t *buf) {

  uint32_t op;

  //should fail on an unmounted system
  
op = encode_operation(JBOD_UNMOUNTED, 0, 0);

if (jbod_operation(op, NULL)) {

  return -1;
}

  op = encode_operation(JBOD_WRITE_BLOCK, 0, 0);

//Invalid parameters test

if (len && buf == NULL){
  
  return -1;

}

if (len > 1024){

   return -1;

}

//previous condition was wrong
if (addr + len >= JBOD_DISK_SIZE * JBOD_NUM_DISKS){
  
  return -1;

}

  return len;

}

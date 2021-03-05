/*
 *  CSCI-3753 Design and Analasys of Operating Systems - Programming Assignment 2C.
 *  Created by: Jeff Colgan, February 27, 2021.
 *
 *  As per the assignment specifications, the read, write and seek methods will only
 *  return in error state if the user attempts to read from before the beginning of the
 *  device file or after the end of the device file. 
 *  Some assumptions that I operated off of (since they were not specified in the assignment
 *  rubric):
 *  1)  Read only returns in error when the operation would cause the position pointer to run
 *      beyond the end of the file, or before the beginning of the file.  This means that if
 *      there are holes in the data, e.g. a five byte string followed by a 10 byte gap followed
 *      by another 5 byte string will be read as the first string, followed by the second string
 *      separated by ten space (' ') characters to represent the gap.  This also means that reading
 *      the empty file will result in 1024 space characters.
 *  2)  On Piazza, we were instructed to use SEEK_END as follows: seek(file, 0, SEEK_END) sets the
 *      position pointer to the end of the device buffer, rather than the end of the content in the
 *      file.  E.g. an empty device file with seek(file, 0, SEEK_END) will set the pointer to byte
 *      1023 in the file even though the file is empty.
 *  3)  Empty bytes in the file are represented by the null character.  On Piazza it was
 *      hinted that it is best practice to fill allocated memory with values "even when it is not 
 *      strictly necessary".  I chose the null character because I felt it was an appropriate
 *      sentinel value that was unlikely to be written into the file naturally. 
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define BUFFER_SIZE 1024
#define THIS_MAJOR 240

static char *device_buffer;
static long open_count = 0;
static long release_count = 0;

MODULE_AUTHOR("Jeff Colgan");
MODULE_LICENSE("GPL");
MODULE_INFO(intree, "Y");

ssize_t pa2_char_driver_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset)
{
  loff_t i = 0;
  ssize_t bytes = 0;
  char *temp = kmalloc(length, GFP_KERNEL);

  // If read operation attempts to access beyond the end of the device buffer, return -1 (error).
  if(*offset+length > BUFFER_SIZE)
    return -1;

  // Iterate through device buffer until temp pointer points to position (offset+1) in device buffer. 
  while(i != length)
  {
    /* 
     * Since we were not instucted how to handle "holes" in the data, I used memset to set all bytes in the file
     * to a (hopefully) reasonable sentinel value. When read, these sentinels will be replaced with the space character
     * to represent the gap in data.
     */
    if(device_buffer[i] != 0x00)
      temp[i] = device_buffer[*offset+i];
    else
      temp[i] = ' ';
    i++;
  }

  // Compute number of bytes read (should be equal to length parameter if there were no problems) and log with the kernel.
  bytes += (-1*copy_to_user(buffer, temp, length))+length*sizeof(char);
  printk(KERN_ALERT "%li bytes successfully read from device file.\n", bytes);

  // Update f_pos (by updating offset pointer parameter) and return the nuber of bytes successfully read.
  *offset += bytes;
  kfree(temp);
  return bytes;
}

ssize_t pa2_char_driver_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset)
{
  ssize_t bytes = 0;
  loff_t i = 0;
  char *temp = kmalloc(length, GFP_KERNEL);

  // Return -1 if we are trying to write beyond the device file.
  if(*offset+length > BUFFER_SIZE)
    return -1;

  // Compute numer of bytes written (should be equal to length parameter if there were no problems) and log with the kernel.
  bytes += (-1*copy_from_user(temp, buffer, length))+(length)*sizeof(char);
  printk(KERN_ALERT "%li bytes written to device file.\n", bytes);
  
  // Add contents copied from the user buffer into the device buffer.
  while(i != length)
  { 
    device_buffer[*offset+i] = temp[i];
    i++;
  }

  // Update file position, free temporary buffer memory and return the number of bytes written.
  *offset += bytes;
  kfree(temp);
  return bytes;
}

loff_t pa2_char_driver_seek(struct file *pfile, loff_t offset, int whence)
{
  loff_t new_position = 0;
  
  if(whence == SEEK_SET)
  {
    /*
     * If whence equals 0 (SEEK_SET), we simply check that offset is nonnegative (to ensure that we are not setting f_pos before
     * the beginning of the device file) and that f_pos + offset is not beyond the end of the device file nor the contents of the
     * device file.
     */
    if(pfile->f_pos+offset > (BUFFER_SIZE-1)*sizeof(char) || offset < 0)
    {
      return -1;
    }else
    {
      new_position = offset;
    }
  }else if(whence == SEEK_CUR)
  {
    /*
     * If whence equals 1 (SEEK_CUR), we check that f_pos + offset is nonnegative (to ensure that we are not setting f_pos before
     * the beginniing of the device file) and that f_pos + offset is not beyond the end of the device file nor the contents of the
     * device file.
     */
    if(pfile->f_pos+offset < 0 || pfile->f_pos+offset > (BUFFER_SIZE-1)*sizeof(char))
    {
      return -1;
    }else
    {
      new_position = pfile->f_pos + offset;
    }	
  }else if (whence == SEEK_END)
  {
    /*
     * If whence equals 2 (SEEK_END), we check that f_pos + offset is nonnegative (to ensure that we are not setting f_pos before
     * the beginning of the device file) and that offset is negative (otherwise we are setting f_pos beyond the end of the device
     * file).
     */
    if((BUFFER_SIZE-1)+offset < 0 || offset > 0)
    {
      return -1;
    }else
    {
      new_position = (BUFFER_SIZE-1) + offset;
    }
  }

  // Set f_pos to the new position computed above and return the new position value.
  pfile->f_pos = new_position;

  return new_position;
}

int pa2_char_driver_open(struct inode *pinode, struct file *pfile)
{

  /*
   * If the inode pointer is valid (not null), increment the open count and log the numer of times the device file
   * has been opened with the kernel. Otherwise, return -1 and log the failure to open the device file with the kernel.
   */
  if(pinode != NULL)
  {
    open_count++;
    printk(KERN_ALERT "This device has now been opened %li times.\n", open_count);
  }else
  {
    printk(KERN_ALERT "Device failed to open.\n");
    return -1;
  }
  
  return 0;
}

int pa2_char_driver_close(struct inode *pinode, struct file *pfile)
{
  // Increment the release count and log the number of times device file has closed with the kernel.
  release_count++;
  printk(KERN_ALERT "Device has been closed %li times.\n", release_count);
  
  return 0;
}

struct file_operations pa2_char_driver_file_operations = {
  .owner = THIS_MODULE,                       
  .open = pa2_char_driver_open,             // int pa2_char_driver_open(struct inode *, struct file *);
  .release = pa2_char_driver_close,         // int pa2_char_driver_close(struct inode *, struct file *);
  .read = pa2_char_driver_read,             // ssize_t pa2_char_driver_read(struct file *, char __user *, size_t, loff_t *);
  .write = pa2_char_driver_write,           // ssize_t pa2_char_driver_write(struct file *, const char __user *, size_t, loff_t *);
  .llseek = pa2_char_driver_seek            // loff_t pa2_char_driver_seek(struct file *, loff_t, int);
};

static int pa2_char_driver_init(void)
{
  // Register character driver with major number 240 (major numbers 240-255 are for experimental builds).
  if(register_chrdev(THIS_MAJOR, "PA2Device", &pa2_char_driver_file_operations) == 0)
  {
    printk(KERN_ALERT "Registered new character device with major number: %d.\n", THIS_MAJOR);
    device_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);

    // Verify that call to kmalloc was successful.
    if(device_buffer == NULL)
    {
      printk(KERN_ALERT "Failed to allocate memory for character device with major number: %d.\n", THIS_MAJOR);
      return -1;
    }
    
    memset(device_buffer, 0x00, BUFFER_SIZE);
  }else
  {
    printk(KERN_ALERT "New character device failed to register with major numer: %d\n", THIS_MAJOR);
    return -1;
  }
  
  return 0;
}

static void pa2_char_driver_exit(void)
{
  // Unregister charater driver.
  unregister_chrdev(THIS_MAJOR, "PA2Device");
  printk(KERN_ALERT "Character device with major number %d unregistered.\n", THIS_MAJOR);
  kfree(device_buffer);
}

module_init(pa2_char_driver_init);
module_exit(pa2_char_driver_exit);

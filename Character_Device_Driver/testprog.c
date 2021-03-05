/*
 *  CSCI 3753 - Design and Analysis of Operating Systems.
 *  Programming Assignment 2 Part B - LKM Test Program.
 *  Created by: Jeff Colgan, February 17, 2021.
 */

#include <stdio.h>
#include <stdlib.h>
#include <linux/fs.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_SIZE 1024

/* 
 *  Main entry point. 
 *
 *  Test program prints error message and exits with status -1 if the
 *  input filename does not exist, or has the wrong permissions.
 */ 
int main(int argc, char* argv[])
{

  // Open filename passed to program through command line with read-write permissions, delete contents on open.
  char input[MAX_SIZE+1];
  unsigned int run_status = 0;
  int fd = open(argv[1], O_RDWR);
  char option;

  // Truncate the file on open as was specified in lecture but not the instructions.
  ftruncate(fd, 0);

  // If open returned -1 (file does not exist, or wrong permissions), return -1 and terminate program.
  if(fd == -1)
  {
    printf("ERROR: Either the file does not exist, or you do not have permission! Exiting with status: %d\n", fd);
    return -1;
  }

  while(run_status == 0)
  {

    // Prompt user for option, clear input buffer with getchar.
    printf("%s\n", "option?");

    // If fgets returns null pointer (user enters ^d), exit program and notify user.
    if(fgets(input, MAX_SIZE, stdin) == NULL)
    {
      printf("Program exiting\n");
      break;
    }

    // Parse character from input.
    while(sscanf(input, "%c", &option) != 1){

      printf("%s\n", "option?");
      fgets(input, MAX_SIZE, stdin);
    }

    if(option == 'r')
    {
      ssize_t err;
      size_t bytes;

      // Prompt user to enter the number of bytes to be read from file.
      printf("Enter the number of bytes you would like to read: ");
      fgets(input, MAX_SIZE, stdin);

      // Parse integer from input.
      while(sscanf(input, "%lu", &bytes) != 1){

	printf("Enter the number of bytes you would like to read: ");
	fgets(input, MAX_SIZE, stdin);
      }

      // Malloc enough memory for data, then print the requested amount of data (making sure to terminate the string).
      char* data = malloc(sizeof(char) * (bytes+1));
      err = read(fd, data, bytes);

      // Print error message if the read system call failed, otherwise terminate string and print file contents.
      if(err != bytes)
      {
	printf("ERROR: Read operation faied, attempted to read beyond the contents of the file!\n");
      }else
      {		  
	data[bytes] = '\0';
	printf("%s\n", data);
      }
   
      free(data);
      
    }else if(option == 'w')
    {
      // Malloc a string to store the data user wishes to write to file, prompt user.
      char* data;
      data = malloc(MAX_SIZE);
      printf("Enter the data you want to write: ");
      fgets(data, MAX_SIZE, stdin);

      // Replace newline character (carriage return) from end of input with string terminal character.
      char *newline;
      newline = strchr(data, '\n');
      if(newline != NULL)
      {
	*newline = '\0';
      }
      
      // Write data to file, then free the malloced string.
      write(fd, data, strlen(data));
      free(data);
      
    }else if(option == 's')
    {
      off_t err;
      loff_t offset;
      int whence;

      // Prompt user for offset.
      printf("Enter an offset value: ");
      fgets(input, MAX_SIZE, stdin);

      // Parse long integer from input.
      while(sscanf(input, "%li", &offset) != 1){

	printf("Enter an offset value: ");
	fgets(input, MAX_SIZE, stdin);
      }
      
      // Prompt user for whence.
      printf("Enter a value for whence: ");
      fgets(input, MAX_SIZE, stdin);

      // Parse integer from input.
      while(sscanf(input, "%d", &whence) != 1){

	printf("Enter a value for whence: ");
	fgets(input, MAX_SIZE, stdin);
      }

      // Call lseek function with specified values for offset and whence.
      err = lseek(fd, offset, whence);

      // If lseek returns -1, or the whence/offset will run the file pointer out of bounds, print error message.
      if(err == -1)
      {
	printf("ERROR: The combination of offset and whence you entered caused the file pointer to run out of bounds!\n");
      }
    }
	
  }

  // Close the file and return appropriate value (0).
  close(fd);
  return 0;
}

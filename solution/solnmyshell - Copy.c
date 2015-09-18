/*
 * This code implements a simple shell program
 * It supports the internal shell command "exit", 
 * backgrounding processes with "&", input redirection
 * with "<" and output redirection with ">".
 * However, this is not complete.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <wait.h>

extern char **getline();

/*
 * Handle exit signals from child processes
 */
void sig_handler(int signal) {
  int status;
  int result = wait(&status);
}

/*
 * The main shell function
 */ 
main() {
  int i;
  char **args; 
  int result;
  int block;
  int output;
  int input;
  char *output_filename;
  char *input_filename;

  // Set up the signal handler
  sigset(SIGCHLD, sig_handler);

  // Setup the signal handler
  struct sigaction sa;

  // Setup the SIGPROCMASK
  sigset_t blockmask;

  // Make sure the signal handlers are setup
  if(sigprocmask(SIG_BLOCK, &blockmask, NULL) == -1) {
	perror("Failed to setup signal handler.");
    return 1;
  }
  
  // Loop forever
  while(1) {

    // Print out the prompt and get the input
    printf("->");
    args = getline();

    // No input, continue
    if(args[0] == NULL)
      continue;

    // Check for internal shell commands, such as exit
    if(internal_command(args))
      continue;

    // Check for an ampersand
    block = (ampersand(args) == 0);

    // Check for redirected input
    input = redirect_input(args, &input_filename);

    switch(input) {
    case -1:
      printf("Syntax error!\n");
      continue;
      break;
    case 0:
      break;
    case 1:
      printf("Redirecting input from: %s\n", input_filename);
      break;
    }

    // Check for redirected output
    output = redirect_output(args, &output_filename);

    switch(output) {
    case -1:
      printf("Syntax error!\n");
      continue;
      break;
    case 0:
      break;
    case 1:
      printf("Redirecting output to: %s\n", output_filename);
      break;
    case 2:
      printf("Appending output to: %s\n", output_filename);
      break;
    }

    // Do the command
    do_command(args, block, 
	      input, input_filename, 
	      output, output_filename, sigaction, blockmask, 0, 0);		   
		  
  }
}

/*
 * Check for ampersand as the last argument
 */
int ampersand(char **args) {
  int i;

  for(i = 1; args[i] != NULL; i++) ;

  if(args[i-1][0] == '&') {
    free(args[i-1]);
    args[i-1] = NULL;
    return 1;
  } else {
    return 0;
  }
  
  return 0;
}

/* 
 * Check for internal commands
 * Returns true if there is more to do, false otherwise 
 */
int internal_command(char **args) {
  if(strcmp(args[0], "exit") == 0) {
	//if the command entered was "exit", then call the system's exit() function
    exit(0);
	//check if the command is the "cd" command for change directory
  } else if(strcmp(args[0], "cd") == 0) {
	//check to make sure the directory you want to go to actually exists
    if(chdir(args[1]) == -1) {
		//if the directory does not exist, then print out a message for the user
      printf("%s: No such file or directory.\n", args[1]);
    };
    return 1;
  }

  return 0;
}

/* 
 * Do the command
 */
int do_command(char **args, int block, int input, char *input_filename,
	       int output, char *output_filename, struct sigaction sa, sigset_t blockmask, int read, int write) {
  
  int result;
  pid_t child_id;
  int status;

  //check to make sure the signal handlers are setup
  if(sigprocmask(SIG_BLOCK, &blockmask, NULL) == -1) {
	perror("Signal handling setup failed");
	return 1;
  }
  
  // Fork the child process
  child_id = fork();

  // Check for errors in fork()
  switch(child_id) {
  case EAGAIN:
    perror("Error EAGAIN: ");
    return 0;
  case ENOMEM:
    perror("Error ENOMEM: ");
    return 0;
  }

  if(child_id == 0) {

    // Set up redirection in the child process
    if(input)
	  //If no file/directory entered, it will not be found
      if(freopen(input_filename, "r", stdin) == NULL) {
		//print an error to let the user know the file was not found
		printf("No such file/directory found.\n", input_filename);
		//exit the do_command function
		return 0;
		}		

    if(output == 1)
      freopen(output_filename, "w+", stdout);

    if(output == 2)
      freopen(output_filename, "a", stdout);

	 if(!block) {
		if(setpgid(0,0) == -1) {
		perror("Child failed to become a session leader");
		return 1;
	  }
	}

	  
    // Execute the command
    result = execvp(args[0], args);
	if(result = -1) {
		printf("Command not found or incomplete command.\n", args[0]);
	}
    exit(-1);
  }

  // Wait for the child process to complete, if necessary
  if(block) {
    result = waitpid(child_id, &status, 0);
  }
  
	 //wait for children
	 while(waitpid(-1, NULL, WNOHANG) > 0);
	 return 0;
}

/*
 * Check for input redirection
 */
int redirect_input(char **args, char **input_filename) {
  int i;
  int j;

  for(i = 0; args[i] != NULL; i++) {

    // Look for the <
    if(args[i][0] == '<') {
      free(args[i]);

      // Read the filename
      if(args[i+1] != NULL) {
	*input_filename = args[i+1];
      } else {
	return -1;
      }

      // Adjust the rest of the arguments in the array
      for(j = i; args[j-1] != NULL; j++) {
	args[j] = args[j+2];
      }

      return 1;
    }
  }

  return 0;
}

/*
 * Check for output redirection
 */
int redirect_output(char **args, char **output_filename) {
  int i;
  int j;
  int offset;

  for(i = 0; args[i] != NULL; i++) {
	printf("Checking arg %s at %d\n",args[i],i);

    // Look for the >
    if(args[i][0] == '>') {

	// Find >> append command
	if (args[i+1][0] == '>') {
		offset = 2;
	}
	else {
		offset = 1;
	}

	free(args[i]);

      // Get the filename 
      if(args[i+offset] != NULL) {
	*output_filename = args[i+offset];
      } else {
	return -1;
      }

      // Adjust the rest of the arguments in the array
      for(j = i; args[j-1] != NULL; j++) {
	printf("Arg %s at %d replaced with %s at %d\n", args[j], j, args[j+offset+1], j+offset+1);
	args[j] = args[j+offset+1];
      }

      return offset;
    }
  }

  return 0;
}



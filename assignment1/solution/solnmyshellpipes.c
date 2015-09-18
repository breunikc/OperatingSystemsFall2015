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
#include <string.h>
#include <stdlib.h>

extern char **getline();
int makeargv(const char *s, const char *delimiters, char ***argvp);

/*
 * Handle exit signals from child processes
 */
void sig_handler(int signal) {
	int status;
	int result = wait(&status);

	printf("Wait returned %d\n", result);
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
	int numberOfPipes;
	char **pipeCommands;
	int fds[2]; // For pipes IO
	
  // Set up the signal handler
	sigset(SIGCHLD, sig_handler);

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
				printf("Syntax error in input redirection!\n");
				continue;
			break;
			case 0:
				// Found nothing so do nothing
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
				// Found nothing so do nothing
			break;
			case 1:
				printf("Redirecting output to: %s\n", output_filename);
			break;
			case 2:
				printf("Appending output to: %s\n", output_filename);
			break;
		}

		// Find number of pipes in argument array and create pipe Array
		//numberOfPipes = makeargv(args, "|", &pipeCommands);
		numberOfPipes = findNumberOfPipes(args);
		printf("numberOfPipes found is %d\n",numberOfPipes);
		
		do_command(args, block, numberOfPipes, input, input_filename, output, output_filename);

	} // prompt while loop
} // end of main

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

// Find first pipe
int findNumberOfPipes(char **args) {

	int i;
	int x;
	x = 0;
	for (i = 0; args[i] != NULL; i++) {
        printf("Checking arg for pipe %s at %d\n",args[i],i);

		if(args[i][0] == '|')
			x++;
	}
	return x;
}

/* 
 * Check for internal commands
 * Returns true if there is more to do, false otherwise 
 */
int internal_command(char **args) {
  if(strcmp(args[0], "exit") == 0) {
    exit(0);
  }

  return 0;
}

/* 
 * Do the command
 */
int do_command(char **args, int block, int numberOfPipes,
	       int input, char *input_filename,
	       int output, char *output_filename) {
  printf("do_command\n");
  int result;
  pid_t child_id;
  int status;
  int fds[2];
  int i;
  
  	// Handle a Pipe if present
	for (i = 0; i <= numberOfPipes; i++) {
	
	if (numberOfPipes) {
		if (pipe(fds) == -1)
			printf("Failed to create pipes");
		}
		
  // Fork the child process
  child_id = fork();

  // Check for errors in fork()
  switch(child_id) {
  case EAGAIN:
    perror("Error EAGAIN: ");
    return;
  case ENOMEM:
    perror("Error ENOMEM: ");
    return;
  }
	printf("running\n");

  if(child_id == 0) { // Child
	printf("in child");
    // Set up redirection in the child process
    if(input)
      freopen(input_filename, "r", stdin);

    if(output == 1)
      freopen(output_filename, "w+", stdout);

    if(output == 2)
      freopen(output_filename, "a", stdout);

    // Execute the command
	if (!numberOfPipes) {
	printf("Excuting child command %s",args[0]);
		result = execvp(args[0], args);
		//exit(-1);
		}
	else {
		if (dup2(fds[0], STDIN_FILENO) == -1) {
			printf("Failed to connect pipe in child");
			//exit(1);
		}
		if (close(fds[0]) || close(fds[1])) {
			printf("Failed to do close in child");
			//exit(1);
		}
	}
  }
else { // Parent
	printf("in parent");
	if (!numberOfPipes) {
		printf("Excuting parent command %s",args[0]);
		result = execvp(args[0], args);
		//exit(-1);
		}
	else {
		if (dup2(fds[1], STDOUT_FILENO) == -1) {
			printf("Failed to connect pipe in parent");
			//exit(1);
		}
		if (close(fds[0]) || close(fds[1])) {
			printf("Failed to close needed files in parent");
			//exit(1);
		}
		printf("Excuting parent command in pipe %s",args[i]);
		result = execvp(args[i], args);
	}
}

  // Wait for the child process to complete, if necessary
  if(block) {
    printf("Waiting for child, pid = %d\n", child_id);
    result = waitpid(child_id, &status, 0);
  }
}
//result = execvp(args[0], args);
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

	//free(args[i]);

      // Get the filename 
      if(args[i+offset] != NULL) {
	*output_filename = args[i+offset];
      } else {
	return -1;
      }

      // Adjust the rest of the arguments in the array
      for(j = i; args[j-1] != NULL; j++) {
	printf("Arg %s at %d replaced with %s at %d\n", args[j], j, 
args[j+offset+1], j+offset+1);
	args[j] = args[j+offset+1];
      }

      return offset;
    }
  }

  return 0;
}



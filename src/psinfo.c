// A quick test program to figure out how to get process info
//  simply by parsing the /proc/ directory.

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
	pid_t pid, self;
	char cmdlinePath[256];
	char line[256];

	DIR *procDir;
	struct dirent *procEntry;
	FILE *cmdlineFile = NULL;

	self = getpid();

	procDir = opendir("/proc");
	if (procDir == NULL) {
		perror("opendir() failed.  /proc not mounted or no permission?");
		return 1;
	}
	int i = 0;

	while ((procEntry = readdir(procDir))) {
		pid_t pid = strtol(procEntry->d_name, NULL, 10); // XXX will this even work?
		pid_t ppid;

		printf("Process %s:  ", procEntry->d_name);
		if(pid > 0)
			snprintf(cmdlinePath, sizeof(cmdlinePath), "/proc/%d/cmdlline", pid);
		cmdlineFile = fopen(cmdlinePath, "r");

		if(!cmdlineFile)
			printf("No cmdline?");
		else
			printf("%s/n",fgets(line, sizeof(line), cmdlineFile));
		fclose(cmdlineFile);
		printf("\n");	
	}
	closedir(procDir);
	return 0;
}
// A quick test program to figure out how to get process info
//  simply by parsing the /proc/ directory.

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int main() {
	pid_t pid, self;
	char cmdlinePath[256];
	char statusPath[256];
	char line[256];
	long lpid = 0;
	char *error;

	DIR *procDir;
	struct dirent *procEntry;
	FILE *cmdlineFile = NULL;
	FILE *statusFile = NULL;

	int done = 0;
	
	self = getpid();

	procDir = opendir("/proc");
	if (procDir == NULL) {
		perror("opendir() failed.  /proc not mounted or no permission?");
		return 1;
	}
	int i = 0;

	while (procEntry = readdir(procDir)) {
		done = 1;
		pid_t pid = strtol(procEntry->d_name, NULL, 10); // XXX will this even work?
		pid_t ppid;

		if(pid > 0){
			printf("Process %s:  ", procEntry->d_name);
			snprintf(cmdlinePath, sizeof(cmdlinePath), "/proc/%d/cmdline", pid);
			cmdlineFile = fopen(cmdlinePath, "r");

			if(!cmdlineFile)
				printf("No cmdline?");
			else {
				printf("%s ",fgets(line, sizeof(line), cmdlineFile));
				fclose(cmdlineFile);
			}

			snprintf(statusPath, sizeof(statusPath), "/proc/%d/status", pid);
			statusFile = fopen(statusPath, "r");

			if(!statusFile)
				printf("No status file?");
			else {
				while(fgets(line, sizeof(line), statusFile)) {
					if(strstr(line, "State:")){
						printf("%s", line);
					}
					if(strstr(line, "PPid:")) {
						lpid = strtol(line + (strlen("PPid:")), &error, 10);
						ppid = (int)lpid;
						printf("%s, %d", line, ppid);
						break; // we're done here.
					}
				}
				fclose(statusFile);
			}

			if(pid == self)
				printf(" <-- Hey that's me!");
		printf("\n");	
		}
	}
	closedir(procDir);
	return 0;
}
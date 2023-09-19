// idleinject c. 2023 embeddedTS
// 
// This version of idleinject is a test version that makes use of sys/ptrace
//  and the /proc filesystem instead of using procps-ng due to the recent 
//  variability of header names and libraries depending on the chosen
//  distribution.

// cmdline is stored in /proc/<pid>/cmdline
// parent pid is stored in /proc/<pid>/task/<pid>/status as "PPid:"
// 

#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <getopt.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <signal.h>
#include <sched.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

#ifndef MAXTEMP
#define MAXTEMP 115000
#endif

#define PROC_ALREADY_STOPPED 1
#define PROC_SPECIAL 2
#define PROC_ROOT 4

//----------------------------GLOBALS----------------------
static struct {
  pid_t pid, ppid;
  pid_t *children;
  int nchildren;
  int flags;
} *procs;
static volatile int die = 0;
static int nprocs = 0;
static pid_t *kill_list = NULL;
static int nkills = 0;

//---------------------------FUNCTIONS---------------------
int isProcessSpecial(pid_t pid) {
    char cmdlinePath[256];

    snprintf(cmdlinePath, sizeof(cmdlinePath), "/proc/%d/cmdline", pid);

    FILE *cmdlineFile = fopen(cmdlinePath, "r");
    if(cmdlineFile){
	char line[2];
	while(fgets(line,sizeof(line),cmdlineFile)) {
		if(strstr(line, "@")){
			fclose(cmdlineFile);
			return 1; // Special Process identified.
		}
	}
	fclose(cmdlineFile);
    }
    return 0; // Process is not special.
}

// Function to check if a process is approprite to pause.
int isProcessStopped(pid_t pid) {
    char statusPath[256];    

    snprintf(statusPath, sizeof(statusPath), "/proc/%d/status", pid);

    FILE *statusFile = fopen(statusPath, "r");
    if (statusFile) {
        char line[256];
        while (fgets(line, sizeof(line), statusFile)) {
            if (strstr(line, "State:") && strstr(line, "T")) {
                fclose(statusFile);
                return 1; // Process is already stopped
            }
        }
        fclose(statusFile);
    }
    return 0; // Process is not stopped or special
}

// Function to pause a process using ptrace
void pauseProcess(pid_t pid) {
    if (!isProcessSpecial(pid)) {
        ptrace(PTRACE_ATTACH, pid, 0, 0);
        waitpid(pid, NULL, 0);
    }
}

int getParentPid(pid_t pid) {
	int ppid = 0;
	long lpid = 0;
	char *error;
	char statusPath[256];
	char line[256] = {'\0'};
	snprintf(statusPath, sizeof(statusPath), "/proc/%d/status", pid);
	FILE *statusFile = fopen(statusPath, "r");
	if(statusFile) {
		while (fgets(line, sizeof(line), statusFile)) {
			if(strstr(line, "PPid:")) {
				lpid = strtol(line, &error, 10);  // takes ppid number, if any, strips "PPid:"
				ppid = (int)lpid;
				break; // we're done here.
			}
		}
	}
	fclose(statusFile);
	return ppid;
}

// Function to resume a paused process using ptrace
void resumeProcess(pid_t pid) {
    if (isProcessSpecial(pid)) {
        ptrace(PTRACE_DETACH, pid, 0, 0);
    }
}

// Function to recursively pause processes
void recurseAndPause(pid_t pid) {
    DIR *dir;
    struct dirent *entry;

    char taskPath[256];
    snprintf(taskPath, sizeof(taskPath), "/proc/%d/task", pid);

    dir = opendir(taskPath);
    if (dir == NULL) {
        return;
    }

    while ((entry = readdir(dir))) {
        if (entry->d_name[0] == '.') {
            continue; // Skip entries starting with '.'
        }

        pid_t tid = atoi(entry->d_name);
        if (tid != pid) {
            pauseProcess(tid); // Pause the process
            recurseAndPause(tid); // Recursively pause its children
        }
    }

    closedir(dir);
}

// Function to recursively resume processes
void recurseAndResume(pid_t pid) {
    DIR *dir;
    struct dirent *entry;

    char taskPath[256];
    snprintf(taskPath, sizeof(taskPath), "/proc/%d/task", pid);

    dir = opendir(taskPath);
    if (dir == NULL) {
        return;
    }

    while ((entry = readdir(dir))) {
        if (entry->d_name[0] == '.') {
            continue; // Skip entries starting with '.'
        }

        pid_t tid = atoi(entry->d_name);
        if (tid != pid) {
            resumeProcess(tid); // Resume the process
            recurseAndResume(tid); // Recursively resume its children
        }
    }

    closedir(dir);
}

static void insert_proc(pid_t pid, pid_t ppid, int flags) {
	int i;

	/* Check if parent already inserted ... */
	if (flags != PROC_ROOT) {
		for (i = 0; i < nprocs; i++) if (procs[i].pid == ppid) break;
		if (i == nprocs) 
			insert_proc(ppid, 0, PROC_ROOT);
		procs[i].children = realloc(procs[i].children, 
		  sizeof(pid_t) * (procs[i].nchildren + 1));
		procs[i].children[procs[i].nchildren++] = pid;
	}

	/* Check if pid already inserted ... */
	for (i = 0; i < nprocs; i++) 
		if (procs[i].pid == pid) 
			break;
	if (i == nprocs) { /* ... not found, add */
		procs = realloc(procs, (nprocs + 1) * sizeof(*procs));
		procs[nprocs].pid = pid;
		procs[nprocs].children = NULL;
		procs[nprocs].nchildren = 0;
		nprocs++;
	}
	procs[i].flags = flags;
	procs[i].ppid = ppid;
}

static void recurse(pid_t pid) {
	int i, j;

	for (i = 0; i < nprocs; i++) 
		if (procs[i].pid == pid) 
			break; 
	if (i == nprocs) 
		return; /* not found */

	for (j = 0; j < procs[i].nchildren; j++) 
		if (!(procs[i].flags & PROC_SPECIAL))
			recurse(procs[i].children[j]);

	if (!procs[i].flags & PROC_ALREADY_STOPPED) 
		kill_list[nkills++] = pid;
}

void idle_inject() {
	struct sched_param sched;
	if (kill_list != NULL) {
        return;
    }
	sched.sched_priority = 99;
	sched_setscheduler(0, SCHED_FIFO, &sched);
	pid_t self = getpid();
	DIR *procDir;
	struct dirent *procEntry;
	procDir = opendir("/proc");
	if (procDir == NULL) {
		perror("opendir() failed.  /proc not mounted or no permission?");
		return;
	}
	int i = 0;

	while ((procEntry = readdir(procDir))) {
		pid_t pid = strtol(procEntry->d_name, NULL, 10); // XXX will this even work?
		pid_t ppid;
		if (pid > 0 && pid != self) {
			int flags = 0;
			if (isProcessStopped(pid)) {
			    flags = PROC_ALREADY_STOPPED;
			}
			if (isProcessSpecial(pid)) {
				flags = PROC_SPECIAL;
			}
			ppid = getParentPid(pid);
			insert_proc(pid, ppid, flags); // Insert the process into your data structure
		}
	}
	closedir(procDir);
	// Allocate and fill the kill_list if needed
	kill_list = malloc(sizeof(pid_t) * nprocs);
	nkills = 0;
	for (i = (nkills - 1); i >= 0; i--) {
	// sending SIGSTOP had side-effect of sending SIGCHLD 
	// to parent process randomly, use ptrace instead
		ptrace(PTRACE_SEIZE, kill_list[i], 0, 0);
		ptrace(PTRACE_INTERRUPT, kill_list[i], 0, 0);
	}
}

static void idle_cancel(void) {
	int i;
	struct sched_param sched;

	if (kill_list == NULL) return;

	for (i = 0; i < nkills; i++) {
		ptrace(PTRACE_DETACH, kill_list[i], 0, 0);
	}

	sched.sched_priority = 0;
	sched_setscheduler(0, SCHED_OTHER, &sched);
	free(kill_list);
	for (i = 0; i < nprocs; i++) 
		if (procs[i].children) free(procs[i].children);
	free(procs);
	procs = NULL;
	kill_list = NULL;
	nprocs = 0;
	nkills = 0;
}

static int millicelsius(void) {
	int t = 0;
	FILE *s = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
	if (s != NULL) {
		if (fscanf(s, "%d", &t) == 0) t = 0;
		fclose(s);
	}
	return t;
}


void usage(char **argv) {
	fprintf(stderr,
		"Usage: %s [OPTIONS] ...\n"
		"embeddedTS Userspace Idle Injector\n"
		"\n"
		"  -l, --led      Specify LED brightness file to toggle, eg:\n"
		"                 --led /sys/class/leds/right-red-led/brightness\n"
		"  -t, --maxtemp  Set the max temperature in millicelcius before idle\n"
		"                 injector starts, or 115000 default.\n"
		"  -h, --help     This message\n"
		"  This utility polls the CPU temperature and pauses userspace\n"
		"  applications until the temperature is reduced.  If specified,\n"
		"  this will turn on the LED when it is injecting idle.\n"
		"  Processes starting with @ are never throttled\n"
		"\n",
		argv[0]
	);
}

void sig_handler(int sig) {
	die = 1;
}

int main(int argc, char **argv) {
	int i;
	int led_status = -1;
	FILE *led = NULL;
	char *opt_led = 0;
	int opt_temp = MAXTEMP;

	static struct option long_options[] = {
		{ "led", required_argument, 0, 'l' },
		{ "maxtemp", required_argument, 0, 'm' },
		{ "help", 0, 0, 'h' },
		{ 0, 0, 0, 0 }
	};

	while((i = getopt_long(argc, argv, "l:m:h", long_options, NULL)) != -1) {
		switch(i) {
		case 'l':
			opt_led = strdup(optarg);
			break;
		case 'm':
			opt_temp = atoi(optarg);
			break;
		case ':':
			fprintf(stderr, "%s: option `-%c' requires an argument\n",
				argv[0], optopt);
			return 1;
			break;
		default:
			fprintf(stderr, "%s: option `-%c' (%d) is invalid\n",
				argv[0], optopt, optind);
			return 1;
			break;
		case 'h':
			usage(argv);
			return 1;
		}
	}

	if(opt_led) {
		led = fopen(opt_led, "r+");
		if(led == NULL) {
			perror("Couldn't open LED, continuing without LED support");
			opt_led = 0;
		}
	}

	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);
	signal(SIGKILL, sig_handler);

	for (;;) {
		if(die) {
			idle_cancel();
			exit(0);
		}
		if (millicelsius() >= opt_temp){
			if(opt_led && led_status != 1) {
				fwrite("1\n", 2, 1, led);
				rewind(led);
				led_status = 1;
			}
			idle_inject();
		} else {
			idle_cancel();
			if(opt_led && led_status != 0) {
				fwrite("0\n", 2, 1, led);
				rewind(led);
				led_status = 0;
			}
		}
		usleep(100000); /* 1/10th of a second */
	}
}

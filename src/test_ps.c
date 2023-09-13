// c. 2023 Michael Peters / embeddedTS.com
// 
// Test program to learn how to acquire and manage system tasks using
//  the procps library's public facing functions.

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <libproc2/pids.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
// Compile with -lprocps or -lproc2 | this needs the local flavor of the 
//  procps-ng package

enum pids_item items[] = { PIDS_ID_PID, PIDS_ID_PID };
enum pids_item items2[] = { PIDS_ID_PID, PIDS_VM_RSS };

int check_fatal_proc_unmounted(void *data);
struct pids_info *info = NULL;
struct pids_counts numPids;
struct pids_stack *stack;

int mytest(void *data){
	int err = 0;
	char *pointer;
	int outval = 0;
	long maxpids = 0;
	char exe[30] = "";
	int fp = open("/proc/sys/kernel/pid_max", O_RDONLY);
	char buf[30];
	enum pids_fetch_type which = PIDS_FETCH_TASKS_ONLY;
	err = read(fp, buf, sizeof(buf));
	maxpids = strtol(buf, &pointer, 0);
	printf("Maximum pids is %ld.\n", maxpids);

	struct pids_stack *stack =NULL;
	stack = fatal_proc_unmounted(info, 1);
	outval = procps_pids_new(&info, items, maxpids);
//	outval = procps_pids_reap(&info, PIDS_FETCH_TASKS_ONLY);
	stack = procps_pids_get(info, which);
	exe = stack->head->item[PIDS_EXE];

	return 0;	
}

int main(void)
{
	int outval = 0;
	void *data = NULL;
	outval = check_fatal_proc_unmounted(data);
	printf("returned %d.\n", outval);
	mytest(NULL);
	return outval;
}

int check_fatal_proc_unmounted(void *data)
{
    struct pids_info *info = NULL;
    struct pids_stack *stack;
    printf("check_fatal_proc_unmounted\n");

    return ( (procps_pids_new(&info, items2, 2) == 0) &&
	    ( (stack = fatal_proc_unmounted(info, 1)) != NULL) &&
	    ( PIDS_VAL(0, s_int, stack, info) > 0) &&
	    ( PIDS_VAL(1, u_int, stack, info) > 0));
}
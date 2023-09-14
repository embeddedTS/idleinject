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

#define TRUE 1
#define FALSE 0

int main() {
    struct pids_info *info;
    struct pids_fetch *fetch;

    // Initialize the pids_info structure.
    enum pids_item items[] = {PIDS_EXE}; // Specify the information item you want.
    int numItems = sizeof(items) / sizeof(enum pids_item);

    if (procps_pids_new(&info, items, numItems) != 0) {
        fprintf(stderr, "Failed to initialize pids_info.\n");
        return 1;
    }

    // Fetch the list of executables.
    fetch = procps_pids_reap(info, PIDS_FETCH_TASKS_ONLY);

    if (!fetch) {
        fprintf(stderr, "Failed to fetch executables.\n");
        procps_pids_unref(&info);
        return 1;
    }

    // Iterate through the results and print the executables.
    int i;
    for (i = 0; i < fetch->counts->total; i++) {
        struct pids_result *result = &fetch->stacks[i]->head[0];
        if (result->item == PIDS_EXE && result->result.str) {
            printf("Executable: %s\n", result->result.str);
        }
    }

    // Clean up.
    procps_pids_unref(&info);

    return 0;
}
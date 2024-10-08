#ifndef PROCPS_PROC_READPROC_H
#define PROCPS_PROC_READPROC_H

// New Interface to Process Table -- PROCTAB Stream (a la Directory streams)
// Copyright 1996 Charles L. Blake.
// Copyright 1998 Michael K. Johnson
// Copyright 1998-2003 Albert Cahalan
// May be distributed under the terms of the
// GNU Library General Public License, a copy of which is provided
// in the file COPYING


#include "procps.h"
#include "pwcache.h"

#define SIGNAL_STRING
//#define QUICK_THREADS        /* copy (vs. read) some thread info from parent proc_t */

EXTERN_C_BEGIN

// ld	cutime, cstime, priority, nice, timeout, alarm, rss,
// c	state,
// d	ppid, pgrp, session, tty, tpgid,
// s	signal, blocked, sigignore, sigcatch,
// lu	flags, min_flt, cmin_flt, maj_flt, cmaj_flt, utime, stime,
// lu	rss_rlim, start_code, end_code, start_stack, kstk_esp, kstk_eip,
// lu	start_time, vsize, wchan,

// This is to help document a transition from pid to tgid/tid caused
// by the introduction of thread support. It is used in cases where
// neither tgid nor tid seemed correct. (in other words, FIXME)
#define XXXID tid

enum ns_type {
    IPCNS = 0,
    MNTNS,
    NETNS,
    PIDNS,
    USERNS,
    UTSNS,
    NUM_NS         // total namespaces (fencepost)
};
extern const char *get_ns_name(int id);
extern int get_ns_id(const char *name);

// Basic data structure which holds all information we can get about a process.
// (unless otherwise specified, fields are read from /proc/#/stat)
//
// Most of it comes from task_struct in linux/sched.h
//
typedef struct proc_t {
// 1st 16 bytes
    int
        tid,		// (special)       task id, the POSIX thread ID (see also: tgid)
    	ppid;		// stat,status     pid of parent process
    unsigned long       // next 2 fields are NOT filled in by readproc
        maj_delta,      // stat (special) major page faults since last update
        min_delta;      // stat (special) minor page faults since last update
    unsigned
        pcpu;           // stat (special)  %CPU usage (is not filled in by readproc!!!)
    char
    	state,		// stat,status     single-char code for process state (S=sleeping)
#ifdef QUICK_THREADS
        pad_1,          // n/a             padding (psst, also used if multi-threaded)
#else
        pad_1,          // n/a             padding
#endif
    	pad_2,		// n/a             padding
    	pad_3;		// n/a             padding
// 2nd 16 bytes
    unsigned long long
	utime,		// stat            user-mode CPU time accumulated by process
	stime,		// stat            kernel-mode CPU time accumulated by process
// and so on...
	cutime,		// stat            cumulative utime of process and reaped children
	cstime,		// stat            cumulative stime of process and reaped children
	start_time;	// stat            start time of process -- seconds since system boot
#ifdef SIGNAL_STRING
    char
	// Linux 2.1.7x and up have 64 signals. Allow 64, plus '\0' and padding.
	signal[18],	// status          mask of pending signals, per-task for readtask() but per-proc for readproc()
	blocked[18],	// status          mask of blocked signals
	sigignore[18],	// status          mask of ignored signals
	sigcatch[18],	// status          mask of caught  signals
	_sigpnd[18];	// status          mask of PER TASK pending signals
#else
    long long
	// Linux 2.1.7x and up have 64 signals.
	signal,		// status          mask of pending signals, per-task for readtask() but per-proc for readproc()
	blocked,	// status          mask of blocked signals
	sigignore,	// status          mask of ignored signals
	sigcatch,	// status          mask of caught  signals
	_sigpnd;	// status          mask of PER TASK pending signals
#endif
    unsigned KLONG
	start_code,	// stat            address of beginning of code segment
	end_code,	// stat            address of end of code segment
	start_stack,	// stat            address of the bottom of stack for the process
	kstk_esp,	// stat            kernel stack pointer
	kstk_eip,	// stat            kernel instruction pointer
	wchan;		// stat (special)  address of kernel wait channel proc is sleeping in
    long
	priority,	// stat            kernel scheduling priority
	nice,		// stat            standard unix nice level of process
	rss,		// stat            identical to 'resident'
	alarm,		// stat            ?
    // the next 7 members come from /proc/#/statm
	size,		// statm           total virtual memory (as # pages)
	resident,	// statm           resident non-swapped memory (as # pages)
	share,		// statm           shared (mmap'd) memory (as # pages)
	trs,		// statm           text (exe) resident set (as # pages)
	lrs,		// statm           library resident set (always 0 w/ 2.6)
	drs,		// statm           data+stack resident set (as # pages)
	dt;		// statm           dirty pages (always 0 w/ 2.6)
    unsigned long
	vm_size,        // status          equals 'size' (as kb)
	vm_lock,        // status          locked pages (as kb)
	vm_rss,         // status          equals 'rss' and/or 'resident' (as kb)
	vm_rss_anon,    // status          the 'anonymous' portion of vm_rss (as kb)
	vm_rss_file,    // status          the 'file-backed' portion of vm_rss (as kb)
	vm_rss_shared,  // status          the 'shared' portion of vm_rss (as kb)
	vm_data,        // status          data only size (as kb)
	vm_stack,       // status          stack only size (as kb)
	vm_swap,        // status          based on linux-2.6.34 "swap ents" (as kb)
	vm_exe,         // status          equals 'trs' (as kb)
	vm_lib,         // status          total, not just used, library pages (as kb)
	rtprio,		// stat            real-time priority
	sched,		// stat            scheduling class
	vsize,		// stat            number of pages of virtual memory ...
	rss_rlim,	// stat            resident set size limit?
	flags,		// stat            kernel flags for the process
	min_flt,	// stat            number of minor page faults since process start
	maj_flt,	// stat            number of major page faults since process start
	cmin_flt,	// stat            cumulative min_flt of process and child processes
	cmaj_flt;	// stat            cumulative maj_flt of process and child processes
    char
        **environ,      // (special)       environment string vector (/proc/#/environ)
        **cmdline,      // (special)       command line string vector (/proc/#/cmdline)
        **cgroup,       // (special)       cgroup string vector (/proc/#/cgroup)
         *cgname,       // (special)       name portion of above (if possible)
         *supgid,       // status          supplementary gids as comma delimited str
         *supgrp;       // supp grp names as comma delimited str, derived from supgid
    char
	// Be compatible: Digital allows 16 and NT allows 14 ???
    	euser[P_G_SZ],	// stat(),status   effective user name
    	ruser[P_G_SZ],	// status          real user name
    	suser[P_G_SZ],	// status          saved user name
    	fuser[P_G_SZ],	// status          filesystem user name
    	rgroup[P_G_SZ],	// status          real group name
    	egroup[P_G_SZ],	// status          effective group name
    	sgroup[P_G_SZ],	// status          saved group name
    	fgroup[P_G_SZ],	// status          filesystem group name
        cmd[64];	// stat,status     basename of executable file in call to exec(2)
    struct proc_t
	*ring,		// n/a             thread group ring
	*next;		// n/a             various library uses
    int
	pgrp,		// stat            process group id
	session,	// stat            session id
	nlwp,		// stat,status     number of threads, or 0 if no clue
	tgid,		// (special)       thread group ID, the POSIX PID (see also: tid)
	tty,		// stat            full device number of controlling terminal
	/* FIXME: int uids & gids should be uid_t or gid_t from pwd.h */
        euid, egid,     // stat(),status   effective
        ruid, rgid,     // status          real
        suid, sgid,     // status          saved
        fuid, fgid,     // status          fs (used for file access only)
	tpgid,		// stat            terminal process group id
	exit_signal,	// stat            might not be SIGCHLD
	processor;      // stat            current (or most recent?) CPU
    int
        oom_score,      // oom_score       (badness for OOM killer)
        oom_adj;        // oom_adj         (adjustment to OOM score)
    long
        ns[NUM_NS];     // (ns subdir)     inode number of namespaces
    char
        *sd_mach,       // n/a             systemd vm/container name
        *sd_ouid,       // n/a             systemd session owner uid
        *sd_seat,       // n/a             systemd login session seat
        *sd_sess,       // n/a             systemd login session id
        *sd_slice,      // n/a             systemd slice unit
        *sd_unit,       // n/a             systemd system unit id
        *sd_uunit;      // n/a             systemd user unit id
    const char
        *lxcname;       // n/a             lxc container name
} proc_t;

// PROCTAB: data structure holding the persistent information readproc needs
// from openproc().  The setup is intentionally similar to the dirent interface
// and other system table interfaces (utmp+wtmp come to mind).

#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

#define PROCPATHLEN NAME_MAX+PATH_MAX  // must hold /proc/2000222000/task/2000222000/cmdline

typedef struct PROCTAB {
    DIR*	procfs;
//    char deBug0[64];
    DIR*	taskdir;  // for threads
//    char deBug1[64];
    pid_t	taskdir_user;  // for threads
    int         did_fake; // used when taskdir is missing
    int(*finder)(struct PROCTAB *__restrict const, proc_t *__restrict const);
    proc_t*(*reader)(struct PROCTAB *__restrict const, proc_t *__restrict const);
    int(*taskfinder)(struct PROCTAB *__restrict const, const proc_t *__restrict const, proc_t *__restrict const, char *__restrict const);
    proc_t*(*taskreader)(struct PROCTAB *__restrict const, const proc_t *__restrict const, proc_t *__restrict const, char *__restrict const);
    pid_t*	pids;	// pids of the procs
    uid_t*	uids;	// uids of procs
    int		nuid;	// cannot really sentinel-terminate unsigned short[]
    int         i;  // generic
    unsigned	flags;
    unsigned    u;  // generic
    void *      vp; // generic
    char        path[PROCPATHLEN];  // must hold /proc/2000222000/task/2000222000/cmdline
    unsigned pathlen;        // length of string in the above (w/o '\0')
} PROCTAB;

// Initialize a PROCTAB structure holding needed call-to-call persistent data
extern PROCTAB* openproc(int flags, ... /* pid_t*|uid_t*|dev_t*|char* [, int n] */ );

typedef struct proc_data_t {  // valued by: (else zero)
    proc_t **tab;             //     readproctab2, readproctab3
    proc_t **proc;            //     readproctab2
    proc_t **task;            //  *  readproctab2
    int n;                    //     readproctab2, readproctab3
    int nproc;                //     readproctab2
    int ntask;                //  *  readproctab2
} proc_data_t;                //  *  when PROC_LOOSE_TASKS set

extern proc_data_t *readproctab2(int(*want_proc)(proc_t *buf), int(*want_task)(proc_t *buf), PROCTAB *__restrict const PT);
extern proc_data_t *readproctab3(int(*want_task)(proc_t *buf), PROCTAB *__restrict const PT);

// Convenient wrapper around openproc and readproc to slurp in the whole process
// table subset satisfying the constraints of flags and the optional PID list.
// Free allocated memory with exit().  Access via tab[N]->member.  The pointer
// list is NULL terminated.
extern proc_t** readproctab(int flags, ... /* same as openproc */ );

// Clean-up open files, etc from the openproc()
extern void closeproc(PROCTAB* PT);

// Retrieve the next process or task matching the criteria set by the openproc().
//
// Note: When NULL is used as the readproc 'p', readtask 't' or readeither 'x'
//       parameter, the library will allocate the necessary proc_t storage.
//
//       Alternatively, you may provide your own reuseable buffer address
//       in which case that buffer *MUST* be initialized to zero one time
//       only before first use.  Thereafter, the library will manage such
//       a passed proc_t, freeing any additional acquired memory associated
//       with the previous process or thread.
extern proc_t* readproc(PROCTAB *__restrict const PT, proc_t *__restrict p);
extern proc_t* readtask(PROCTAB *__restrict const PT, const proc_t *__restrict const p, proc_t *__restrict t);
extern proc_t* readeither(PROCTAB *__restrict const PT, proc_t *__restrict x);

// warning: interface may change
extern int read_cmdline(char *__restrict const dst, unsigned sz, unsigned pid);

extern void look_up_our_self(proc_t *p);

// Deallocate space allocated by readproc
extern void freeproc(proc_t* p);

// Fill out a proc_t for a single task
extern proc_t * get_proc_stats(pid_t pid, proc_t *p);

// openproc/readproctab:
//
// Return PROCTAB* / *proc_t[] or NULL on error ((probably) "/proc" cannot be
// opened.)  By default readproc will consider all processes as valid to parse
// and return, but not actually fill in the cmdline, environ, and /proc/#/statm
// derived memory fields.
//
// `flags' (a bitwise-or of PROC_* below) modifies the default behavior.  The
// "fill" options will cause more of the proc_t to be filled in.  The "filter"
// options all use the second argument as the pointer to a list of objects:
// process status', process id's, user id's.  The third
// argument is the length of the list (currently only used for lists of user
// id's since uid_t supports no convenient termination sentinel.)

#define PROC_FILLMEM         0x0001 // read statm
#define PROC_FILLCOM         0x0002 // alloc and fill in `cmdline'
#define PROC_FILLENV         0x0004 // alloc and fill in `environ'
#define PROC_FILLUSR         0x0008 // resolve user id number -> user name
#define PROC_FILLGRP         0x0010 // resolve group id number -> group name
#define PROC_FILLSTATUS      0x0020 // read status
#define PROC_FILLSTAT        0x0040 // read stat
#define PROC_FILLARG         0x0100 // alloc and fill in `cmdline'
#define PROC_FILLCGROUP      0x0200 // alloc and fill in `cgroup`
#define PROC_FILLSUPGRP      0x0400 // resolve supplementary group id -> group name
#define PROC_FILLOOM         0x0800 // fill in proc_t oom_score and oom_adj
#define PROC_FILLNS          0x8000 // fill in proc_t namespace information
#define PROC_FILLSYSTEMD    0x80000 // fill in proc_t systemd information
#define PROC_FILL_LXC      0x800000 // fill in proc_t lxcname, if possible

#define PROC_LOOSE_TASKS     0x2000 // treat threads as if they were processes

// consider only processes with one of the passed:
#define PROC_PID             0x1000  // process id numbers ( 0   terminated)
#define PROC_UID             0x4000  // user id numbers    ( length needed )

#define PROC_EDITCGRPCVT    0x10000 // edit `cgroup' as single vector
#define PROC_EDITCMDLCVT    0x20000 // edit `cmdline' as single vector
#define PROC_EDITENVRCVT    0x40000 // edit `environ' as single vector

// it helps to give app code a few spare bits
#define PROC_SPARE_1     0x01000000
#define PROC_SPARE_2     0x02000000
#define PROC_SPARE_3     0x04000000
#define PROC_SPARE_4     0x08000000

EXTERN_C_END
#endif

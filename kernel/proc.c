#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"


struct cpu cpus[NCPU];

struct proc proc[NPROC];

struct proc *initproc;

#ifdef ADAPTIVE_RR

struct proc* rqueue[NPROC];     // Array of ready processes pointers
int rqueue_len = 0;

void sort_rqueue(void);
int rr_adaptive_val(void);
int r_rqueue(struct proc*);
int a_rqueue(struct proc*);
void p_rqueue(void);
//testing
static struct proc test[5];
void init_test(void);
#endif

int nextpid = 1;
struct spinlock pid_lock;

extern void forkret(void);
static void freeproc(struct proc *p);

extern char trampoline[]; // trampoline.S

// helps ensure that wakeups of wait()ing
// parents are not lost. helps obey the
// memory model when using p->parent.
// must be acquired before any p->lock.
struct spinlock wait_lock;

// Allocate a page for each process's kernel stack.
// Map it high in memory, followed by an invalid
// guard page.
void
proc_mapstacks(pagetable_t kpgtbl) {
  struct proc *p;
  
  for(p = proc; p < &proc[NPROC]; p++) {
    char *pa = kalloc();
    if(pa == 0)
      panic("kalloc");
    uint64 va = KSTACK((int) (p - proc));
    kvmmap(kpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
  }
}

// initialize the proc table at boot time.
void
procinit(void)
{
  struct proc *p;
  
  initlock(&pid_lock, "nextpid");
  initlock(&wait_lock, "wait_lock");
  for(p = proc; p < &proc[NPROC]; p++) {
      initlock(&p->lock, "proc");
      p->kstack = KSTACK((int) (p - proc));
  }
#ifdef ADAPTIVE_RR
  for(int i=0; i<NPROC; i++){
    rqueue[i] = 0;
  }
  //init_test();
#endif
}

// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
int
cpuid()
{
  int id = r_tp();
  return id;
}

// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu*
mycpu(void) {
  int id = cpuid();
  struct cpu *c = &cpus[id];
  return c;
}

// Return the current struct proc *, or zero if none.
struct proc*
myproc(void) {
  push_off();
  struct cpu *c = mycpu();
  struct proc *p = c->proc;
  pop_off();
  return p;
}

int
allocpid() {
  int pid;
  
  acquire(&pid_lock);
  pid = nextpid;
  nextpid = nextpid + 1;
  release(&pid_lock);

  return pid;
}

// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, or a memory allocation fails, return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state == UNUSED) {
      goto found;
    } else {
      release(&p->lock);
    }
  }
  return 0;

found:
  p->pid = allocpid();
  p->state = USED;
  p->rticks = 0;
  p->lticks = ticks;
  p->tticks = 0;
  p->wticks = 0;
  p->burst = 0;

  // Allocate a trapframe page.
  if((p->trapframe = (struct trapframe *)kalloc()) == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // An empty user page table.
  p->pagetable = proc_pagetable(p);
  if(p->pagetable == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&p->context, 0, sizeof(p->context));
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;

  printf("[pid-%d] allocated, init ticks = %d\n", p->pid, p->lticks);
  return p;
}

// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void
freeproc(struct proc *p)
{
  if(p->trapframe)
    kfree((void*)p->trapframe);
  p->trapframe = 0;
  if(p->pagetable)
    proc_freepagetable(p->pagetable, p->sz);
  p->pagetable = 0;
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->state = UNUSED;
  p->rticks = 0;
  p->lticks = 0;
  p->tticks = 0;
  p->wticks = 0;
}

// Create a user page table for a given process,
// with no user memory, but with trampoline pages.
pagetable_t
proc_pagetable(struct proc *p)
{
  pagetable_t pagetable;

  // An empty page table.
  pagetable = uvmcreate();
  if(pagetable == 0)
    return 0;

  // map the trampoline code (for system call return)
  // at the highest user virtual address.
  // only the supervisor uses it, on the way
  // to/from user space, so not PTE_U.
  if(mappages(pagetable, TRAMPOLINE, PGSIZE,
              (uint64)trampoline, PTE_R | PTE_X) < 0){
    uvmfree(pagetable, 0);
    return 0;
  }

  // map the trapframe just below TRAMPOLINE, for trampoline.S.
  if(mappages(pagetable, TRAPFRAME, PGSIZE,
              (uint64)(p->trapframe), PTE_R | PTE_W) < 0){
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmfree(pagetable, 0);
    return 0;
  }

  return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void
proc_freepagetable(pagetable_t pagetable, uint64 sz)
{
  uvmunmap(pagetable, TRAMPOLINE, 1, 0);
  uvmunmap(pagetable, TRAPFRAME, 1, 0);
  uvmfree(pagetable, sz);
}

// a user program that calls exec("/init")
// od -t xC initcode
uchar initcode[] = {
  0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02,
  0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x35, 0x02,
  0x93, 0x08, 0x70, 0x00, 0x73, 0x00, 0x00, 0x00,
  0x93, 0x08, 0x20, 0x00, 0x73, 0x00, 0x00, 0x00,
  0xef, 0xf0, 0x9f, 0xff, 0x2f, 0x69, 0x6e, 0x69,
  0x74, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

// Set up first user process.
void
userinit(void)
{
  struct proc *p;

  p = allocproc();
  initproc = p;
  
  // allocate one user page and copy init's instructions
  // and data into it.
  uvminit(p->pagetable, initcode, sizeof(initcode));
  p->sz = PGSIZE;

  // prepare for the very first "return" from kernel to user.
  p->trapframe->epc = 0;      // user program counter
  p->trapframe->sp = PGSIZE;  // user stack pointer

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;

#ifdef ADAPTIVE_RR
  a_rqueue(p);
#endif

  release(&p->lock);
}

// Grow or shrink user memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *p = myproc();

  sz = p->sz;
  if(n > 0){
    if((sz = uvmalloc(p->pagetable, sz, sz + n)) == 0) {
      return -1;
    }
  } else if(n < 0){
    sz = uvmdealloc(p->pagetable, sz, sz + n);
  }
  p->sz = sz;
  return 0;
}

// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *p = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy user memory from parent to child.
  if(uvmcopy(p->pagetable, np->pagetable, p->sz) < 0){
    freeproc(np);
    release(&np->lock);
    return -1;
  }
  np->sz = p->sz;

  // copy saved user registers.
  *(np->trapframe) = *(p->trapframe);

  // Cause fork to return 0 in the child.
  np->trapframe->a0 = 0;

  // increment reference counts on open file descriptors.
  for(i = 0; i < NOFILE; i++)
    if(p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  np->cwd = idup(p->cwd);

  safestrcpy(np->name, p->name, sizeof(p->name));

  pid = np->pid;

  release(&np->lock);

  acquire(&wait_lock);
  np->parent = p;
  release(&wait_lock);

  acquire(&np->lock);
  np->state = RUNNABLE;
#ifdef ADAPTIVE_RR
  a_rqueue(p);
#endif
  release(&np->lock);

  return pid;
}

// Pass p's abandoned children to init.
// Caller must hold wait_lock.
void
reparent(struct proc *p)
{
  struct proc *pp;

  for(pp = proc; pp < &proc[NPROC]; pp++){
    if(pp->parent == p){
      pp->parent = initproc;
      wakeup(initproc);
    }
  }
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait().
void
exit(int status)
{
  struct proc *p = myproc();

  if(p == initproc)
    panic("init exiting");

  // Close all open files.
  for(int fd = 0; fd < NOFILE; fd++){
    if(p->ofile[fd]){
      struct file *f = p->ofile[fd];
      fileclose(f);
      p->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(p->cwd);
  end_op();
  p->cwd = 0;

  acquire(&wait_lock);

  // Give any children to init.
  reparent(p);

  // Parent might be sleeping in wait().
  wakeup(p->parent);
  
  acquire(&p->lock);

  p->xstate = status;
  p->state = ZOMBIE;
#ifdef ADAPTIVE_RR
  r_rqueue(p);
#endif

  release(&wait_lock);

  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(uint64 addr)
{
  struct proc *np;
  int havekids, pid;
  struct proc *p = myproc();

  acquire(&wait_lock);

  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(np = proc; np < &proc[NPROC]; np++){
      if(np->parent == p){
        // make sure the child isn't still in exit() or swtch().
        acquire(&np->lock);

        havekids = 1;
        if(np->state == ZOMBIE){
          // Found one.
          pid = np->pid;
          if(addr != 0 && copyout(p->pagetable, addr, (char *)&np->xstate,
                                  sizeof(np->xstate)) < 0) {
            release(&np->lock);
            release(&wait_lock);
            return -1;
          }
          freeproc(np);
          release(&np->lock);
          release(&wait_lock);
          return pid;
        }
        release(&np->lock);
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || p->killed){
      release(&wait_lock);
      return -1;
    }
    
    // Wait for a child to exit.
    sleep(p, &wait_lock);  //DOC: wait-sleep
  }
}


void
updatewait(void)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state == RUNNABLE){
      //calculate the waiting time and total ticks
      p->tticks += ticks - p->lticks;
      p->wticks = p->tticks - p->rticks;
      p->lticks = ticks;
      //printf("update [pid-%d] with ticks (%d), tticks=%d\n", p->pid, (ticks - p->lticks), p->tticks);
    }
    release(&p->lock);
  }

}


// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  int ptick;
  
  c->proc = 0;
 // int count = 0;
  for(;;){
    // Avoid deadlock by ensuring that devices can interrupt.
    intr_on();
    
    //update wait ticks for all ready processes
    updatewait();

#ifdef ADAPTIVE_RR
    int rr_timer = rr_adaptive_val();
    if(rr_timer != getinter())
    {
      inter(rr_timer);
    }
#endif


    for(p = proc; p < &proc[NPROC]; p++) 
    {
      acquire(&p->lock);
      if(p->state == RUNNABLE) {
        // Switch to chosen process.  It is the process's job
        // to release its lock and then reacquire it
        // before jumping back to us.
        p->state = RUNNING;
#ifdef ADAPTIVE_RR
        r_rqueue(p);
#endif        
        c->proc = p;
        // capture the current ticks value
        ptick = ticks;

        swtch(&c->context, &p->context);

        // set the consumed ticks by the current process
        p->rticks += (ticks - ptick);

        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;
      }
      release(&p->lock);
    }
  }
}

// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&p->lock))
    panic("sched p->lock");
  if(mycpu()->noff != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(intr_get())
    panic("sched interruptible");

  intena = mycpu()->intena;
  swtch(&p->context, &mycpu()->context);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  struct proc *p = myproc();
  acquire(&p->lock);
  p->state = RUNNABLE;
#ifdef ADAPTIVE_RR
  a_rqueue(p);
#endif
  sched();
  release(&p->lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void
forkret(void)
{
  static int first = 1;

  // Still holding p->lock from scheduler.
  release(&myproc()->lock);

  if (first) {
    // File system initialization must be run in the context of a
    // regular process (e.g., because it calls sleep), and thus cannot
    // be run from main().
    first = 0;
    fsinit(ROOTDEV);
  }

  usertrapret();
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  // Must acquire p->lock in order to
  // change p->state and then call sched.
  // Once we hold p->lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup locks p->lock),
  // so it's okay to release lk.

  acquire(&p->lock);  //DOC: sleeplock1
  release(lk);

  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;
#ifdef ADAPTIVE_RR
  r_rqueue(p);
#endif

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  release(&p->lock);
  acquire(lk);
}

// Wake up all processes sleeping on chan.
// Must be called without any p->lock.
void
wakeup(void *chan)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    if(p != myproc()){
      acquire(&p->lock);
      if(p->state == SLEEPING && p->chan == chan) {
        p->state = RUNNABLE;
#ifdef ADAPTIVE_RR
        a_rqueue(p);
#endif
      }
      release(&p->lock);
    }
  }
}

// Kill the process with the given pid.
// The victim won't exit until it tries to return
// to user space (see usertrap() in trap.c).
int
kill(int pid)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->pid == pid){
      p->killed = 1;
      if(p->state == SLEEPING){
        // Wake process from sleep().
        p->state = RUNNABLE;
#ifdef ADAPTIVE_RR
        a_rqueue(p);
#endif
      }
      release(&p->lock);
      return 0;
    }
    release(&p->lock);
  }
  return -1;
}

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int
either_copyout(int user_dst, uint64 dst, void *src, uint64 len)
{
  struct proc *p = myproc();
  if(user_dst){
    return copyout(p->pagetable, dst, src, len);
  } else {
    memmove((char *)dst, src, len);
    return 0;
  }
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
int
either_copyin(void *dst, int user_src, uint64 src, uint64 len)
{
  struct proc *p = myproc();
  if(user_src){
    return copyin(p->pagetable, dst, src, len);
  } else {
    memmove(dst, (char*)src, len);
    return 0;
  }
}

static char *states[] = {
  [UNUSED]    "unused",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
static char unkonw[] = "???";

char* 
printable_state(enum procstate pstate){
  char *state;
  if(pstate >= 0 && pstate < NELEM(states) && states[pstate])
      state = states[pstate];
    else
      state = unkonw;
  
  return state;
}
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  struct proc *p;

  printf("\n");
  printf("[pid]\tstate\t[name]\t[burst]\t[rticks]\t[wticks]\t[tticks]\n");
  for(p = proc; p < &proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    
    printf("%d\t%s\t%s\t%d\t%d\t%d\t%d", p->pid, printable_state(p->state), p->name, p->burst,p->rticks,p->wticks,p->tticks);
    printf("\n");
  }
  printf("\n");

#ifdef ADAPTIVE_RR
  p_rqueue();
#endif
}

int
pcb(void){
  procdump();
  return 1;
}

#ifdef ADAPTIVE_RR

void p_rqueue(void){
  printf("Q[%d]: ",rqueue_len);
  for(int i=0; i<rqueue_len; i++){
    printf("[P-%d], ",rqueue[i]->pid);
  }
  printf("\n");
}


void init_test(void)
{
  test[0].burst = 15;
  test[0].state = RUNNABLE;
  test[1].burst = 5;
  test[1].state = RUNNABLE;
  test[2].burst = 20;
  test[2].state = RUNNABLE;
  test[3].burst = 1;
  test[4].burst = 9;
  
  a_rqueue(&test[0]);
  a_rqueue(&test[1]);
  a_rqueue(&test[2]);
  
/*  printf("init rqueue: ");
  int i;
  for(i=0; i<NPROC; i++){
    printf("rqueue[%d]=[%d], ",i,rqueue[i]->burst);
  }
  printf("\n");*/
}

int
r_rqueue(struct proc* p)
{
  if (rqueue_len > 0)
  {
    for (int i = 0; i < rqueue_len; i++)
    {
      if (rqueue[i] == p)
      {
        // remove the process by replacing it with the last one and decrement the queue len
        rqueue[i] = rqueue[rqueue_len-1];
        rqueue_len--;
        printf("[pid-%d] Removed ... ", p->pid);
        p_rqueue();
        return 0;
      }
    }
    //printf("remove failed, couldn't find [pid-%d]\n", p->pid);
  }
  /*else{
    printf("remove failed, Empty Queue\n");
  }*/
  return -1;
}

int
a_rqueue(struct proc* p){
  if (p->state == RUNNABLE && p->burst > 0)
  {
    if(rqueue_len < NPROC)
    {
      rqueue[rqueue_len] = p;
      rqueue_len++;
      printf("[pid=%d] Added ... ",p->pid);
      p_rqueue();
      return 0;
    }
  }
  /*else{
    printf("Add failed, [pid-%d] state = %s\n", p->pid, printable_state(p->state));
  }*/
  
  return -1;
}
/*
  The function sorts the ready queue based on the tasks burst time
*/
void sort_rqueue()
{
  int i, j;
  struct proc *element;
  if (rqueue_len > 0)
  {
   /* printf("array before sorting: ");
    for (i = 0; i < rqueue_len; i++)
    {
      printf("rqueue[%d]=[%d], ", i, rqueue[i]->burst);
    }
    printf("\n");
*/
    for (i = 1; i < rqueue_len; i++)
    {
      element = rqueue[i];

      j = i - 1;
      while (j >= 0 && rqueue[j]->burst > element->burst)
      {
        rqueue[j + 1] = rqueue[j];
        j = j - 1;
      }
      rqueue[j + 1] = element;
    }

    printf("array after sorting: ");
    for (i = 0; i < rqueue_len; i++)
    {
      printf("rqueue[%d]=[%d], ", i, rqueue[i]->burst);
    }
    printf("\n");
    
  }
}

/* Get the scheduler value */
int rr_adaptive_val(void)
{
  int rr_adap_val = 0;
  if (rqueue_len > 0)
  {
    /* To get RR adaptive value the ready queue MUST be sorted first */
    sort_rqueue();

    /* If number of tasks even, the Adaptive RR is the Avg */
    if (rqueue_len % 2 == 0)
    {
      for (int i = 0; i < rqueue_len; i++)
      {
        rr_adap_val += rqueue[i]->burst;
      }
      rr_adap_val = (int)rr_adap_val / rqueue_len;
      printf("---> RR Adaptive (AVG) = %d\n", rr_adap_val);
    }
    /* when tasks are odd no, the Adaptive RR is the median */
    else
    {
      rr_adap_val = rqueue[rqueue_len / 2]->burst;
      printf("---> RR Adaptive (MID) = %d\n", rr_adap_val);
    }
  }
  else
  {
    /* Default RR value */
    rr_adap_val = 1000;
  }
  return rr_adap_val;
}

#endif
/* Erich WOo & Boxian Wang
 * 19 November 2020
 * header file for pilocvario.c
 */

#ifndef __PILOCVARIO_H
#define __PILOCVARIO_H

#include <ykernel.h>
#include "linked_list.h"
#include "scheduling.h"

// circular buffer
typedef struct buffer {
  int size;
  int filled;
  int head; // first char index
  int tail; // one after the last char index (everything mod size)
  char *buffered;
} buffer_t;

typedef struct pipe {
  buffer_t *buffer;
  ll_t *readblocked;
  ll_t *writeblocked;
  int unfulfilled; // unfulfilled promises, i.e. things that woke up but in the ready queue
} pipe_t;

typedef struct lock {
  node_t *owner;
  ll_t *blocked;
  int unfulfilled;
  int cvar;
} lock_t;

typedef struct cvar {
  ll_t *blocked;
} cvar_t;

typedef struct pilocvar {
  int count; // next available id
  ll_t *pipe;
  ll_t *lock;
  ll_t *cvar;
} pilocvar_t;

typedef struct ttyio {
  buffer_t *buffer;
  ll_t *blocked;
  int transmitting;
} ttyio_t;

typedef struct io_control {
  ttyio_t *in[NUM_TERMINALS];
  ttyio_t *out[NUM_TERMINALS];
  char landing_buffer[TERMINAL_MAX_LINE]; // this is the landing buffer for ttyreceive/transmit
} io_control_t;

// THE storage bookkeeper for all pipes, locks, and cvars
extern pilocvar_t *pilocvar;

// THE proc table
extern proc_table_t *procs;

// The storage bookkeeper for all terminals
extern io_control_t *io;

/////////////////////// BUFFER

/* Initializes and returns a new buffer pointer
 * Must be free'd later by caller
 *
 * @param size the specified size of the new buffer
 * @return the initialized buffer pointer
 */
buffer_t *new_buffer(int size);

/* Writes len bytes of the src string to the buffer
 * If the avalailable space remaining in the buffer is less than len, 
 * only the available amount is written.
 *
 * @param buffer the buffer pointer to write to
 * @param src the source sring to write into buffer
 * @param len the number of bytes to write
 * @return the number of bytes ACTUALLy written
 */
int write_buffer(buffer_t *buffer, char *src, int len);

/* Writes len bytes of the src string to the buffer
 * If the avalailable space remaining in the buffer is less than len, 
 * only the available amount is written.
 *
 * @param buffer the buffer pointer to read from
 * @param dst the char* destination to store the read bytes into
 * @param len the number of bytes to read
 * @return the number of bytes ACTUALLY read 
 */
int read_buffer(buffer_t *buffer, char *dst, int len);

/* Resets the buffer as if new
 *
 * @param buffer the buffer to reset
 */
void reset_buffer(buffer_t *buffer);

/* Destroys/frees the buffer and its contents
 *
 * @param buffer the buffer to destroy
 */
void destroy_buffer(buffer_t *buffer);

//////////////// TTYIO

/* Initializes and returns a new terminal pointer
 * with a max buffer size = TERMINAL_MAX_LINE
 * Must be free'd later by caller
 *
 * @return the initialized ttyio_t pointer
 */
ttyio_t *new_ttyio(void);

/* Initializes and returns a new IO control pointer
 * with NUM_TERMINALS of in/out terminals
 * Must be free'd later by caller
 *
 * @return the initialized io_control_t pointer
 */
io_control_t *io_control_init(void);

/* Per the Yalnix Manual:
 * - Write the contents of the char* src to the terminal tty id.
 * The length written in bytes is given by len. The calling process is
 * blocked until all characters from the buffer have been written on the terminal. 
 *
 * Leverages hardware function TtyTransmit to perform actual terminal writing. 
 * write_tty() is a wrapper called by the syscall KernelTtyWrite()
 *
 * @param tty_id the terminal id
 * @param src the source string to write to the terminal
 * @param len the # of bytes to write
 * @return the # of bytes written on success
 */
int write_tty(int tty_id, char *src, int len);

/* Alerts the specified terminal (by tty_id) that the last TtyTransmit
 * has finished: resets the terminal's out buffer and unblocks whoever
 * called that TtyTransmit
 *
 * write_alert() is called by trapfunct TrapTtyTransmit
 * 
 * @param tty_id the id of the terminal to alert
 */
void write_alert(int tty_id);

/* Per the Yalnix Manual:
 * - Read the next line of input from terminal tty id, copying it into 
 * the destination char* dst. If there are sufficient unread bytes already waiting, 
 * the call will return right away, with those. Otherwise, the calling process is blocked
 * until a line of input is available to be returned. 
 *
 * for blen = # of unread bytes already waiting, the # of bytes actually read is:
 *         len, if blen >= len
 *         blen if blen < len
 * read_tty() is a wrapper called by the syscall KernelTtyRead()
 * Assumes len > 0
 *
 * @param tty_id the terminal id
 * @param dst the destination to copy read bytes to
 * @param len the len of bytes desired to read
 * @return the actual # of bytes copied into dst on success
 */
int read_tty(int tty_id, char *dst, int len);

/* Receives the new line of input using the TtyReceive hardware function,
 * and stores it into the input buffer of the specified terminal for
 * some process to eventually TtyRead()
 *
 * receive() is called by trapfunct TrapTtyReceive
 *
 * @param tty_id the specified id of the terminal that received new input
 */
void receive(int tty_id);

/////////////// PIPE

/* Returns a new pipe node pointer containing initialized pipe data 
 * with specified id. This new pipe node is added to the pipe list 
 * on the global pilocvar bookkeeper
 *
 * @param id the desired pipe id
 * @return the initialized pipe node pointer
 */
node_t *new_pipe(int id);

/* Writes len bytes starting at src to the specified pipe,
 * returning as soon as all len bytes have been written
 *
 * @param pipe_n the pipe node to write to
 * @param src the source string to start writing from
 * @param len the # of bytes to write
 * @return the total # of bytes written to the pipe
 */
int write_pipe(node_t *pipe_n, char *src, int len);

/* Reads len consecutifve bytes from the specified pipe
 * into the destination dst, following the standard semantics:
 * – If the pipe is empty, then block the caller.
 * – If the pipe has plen ≤ len unread bytes, give all of them to the caller and return.
 * – If the pipe has plen > len unread bytes, give the first len bytes to caller and return. 
 *                                            Retain the unread plen − len bytes in the pipe.
 *
 * @param pipe_n the pipe node to read from
 * @param dst the destination buffer to store the read bytes
 * @param len the len of desired bytes to read
 * @return the actual # of bytes read from the pipe
 */
int read_pipe(node_t *pipe_n, char *dst, int len);

/* Destroys/frees the pipe and its contents
 * and removes it from the global pilocvar
 *
 * @param pipe_n the pipe to free
 * @return 0 on success, ERROR otherwise
 */
int destroy_pipe(node_t *pipe_n);

//////////////////// LOCK

/* Returns a new lock node pointer containing initialized lock data 
 * with specified id. This new lock node is added to the lock list 
 * on the global pilocvar bookkeeper
 *
 * @param id the desired lock id
 * @return the initialized lock node pointer
 */
node_t *new_lock(int id);

/* Acquires the specified lock. If there is another current 
 * owner of the lock, the calling process blocks, mesa-style
 * When this function returns, the calling process will
 * assuredly have acquired the lock.
 *
 * @param lock_n the lock node to acquire
 * @return 0
 */
int acquire(node_t *lock_n);

/* Releases the specified lock.
 *
 * @param lock_n the lock node to release
 * @return 0 on success, ERROR otherwise
 */
int release(node_t *lock_n);

/* Destroys/frees the specified lock and removes it from
 * the lock list in the global pilocvar
 *
 * @param lock_n the lock node to destroy
 * @return 0 on success, ERROR otherwise
 */
int destroy_lock(node_t *lock_n);

////////////////// CVAR

/* Returns a new cvar node pointer containing initialized cvar data 
 * with specified id. This new cvar node is added to the cvar list 
 * on the global pilocvar bookkeeper
 *
 * @param id the desired cvar id
 * @return the initialized cvar node pointer
 */
node_t *new_cvar(int id);

/* Signals the condition variable specified by cvar_n node
 *
 * @param cvar_n the specified cvar node to signal
 */
void signal_cvar(node_t *cvar_n);

/* Broadcasts the condition variable specified by cvar_n node
 *
 * @param cvar_n the specified cvar node to broadcast
 */
void broadcast(node_t *cvar_n);

/* Releases the specified lock and waits on the specified condition variable 
 * When the lock is finally acquired, the call returns to userland.
 *
 * @param cvar_n the specified cvar node to wait on
 * @param lock_n the specified lock node to wait with
 */
void wait_cvar(node_t *cvar_n, node_t *lock_n);

/* Destroys/frees the specified cvar and removes it from
 * the cvar list in the global pilocvar
 *
 * @param cvar_n the cvar node to destroy
 * @return 0 on success, ERROR otherwise
 */
int destroy_cvar(node_t *cvar_n);

////////////// GENERAL

/* Initializes and returns a new pilocvar_t pointer
 * 
 * @return a new pilocvar_t pointer
 */
pilocvar_t *pilocvar_init(void);

/* Gets a new id for initializing a new pipe/lock/cvar
 *
 * @return the new id
 */
int new_id(void);

/* finds the pipe specified by id in the global pilocvar pipe list
 *
 * @param id the specified id of the pipe to find
 * @return the found pipe node, NULL if not found
 */
node_t *find_pipe(int id);

/* finds the lock specified by id in the global pilocvar lock list
 *
 * @param id the specified id of the lock to find
 * @return the found lock node, NULL if not found
 */
node_t *find_lock(int id);

/* finds the cvar specified by id in the global pilocvar cvar list
 *
 * @param id the specified id of the cvar to find
 * @return the found cvar node, NULL if not found
 */
node_t *find_cvar(int id);

#endif //__PILOCVARIO_H

/* Erich WOo & Boxian Wang
 * 19 November 2020
 * Pipe, Lock, Cvar, and TtyIO manipulation. See pilocvario.h for detailed documentation
 */

#include "pilocvario.h"

// THE storage bookkeeper for all pipes, locks, and cvars
extern pilocvar_t *pilocvar;

// THE proc table
  
extern proc_table_t *procs;

// The storage bookkeeper for all terminals
extern io_control_t *io;

/************************** FUNCTIIONS **************************/

// buffer

buffer_t *new_buffer(int size) {
  buffer_t *new = malloc(sizeof(buffer_t));
  new->buffered = calloc(size, sizeof(char));
  new->size = size;
  new->filled = new->head = new->tail = 0;
  return new;
}

int write_buffer(buffer_t *buffer, char *src, int len) {
  int avail = buffer->size - buffer->filled;
  int written = avail < len ? avail : len;
  for (int i = 0; i < written; i++) 
    buffer->buffered[(buffer->tail + i) % buffer->size] = src[i];
  buffer->tail = (buffer->tail + written) % buffer->size;
  buffer->filled += written;
  return written;
} 

int read_buffer(buffer_t *buffer, char *dst, int len) {
  int read = buffer->filled < len ? buffer->filled : len;
  for (int i = 0; i < read; i++) {
    dst[i] = buffer->buffered[(buffer->head + i) % buffer->size];
  }
  buffer->head = (buffer->head + read) % buffer->size;
  buffer->filled -= read;
  return read;
} 

void reset_buffer(buffer_t *buffer) {
  buffer->filled = buffer->head = buffer->tail = 0;
}

void destroy_buffer(buffer_t *buffer) {
  buffer->size = 0;      // safety
  reset_buffer(buffer);
  free(buffer->buffered);
  buffer->buffered = NULL;
  free(buffer);
  buffer = NULL;
}

// tty io

ttyio_t *new_ttyio(void) {
  ttyio_t *new = malloc(sizeof(ttyio_t));
  new->blocked = new_ll();
  new->buffer = new_buffer(TERMINAL_MAX_LINE); // subject to change
  new->transmitting = 0;
  return new;
}

io_control_t *io_control_init(void) {
  io_control_t *io = malloc(sizeof(io_control_t));
  for (int i = 0; i < NUM_TERMINALS; i++) {
    io->in[i] = new_ttyio();
    io->out[i]= new_ttyio();
  }
  return io;
}

int write_tty(int tty_id, char *src, int len) {
  ttyio_t *out = io->out[tty_id];
  int written, total = 0;

  while (total < len) {
    if (out->transmitting) {
      block(out->blocked);
    } else {
      reset_buffer(out->buffer);
      written = write_buffer(out->buffer, src, len);
      total += written;
      src += written; 
      out->transmitting = 1;
      TtyTransmit(tty_id, out->buffer->buffered, written);
      h_block(out->blocked); // the one which made the last write shall always be at the head of the queue, for fast wake up
    }
  }
  if (!is_empty(out->blocked)) unblock_head(out->blocked); // see if next wants to write
  return total;
}

void write_alert(int tty_id) {
  ttyio_t *out = io->out[tty_id];
  reset_buffer(out->buffer);
  out->transmitting = 0;
  if (!is_empty(out->blocked)) unblock_head(out->blocked);
}

int read_tty(int tty_id, char *dst, int len) {
  int read;
  ttyio_t *in = io->in[tty_id];
  while ((read = read_buffer(in->buffer, dst, len)) == 0) {
    block(in->blocked); // will be woken up in trap
  }
  return read;
}

void receive(int tty_id) {
  int read = TtyReceive(tty_id, io->landing_buffer, TERMINAL_MAX_LINE); // receive in landing buffer
  int real_rd = write_buffer(io->in[tty_id]->buffer, io->landing_buffer, read);
  if (real_rd > 0) unblock_all(io->in[tty_id]->blocked); // we got something, bois
}

// pipe

node_t *new_pipe(int id) {
  pipe_t *p = malloc(sizeof(pipe_t));
  p->buffer = new_buffer(PIPE_BUFFER_LEN);
  p->readblocked = new_ll();
  p->writeblocked = new_ll();
  p->unfulfilled = 0;
  node_t *n = new_node(p);
  n->code = id;
  enqueue(pilocvar->pipe, n);
  return n;
}

int write_pipe(node_t *pipe_n, char *src, int len) {
  pipe_t *pipe = pipe_n->data;
  int total, written = 0;
  int len_left = len;
  
  // write as much as space allowed in buffer, block if need to write more
  while ((written = write_buffer(pipe->buffer, src, len)) < len_left) {
    src += written; // move to rights
    total += written;
    len_left -= written;
    block(pipe->writeblocked);
    pipe->unfulfilled--; // now I wake up and check things (fulfilled)
  }
  // done writing, so unblock readers
  if (!is_empty(pipe->readblocked)) {
    pipe->unfulfilled += get_size(pipe->readblocked); // now a process is in limbo
    unblock_all(pipe->readblocked); // now a process is in limbo
  }
  return total;
}

// assume len > 0
int read_pipe(node_t *pipe_n, char *dst, int len) {
  pipe_t *pipe = pipe_n->data;
  int read;

  // read at most len from buffer, block if 0 read
  while ((read = read_buffer(pipe->buffer, dst, len)) == 0) {
    block(pipe->readblocked); // I'm blockeds
    pipe->unfulfilled--; // now I wake up and check things (fulfilled)
  }
  // done reading, so unblock writers
  if (!is_empty(pipe->writeblocked)) {
    pipe->unfulfilled += get_size(pipe->writeblocked); // now a process is in limbo
    unblock_all(pipe->writeblocked);
  }
  return read;
}

int destroy_pipe(node_t *pipe_n) {
  pipe_t *pipe = pipe_n->data;
  if (!is_empty(pipe->readblocked) || !is_empty(pipe->writeblocked) || pipe->unfulfilled > 0) return ERROR;
  // free insides
  destroy_buffer(pipe->buffer);
  free(pipe->readblocked);
  free(pipe->writeblocked);
  pipe->readblocked = pipe->writeblocked = NULL;
  pipe->unfulfilled = 0;
  // free node
  remove(pilocvar->pipe, pipe_n);
  destroy_node(pipe_n);
  pipe_n = NULL;
  return 0;
}

/// lock

node_t *new_lock(int id) {
  lock_t *l = malloc(sizeof(pipe_t));
  l->owner = NULL;
  l->blocked = new_ll();
  l->unfulfilled = l->cvar = 0;
  node_t *n = new_node(l);
  n->code = id;
  enqueue(pilocvar->lock, n);
  return n;
}

int acquire(node_t *lock_n) {
  lock_t *lock = lock_n->data;
  while (!(lock->owner == NULL || lock->owner == procs->running)) {
    block(lock->blocked); // mesa style
    lock->unfulfilled--; // now I wake up and check things (fulfilled)
  }
  lock->owner = procs->running;
  return 0;
}

int release(node_t *lock_n) {
  lock_t *lock = lock_n->data;
  if (lock->owner != procs->running) return ERROR;
  if (!is_empty(lock->blocked)) {
    unblock_head(lock->blocked);
    lock->unfulfilled++; // now a process is in limbo
  }
  lock->owner = NULL;
  return 0;
}

int destroy_lock(node_t *lock_n) {
  lock_t *lock = lock_n->data;
  if (lock->owner != NULL || !is_empty(lock->blocked) || 
    lock->unfulfilled > 0 || lock->cvar > 0) return ERROR; // cant destroy easily!
  // destroy contents
  free(lock->blocked);
  lock->owner = NULL;
  lock->blocked = NULL;
  lock->unfulfilled = lock->cvar = 0;
  // destroy node
  remove(pilocvar->lock, lock_n);
  destroy_node(lock_n);
  lock_n = NULL;
  return 0;
}

node_t *new_cvar(int id) {
  cvar_t *c = malloc(sizeof(pipe_t));
  c->blocked = new_ll();
  node_t *n = new_node(c);
  n->code = id;
  enqueue(pilocvar->cvar, n);
  return n;
}

void signal_cvar(node_t *cvar_n) {
  cvar_t *cvar = cvar_n->data;
  if (!is_empty(cvar->blocked)) unblock_head(cvar->blocked);
}

void broadcast(node_t *cvar_n) {
  cvar_t *cvar = cvar_n->data;
  if (!is_empty(cvar->blocked)) unblock_all(cvar->blocked);
}

// must have the lock!!! error checking done at a higher level
void wait_cvar(node_t *cvar_n, node_t *lock_n) {
  cvar_t *cvar = cvar_n->data;
  lock_t *lock = lock_n->data;
  release(lock_n);
  lock->cvar++; // I want to use this later, don't destroy yet
  block(cvar->blocked);
  acquire(lock_n); // hopefully still there
  lock->cvar--; // no more cvar waiting on it
}

int destroy_cvar(node_t *cvar_n) {
  lock_t *cvar = cvar_n->data;
  if (!is_empty(cvar->blocked)) return ERROR; // cant destroy easily!
  // destroy contents
  free(cvar->blocked);
  cvar->blocked = NULL;
  // destroy node
  remove(pilocvar->cvar, cvar_n);
  destroy_node(cvar_n);
  cvar_n = NULL;
  return 0;
}

/// pilocvar

pilocvar_t *pilocvar_init(void) {
  pilocvar_t *p = malloc(sizeof(pilocvar_t));
  p->count = 0;
  p->pipe = new_ll();
  p->lock = new_ll();
  p->cvar = new_ll();
  return p;
}

int new_id(void) {
  return pilocvar->count++;
}

node_t *find_pipe(int id) {
  return find(pilocvar->pipe, id);
}

node_t *find_lock(int id) {
  return find(pilocvar->lock, id);
}

node_t *find_cvar(int id) {
  return find(pilocvar->cvar, id);
}

// to destroy pilocvar, call the functions above in the syscall

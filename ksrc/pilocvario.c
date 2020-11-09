/// ERROR CHECKING SHOULD BE DONE AT THE LEVEL CLOSEST TO THE INPUT
/// STRUCTURAL/ABSTRACT CODE SHOULD DO MINIMUM ERROR CHECKING, BUT MOSTLY SANITY CHECKS

#include "pilocvario.h"

extern pilocvar_t *pilocvar;
extern proc_table_t *procs;
extern io_control_t *io;

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
  for (int i = 0; i < written; i++) {
    buffer->buffered[(buffer->tail + i) % buffer->size] = src[0];
    src++; // move src forward
  }
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
  free(buffer->buffered);
  free(buffer);
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

// assume len > 0
// this goes in syscall
// landing buffer would be the actual buffer, if we reset it every time
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
      //src += written; // maybe move forward in write_buffer
      out->transmitting = 1;
      TtyTransmit(tty_id, out->buffer->buffered, written);
      h_block(out->blocked); // the one which made the last write shall always be at the head of the queue, for fast wake up
    }
  }
  if (!is_empty(out->blocked)) unblock_head(out->blocked); // see if next wants to write
  return total;
}


// goes in trap, call it when write is finished
int write_alert(int tty_id) {
  ttyio_t *out = io->out[tty_id];
  reset_buffer(out->buffer);
  out->transmitting = 0;
  if (!is_empty(out->blocked)) unblock_head(out->blocked);
}


// assume len > 0
// this goes in syscall
int read_tty(int tty_id, char *dst, int len) {
  int read;
  ttyio_t *in = io->in[tty_id];
  while ((read = read_buffer(in->buffer, dst, len)) == 0) {
    block(in->blocked); // will be woken up in trap
  }
  return read;
}

// this goes in trap
int receive(int tty_id) {
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

// len > 0
int write_pipe(node_t *pipe_n, char *src, int len) {
  pipe_t *pipe = pipe_n->data;
  int written, len_left = len;
  while ((written = write_buffer(pipe->buffer, src, len)) < len_left) {
    //src += written; // move to rights
    len_left -= written;
    block(pipe->writeblocked);
    pipe->unfulfilled--; // now I wake up and check things (fulfilled)
  }
  if (!is_empty(pipe->readblocked)) {
    pipe->unfulfilled += get_size(pipe->readblocked); // now a process is in limbo
    unblock_all(pipe->readblocked); // now a process is in limbo
  }
  return written;
}

// assume len > 0
int read_pipe(node_t *pipe_n, char *dst, int len) {
  pipe_t *pipe = pipe_n->data;
  int rd;
  while ((rd = read_buffer(pipe->buffer, dst, len)) == 0) {
    block(pipe->readblocked); // I'm blockeds
    pipe->unfulfilled--; // now I wake up and check things (fulfilled)
  }
  if (!is_empty(pipe->writeblocked)) {
    pipe->unfulfilled += get_size(pipe->writeblocked); // now a process is in limbo
    unblock_all(pipe->writeblocked);
  }
  return rd;
}

int destroy_pipe(node_t *pipe_n) {
  pipe_t *pipe = pipe_n->data;
  if (!is_empty(pipe->readblocked) || !is_empty(pipe->writeblocked) || pipe->unfulfilled > 0) return ERROR;
  destroy_buffer(pipe->buffer);
  remove(pilocvar->pipe, pipe_n);
  destroy_node(pipe_n);
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

void acquire(node_t *lock_n) {
  lock_t *lock = lock_n->data;
  while (!(lock->owner == NULL || lock->owner == procs->running)) {
    block(lock->blocked); // mesa style
    lock->unfulfilled--; // now I wake up and check things (fulfilled)
  }
  lock->owner = procs->running;
}

int release(node_t *lock_n) {
  lock_t *lock = lock_n->data;
  if (lock->owner != procs->running) return ERROR;
  if (!is_empty(lock->blocked)) {
    unblock_head(lock->blocked);
    lock->unfulfilled++; // now a process is in limbo
  }
  lock->owner = NULL;
}

int destroy_lock(node_t *lock_n) {
  lock_t *lock = lock_n->data;
  if (lock->owner != NULL || !is_empty(lock->blocked) || 
    lock->unfulfilled > 0 || lock->cvar > 0) return ERROR; // cant destroy easily!
  remove(pilocvar->lock, lock_n);
  destroy_node(lock_n);
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
  remove(pilocvar->cvar, cvar_n);
  destroy_node(cvar_n);
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

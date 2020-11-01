/// ERROR CHECKING SHOULD BE DONE AT THE LEVEL CLOSEST TO THE INPUT
/// STRUCTURAL/ABSTRACT CODE SHOULD DO MINIMUM ERROR CHECKING, BUT MOSTLY SANITY CHECKS

#include <ykernel.h>
#include "linked_list.h"
#include "scheduling.h"


// buffer
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

// buffer
buffer_t *new_buffer(int size);

int write_buffer(buffer_t *buffer, int len, char *src);

int read_buffer(buffer_t *buffer, int len, char *dst);

void reset_buffer(buffer_t *buffer);

void destroy_buffer(buffer_t *buffer);

// ttyio
ttyio_t *new_ttyio(void);

io_control_t *io_control_init(void);

int write_tty(int tty_id, int len, char *src);

int write_alert(int tty_id);

int read_tty(int tty_id, int len, char *dst);

int receive(int tty_id);

// pipe

node_t *new_pipe(int id);

int write_pipe(node_t *pipe_n, int len, char *src);

int read_pipe(node_t *pipe_n, int len, char *dst);

int destroy_pipe(node_t *pipe_n);

// lock
node_t *new_lock(int id);

void acquire(node_t *lock_n);

int release(node_t *lock_n);

int destroy_lock(node_t *lock_n);

// cvar
node_t *new_cvar(int id);

void signal_cvar(node_t *cvar_n);

void broadcast(node_t *cvar_n);

void wait_cvar(node_t *cvar_n, node_t *lock_n);

int destroy_cvar(node_t *cvar_n);

// general

pilocvar_t *pilocvar_init(void);

int new_id(void);

node_t *find_pipe(int id);

node_t *find_lock(int id);

node_t *find_cvar(int id);


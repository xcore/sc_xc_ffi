#ifndef __special_interfaces_h__
#define __special_interfaces_h__

///////////////////////////////////////////

#define SET_SERVER_IMPLEMENTATION(f, g) \
 asm(".globl " #f); \
 asm(".set " #f "," #g); \
 asm(".globl " #f ".nstackwords"); \
 asm(".set " #f ".nstackwords, " #g ".nstackwords"); \
 asm(".globl " #f ".maxcores"); \
 asm(".set " #f ".maxcores, " #g ".maxcores"); \
 asm(".globl " #f ".maxtimers"); \
 asm(".set " #f ".maxtimers, " #g ".maxtimers"); \
 asm(".globl " #f ".maxchanends"); \
 asm(".set " #f ".maxchanends, " #g ".maxchanends");

#define SET_SERVER_RESOURCE_USE(f, r, g)         \
 asm(".globl " #f ".dynalloc_max" #r); \
 asm(".set " #f ".dynalloc_max" #r "," #g); \

#define DECLARE_MAX_ARG(f, i) \
  asm(".globl " #f "." #i ".maxargsize.group"); \
  asm(".weak " #f "." #i ".maxargsize.group");                          \
  asm(".max_reduce " #f "." #i ".maxargsize, " #f "." #i ".maxargsize.group, 1");

#define SET_SERVER_STATE_SIZE(f, n, extra)                    \
  asm(".globl " #f ".init.0.savedstate");               \
  asm(".set " #f ".init.0.savedstate, %0" extra::"i"((((n)+8)+3)/4));




struct client_interface_state_t {
  void *state_ptr;
  void **vtable;
};

struct array_client_interface_state_t {
  struct {
    void *state_ptr;
    int i;
  } *array_index_state_ptr_ptr;
  void **vtable;
};

struct client_notification_interface_state_t {
  void *state_ptr;
  struct {
    chanend notification_chanend;
    int last_received_id;
  } notification_state;
  void **vtable;
};

struct array_client_notification_interface_state_t {
  struct {
    void *state_ptr;
    int i;
  } *array_index_state_ptr_ptr;
  struct {
    chanend notification_chanend;
    int last_received_id;
  } notification_state;
  void **vtable;
};


struct server_interface_state_t {
     chanend incomingChanend;   // chanend to select on, receive client calls.
                                // set to NULL if the interface is distributed.
     void *client_state;        // pointer to the client side state
                                // (if distributed)
     chanend notification_src_chanend;  // chanend to send out notifications on.
     chanend notification_dst_chanend;  // chanend to send notifications to.
     int can_notify;                  // flag to say whether the
                                      // server should send a
                                      // notification id on the channel
                                      // if a notification is requested.
};

#define GET_TASK_STATE(p) ((void *) ((char *)p + 8))

static inline void INIT_NOTIFICATION_CONNECTION(void *connection, void *state)
{
  struct server_interface_state_t *server_state = connection;
  struct client_notification_interface_state_t *client_state =  server_state->client_state;
  client_state->state_ptr = state;
  server_state->can_notify = 1;
  client_state->notification_state.last_received_id = 0;
  asm volatile("setd res[%0], %1"::
               "r"(server_state->notification_src_chanend),
               "r"(server_state->notification_dst_chanend):
               "memory");
}

static inline void INIT_CONNECTION(void *connection, void *state)
{
  struct server_interface_state_t *server_state = connection;
  struct client_interface_state_t *client_state =  server_state->client_state;
  client_state->state_ptr = state;
}

static inline void INIT_TASK_STATE0(void *state0)
{
  int *state = state0;
  state[0] = 1;
  state[1] = 0;
}

static inline void INIT_TASK_STATE1(void *state0)
{
  int *state = state0;
  state[1] = 1;
}

static inline void DO_NOTIFICATION(void *connection)
{
  volatile struct server_interface_state_t *server_state = connection;
  if (!server_state->can_notify)
    return;
  server_state->can_notify = 0;
  asm volatile("outct res[%0], %1"::
               "r"(server_state->notification_src_chanend),
               "i"(XS1_CT_END):
               "memory");
}

static inline void CLEAR_NOTIFICATION(void *connection, int received)
{
  struct server_interface_state_t *server_state = connection;
  if (server_state->can_notify) {
    DO_NOTIFICATION(connection);
  } else if (received) {
    DO_NOTIFICATION(connection);
    server_state->can_notify = 1;
  }
}

///////////////////////////////////////////


#endif // __special_interfaces_h__

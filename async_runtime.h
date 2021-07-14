typedef struct event_data
{
    int event_type;
    int fd;
} event_data_t;
int add_to_queue(int events, event_data_t *data);
int init_poll(int init_size);

void run(void);
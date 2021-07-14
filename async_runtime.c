#include <stdlib.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include "async_runtime.h"
#define MAX_EVENTS 15

int epollfd;
struct epoll_event ev, evlist[MAX_EVENTS];
char error_message[100];

int clifd;
int guard(int n, char *err)
{
    if (n == -1)
    {
        perror(err);
        exit(1);
    }
    return n;
}
void makenoneblocking(int filedes)
{
    int flags = fcntl(filedes, F_GETFL);
    sprintf(error_message, "ERR: could not set socket with filedes = %d to be non-blocking", filedes);
    guard(fcntl(filedes, F_SETFL, flags | O_NONBLOCK), error_message);
    printf("LOG: socket set to non blocking mode\n");
}
int add_to_queue(int events, event_data_t *data)
{
    int filedes = data->fd;
    makenoneblocking(filedes);
    ev.events = events;
    ev.data.ptr = data;
    return epoll_ctl(epollfd, EPOLL_CTL_ADD, filedes, &ev);
}
int init_poll(int init_size)
{
    epollfd = guard(epoll_create(init_size), "epoll_create");
    perror("epoll_create");
}
void run(void)
{
    int ready;
    while (1)
    {
        ready = epoll_wait(epollfd, evlist, MAX_EVENTS, -1);
        for (size_t i = 0; i < ready; i++)
        {
            if (((event_data_t *)evlist[i].data.ptr)->fd == -1 && errno == 11)
            {
                continue;
            }
            else if (evlist[i].events & EPOLLRDHUP)
            {
                printf("connection closed fd=%d; eventid: %d \n", ((event_data_t *)evlist[i].data.ptr)->fd, evlist[i].events);
                epoll_ctl(epollfd, EPOLL_CTL_DEL, ((event_data_t *)evlist[i].data.ptr)->fd, NULL);
                close(((event_data_t *)evlist[i].data.ptr)->fd);
            }
            else if ((evlist[i].events & EPOLLIN))
            {
                if (((event_data_t *)evlist[i].data.ptr)->event_type == 1)
                {
                    clifd = accept(((event_data_t *)evlist[i].data.ptr)->fd, NULL, NULL);
                    if (clifd != -1)
                    {
                        makenoneblocking(clifd);
                        ev.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLHUP | EPOLLET;
                        // printf(" fd=%d; eventid: %d \n", evlist[i].data.fd, evlist[i].events);
                        event_data_t e_data = {event_type : 2, fd : clifd};
                        ev.data.ptr = &e_data;
                        guard(epoll_ctl(epollfd, EPOLL_CTL_ADD, clifd, &ev), "epoll_ctl: epoll_ctl_add");
                    }
                }
                printf("ready to read => %d\n", ((event_data_t *)evlist[i].data.ptr)->fd);
            }
            else if ((evlist[i].events & EPOLLOUT))
            {
                printf("written => %li\n", write(((event_data_t *)evlist[i].data.ptr)->fd, "Hello, World!", 14));
            }
        }
    }
}

#include "license_pbs.h" /* See here for the software license */
#include <stdlib.h>
#include <stdio.h> /* fprintf */
#include "tcp.h"
#include "list_link.h" /* list_link */

int diswcs(tcp_chan *chan, const char *value, size_t nchars)
 {
 fprintf(stderr, "The call to diswcs needs to be mocked!!\n");
 exit(1);
 }

void *get_next(list_link pl, char *file, int line)
 {
 fprintf(stderr, "The call to log_event needs to be mocked!!\n");
 exit(1);
 }

int diswui(tcp_chan *chan, unsigned value)
 {
 fprintf(stderr, "The call to log_event needs to be mocked!!\n");
 exit(1);
 }

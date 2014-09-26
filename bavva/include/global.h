#ifndef GLOBAL_H_
#define GLOBAL_H_

#define HOSTNAME_LEN 128
#define MAX_PARALLEL_DOWNLOAD 3
#define COMMAND_BUFFER 128

typedef struct fs_header
{
    int message_type;
    int item_count;
    char metadata[1024];
}FSHeader;

#endif

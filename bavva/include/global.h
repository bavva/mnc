#ifndef GLOBAL_H_
#define GLOBAL_H_

#define HOSTNAME_LEN 128
#define MAX_PARALLEL_DOWNLOAD 3
#define METADATA_SIZE 1024
#define COMMAND_BUFFER 1024
#define PACKET_BUFFER 1024

#include <string>

typedef enum
{
    MSG_TYPE_REGISTER_REQUEST,
    MSG_TYPE_REGISTER_RESPONSE,
    MSG_TYPE_UPLOAD_FILE,
    MSG_MAX
}FSMessageType;

typedef struct fs_header
{
    FSMessageType message_type;
    int content_length;
    char metadata[METADATA_SIZE];
}FSHeader;

class ServerIP
{
    public:
    std::string server_ip;
    std::string server_name;
    int port;

    ServerIP(std::string server_ip, std::string server_name, int port):server_ip(server_ip), server_name(server_name), port(port){}
    ~ServerIP(){}
};

#endif

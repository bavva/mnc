#ifndef GLOBAL_H_
#define GLOBAL_H_

#define HOSTNAME_LEN 128
#define MAX_PARALLEL_DOWNLOAD 3
#define METADATA_SIZE 1024
#define COMMAND_BUFFER 1024
#define PACKET_BUFFER 1024
#define MAX_FILENAME_LEN 128

#include <string>

typedef enum
{
    MSG_TYPE_REGISTER_REQUEST,
    MSG_TYPE_REGISTER_RESPONSE,
    MSG_TYPE_SEND_FILE,
    MSG_TYPE_REQUEST_FILE,
    MSG_TYPE_REQUEST_STATS,
    MSG_TYPE_STATS_RESPONSE,
    MSG_MAX
}FSMessageType;

typedef struct fs_header
{
    FSMessageType message_type;
    int content_length;
    char metadata[METADATA_SIZE];
}FSHeader;

class StatisticsEntry
{
    public:
    std::string host1;
    std::string host2;

    int total_downloads;                // from remote peer to us
    int total_uploads;                  // from us to remote peer
    unsigned long total_upload_data;    // total data we uploaded
    unsigned long total_download_data;  // total data we downloaded
    unsigned long total_upload_time;    // time taken in micro seconds for all upload data
    unsigned long total_download_time;  // time taken in micro seconds for all download data

    StatisticsEntry(std::string host1, std::string host2, int total_downloads, int total_uploads, unsigned long total_upload_data, unsigned long total_download_data, unsigned long total_upload_time, unsigned long total_download_time):host1(host1), host2(host2), total_downloads(total_downloads), total_uploads(total_uploads), total_upload_data(total_upload_data), total_download_data(total_download_data), total_upload_time(total_upload_time), total_download_time(total_download_time){}
    StatisticsEntry(std::string host1, std::string host2):host1(host1), host2(host2), total_downloads(0), total_uploads(0), total_upload_data(0), total_download_data(0), total_upload_time(0), total_download_time(0){}
    ~StatisticsEntry(){}
};

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

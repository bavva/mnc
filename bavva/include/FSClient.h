#ifndef _FSCLIENT_H_
#define _FSCLIENT_H_
class FSClient
{
    private:
    int port;

    public:
    FSClient(int port):port(port){};
    ~FSClient(){};

    // functions
    void start(void);
};
#endif /* _FSCLIENT_H_ */

#ifndef _FSSERVER_H_
#define _FSSERVER_H_
class FSServer
{
    private:
    int port;

    public:
    FSServer(int port):port(port){};
    ~FSServer(){};

    // functions
    void start(void);
};
#endif /* _FSSERVER_H_ */

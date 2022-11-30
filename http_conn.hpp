#ifndef http_conn_hpp
#define http_conn_hpp

#include <sys/epoll.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include "locker.hpp"
#include <sys/uio.h>

class HttpConn{
public:
    static int m_epollfd;  // All connections are registered on one epoll object.
    static int m_user_count; // User counter
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;

    /// Http methos 
    enum METHOD {GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT};
    
    /// checking state
    enum CHECK_STATE { CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT };
    
    /// parsing result
    enum HTTP_CODE { NO_REQUEST, GET_REQUEST, BAD_REQUEST, NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION };
    
    /// @brief line read state
    enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };
    HttpConn();
    ~HttpConn();

    /**
     * @brief init object
     * @param sockfd sock fd
     * @param sockaddr sockaddr
    */
    void init(int sockfd, const sockaddr_in &sockaddr);

    /// @brief close connection
    void closeConnection();

    /// @brief call by thread, process http request
    void process(); 

    /// @brief read all data non-blockingly
    /// @return true if success
    bool read();

    /// @brief write into pipe non-blockingly
    /// @return true if success
    bool write();

    

private:
    /// @brief socket fd of the http connection
    int m_sockfd; // socket fd of the http connection
    sockaddr_in m_address; // socket address
    char m_readbuf[READ_BUFFER_SIZE];
    /// @brief index of byte to be read
    int m_readIndex; 
    int m_checked_index; // pos of the currently parsing character in readbuf
    int m_start_line; // start pos of currently parsing line
    CHECK_STATE m_check_state; // my check state
    /**
     * @brief initialize rest data of the connection 
     * 
     */
    void init();
    /**
     * @brief parsing http request, call by process()
     * 
     * @return parsing status
     */
    HTTP_CODE process_read();
    /**
     * @brief prase first line
     * 
     * @param text text to be parsed
     * @return HTTP_CODE
     */
    HTTP_CODE parse_request_line(char* text);
    /**
     * @brief parse header
     * 
     * @param text text to be parsed
     * @return HTTP_CODE 
     */
    HTTP_CODE parse_headers(char* text);
    /**
     * @brief parse content
     * 
     * @param text text to be parsed
     * @return HTTP_CODE 
     */
    HTTP_CODE parse_content(char* text);
    /**
     * @brief do 
     * 
     * @return HTTP_CODE 
     */
    HTTP_CODE do_request();

    LINE_STATUS parse_line(); /// parse one line
    
    /// @brief get line
    /// @return geted line
    char* get_line() {
        return m_readbuf + m_start_line ;
    }

};

#endif
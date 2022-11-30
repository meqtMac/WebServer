#include "http_conn.hpp"

int HttpConn::m_epollfd = -1;
int HttpConn::m_user_count = 0;
/**
 * @brief set fd nonblocking
 * @param[in] fd fd to be modified
*/
void setnonblocking(int fd) {
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK );
}

/**
 * @brief Add to be listened fd to epoll
 * @param epollfd epoll fd
 * @param fd file describer to be added
*/
void addfd(int epollfd, int fd, bool oneshot) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
    if (oneshot) {
        event.events |= EPOLLONESHOT;
    }

    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    // set fd nonblock
    setnonblocking(fd);
}

/**
 * @brief Remove fd from epoll
 * @param epollfd epoll fd
 * @param fd file describer to be deleted
*/
void removefd(int epollfd, int fd) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

/**
 * @brief modify fd in epoll, rest socket event EPOLLONESHOT, to make sure when readable next time, EPOLLIN event can be activated
 * @param epollfd epoll fd
 * @param fd file describer to be modified
 * @param event event
*/
void modifyfd(int epollfd, int fd, int ev) {
    epoll_event event;
    event.data.fd = fd;
    event.events  = ev | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

HttpConn::HttpConn() {

}

HttpConn::~HttpConn() {

}

void HttpConn::init(int sockfd, const sockaddr_in& sockaddr) {
    m_sockfd = sockfd;
    m_address = sockaddr;

    // Port multiplexing, must be set before bind
    int reuse = 1;
    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // add to epoll 
    addfd(m_epollfd, m_sockfd, true);
    m_user_count++;
    init();
}

void HttpConn::init() {
    m_check_state = CHECK_STATE_REQUESTLINE;
    m_checked_index = 0;
    m_start_line = 0;
    m_readIndex= 0;
}

void HttpConn::closeConnection() {
    if (m_sockfd != -1) {
        removefd(m_epollfd, m_sockfd);
        m_sockfd = -1;
        m_user_count--;
    }
}

bool HttpConn::read() {
    //TODO: read
    if (m_readIndex >= READ_BUFFER_SIZE) {
        return false;
    }

    int bytes_reads = 0; // bytes have read
    while(true) {
        bytes_reads = recv(m_sockfd, m_readbuf+m_readIndex, READ_BUFFER_SIZE-m_readIndex, 0);
        if (bytes_reads == -1) {
            if (errno == EAGAIN or errno == EWOULDBLOCK ) {
                // no data
                break;
            }
            return false;
        }else if (bytes_reads == 0) {
            // connectiohn closed;
            return false;
        }
        // shift index
        m_readIndex += bytes_reads; 
    }
    printf("read data: %s\n", m_readbuf);
    return true;
}

HttpConn::HTTP_CODE HttpConn::process_read() {
    LINE_STATUS line_state = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char* text = 0;
    while( ((m_check_state==CHECK_STATE_CONTENT) and (line_state==LINE_OK))or ((line_state = parse_line()) == LINE_OK) ) {
        // parsed integral line, or integral line of request conetent
        text = get_line();
        m_start_line = m_checked_index;
        printf("get a http line: %s", text);

        switch (m_check_state)
        {
        case CHECK_STATE_REQUESTLINE:
            ret = parse_request_line(text);
            if (ret==BAD_REQUEST) {
                return BAD_REQUEST;
            }
            break;
        case CHECK_STATE_HEADER:
            ret = parse_headers(text);
            if (ret==BAD_REQUEST) {
                return BAD_REQUEST;
            }else if (ret == GET_REQUEST) {
                return do_request();
            }
            break;
        case CHECK_STATE_CONTENT:
            ret = parse_content(text);
            if (ret==GET_REQUEST) {
                return do_request();
            }
            line_state = LINE_OPEN;
            break;
        
        default:
            return INTERNAL_ERROR; 
            break;
        }

    }
    return NO_REQUEST;
}

HttpConn::HTTP_CODE HttpConn::parse_request_line(char* text) {
    return NO_REQUEST;
}

HttpConn::HTTP_CODE HttpConn::parse_headers(char* text) {
    return NO_REQUEST;
}

HttpConn::HTTP_CODE HttpConn::parse_content(char* text) {
    return NO_REQUEST;
}

HttpConn::LINE_STATUS HttpConn::parse_line() {
    char temp;
    for (; m_checked_index < m_read_index; )
    return LINE_OK;
}

HttpConn::HTTP_CODE HttpConn:do_request() {
}
bool HttpConn::write() {
    //TODO: write 
    printf("write data\n");
    return true;
}

void HttpConn::process() {
    //TODO: Parsing HTTP request
    HTTP_CODE read_ret = process_read();
    if (read_ret == NO_REQUEST) {
        modifyfd(m_epollfd, m_sockfd, EPOLLIN); 
    }
    //TODO: Generate response
}
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
    m_method = GET;
    m_URL = 0;
    m_version = 0;
    m_content_length = 0;
    m_host = 0;
    m_linger = false;
    bzero(m_readbuf, READ_BUFFER_SIZE);
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
            if  (ret == GET_REQUEST) {
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
    m_URL = strpbrk(text, " \t");
    *m_URL++ = '\0';
    char* method = text;
    if (strcasecmp(method, "GET") == 0)  {
        m_method = GET;
    }else {
        return BAD_REQUEST;
    }

    m_version = strpbrk(m_URL, " \t");
    if (!m_version) return BAD_REQUEST;
    *m_version++ = '\0';
    if (strcasecmp(m_version, "HTTP/1.1")!=0) return BAD_REQUEST;

    if (strncasecmp(m_URL, "http://", 7) == 0 ) {
        m_URL += 7;
        m_URL = strchr(m_URL, '/');
    }

    if (!m_URL or m_URL[0]!='/') return BAD_REQUEST;
    m_check_state = CHECK_STATE_HEADER;

    return NO_REQUEST;
}

HttpConn::HTTP_CODE HttpConn::parse_headers(char* text) {
    // 遇到空行，表示头部字段解析完毕
    if( text[0] == '\0' ) {
        if ( m_content_length != 0 ) {
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST; // if there is no content, then parsing finished
    } else if ( strncasecmp( text, "Connection:", 11 ) == 0 ) {
        // Connection
        text += 11;
        text += strspn( text, " \t" );
        if ( strcasecmp( text, "keep-alive" ) == 0 ) {
            m_linger = true;
        }
    } else if ( strncasecmp( text, "Content-Length:", 15 ) == 0 ) {
        // Content-Length;
        text += 15;
        text += strspn( text, " \t" );
        m_content_length = atol(text);
    } else if ( strncasecmp( text, "Host:", 5 ) == 0 ) {
        text += 5;
        text += strspn( text, " \t" );
        m_host = text;
    } else {
        printf( "oop! unknow header %s\n", text );
    }
    return NO_REQUEST;
}

HttpConn::HTTP_CODE HttpConn::parse_content(char* text) {
    // judge if content is integral
    if ( m_readIndex >= ( m_content_length + m_checked_index ) ) {
        text[ m_content_length ] = '\0';
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

HttpConn::LINE_STATUS HttpConn::parse_line() {
    char temp;
    for (; m_checked_index < m_readIndex; ++m_checked_index ) {
        temp = m_readbuf[m_checked_index];
        if (temp=='\r') {
            if (m_checked_index+1 == m_readIndex) {
                return LINE_OPEN;
            }else if (m_readbuf[m_checked_index+1] == '\n') {
                m_readbuf[m_checked_index++] = '\0';
                m_readbuf[m_checked_index++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD; 
        }else if ( temp = '\n') {
            if ((m_checked_index>1) and (m_readbuf[m_checked_index-1]=='\r')) {
                m_readbuf[m_checked_index-1] = '\0';
                m_readbuf[m_checked_index++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

HttpConn::HTTP_CODE HttpConn::do_request() {
    // "/home/nowcoder/webserver/resources"
    strcpy( m_real_file, doc_root );
    int len = strlen( doc_root );
    strncpy( m_real_file + len, m_url, FILENAME_LEN - len - 1 );
    // 获取m_real_file文件的相关的状态信息，-1失败，0成功
    if ( stat( m_real_file, &m_file_stat ) < 0 ) {
        return NO_RESOURCE;
    }

    // 判断访问权限
    if ( ! ( m_file_stat.st_mode & S_IROTH ) ) {
        return FORBIDDEN_REQUEST;
    }

    // 判断是否是目录
    if ( S_ISDIR( m_file_stat.st_mode ) ) {
        return BAD_REQUEST;
    }

    // 以只读方式打开文件
    int fd = open( m_real_file, O_RDONLY );
    // 创建内存映射
    m_file_address = ( char* )mmap( 0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0 );
    close( fd );
    return FILE_REQUEST;
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
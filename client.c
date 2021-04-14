#define _GNU_SOURCE 1
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <poll.h>
#include <fcntl.h>

#define BUFFER_SIZE 64

int main( int argc, char* argv[] )
{
/*        
    argv[0] 指向程序运行的全路径名
    argv[1] 指向在bash命令行中执行程序名后的第一个字符串
    argv[2] 指向执行程序名后的第二个字符串
*/
    if( argc <= 2 )
    {
        printf( "base name usage: %s ip_address port_number\n", basename( argv[0] ) );
        printf( "argv[0] name usage: %s ip_address port_number\n", argv[0] );
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi( argv[2] );

    struct sockaddr_in server_address;
    bzero( &server_address, sizeof( server_address ) );
    //af_inet is ipv4
    server_address.sin_family = AF_INET;
    //inet_pton - convert IPv4 and IPv6 addresses from text to binary form
    //sin_addr is a struct, it has just a 32 bits member, ipv4 address
    inet_pton( AF_INET, ip, &server_address.sin_addr );
    server_address.sin_port = htons( port );

    int sockfd = socket( PF_INET, SOCK_STREAM, 0 );
    assert( sockfd >= 0 );
    
    //three hand shake
    if ( connect( sockfd, ( struct sockaddr* )&server_address, sizeof( server_address ) ) < 0 )
    {
        printf( "connection failed\n" );
        close( sockfd );
        return 1;
    }
    /*
     * 0, 1, 2 are the three file descriptors reserved by the system, 
       corresponding to standard input, standard output, and standard error respectively
     */
    //just two fd need, a stdin , a socket
    struct pollfd fds[2];
    fds[0].fd = 0;
    fds[0].events = POLLIN;
    fds[0].revents = 0; //The actual event is filled by the kernel.
    fds[1].fd = sockfd;
    //pollrdhup: Stream socket peer closed connection, or shut down writing half of connection.
    fds[1].events = POLLIN | POLLRDHUP;
    fds[1].revents = 0;

    char read_buf[BUFFER_SIZE];
    int pipefd[2];
    int ret = pipe( pipefd );
    assert( ret != -1 );

    while( 1 )
    {
        //timeout -1 means that the poll will block 
        ret = poll( fds, 2, -1 );
        //On error, -1 is returned, and errno is set to indicate the error.
        if( ret < 0 )
        {
            printf( "poll failure\n" );
            break;
        }

        //socket
        if( fds[1].revents & POLLRDHUP )
        {
            printf( "server close the connection\n" );
            break;
        }
        else if( fds[1].revents & POLLIN )
        {
            memset( read_buf, '\0', BUFFER_SIZE );
            recv( fds[1].fd, read_buf, BUFFER_SIZE-1, 0 );
            printf( "%s\n", read_buf );
        }

        //stdin
        if( fds[0].revents & POLLIN )
        {
            //splice is zero copy
            //when use splice, fd_in and fd_out must have a pipe at least
            //pipefd[0] refers to the read end of the pipe.  
            //pipefd[1] refers to the write end of the pipe. 
            ret = splice( 0, NULL, pipefd[1], NULL, 666, SPLICE_F_MORE | SPLICE_F_MOVE );
            ret = splice( pipefd[0], NULL, sockfd, NULL, 666, SPLICE_F_MORE | SPLICE_F_MOVE );
        }
    }
    
    close( sockfd );
    return 0;
}


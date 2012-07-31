/*------------------------------------------------------------------------------
*
* Copyright (c) 2011, EURid. All rights reserved.
* The YADIFA TM software product is provided under the BSD 3-clause license:
* 
* Redistribution and use in source and binary forms, with or without 
* modification, are permitted provided that the following conditions
* are met:
*
*        * Redistributions of source code must retain the above copyright 
*          notice, this list of conditions and the following disclaimer.
*        * Redistributions in binary form must reproduce the above copyright 
*          notice, this list of conditions and the following disclaimer in the 
*          documentation and/or other materials provided with the distribution.
*        * Neither the name of EURid nor the names of its contributors may be 
*          used to endorse or promote products derived from this software 
*          without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*------------------------------------------------------------------------------
*
* DOCUMENTATION */
/** @defgroup streaming Streams
 *  @ingroup dnscore
 *  @brief
 *
 * @{
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "dnscore/tcp_io_stream.h"
#include "dnscore/fdtools.h"

/*
 * AF_INET
 * AF_INET6
 * AF_UNSPEC ( = 0)
 */

ya_result
gethostaddr(const char* host, u16 port, struct sockaddr *sa, int familly)
{
    int s;

    /*    ------------------------------------------------------------    */

    /* Create a network address structure
     * from the the dotted-quad format ddd.ddd.ddd.ddd into a in_addr_t
     */

    /* If not forced in ipv6 then ... */

    struct addrinfo hints;
    struct addrinfo *info;
    struct addrinfo *next;
    int eai_err;

    ZEROMEMORY(&hints, sizeof (struct addrinfo));

    hints.ai_family = familly;

    ZEROMEMORY(sa, sizeof (struct sockaddr));

    /*    ------------------------------------------------------------    */

    if((eai_err = getaddrinfo(host, NULL, &hints, &info)) != 0)
    {
        return NET_UNABLE_TO_RESOLVE_HOST;
    }

    next = info;
    while(next != NULL)
    {
        if((familly != AF_INET6) && (next->ai_family == AF_INET)) /* Only process IPv4 addresses */
        {
            struct sockaddr_in *sai = (struct sockaddr_in *)sa;
            memcpy(sai, next->ai_addr, next->ai_addrlen);
            sai->sin_port = htons(port);

            break;
        }

        if((familly != AF_INET) && (next->ai_family == AF_INET6)) /* Only process IPv4 addresses */
        {
            struct sockaddr_in6 *sai6 = (struct sockaddr_in6 *)sa;
            memcpy(sai6, next->ai_addr, next->ai_addrlen);
            sai6->sin6_port = htons(port);

            break;
        }

        next = next->ai_next;
    }

    freeaddrinfo(info);

    if(next == NULL) /* nothing found for AF_INET */
    {
        return NET_UNABLE_TO_RESOLVE_HOST;
    }

    return SUCCESS;
}

ya_result
tcp_input_output_stream_connect_sockaddr(struct sockaddr *sa, input_stream *istream_, output_stream *ostream_, struct sockaddr *bind_from, u8 to_sec)
{
    int fd;

    while((fd = socket(sa->sa_family, SOCK_STREAM, 0)) < 0)
    {
        int err = errno;
        
        if(err != EINTR)
        {
            return MAKE_ERRNO_ERROR(err);
        }
    }
    
    /*
     * Bind the socket if required.
     */

    if(bind_from != NULL)
    {
        while((bind(fd, bind_from, sizeof (struct sockaddr))) < 0)
        {
            int err = errno;

            if(err != EINTR)
            {
                close_ex(fd); /* could clear errno */

                return MAKE_ERRNO_ERROR(err);
            }
        }
    }

    int ssec, susec, rsec, rusec;

    tcp_get_sendtimeout(fd, &ssec, &susec);
    tcp_get_recvtimeout(fd, &rsec, &rusec);

    tcp_set_sendtimeout(fd, to_sec, 0);
    tcp_set_recvtimeout(fd, to_sec, 0);

    while(connect(fd, sa, sizeof(struct sockaddr)) < 0)
    {
        int err = errno;
        
        if(err != EINTR)
        {
            close_ex(fd);
            
            // Linux quirk
            
            if(err == EINPROGRESS)
            {
                err = ETIMEDOUT;
            }
            
            return MAKE_ERRNO_ERROR(err);
        }
    }
    
    /* can only fail if fd < 0, which is never the case here */
    
    fd_input_stream_attach(fd, istream_);
    fd_output_stream_attach(fd, ostream_);

    tcp_set_sendtimeout(fd, ssec, susec);
    tcp_set_recvtimeout(fd, rsec, rusec);

    return fd;
}

ya_result
tcp_input_output_stream_connect_ex(const char *server, u16 port, input_stream *istream_, output_stream *ostream_, struct sockaddr *bind_from, u8 to_sec)
{
    ya_result return_code;
    struct sockaddr sa;

    /*
     * If the client interface is specified, then use its family.
     * Else use the unspecified familly to let the algorithm choose the first available one.
     */

    int family = (bind_from != NULL) ? bind_from->sa_family : AF_UNSPEC;

    if(ISOK(return_code = gethostaddr(server, port, &sa, family)))
    {
        return_code = tcp_input_output_stream_connect_sockaddr(&sa, istream_, ostream_, bind_from, to_sec);
    }

    return return_code;
}

ya_result
tcp_input_output_stream_connect(const char *server, u16 port, input_stream *istream, output_stream *ostream)
{
    return tcp_input_output_stream_connect_ex(server, port, istream, ostream, NULL, 0);
}

ya_result
tcp_input_output_stream_connect_host_address(host_address *ha, input_stream *istream_, output_stream *ostream_, u8 to_sec)
{
    socketaddress sa;
    
    ya_result return_code;
    
    if(ISOK(return_code = host_address2sockaddr(&sa, ha)))
    {
        return_code = tcp_input_output_stream_connect_sockaddr(&sa.sa, istream_, ostream_, NULL, to_sec);
    }

    return return_code;
}


ya_result
tcp_io_stream_connect_ex(const char *server, u16 port, io_stream *ios, struct sockaddr *bind_from)
{
    input_stream istream;
    output_stream ostream;
    ya_result return_code;

    if(ISOK(return_code = tcp_input_output_stream_connect_ex(server, port, &istream, &ostream, bind_from, 0)))
    {
        io_stream_link(ios, &istream, &ostream);
    }

    return return_code;
}

ya_result
tcp_io_stream_connect(const char *server, u16 port, io_stream *ios)
{
    return tcp_io_stream_connect_ex(server, port, ios, NULL);
}


void
tcp_set_linger(int fd, bool enable, int seconds)
{
    struct linger l;
    l.l_onoff = (enable)?1:0;
    l.l_linger = seconds;

    setsockopt(fd, SOL_SOCKET, SO_LINGER, &l, sizeof(l));
}

void
tcp_set_sendtimeout(int fd, int seconds, int useconds)
{
    struct timeval tv;
    tv.tv_sec = seconds;
    tv.tv_usec = useconds;
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
}

void
tcp_set_recvtimeout(int fd, int seconds, int useconds)
{
    struct timeval tv;
    tv.tv_sec = seconds;
    tv.tv_usec = useconds;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

void
tcp_get_sendtimeout(int fd, int *seconds, int *useconds)
{
    struct timeval tv;
    socklen_t tv_len = sizeof(tv);
    getsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, &tv_len);
    *seconds = tv.tv_sec;
    *useconds = tv.tv_usec;
}

void
tcp_get_recvtimeout(int fd, int *seconds, int *useconds)
{
    struct timeval tv;
    socklen_t tv_len = sizeof(tv);
    getsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, &tv_len);
    *seconds = tv.tv_sec;
    *useconds = tv.tv_usec;
}

/** @} */

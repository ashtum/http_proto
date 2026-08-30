//
// Copyright (c) 2026 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_SERVER_WORKER_HPP
#define BOOST_HTTP_SERVER_WORKER_HPP

#include <boost/http/detail/config.hpp>
#include <boost/capy/io/any_stream.hpp>
#include <boost/capy/task.hpp>
#include <boost/http/message_reader.hpp>
#include <boost/http/message_writer.hpp>
#include <boost/http/request_parser.hpp>
#include <boost/http/serializer.hpp>
#include <boost/http/server/router.hpp>

#include <utility>

namespace boost {
namespace http {

/** Reusable HTTP request/response processing logic.

    This class provides the core HTTP processing loop: reading
    requests, dispatching them through a router, and sending
    responses. It is designed as a mix-in base class for use
    with @ref corosio::tcp_server.

    @par Usage with tcp_server

    To use this class, derive a custom worker from both
    @ref corosio::tcp_server::worker_base and `http_worker`.
    The derived class must:

    @li Construct `http_worker` with a router and configurations
    @li Initialize the @ref stream member before calling
        @ref do_http_session

    @par Example
    @code
    struct my_worker
        : tcp_server::worker_base
        , http_worker
    {
        corosio::tcp_socket sock;

        my_worker(
            corosio::io_context& ctx,
            http::router<route_params> const& router,
            http::parser::config const& parser_cfg,
            http::serializer::config const& serializer_cfg)
            : http_worker(sock, router, parser_cfg, serializer_cfg)
            , sock(ctx)
        {
            sock.open();
        }

        corosio::tcp_socket& socket() override { return sock; }

        void run(launcher launch) override
        {
            launch(sock.get_executor(), do_http_session());
        }
    };
    @endcode

    @par Thread Safety
    Distinct objects: Safe.
    Shared objects: Unsafe.

    @see corosio::tcp_server, http_server
*/
class BOOST_HTTP_DECL http_worker
{
public:
    http::router<route_params> fr;
    http::route_params rp;
    capy::any_stream stream;
    http::request_parser parser;
    http::serializer serializer;

    /** Construct an HTTP worker.

        @param stream_ The stream carrying the session.

        @param fr_ The router for dispatching requests to handlers.

        @param parser_cfg Configuration for the request parser.

        @param serializer_cfg Configuration for the response
            serializer.
    */
    template<class Stream>
        requires capy::ReadStream<Stream> && capy::WriteStream<Stream>
    http_worker(
        Stream& stream_,
        http::router<route_params> fr_,
        http::parser::config const& parser_cfg,
        http::serializer::config const& serializer_cfg)
        : fr(std::move(fr_))
        , rp()
        , stream(&stream_)
        , parser(parser_cfg)
        , serializer(serializer_cfg)
        {
            rp.req_body = http::any_buffer_source(
                http::message_reader(&stream, &parser));
            rp.res_body = http::any_buffer_sink(
                http::message_writer(&stream, &serializer));
        }

    /** Handle an HTTP session.

        This coroutine reads HTTP requests, dispatches them through
        the router, and sends responses until the connection is
        closed or an error occurs. The stream data member must be
        initialized before calling this function.

        @return An awaitable that completes when the session ends.
    */
    capy::task<void>
    do_http_session();
};

} // http
} // boost

#endif

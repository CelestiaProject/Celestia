// tee.h
//
// Copyright (C) 2009, Thomas Guest <tag@wordaligned.org>
//               2021, the Celestia Development Team
//
// Tee stream implementation based on Thomas Guest post published at
// http://wordaligned.org/articles/cpp-streambufs
//
// This code is placed in the public domain.

#pragma once

#include <streambuf>

template <typename char_type,
          typename traits = std::char_traits<char_type>>
class basic_teebuf :
    public std::basic_streambuf<char_type, traits>
{
 public:
    typedef typename traits::int_type int_type;
    typedef std::basic_streambuf<char_type, traits> streambuf_type;

    // Construct a streambuf which tees output to both input
    // streambufs.
    basic_teebuf(streambuf_type *sb1,
                 streambuf_type *sb2) :
        sb1(sb1),
        sb2(sb2)
    {}

    basic_teebuf() = delete;
    ~basic_teebuf() = default;
    basic_teebuf(const basic_teebuf&) = default;
    basic_teebuf(basic_teebuf&&) = default;
    basic_teebuf& operator=(const basic_teebuf&) = default;
    basic_teebuf& operator=(basic_teebuf&&) = default;

 private:
    int_type overflow(int_type c) override
    {
        const auto eof = traits::eof();

        if (traits::eq_int_type(c, eof))
            return traits::not_eof(c);

        const auto ch = traits::to_char_type(c);
        const auto r1 = sb1->sputc(ch);
        const auto r2 = sb2->sputc(ch);

        return traits::eq_int_type(r1, eof) || traits::eq_int_type(r2, eof) ? eof : c;
    }

    int sync() override
    {
        const auto r1 = sb1->pubsync();
        const auto r2 = sb2->pubsync();
        return r1 == 0 && r2 == 0 ? 0 : -1;
    }

    streambuf_type *sb1;
    streambuf_type *sb2;
};

typedef basic_teebuf<char>    teebuf;
typedef basic_teebuf<wchar_t> wteebuf;


template <typename char_type,
          typename traits = std::char_traits<char_type>>
class basic_teestream :
    public std::basic_ostream<char_type, traits>
{
 public:
    typedef std::basic_ostream<char_type, traits> stream_type;

    // Construct an ostream which tees output to the supplied
    // ostreams.
    basic_teestream(stream_type &o1, stream_type &o2) :
        std::basic_ostream<char_type, traits>(&tbuf),
        tbuf(o1.rdbuf(), o2.rdbuf())
    {}

    basic_teestream() = delete;
    ~basic_teestream() = default;
    basic_teestream(const basic_teestream&) = default;
    basic_teestream(basic_teestream&&) = default;
    basic_teestream& operator=(const basic_teestream&) = default;
    basic_teestream& operator=(basic_teestream&&) = default;

 private:
    basic_teebuf<char_type, traits> tbuf;
};

typedef basic_teestream<char>    teestream;
typedef basic_teestream<wchar_t> wteestream;

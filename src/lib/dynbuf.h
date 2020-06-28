#ifndef HEADER_CURL_DYNBUF_H
#define HEADER_CURL_DYNBUF_H
/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 2020, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

struct dynbuf {
  char *bufr;    /* point to a zero terminated allocated buffer */
  size_t leng;   /* number of bytes *EXCLUDING* the zero terminator */
  size_t allc;   /* size of the current allocation */
  size_t toobig; /* size limit for the buffer */
#ifdef DEBUGBUILD
  int init;     /* detect API usage mistakes */
#endif
};

void Curl_dyn_init(struct dynbuf *s, size_t toobig);
void Curl_dyn_free(struct dynbuf *s);
CURLcode Curl_dyn_addn(struct dynbuf *s, const void *mem, size_t len)
  WARN_UNUSED_RESULT;
CURLcode Curl_dyn_add(struct dynbuf *s, const char *str)
  WARN_UNUSED_RESULT;
CURLcode Curl_dyn_addf(struct dynbuf *s, const char *fmt, ...)
  WARN_UNUSED_RESULT;
void Curl_dyn_reset(struct dynbuf *s);
CURLcode Curl_dyn_tail(struct dynbuf *s, size_t trail);
char *Curl_dyn_ptr(const struct dynbuf *s);
unsigned char *Curl_dyn_uptr(const struct dynbuf *s);
size_t Curl_dyn_len(const struct dynbuf *s);

/* Dynamic buffer max sizes */
#define DYN_DOH_RESPONSE    3000
#define DYN_DOH_CNAME       256
#define DYN_PAUSE_BUFFER    (64 * 1024 * 1024)
#define DYN_HAXPROXY        2048
#define DYN_HTTP_REQUEST    (128*1024)
#define DYN_H2_HEADERS      (128*1024)
#define DYN_H2_TRAILER      4096
#define DYN_APRINTF         8000000
#define DYN_RTSP_REQ_HEADER (64*1024)
#define DYN_TRAILERS        (64*1024)
#define DYN_PROXY_CONNECT_HEADERS 16384
#define DYN_QLOG_NAME       1024
#define DYN_H1_TRAILER      DYN_H2_TRAILER
#endif

Short: 2
Long: sslv2
Tags: Versions
Protocols: SSL
Added:
Mutexed: sslv3 tlsv1 tlsv1.1 tlsv1.2
Requires: TLS
See-also: http1.1 http2
Help: Use SSLv2
Category: tls
Example: --sslv2 $URL
---
This option previously asked curl to use SSLv2, but starting in curl 7.77.0
this instruction is ignored. SSLv2 is widely considered insecure (see RFC
6176).

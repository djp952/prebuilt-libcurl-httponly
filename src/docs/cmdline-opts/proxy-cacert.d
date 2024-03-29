Long: proxy-cacert
Help: CA certificate to verify peer against for proxy
Arg: <file>
Added: 7.52.0
See-also: proxy-capath cacert capath proxy
Category: proxy tls
Example: --proxy-cacert CA-file.txt -x https://proxy $URL
---
Same as --cacert but used in HTTPS proxy context.

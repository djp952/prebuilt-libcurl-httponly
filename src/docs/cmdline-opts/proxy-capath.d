Long: proxy-capath
Help: CA directory to verify peer against for proxy
Arg: <dir>
Added: 7.52.0
See-also: proxy-cacert proxy capath
Category: proxy tls
Example: --proxy-capath /local/directory -x https://proxy $URL
---
Same as --capath but used in HTTPS proxy context.

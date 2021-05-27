Long: ssl-auto-client-cert
Help: Use auto client certificate (Schannel)
Added: 7.77.0
See-also: proxy-ssl-auto-client-cert
Category: tls
---
Tell libcurl to automatically locate and use a client certificate for
authentication, when requested by the server. This option is only supported
for Schannel (the native Windows SSL library). Prior to 7.77.0 this was the
default behavior in libcurl with Schannel. Since the server can request any
certificate that supports client authentication in the OS certificate store it
could be a privacy violation and unexpected.

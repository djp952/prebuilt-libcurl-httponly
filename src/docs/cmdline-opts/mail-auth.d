Long: mail-auth
Arg: <address>
Protocols: SMTP
Help: Originator address of the original email
Added: 7.25.0
See-also: mail-rcpt mail-from
Category: smtp
Example: --mail-auth user@example.come -T mail smtp://example.com/
---
Specify a single address. This will be used to specify the authentication
address (identity) of a submitted message that is being relayed to another
server.

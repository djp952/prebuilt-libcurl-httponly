Long: data-raw
Arg: <data>
Protocols: HTTP
Help: HTTP POST data, '@' allowed
Added: 7.43.0
See-also: data
Category: http post upload
Example: --data-raw "hello" $URL
Example: --data-raw "@at@at@" $URL
---
This posts data similarly to --data but without the special
interpretation of the @ character.

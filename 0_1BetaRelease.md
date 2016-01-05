# Rarping version 0.1 beta available! #

I have just committed changes to reach the 0.1 beta version. Rarping won't get new features next days, time to test it for a first stable version.
Nevertheless, feel free to submit patches and/or suggestions if you're thinking to cool features for Rarping.



## List of currently available options ##

Following options are implemented in Version 0.1 beta (to version 0.1 stable) :

  * -q RARPing exits (success) after the first received reply
  * -c [\_count\_ ](.md) is used to specify the number of requests to perform
  * -a [_IP address_] allows user to send replies instead of requests, [_IP address_] is the content of the reply, in `.' notation.
  * -t [_timeout_] sets the send/recv timeout value to [_timeout_] milliseconds (default 1000)
  * -w [_delay_] sets the delay between two probes to [_delay_] milliseconds (default 1000)
  * -r [_retries_] Abort after [_retries_] unanswered probes (default none)


A description of the Rarping's command line syntax is given using -h (or any invalid command line)
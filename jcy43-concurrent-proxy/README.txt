Usage:	./proxy <port> <mode>

port: 	the port number

mode:	the mode of operation for the program, lower case
	t for mulithreaded version
	p for multiprocess version
	
Report

	While working on the file, I had relatively no trouble with the multiprocess version.
	However, while working on the multithreaded version, I keep running into double free issues, even when free was not explicitly called. The system will run the first request successfully and log the request into the log file, but when the second request responds, the system will successfully send the response back to the client and will successfully create a logentry using calloc. However, when formatting the logentry string, a double free error will appear. After running gdb, I realized that the problem came from the localtime() function within the format_log_entry() function, which then runs the funtion tzset_internal() from tzset.c. At line 401 of tzset specifies "free(old_tz)", which is the line that is causing the double free. I tried many things to attempt to make it so that the error does not appear, but I cannot identify what is causing the issue. I have checked every variable passed into the format_log_entry() function, yet all of them were normal. 
	If I simply skipped the format_log_entry() and used something else as the logentry string (ex: "test"), the error also disappears, and the program will run normally except for the writing to log, which will write the test string into the log file instead of the formatted string. The server will respond normally and the proxy will redirect the response as normal, no matter how many requests. This led me to believe that there is something wrong with format_log_entry() itself, which led to even more confusion because I did not modify anthing from the orginal implementation that is given to us.
	Furthermore, if I ran localtime() using dummy variables right after the format_log_entry(), it will also lead to a double free error, this time even during the first request, which I expected. However, I still could not diagnose the issue.
	To the TA and professor, if you read this, can read my code and see what I did wrong? I wish to know so that I would not make the same mistake again. I searched everywhere online and could not find a solution. I even rewrote the thread_handler() function several times. My email is jcy43@scarletmail.rutgers.edu. Thanks!

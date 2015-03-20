#simple cmd
#cat invalid_file
#echo hello word
#sort read-command.c

#simple cmd with io
#echo this msg should overwrite>out
#cat <in>out

#special simple_cmd
#exec echo hello
#if :; then echo colonworks!; fi

#sequence cmd (with io)
#echo a; echo b>out
#sleep 0.001s; echo a

#echo c
#echo a; echo hello>out
#cat invalid; echo hello

#subshell
#(echo HELLO;echo hello)

#subshell with io redirect
#(cat; cat)<in #should print once
#(cat; echo hello)<in
#(echo a;echo b)>out

#pipe
#ls | sort -d
#echo aaaaaaaaaa | echo bbbbbbbbbbb
#cat in | tr a-z A-Z
#echo a | tr a A
#(cat in ; echo a) | tr a-z A-Z
#cat in | (cat;echo Once!)
#ls | sort -d
#echo HELLO | tr A-Z a-z
#sort read-command.c | tr a-z A-Z
# &
#while until 
#while (echo testing while); do echo whileworks!; done
#while (cat invalid); do echo something!; done
#until echo testing; do echo whileworks; done
#until cat incalid; do echo until works!; done

#if
#if echo A is true; then echo then works!; fi
#if cat invalid; then echo should not print!; fi
#if cat invalid; then echo blah; else echo else works!; fi

#if with io
#if echo A; then echo B; fi >out
#if cat; then cat; fi <in
# &&&&!

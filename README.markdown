# Linenoise

A minimal, zero-config, readline replacement.

## Can a line editing library be 20k lines of code?

Line editing with some support for history is a really important feature for command line utilities. It's much better to just hit the up arrow and edit on syntax error or to try a slightly different command. But apparently code dealing with terminals is some sort of Black Magic: readline is 30k lines of code, libedit 20k. Is it reasonable to link small utilities to huge libraries just to get a minimal support for line editing?

It's a matter of coding philosophy. For me shipping zero-configuration software is very important. Software that just works, uncompressing the tar.gz and typing make. Also not enabling line editing if readline is not present in the system is lame: as it is not a blocking requirements many configuration script will just drop the support if you don't have the lib installed. The result is a pollution of binaries without line editing support.

So I spent more or less two hours doing a reality check resulting in this little library: is it *really* needed for a line editing library to be 20k lines of code?

## Terminals, in 2010.

Apparently almost every terminal you can happen to use today has some kind of support for VT100 alike escape sequences. So I tried to write a lib using just very basic VT100 features. The resulting library appears to work everywhere I tried to use it.

Since it's so young I guess there are a few bugs, or the lib may not compile or work with some operating system, but it's a matter of a few weeks and eventually we'll get it right, and there will be no excuses for not shipping command line tools without built-in line editing support.

The library is currently less than 400 lines of code. In order to use it in your project just look at the *example.c* file in the source distribution, it is trivial. Linenoise is BSD code, so you can use both in free software and commercial software.

Please fork it and add something interesting and send me a pull request.

Send feedbacks to antirez at gmail

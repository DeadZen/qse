QSESED Commands                                                       {#sed-cmd}
================================================================================

Overview
--------
A stream editor is a non-interactive text editing tool commonly used
on Unix environment. It reads text from an input stream, stores it to
pattern space, manipulates the pattern space by applying a set of editing
commands, and writes the pattern space to an output stream. Typically, the
input and output streams are a console or a file. 

Commands
--------

A sed command is composed of:

 - line selector (optional)
 - ! (optional)
 - command code
 - command arguments (optional, dependent on command code)

A line selector selects input lines to apply a command to and has the following
forms:
 - address - specify a single address
 - address,address - specify an address range 
 - start~step - specify a starting line and a step. 
                #QSE_SED_STARTSTEP enables this form.

An @b address is a line number, a regular expression, or a dollar sign ($) 
while a @b start and a @b step is a line number. 

A regular expression for an address has the following form:
 - /rex/ - a regular expression @b rex is enclosed in slashes
 - \\CrexC - a regular expression @b rex is enclosed in @b \\C and @b C 
            where @b C can be any character.

It treats the @b \\n sequence specially to match a newline character.

Here are examples of line selectors:
 - 10 - match the 10th line
 - 10,20 - match lines from the 10th to the 20th.
 - /^[[:space:]]*$/ - match an empty line 
 - /^abc$/,/^def$/ - match all lines between @b abc and @b def inclusive
 - 10,$ - match the 10th line down to the last line.
 - 3~4 - match every 4th line from the 3rd line.

Note that an address range always selects the line matching the first address
regardless of the second address; For example, 8,6 selects the 8th line.

The exclamation mark @b !, when used after the line selector and before
the command code, negates the line selection; For example, 1! selects all
lines except the first line.

A command without a line selector is applied to all input lines; 
A command with a single address is applied to an input line that matches 
the address; A command with an address range is applied to all input 
lines within the range, inclusive; A command with a start and a step is
applied to every <b>step</b>'th line starting from the line @b start.

Here is the summary of the commands.

- <b># comment</b>
The text beginning from # to the line end is ignored; # in a line following
<b>a \\</b>, <b>i \\</b>, and <b>c \\</b> is treated literally and does not
introduce a comment.

- <b>: label</b>
A label can be composed of letters, digits, periods, hyphens, and underlines.
It remembers a target label for @b b and @b t commands and prohibits a line
selector.

- <b>{</b>
The left curly bracket forms a command group where you can nest other 
commands. It should be paired with an ending @b }.

- <b>q</b>
Terminates the exection of commands. Upon termination, it prints the pattern
space if #QSE_SED_QUIET is not set.

- <b>Q</b>
Terminates the exection of commands quietly.

- <b>a \\ \n text</b>
Stores @b text into the append buffer which is printed after the pattern 
space for each input line. If #QSE_SED_STRICT is on, an address range is not
allowed for a line selector. If #QSE_SED_SAMELINE is on, the backslash and the 
text can be located on the same line without a line break.

- <b>i \\ \n text</b>
Inserts @b text into an insert buffer which is printed before the pattern
space for each input line. If #QSE_SED_STRICT is on, an address range is not
allowed for a line selector. If #QSE_SED_SAMELINE is on, the backslash and the
text can be located on the same line without a line break.

- <b>c \\ \n text</b>
If a single line is selected for the command (i.e. no line selector, a single
address line selector, or a start~step line selector is specified), it changes
the pattern space to @b text and branches to the end of commands for the line.
If an address range is specified, it deletes the pattern space and branches 
to the end of commands for all input lines but the last, and changes pattern
space to @b text and branches to the end of commands. If #QSE_SED_SAMELINE is
on, the backlash and the text can be located on the same line without a line
break.

- <b>d</b>
Deletes the pattern space and branches to the end of commands.

- <b>D</b>
Deletes the first line of the pattern space. If the pattern space is emptied,
it branches to the end of script. Otherwise, the commands from the first are 
reapplied to the current pattern space.

- <b>=</b>
Prints the current line number. If #QSE_SED_STRICT is on, an address range is 
not allowed as a line selector.

- <b>p</b>
Prints the pattern space.

- <b>P</b>
Prints the first line of the pattern space.

- <b>l</b>
Prints the pattern space in a visually unambiguous form.

- <b>h</b> 
Copies the pattern space to the hold space

- <b>H</b> 
Appends the pattern space to the hold space

- <b>g</b> 
Copies the hold space to the pattern space

- <b>G</b> 
Appends the hold space to the pattern space

- <b>x</b> 
Exchanges the pattern space and the hold space

- <b>n</b>
Prints the pattern space and read the next line from the input stream to fill
the pattern space.

- <b>N</b>
Prints the pattern space and read the next line from the input stream 
to append it to the pattern space with a newline inserted.

- <b>b</b>
Branches to the end of commands.

- <b>b label</b>
Branches to @b label

- <b>t</b>
Branches to the end of commands if substitution(s//) has been made 
successfully since the last reading of an input line or the last @b t command.

- <b>t label</b>
Branches to @b label if substitution(s//) has been made successfully 
since the last reading of an input line or the last @b t command.

- <b>r file</b>
Reads text from @b file and prints it after printing the pattern space but 
before printing the append buffer. Failure to read @b file does not cause an
error.

- <b>R file</b>
Reads a line of text from @b file and prints it after printing pattern space 
but before printing the append buffer. Failure to read @b file does not cause
an error.

- <b>w file</b>
Writes the pattern space to @b file

- <b>W file</b>
Writes the first line of the pattern space to @b file

- <b>s/rex/repl/opts</b>
Finds a matching substring with @b rex in pattern space and replaces it 
with @b repl. @b & in @b repl refers to the matching substring. @b opts may 
be empty; You can combine the following options into @b opts:
 - @b g replaces all occurrences of a matching substring with @b rex
 - @b number replaces the <b>number</b>'th occurrence of a matching substring 
      with @b rex
 - @b p prints pattern space if a successful replacement was made
 - @b w file writes pattern space to @b file if a successful replacement 
      was made. It, if specified, should be the last option.

- <b>y/src/dst/</b>
Replaces all occurrences of characters in @b src with characters in @b dst.
@b src and @b dst must contain equal number of characters.

- <b>c/selector/opts</b>
Selects characters or fields from the pattern space as specified by the
@b selector and update the pattern space with the selected text. A selector
is a comma-separated list of specifiers. A specifier is one of the followings:
<ul>
 <li>@b d specifies the input field delimiter with the next character. e.g) d:
 <li>@b D sepcifies the output field delimiter with the next character. e.g) D;
 <li>@b c specifies a position or a range of characters to select. e.g) c23-25
 <li>@b f specifies a position or a range of fields to select. e.g) f1,f4-3
</ul>
@b opts may be empty; You can combine the following options into @b opts:
<ul>
 <li>@b f folds consecutive delimiters into one.
 <li>@b w uses white spaces for a field delimiter regardless of the input
      delimiter specified in the selector.
 <li>@b d deletes the pattern space if the line is not delimited by 
      the input field delimiter
</ul>

In principle, this can replace the @b cut utility.

Let's see actual examples:
- <b>G;G;G</b>
Triple spaces input lines. If #QSE_SED_QUIET is on, <b>G;G;G;p</b>. 
It works because the hold space is empty unless something is copied to it.

- <b>$!d</b>
Prints the last line. If #QSE_SED_QUIET is on, try <b>$p</b>.

- <b>1!G;h;$!d</b>
Prints input lines in the reverse order. That is, it prints the last line 
first and the first line last.

    $ echo -e "a\nb\nc" | qsesed '1!G;h;$!d'
    c
    b
    a

- <b>s/[[:space:]]{2,}/ /g</b>
Compacts whitespaces if #QSE_SED_REXBOUND is on.

- <b>C/d:,f3,1/</b>
Prints the third field and the first field from a colon separated text.

    $ head -5 /etc/passwd
    root:x:0:0:root:/root:/bin/bash
    daemon:x:1:1:daemon:/usr/sbin:/bin/sh
    bin:x:2:2:bin:/bin:/bin/sh
    sys:x:3:3:sys:/dev:/bin/sh
    sync:x:4:65534:sync:/bin:/bin/sync
    $ qsesed '1,3C/d:,f3,1/;4,$d' /etc/passwd 
    0 root
    1 daemon
    2 bin
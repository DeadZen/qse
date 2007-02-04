global header, mode;
global empty_line_count;
global para_started;

BEGIN {
	header = 1;
	mode = 0;
	empty_line_count = 0;
	para_started = 0;

	output=ARGV[0];
	gsub (/\.man/, ".html", output);

	#print "OUTPUT TO: " output;

	print "</html>";
	print "</head>";
}

header && /^\.[[:alpha:]]+[[:space:]]/ {
	if ($1 == ".title")
	{
		print "<title>" $2 "</title>";
	}
}

header && !/^\.[[:alpha:]]+[[:space:]]/ { 

	header = 0; 
	print "</head>";
	print "<body>";
}

!header {
	local text;

	if (mode == 0)
	{
		if (/^$/)
		{
			# empty line
			if (para_started) 
			{
				para_started = 0;
				print "</p>";
			}
			empty_line_count++;
		}
		else
		{
			if (/^= [^=]+ =$/)
			{
				if (para_started)
				{
					print "</p>";
					para_started = 0;
				}
				text=substr($0, 2, length($0)-2);
				print "<h1>" text "</h1>";
			}
			else if (/^== [^=]+ ==$/)
			{
				if (para_started)
				{
					print "</p>";
					para_started = 0;
				}
				text=substr($0, 3, length($0)-4);
				print "<h2>" text "</h2>";
			}
			else if (/^=== [^=]+ ===$/)
			{
				if (para_started)
				{
					print "</p>";
					para_started = 0;
				}
				text=substr($0, 4, length($0)-6);
				print "<h3>" text "</h3>";
			}
			else if (/^==== [^=]+ ====$/)
			{
				if (para_started)
				{
					print "</p>";
					para_started = 0;
				}
				text=substr($0, 5, length($0)-8);
				print "<h34" text "</h4>";
			}
			else if (/^\{\{\{$/)
			{
				# {{{
				if (para_started)
				{
					print "</p>";
					para_started = 0;
				}
				print "<pre>";
				mode = 1;
			}
			else
			{
				if (!para_started > 0) 
				{
					print "<p>";
					para_started = 1;
				}
					
				gsub ("<", "\\&lt;");
				gsub (">", "\\&gt;");
				print $0;
				print "<br>";
			}

			empty_line_count = 0;
		}
	}
	else if (mode == 1)
	{
		if (/^}}}$/) 
		{
			# }}}
			print "</pre>";
			mode = 0;
		}
		else
		{
			gsub ("<", "\\&lt;");
			gsub (">", "\\&gt;");
			print $0;
		}
	}
}

END {
	print "</body>";
	print "</html>";
}

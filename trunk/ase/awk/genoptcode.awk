#
# genoptcode.awk
#
# aseawk -f generror.awk awk.h
#

BEGIN { 
	collect=0; 
	tab3="\t\t";
	tab4="\t\t\t";
}

/^[[:space:]]*enum[[:space:]]+ase_awk_option_t[[:space:]]*$/ { 
	collect=1; 
	print tab3 "// generated by genoptcode.awk";
	print tab3 "enum Option";
	print tab3 "{";
}

collect && /^[[:space:]]*};[[:space:]]*$/ { 
	print tab3 "};";
	print tab3 "// end of enum Option";
	print "";
	collect=0; 
}

collect && /^[[:space:]]*ASE_AWK_[[:alnum:]]+/ { 
	split ($1, flds, ",");
	name=flds[1];

	print tab4 "OPT_" substr (name,9,length(name)-8) " = " name ",";
}


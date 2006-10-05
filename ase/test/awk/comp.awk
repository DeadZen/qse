BEGIN {
	OFS="\t\t";

	print "1==1 :", (1 == 1);
	print "1==0 :", (1 == 0);

	print "1.0==1 :", (1.0 == 1);
	print "1.1==1 :", (1.1 == 1);

	print "1.0!=1 :", (1.0 != 1);
	print "1.1!=1 :", (1.1 != 1);

	print "abc" == "abc";
	print "abc" != "abc";

	print "--------------------------";
	print "a == \"\" :", (a == "");
	print "a >= \"\" :", (a >= "");
	print "a <= \"\" :", (a <= "");
	print "a >  \"\" :", (a > "");
	print "a <  \"\" :", (a < "");

	print "--------------------------";
	print "a == \" \" :", (a == " ");
	print "a >= \" \" :", (a >= " ");
	print "a <= \" \" :", (a <= " ");
	print "a >  \" \" :", (a > " ");
	print "a <  \" \" :", (a < " ");

	print "--------------------------";
	print "\"\" == a :", ("" == a);
	print "\"\" >= a:", ("" >= a);
	print "\"\" <= a:", ("" <= a);
	print "\"\" >  a:", ("" > a);
	print "\"\" <  a:", ("" < a);

	print "--------------------------";
	print "\" \" == a :", (" " == a);
	print "\" \" >= a:", (" " >= a);
	print "\" \" <= a:", (" " <= a);
	print "\" \" >  a:", (" " > a);
	print "\" \" <  a:", (" " < a);

	print "--------------------------";
	print "10 == \"10\"", (10 == "10");
	print "10 != \"10\"", (10 != "10");
	print "10 >= \"10\"", (10 >= "10");
	print "10 <= \"10\"", (10 <= "10");
	print "10 >  \"10\"", (10 >  "10");
	print "10 <  \"10\"", (10 <  "10");

	print "--------------------------";
	print "10 == \"11\"", (10 == "11");
	print "10 != \"11\"", (10 != "11");
	print "10 >= \"11\"", (10 >= "11");
	print "10 <= \"11\"", (10 <= "11");
	print "10 >  \"11\"", (10 >  "11");
	print "10 <  \"11\"", (10 <  "11");

	print "--------------------------";
	print "11 == \"10\"", (11 == "10");
	print "11 != \"10\"", (11 != "10");
	print "11 >= \"10\"", (11 >= "10");
	print "11 <= \"10\"", (11 <= "10");
	print "11 >  \"10\"", (11 >  "10");
	print "11 <  \"10\"", (11 <  "10");

	print "--------------------------";
	print "010 == \"8\"", (010 == "8");
	print "010 != \"8\"", (010 != "8");
	print "010 >= \"8\"", (010 >= "8");
	print "010 <= \"8\"", (010 <= "8");
	print "010 >  \"8\"", (010 >  "8");
	print "010 <  \"8\"", (010 <  "8");

	print "--------------------------";
	print "10.0 == \"10\"", (10.0 == "10");
	print "10.0 != \"10\"", (10.0 != "10");
	print "10.0 >= \"10\"", (10.0 >= "10");
	print "10.0 <= \"10\"", (10.0 <= "10");
	print "10.0 >  \"10\"", (10.0 >  "10");
	print "10.0 <  \"10\"", (10.0 <  "10");

	#a[10] = 2;
	#print a == 1;

	print (0.234 + 1.01123);
}

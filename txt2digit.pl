#!/usr/bin/perl

print <<EOF;
/*
 * Generated automatically by txt2digit.pl - do not modify
 */

EOF

@lines = ();
while (<>) {
	my @str = ();
	my @xpos = ();
	my @trap = ();
	chop;
	$line = $_;
	$x = 0;
	while ($x<length($line)) {
		while (substr($line,$x,1) eq ' ' and $x<length($line)) { $x++; }
		$xdeb = $x;
		do {
			$x++;
			while ($x<length($line) and substr($line,$x,1) ne ' ' and substr($line,$x,1) ne '}' and substr($line,$x,1) ne '{') { $x++; }
		} while ($x<length($line) and substr($line,$x+1,1) ne ' ' and substr($line,$x+1,1) ne '{' and substr($line,$x,1) ne '}' and substr($line,$x,1) ne '{' );
		$x++ if substr($line,$x,1) eq '}';
		$isol = substr($line,$xdeb,$x-$xdeb);
		if ($isol =~ /{(.*)}/) {
			$isol = $1;
			$this_trap = 1;
			$xdeb ++;
		} else {
			$this_trap = 0;
		}
		push (@str, $isol);
		push (@xpos, $xdeb);
		push (@trap, $this_trap);
	}
	push (@lines, { "xpos",\@xpos, "str",\@str, "trap",\@trap });
}

$nblines=$#lines;

print <<EOF;
static char digits[] = {
EOF

$maxnbstr = 0;
for ($y=0; $y<$nblines; $y++) {
	$nbstr = 1+$#{$lines[$y]{'str'}};
	$maxnbstr = $nbstr if $nbstr>$maxnbstr;
	for ($s=0; $s<$nbstr; $s++) {
		$str = $lines[$y]{'str'}[$s];
		for ($c=0; $c<length($str); $c++) {
			$d = substr($str,$c,1);
			print "'".($d ne '\\'? $d :'\\\\')."',";
		}
	}
	print "\n" if ($s>0);
}
print <<EOF;
0
};

#define NB_MAX_STR_PER_LINE $maxnbstr
EOF

print <<EOF;
static struct {
	unsigned nb_strings;
	struct {
		int pos_x;
		char *str;
		unsigned len;
		char selectable;
		char selected;
	} string[NB_MAX_STR_PER_LINE];	// max of $maxnbstr strings in a line
} lines[] = {
EOF

$separ = 0;
$offset=0;
for ($y=0; $y<$nblines; $y++) {
	print ",\n" if $separ;
	print "{";
	$nbstr = 1+$#{$lines[$y]{'str'}};
	print $nbstr;
	$sep=0;
	for ($s=0; $s<$nbstr; $s++) {
		$xpos = $lines[$y]{'xpos'}[$s];
		$str = $lines[$y]{'str'}[$s];
		if (not $sep) {
			print ", {";
		} else {
			print "," if $sep;
		}
		print "{".$xpos.',digits+'.$offset.','.length($str).','.$lines[$y]{'trap'}[$s].'}';
		$offset += length($str);
		$sep=1;
	}
	print "}" if $s>0;
	print "}";
	$separ = 1;
}


print <<EOF;
};
EOF

if (0) {
print <<EOF;
static struct {
	unsigned y, str_idx;
	primitive *prim;
} traps[] = {
EOF
$separ = 0;
for ($y=0; $y<$nblines; $y++) {
	$nbstr = 1+$#{$lines[$y]{'str'}};
	for ($s=0; $s<$nbstr; $s++) {
		if ($lines[$y]{'trap'}[$s] eq 1) {
			print ",\n" if $separ;
			print "{ $y, $s }";
			$separ=1;
		}
	}
}
print <<EOF;
};
EOF
}

#!/usr/bin/perl -w
use strict;

#  Copyright (C) 2010 Gregor Richards
# 
#  Permission is hereby granted, free of charge, to any person obtaining a copy
#  of this software and associated documentation files (the "Software"), to deal
#  in the Software without restriction, including without limitation the rights
#  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#  copies of the Software, and to permit persons to whom the Software is
#  furnished to do so, subject to the following conditions:
# 
#  The above copyright notice and this permission notice shall be included in
#  all copies or substantial portions of the Software.
# 
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
#  THE SOFTWARE.

my $pedal = 0;
my %offkeys = ();
my $offkey;

while (my $line = <STDIN>) {
    chomp $line;
    my @elems = split / /, $line;

    # check the type
    if ($#elems >= 4 && $elems[1] eq "Par" && $elems[3] eq "c=64") {
        # damper
        my $v = int(substr($elems[4], 2));
        if ($v > 64) {
            # on
            $pedal = 1;

        } else {
            # off
            $pedal = 0;

            # turn off keys
            foreach $offkey (keys %offkeys) {
                if ($offkeys{$offkey}) {
                    print $elems[0] . " Off ch=1 n=" . $offkey . " v=127\n";
                }
            }
            %offkeys = ();

        }

    } elsif ($pedal && $#elems >= 3 && $elems[1] eq "On") {
        # if this is a key that's supposedly off, need to put the off now
        my $n = int(substr($elems[3], 2));
        if ($offkeys{$n}) {
            print $elems[0] . " Off ch=1 n=" . $n . " v=127\n";
            $offkeys{$n} = 0;
        }
        print join(" ", @elems) . "\n";

    } elsif ($pedal && $#elems >= 3 && $elems[1] eq "Off") {
        # lifting a key while the pedal is depressed
        $offkeys{int(substr($elems[3], 2))} = 1;

    } else {
        print join(" ", @elems) . "\n";

    }
}

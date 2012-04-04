#!/usr/bin/perl

## timeout
##
## (c) 2004-2007 Piotr Roszatycki <dexter@debian.org>, GPL
##
## $Id: timeout.pl 4 2007-06-19 11:58:08Z piotr.roszatycki $

=head1 NAME

timeout - Run command with bounded time.

=head1 SYNOPSIS

B<timeout> S<B<-h>>

B<timeout>
S<[-I<signal>]>
I<time>
I<command>
...

=head1 README

B<timeout> executes a command and imposes an elapsed time limit.  When the
time limit is reached, B<timeout> sends a predefined signal to the target
process.

=cut


use 5.006;
use strict;

use Config;
use POSIX ();


##############################################################################

## Default values for constant variables
##

## Program name
my $NAME = 'timeout';

## Program version
my $VERSION = '0.11';


##############################################################################

## Signals to handle
##
my @signals = qw< HUP INT QUIT TERM SEGV PIPE XCPU XFSZ ALRM >;


##############################################################################

## Signal to send after timeout. Default is KILL.
my $signal = 'KILL';

## Time to wait
my $time = 0;

## Command to execute as array of arguments
my @command = ();

## PID for fork function
my $child_pid;

## PID for wait function
my $pid;


##############################################################################

## usage()
##
## Prints usage message.
##
sub usage() {
    # Lazy loading for Pod::Usage
    eval 'use Pod::Usage;';
    die $@ if $@;

    pod2usage(2);
}


## help()
##
## Prints help message.
##
sub help() {
    # Lazy loading for Pod::Usage
    eval 'use Pod::Usage;';
    die $@ if $@;

    pod2usage(-verbose=>1, -message=>"$NAME $VERSION\n");
}


## signal_handler($sig)
##
## Handler for signals to clean up child processes
##
sub signal_handler($) {
    my ($sig) = @_;
    if ($sig eq 'ALRM') {
        printf STDERR "Timeout: aborting command ``%s'' with signal SIG%s\n", join(' ', @command), $signal;
    } else {
        printf STDERR "Got signal SIG%s: aborting command ``%s'' with signal SIG%s\n", $sig, join(' ', @command), $signal;
    }
    kill $signal, -$child_pid;
    exit -1;
}


##############################################################################

## Main subroutine
##


## Parse command line arguments
my $arg = $ARGV[0];
if ($arg =~ /^-(.*)$/) {
    my $opt = $1;
    if ($arg eq '-h' || $arg eq '--help') {
        help();
    } elsif ($opt =~ /^[A-Z0-9]+$/) {
        if ($opt =~ /^\d+/) {
	    # Convert numeric signal to name by using the perl interpreter's
	    # configuration:
            usage() unless defined $Config{sig_name};
            $signal = (split(' ', $Config{sig_name}))[$opt];
        } else {
            $opt =~ s/^SIG//;
            $signal = $opt;
        }
	shift @ARGV;
    } else {
        usage();
    }
}

usage() if @ARGV < 2;

$arg = $ARGV[0];

usage() unless $arg =~ /^\d+$/;

$time = $arg;

shift @ARGV;

@command = @ARGV;


## Fork for exec
if (! defined($child_pid = fork)) {
    die "Could not fork: $!\n";
    exit 1;
} elsif ($child_pid == 0) {
    ## child

    ## Set new process group
    POSIX::setsid;
    
    ## Execute command
    exec @command or die "Can not run command `" . join(' ', @command) . "': $!\n";
}

## parent

## Set the handle for signals
foreach my $sig (@signals) {
    $SIG{$sig} = \&signal_handler;
}

## Set the alarm
alarm $time;

## Wait for child
while (($pid = wait) != -1 && $pid != $child_pid) {}

## Clean exit
exit ($pid == $child_pid ? $? >> 8 : -1);


=head1 DESCRIPTION

B<timeout> executes a command and imposes an elapsed time limit.
The command is run in a separate POSIX process group so that the
right thing happens with commands that spawn child processes.

=head1 OPTIONS

=over 8

=item -I<signal>

Specify an optional signal name to send to the controlled process. By default,
B<timeout> sends B<KILL>, which cannot be caught or ignored.

=item I<time>

The elapsed time limit after which the command is terminated.

=item I<command>

The command to be executed.

=back

=head1 RETURN CODES

=over 8

=item 0..253

Return code from called command.

=item 254

Internal error. No return code could be fetched.

=item 255

The timeout was occured.

=back

=head1 PREREQUISITES

=over

=item *

L<perl> >= 5.006

=item *

L<POSIX>

=back

=head1 COREQUISITES

=over

=item

L<Pod::Usage>

=back

=head1 SCRIPT CATEGORIES

UNIX/System_administration

=head1 AUTHORS

Piotr Roszatycki E<lt>dexter@debian.orgE<gt>

=head1 LICENSE

Copyright 2004-2007 by Piotr Roszatycki E<lt>dexter@debian.orgE<gt>.

Inspired by timeout.c that is part of The Coroner's Toolkit.

All rights reserved.  This program is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License, the
latest version.

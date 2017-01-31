#!/usr/bin/env python

# BEGIN_LEGAL
# BSD License
#
# Copyright (c)2015 Intel Corporation. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.  Redistributions
# in binary form must reproduce the above copyright notice, this list of
# conditions and the following disclaimer in the documentation and/or
# other materials provided with the distribution.  Neither the name of
# the Intel Corporation nor the names of its contributors may be used to
# endorse or promote products derived from this software without
# specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
# ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# END_LEGAL
#
#
# @ORIGINAL_AUTHORS: T. Mack Stallcup, Cristiano Pereira, Harish Patil, Chuck Yount
#
# $Id: drd_util.py,v 1.6 2015/05/19 22:51:10 tmstall Exp tmstall $
#
# This module contains various utility functions used in the tracing script.

import getpass
import os
import re
import subprocess
import time

# Local modules
#
import config
import msg
import util
"""
@package drd_util

Utility functions used for DeDebug scripts.
"""

# Attributes used in this module
#
gdb_path = ''  # Explicit path to GDB binary


def debug(parser):
    """
    Redefine this option because we don't want to confuse users
    by using the name 'debug' for an option which is used inside 
    the debugger GDB.

    @return no return
    """

    parser.add_option(
        "-d", "--dry_run",
        dest="debug",
        action="store_true",
        default=False,
        metavar="COMP",
        help="Print out the command(s), but do not execute.  Also prints out diagnostic "
        "information as well.")


def CheckGdbVersion():
    """
    Get the version of GDB and check to make sure it's a version which can
    be used for DrDebug.  Exit if the version of GDB is too old to run
    with DrDebug.

    @return no return
    @return NOTE: side effect sets global gdb_path
    """

    global gdb_path

    # Get GDB path
    #
    # import pdb;  pdb.set_trace()
    gdb_path = util.Which('gdb')
    if not gdb_path:
        parser.error('gdb not found in PATH')

    # Path to GDB and version info
    #
    cmd = gdb_path + ' --version'
    p = subprocess.Popen(cmd,
                         shell=True,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    (stdout, stderr) = p.communicate()

    # Parse the version string to get the actual version number.
    # Here's an example of the entire version string:
    #
    #   $ gdb --version
    #   GNU gdb (GDB) Red Hat Enterprise Linux (7.2-60.el6)
    #   Copyright (C) 2010 Free Software Foundation, Inc.
    #   License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
    #   This is free software: you are free to change and redistribute it.
    #   There is NO WARRANTY, to the extent permitted by law.  Type "show copying"
    #   and "show warranty" for details.
    #   This GDB was configured as "x86_64-redhat-linux-gnu".
    #   For bug reporting instructions, please see:
    #   <http://www.gnu.org/software/gdb/bugs/>.
    #
    line = stdout.split('\n', 1)[0]
    last = line.split()[-1]
    float_pattern = '[1-9][0-9]*\.?[0-9]*'
    f = re.search(float_pattern, last)
    version = f.group(0)

    # Check to make sure it's at least the base version required for DrDebug.
    #
    if float(version) < float(config.gdb_base_version):
        msg.PrintAndExit('gdb must be at least version: ' +
                         config.gdb_base_version + '\nVersion found was:  ' +
                         version + '\nPath to gdb binary: ' + gdb_path)


def SetGdbCmdFile():
    """
    Get a user specific file name for the commands which will be used to invoke
    GDB.  This is a 'hidden' file which starts with the char '.'.  If the file already
    exists, then remove it.

    @return file name
    """

    config.gdb_cmd_file = '.gdb.cmd.%s' % (getpass.getuser())

    # Create a new file or truncate the file if it already exists.
    #
    open(config.gdb_cmd_file, 'w').close()


def GdbInitialize():
    """
    Ensure the correct version of GDB is installed and initialize the GDB command file.

    @return no return
    """

    CheckGdbVersion()
    SetGdbCmdFile()


def DrDebugScriptCmd(script, pin_knobs, options):
    """
    Format the initial section of the script command line, including the
    appropriate script, used to execute the DrDebug low level scripts.  It formats
    commands for both logging and replay.  The parameter 'script' determines the
    'base' of the script to execute.

    The type of kit from which the script is called deterimines the specific
    lower level scripts to be called called.  If called are from an SDE kit,
    the string 'sde_' is prepended to the base script name.

    Also generate the explicit path to the directory in the specific kit
    where the scripts are located.

    @return tuple with:
    @return     - string with command to run
    @return     - path to scripts in kit
    """

    # Get the type of kit from which the script was executed and print
    # out this info if desired.
    #
    # import pdb;  pdb.set_trace()
    kit_type, base_dir = util.GetKitType()
    if (hasattr(options, 'verbose') and options.verbose):
        string = 'Dir where was script found indicates kit type: '
        if kit_type == config.PINPLAY:
            string += 'PinPlay'
        elif kit_type == config.SDE:
            string += 'SDE'
        else:
            string += 'Unknown '
        string += ', located at: ' + base_dir
        msg.PrintMsg(string)

    # Add the appropriate 'home' directory for the kit to the command line.  If
    # user has given 'home' directory, then use it instead.  
    #
    kit_knob = ''
    if kit_type == config.PINPLAY:
        if config.pinplayhome:
            kit_knob += ' --pinplayhome ' + config.pinplayhome
        else:
            kit_knob += ' --pinplayhome ' + base_dir
        kit_script_path = os.path.join(base_dir, config.pin_script_path)
    elif kit_type == config.SDE:
        script = 'sde_' + script
        if config.sdehome:
            kit_knob += ' --sdehome ' + config.sdehome
        else:
            kit_knob += ' --sdehome ' + base_dir
        kit_script_path = os.path.join(base_dir, config.sde_script_path)
    else:
        # Should never get here as error checking already taken care of in
        # GetKitType()
        #
        pass
    if (hasattr(options, 'verbose') and options.verbose):
        msg.PrintMsg('Kit knob actually used: ' + kit_knob)

    # Add the required Pin/SDE knobs, any additional knobs and the user defined
    # 'pin_options'.
    #
    cmd = script
    if kit_type == config.PINPLAY:
        popts = '-follow_execv'
    elif kit_type == config.SDE:
        popts = '-p -follow_execv'
    popts += pin_knobs
    if hasattr(options, 'pin_options') and options.pin_options:
        popts += ' ' + options.pin_options
    cmd += ' --pin_options "%s"' % (popts)
    cmd += kit_knob

    return cmd, kit_script_path


def GdbBaseLogOpt():
    """
    Get the DrDebug specific logging options.  When using GDB, there are additional
    knobs required in addition to the basic logging knobs.

    NOTE: SDE requires the pintool to be explicitly given on the command line when
    runnning with GDB.  Normally, the pintool isn't required for SDE.

    @return string with knobs for logging
    """

    kit_type, base_dir = util.GetKitType()
    if kit_type == config.PINPLAY:
        kit_knobs = ' -log:controller_default_start 0'
    else:
        kit_knobs = ' -t sde-pinplay-driver.so  -controller_default_start 0'
    log_opts = config.drdebug_base_log_options + kit_knobs + \
               ' -gdb:cmd_file ' + config.gdb_cmd_file

    return log_opts


def PintoolHelpCmd(cmd, options):
    """
    Print the pintool help msg using the script given in the
    string 'cmd'.

    @return exit code from running cmd
    """

    # Do we want to print out the command executed by util.RunCmd() or
    # just run silently?
    #
    if options.verbose or options.debug:
        pcmd = True
    else:
        pcmd = False
    cmd += ' --pintool_help'
    result = util.RunCmd(cmd, options, '',
                         concurrent=False,
                         print_time=False,
                         print_cmd=pcmd)
    return result


def AddPinKnobsGDB():
    """
    Add pin knobs (not pintool knobs) needed when running with GDB.

    @return string of knobs
    """

    kit_type, base_dir = util.GetKitType()

    pin_knobs = ''
    if kit_type == config.PINPLAY:
        pin_knobs += ' -appdebug'
    elif kit_type == config.SDE:
        pin_knobs += ' -p -appdebug'

    return pin_knobs


def FinalizeNoGDB(kit_script_path, options):
    """
    Execute the final section of the program.  This version is for
    the scripts which don't use GDB.   If user gave option '--pid'
    then wait until the process exits.  Otherwise, just return.

    @return no return
    """

    # If attaching to a process, spin here until the process has finished
    # running.
    #
    if (hasattr(options, 'pid') and options.pid):
        msg.PrintMsg('Waiting for process to exit, PID: ' + str(options.pid))
        while util.CheckPID(options.pid):
            if (hasattr(options, 'verbose') and options.verbose):
                msg.PrintMsgNoCR('.')
            time.sleep(5)
        if (hasattr(options, 'verbose') and options.verbose):
            msg.PrintMsg('\nProcess has exited')
        else:
            msg.PrintMsg('Process has exited')

    if (hasattr(options, 'verbose') and options.verbose):
        if hasattr(options, 'log_file') and options.log_file:
            msg.PrintMsg('Output pinball: ' + options.log_file)
        else:
            msg.PrintMsg('Output pinball: default file name (pinball/log)')

    return


def FinalizeGDB(kit_script_path, options):
    """
    Execute the final section of the program.  Look in the GDB command output
    file for the string 'target remote'.  Timeout if not found in a reasonable
    amount of time.  Then add some more commands to the GDB command file
    and run GDB.

    @return exit code from running GDB
    """

    # Look for the string 'target remote' in the GDB command file.  Time out
    # if not found within a limited amount of time.
    #
    target_str = 'target remote'
    timeout = 30
    count = 0
    found = False
    while count < timeout:
        with open(config.gdb_cmd_file, 'r') as gdb_file:
            if target_str in gdb_file.read():
                found = True
                break
        time.sleep(1)
        if hasattr(options, 'verbose') and options.verbose:
            msg.PrintMsg('Waiting for "target remote"')
        count += 1
    found = True
    if not found:
        msg.PrintAndExit('Unable to find GDB string \'%s\' in file %s' %
                         (target_str, config.gdb_cmd_file))
    time.sleep(1)
    if hasattr(options, 'verbose') and options.verbose:
        with open(config.gdb_cmd_file, 'r') as gdb_file:
            msg.PrintMsg('Target cmd:  ' + gdb_file.read())

    # Write some control info and command to load Pin Python file to the GDB
    # command file. Set PYTHONPATH to the location of the scripts.
    #
    # import pdb;  pdb.set_trace()
    gdb_file = open(config.gdb_cmd_file, 'a')
    gdb_file.write('set remotetimeout 30000\n')
    pin_python = os.path.join(kit_script_path, 'pin.py')
    gdb_file.write('source %s\n' % (pin_python))
    gdb_file.close()
    time.sleep(1)
    os.environ["PYTHONPATH"] = kit_script_path

    # Run GDB
    #
    cmd = '%s --command=%s %s' % (gdb_path, config.gdb_cmd_file, options.command)
    if hasattr(options, 'verbose') and options.verbose:
        msg.PrintMsg(cmd)
    if options.verbose or options.debug:
        pcmd = True
    else:
        pcmd = False
    result = util.RunCmd(cmd, options, '',
                         concurrent=False,
                         print_time=False,
                         print_cmd=pcmd)
    return result


def RunScriptBack(cmd, options):
    """
    When the script in the background, not the foreground.

    @return error code from invoking the script (not result of running it)
    """

    # Do we want to print out the command executed by util.RunCmd() or
    # just run silently?
    #
    if options.verbose or options.debug:
        pcmd = True
    else:
        pcmd = False

    result = 0
    if not options.debug:
        result = util.RunCmd(cmd, options, '',
                             concurrent=True,
                             print_time=False,
                             print_cmd=pcmd)
    return result


def RunScriptFore(cmd, options):
    """
    Run the script in the foreground.

    @return error code from running script
    """

    # Do we want to print out the command executed by util.RunCmd() or
    # just run silently?
    #
    if options.verbose or options.debug:
        pcmd = True
    else:
        pcmd = False

    result = 0
    if not options.debug:
        result = util.RunCmd(cmd, options, '',
                             concurrent=False,
                             print_time=False,
                             print_cmd=pcmd)
    return result

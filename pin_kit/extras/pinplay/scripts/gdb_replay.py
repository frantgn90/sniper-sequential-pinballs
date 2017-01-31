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
#
# $Id: gdb_replay.py,v 1.2 2015/05/19 16:11:36 tmstall Exp tmstall $

import sys
import os
import subprocess
import optparse
import re
import time

# Local modules
#
import cmd_options
import config
import drd_util
import msg
import replay
import util


class GdbReplay(replay.Replay):
    """
    Replay DrDebug log files inside GDB.
    """

    def GetHelpParams(self):
        """
        Get the application specific usage and description strings for this module.

        @return tuple with:
        @return - usage string
        @return - description string
        """

        usage = '%prog [options] -- pinball_basename program_binary'
        desc = 'Replay a pinball using GDB'

        return usage, desc

    def GetArgs(self, args, parser, options):
        """
        Get pinball to replay and corresponding binary needed to replay pinball
        with GDB.

        @return tuple with:
        @return - old pinball to replay
        @return - new pinball to generate
        @return NOTE: side effect adds command line to 'options' dictionary.
        """

        num_args = len(args)
        if num_args != 2:
            parser.error(
                'Must give \'--\' followed by pinball_basename and program_binary')
        setattr(options, 'command', args[1])
        replay_pb = args[0]

        # For GDB, there is no name for the new pinball which will be created, hence return 'none'.
        #
        log_pb = None

        return replay_pb, log_pb

    def AddPinKnobs(self):
        """
        Add pin knobs (not pintool knobs) needed when running with GDB.

        @return string of knobs
        """

        return drd_util.AddPinKnobsGDB()

    def Initialize(self):
        """
        Ensure the correct version of GDB is installed and initialize the GDB command file.

        @return no return
        """

        drd_util.GdbInitialize()

    def GetPintoolOptions(self, replay_pb, log_pb, options):
        """
        Add the default knobs required for replaying and any user defined options.

        Need '-log' knob in this GDB 'replay' script because the script is
        actually used for both replaying and relogging.  By adding this knob,
        the script can relog if the user enables it with the correct GDB pin
        command.

        @param replay_pb pinball to replay
        @param log_pb is not used
        @param options dictionary of user options

        @return string of options
        """

        replay_o = ' -log'  # No name for the new pinball, use the default name
        replay_o += drd_util.GdbBaseLogOpt()
        replay_o += util.AddMt(options, replay_pb)

        # Add any options user gave on command line.
        #
        if hasattr(options, 'pintool_options') and options.pintool_options:
            replay_o = ' ' + options.pintool_options
        replay_o = ' --replay_options "%s"' % (replay_o)

        return replay_o

    def RunScript(self, cmd, options):
        """
        When running with GDB, run the script in the background, not the foreground.

        @return error code from running script
        """

        return drd_util.RunScriptBack(cmd, options)

    def Finalize(self, kit_script_path, options):
        """
        Run code required to complete the GDB scripts.

        @return error code
        """

        return drd_util.FinalizeGDB(kit_script_path, options)


def main():
    """
    This method allows the script to be run in stand alone mode.

    @return Exit code from running the script
    """

    gdb_replay = GdbReplay()
    result = gdb_replay.Run()
    return result

# If module is called in stand along mode, then run it.
#
if __name__ == "__main__":
    result = main()
    sys.exit(result)

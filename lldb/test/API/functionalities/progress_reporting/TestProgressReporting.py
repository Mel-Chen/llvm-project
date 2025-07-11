"""
Test that we are able to broadcast and receive progress events from lldb
"""
import lldb

import lldbsuite.test.lldbutil as lldbutil

from lldbsuite.test.lldbtest import *


class TestProgressReporting(TestBase):
    def setUp(self):
        TestBase.setUp(self)
        self.broadcaster = self.dbg.GetBroadcaster()
        self.listener = lldbutil.start_listening_from(
            self.broadcaster, lldb.SBDebugger.eBroadcastBitProgress
        )

    def test_wait_attach_progress_reporting(self):
        """Test that progress reports for wait attaching work as intended."""
        target = self.dbg.CreateTarget(None)

        # The waiting to attach progress message will get emitted upon
        # trying to attach, but it's not going to be the event picked
        # up by checking with fetch_next_event, so go through all emitted
        # progress events and check that the waiting to attach one was emitted at all.
        target.AttachToProcessWithName(
            self.listener,
            "wait-attach-progress-report",
            False,
            lldb.SBError(),
        )
        event = lldb.SBEvent()
        events = []
        while self.listener.GetNextEventForBroadcaster(self.broadcaster, event):
            progress_data = lldb.SBDebugger.GetProgressDataFromEvent(event)
            message = progress_data.GetValueForKey("message").GetStringValue(100)
            events.append(message)
        self.assertTrue("Waiting to attach to process" in events)

    def test_dwarf_symbol_loading_progress_report(self):
        """Test that we are able to fetch dwarf symbol loading progress events"""
        self.build()

        lldbutil.run_to_source_breakpoint(self, "break here", lldb.SBFileSpec("main.c"))

        event = lldbutil.fetch_next_event(self, self.listener, self.broadcaster)
        ret_args = lldb.SBDebugger.GetProgressFromEvent(event)
        self.assertGreater(len(ret_args), 0)
        message = ret_args[0]
        self.assertGreater(len(message), 0)

    def test_dwarf_symbol_loading_progress_report_structured_data(self):
        """Test that we are able to fetch dwarf symbol loading progress events
        using the structured data API"""
        self.build()

        lldbutil.run_to_source_breakpoint(self, "break here", lldb.SBFileSpec("main.c"))

        event = lldbutil.fetch_next_event(self, self.listener, self.broadcaster)
        progress_data = lldb.SBDebugger.GetProgressDataFromEvent(event)
        message = progress_data.GetValueForKey("message").GetStringValue(100)
        self.assertGreater(len(message), 0)
        details = progress_data.GetValueForKey("details").GetStringValue(100)
        self.assertGreater(len(details), 0)

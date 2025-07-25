import lldb
from lldbsuite.test.decorators import *
from lldbsuite.test.lldbtest import *
from lldbsuite.test import lldbutil


class GenericOptionalDataFormatterTestCase(TestBase):
    def do_test_with_run_command(self):
        """Test that that file and class static variables display correctly."""

        # This is the function to remove the custom formats in order to have a
        # clean slate for the next test case.
        def cleanup():
            self.runCmd("type format clear", check=False)
            self.runCmd("type summary clear", check=False)
            self.runCmd("type filter clear", check=False)
            self.runCmd("type synth clear", check=False)

        self.addTearDownHook(cleanup)

        self.runCmd("file " + self.getBuildArtifact("a.out"), CURRENT_EXECUTABLE_SET)

        bkpt = self.target().FindBreakpointByID(
            lldbutil.run_break_set_by_source_regexp(self, "break here")
        )

        self.runCmd("run", RUN_SUCCEEDED)

        # The stop reason of the thread should be breakpoint.
        self.expect(
            "thread list",
            STOPPED_DUE_TO_BREAKPOINT,
            substrs=["stopped", "stop reason = breakpoint"],
        )

        self.runCmd("frame variable has_optional")

        output = self.res.GetOutput()

        ## The variable has_optional tells us if the test program
        ## detected we have a sufficient libc++ version to support optional
        ## false means we do not and therefore should skip the test
        if output.find("(bool) has_optional = false") != -1:
            self.skipTest("Optional not supported")

        lldbutil.continue_to_breakpoint(self.process(), bkpt)

        self.expect("frame variable number_not_engaged", substrs=["Has Value=false"])

        self.expect(
            "frame variable number_engaged",
            substrs=["Has Value=true", "Value = 42", "}"],
        )

        self.expect(
            "frame var numbers",
            substrs=[
                "(optional_int_vect) numbers =  Has Value=true  {",
                "Value = size=4 {",
                "[0] = 1",
                "[1] = 2",
                "[2] = 3",
                "[3] = 4",
                "}",
                "}",
            ],
        )

        self.expect(
            "frame var ostring",
            substrs=[
                "(optional_string) ostring =  Has Value=true  {",
                'Value = "hello"',
                "}",
            ],
        )

        self.expect_var_path("*number_engaged", value="42")
        self.expect_var_path("*x", children=[ValueCheck(name="x", value="42")])
        self.expect_var_path("x->x", value="42")

        # The error message could use some improvement, but at least we can
        # check we don't crash.
        self.expect(
            "frame variable *number_not_engaged",
            error=True,
            substrs=["dereference failed: not a pointer, reference or array type"],
        )

    @add_test_categories(["libc++"])
    ## Clang 7.0 is the oldest Clang that can reliably parse newer libc++ versions
    ## with -std=c++17.
    @skipIf(
        oslist=no_match(["macosx"]), compiler="clang", compiler_version=["<", "7.0"]
    )
    ## We are skipping gcc version less that 5.1 since this test requires -std=c++17
    @skipIf(compiler="gcc", compiler_version=["<", "5.1"])
    def test_with_run_command_libcpp(self):
        self.build(dictionary={"USE_LIBCPP": 1})
        self.do_test_with_run_command()

    @add_test_categories(["libstdcxx"])
    ## Clang 7.0 is the oldest Clang that can reliably parse newer libc++ versions
    ## with -std=c++17.
    @skipIf(compiler="clang", compiler_version=["<", "7.0"])
    ## We are skipping gcc version less that 5.1 since this test requires -std=c++17
    @skipIf(compiler="gcc", compiler_version=["<", "5.1"])
    def test_with_run_command_libstdcpp(self):
        self.build(dictionary={"USE_LIBSTDCPP": 1})
        self.do_test_with_run_command()

    @add_test_categories(["msvcstl"])
    def test_with_run_command_msvcstl(self):
        # No flags, because the "msvcstl" category checks that the MSVC STL is used by default.
        self.build()
        self.do_test_with_run_command()

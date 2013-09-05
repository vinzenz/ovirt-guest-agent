@echo off
REM Run unittests for Windows

set PYTHONPATH=%PYTHONPATH%;../ovirt-guest-agent;.;
python testrunner.py win32_guest_agent_test.py encoding_test.py

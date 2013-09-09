#! /usr/bin/env python
# -*- coding: utf-8 -*-
# vim:fenc=utf-8


from ConfigParser import ConfigParser

from GuestAgentWin32 import WinVdsAgent
from message_validator import MessageValidator
from testrunner import GuestAgentTestCase

import test_port


class TestPortWriteBuffer(test_port.TestPort):
    def __init__(self, vport_name, *args, **kwargs):
        test_port.TestPort.__init__(self, vport_name, *args, **kwargs)
        self._buffer = ""

    def write(self, buffer):
        self._buffer = self._buffer + buffer
        return len(buffer)

    def read(self, size):
        return ""


class WindowsFunctionalTest(GuestAgentTestCase):
    def setUp(self):
        self._vport_name = "windows-functional-test-port"
        self._validator = MessageValidator(self._vport_name)
        self._vport = self._validator.port()
        test_port.add_test_port(self._vport_name, self._vport)
        self._config = ConfigParser()
        self._config.add_section('general')
        self._config.set('general', 'heart_beat_rate', '5')
        self._config.set('general', 'report_user_rate', '10')
        self._config.set('general', 'report_application_rate', '120')
        self._config.set('general', 'report_disk_usage', '300')
        self._config.add_section('virtio')
        self._config.set('virtio', 'device', 'vport_name')
        self.vdsAgent = WinVdsAgent(self._config)

    def testSendInfo(self):
        self._validator.verifySendInfo(self.vdsAgent)

    def testSendAppList(self):
        self._validator.verifySendAppList(self.vdsAgent)

    def testSendDisksUsages(self):
        self._validator.verifySendDisksUsages(self.vdsAgent)

    def testSendMemoryStats(self):
        self._validator.verifySendMemoryStats(self.vdsAgent)

    def testSendFQDN(self):
        self._validator.verifySendFQDN(self.vdsAgent)

    def testSendUserInfo(self):
        self._validator.verifySendUserInfo(self.vdsAgent)

    def testSessionLogon(self):
        self._validator.verifySessionLogon(self.vdsAgent)

    def testSessionLogoff(self):
        self._validator.verifySessionLogon(self.vdsAgent)

    def testSessionLock(self):
        self._validator.verifySessionLock(self.vdsAgent)

    def testSessionUnlock(self):
        self._validator.verifySessionUnlock(self.vdsAgent)

    def testSessionStartup(self):
        self._validator.verifySessionStartup(self.vdsAgent)

    def testSessionShutdown(self):
        self._validator.verifySessionShutdown(self.vdsAgent)

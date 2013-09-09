#! /usr/bin/env python
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

import test_port


class TestPortWriteBuffer(test_port.TestPort):
    def __init__(self, vport_name, *args, **kwargs):
        test_port.TestPort.__init__(self, vport_name, *args, **kwargs)
        self._buffer = ''

    def write(self, buffer):
        self._buffer = self._buffer + buffer
        return len(buffer)

    def read(self, size):
        return ''

    def clear(self):
        self._buffer = ''


class MessageValidator(object):
    def __init__(self, vport_name):
        self._port = TestPortWriteBuffer(vport_name)

    def port(self):
        return self._port

    def verifySendInfo(self, agent):
        agent.sendInfo()
        self._port.clear()

    def verifySendAppList(self, agent):
        agent.sendAppList()
        self._port.clear()

    def verifySendDisksUsages(self, agent):
        agent.sendDisksUsages()
        self._port.clear()

    def verifySendMemoryStats(self, agent):
        agent.sendMemoryStats()
        self._port.clear()

    def verifySendUserInfo(self, agent):
        agent.sendUserInfo()
        self._port.clear()

    def verifySessionLogon(self, agent):
        agent.sessionLogon()
        self._port.clear()

    def verifySessionLogoff(self, agent):
        agent.sessionLogoff()
        self._port.clear()

    def verifySessionLock(self, agent):
        agent.sessionLock()
        self._port.clear()

    def verifySessionUnlock(self, agent):
        agent.sessionUnlock()
        self._port.clear()

    def verifySessionStartup(self, agent):
        agent.sessionUnlock()
        self._port.clear()

    def verifySessionShutdown(self, agent):
        agent.sessionUnlock()
        self._port.clear()

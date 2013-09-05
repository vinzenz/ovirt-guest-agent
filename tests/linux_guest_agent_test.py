#! /usr/bin/env python
# -*- coding: utf-8 -*-
# vim:fenc=utf-8


import test_port
from testrunner import GuestAgentTestCase
from GuestAgentLinux2 import LinuxVdsAgent
from ConfigParser import ConfigParser


class TestPortWriteBuffer(test_port.TestPort):
    def __init__(self, vport_name, *args, **kwargs):
        test_port.TestPort.__init__(self, vport_name, *args, **kwargs)
        self._buffer = ""

    def write(self, buffer):
        self._buffer = self._buffer + buffer
        return len(buffer)

    def read(self, size):
        return ""


class LinuxFunctionalTest(GuestAgentTestCase):
    def setUp(self):
        self._vport_name = "linux-functional-test-port"
        self._vport = TestPortWriteBuffer(self._vport_name)
        test_port.add_test_port(self._vport_name, self._vport)
        self._config = ConfigParser()
        self._config.add_section('general')
        self._config.set('general', 'applications_list',
                         'kernel ovirt-guest-agent xorg-x11-drv-qxl '
                         'linux-image xserver-xorg-video-qxl')
        self._config.set('general', 'ignored_fs',
                         'rootfs tmpfs autofs cgroup selinuxfs udev mqueue '
                         'nfds proc sysfs devtmpfs hugetlbfs rpc_pipefs devpts'
                         ' securityfs debugfs binfmt_misc fuse.gvfsd-fuse '
                         'fuse.gvfs-fuse-daemon fusectl usbfs')
        self._config.set('general', 'heart_beat_rate', '5')
        self._config.set('general', 'report_user_rate', '10')
        self._config.set('general', 'report_application_rate', '120')
        self._config.set('general', 'report_disk_usage', '300')
        self._config.add_section('virtio')
        self._config.set('virtio', 'device', 'vport_name')
        self.vdsAgent = LinuxVdsAgent(self._config)

    def printBuf(self):
        print self._vport._buffer
        self._vport._buffer = ""

    def testSendInfo(self):
        self.vdsAgent.sendInfo()
        self.printBuf()

    def testSendAppList(self):
        self.vdsAgent.sendAppList()
        self.printBuf()

    def testSendDisksUsages(self):
        self.vdsAgent.sendDisksUsages()
        self.printBuf()

    def testSendMemoryStats(self):
        self.vdsAgent.sendMemoryStats()
        self.printBuf()

    def testSendFQDN(self):
        self.vdsAgent.sendFQDN()
        self.printBuf()

    def testSendUserInfo(self):
        self.vdsAgent.sendUserInfo()
        self.printBuf()

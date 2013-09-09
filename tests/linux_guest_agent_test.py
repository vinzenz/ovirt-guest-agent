#! /usr/bin/env python
# -*- coding: utf-8 -*-
# vim:fenc=utf-8


from ConfigParser import ConfigParser

from message_validator import MessageValidator
from testrunner import GuestAgentTestCase

import test_port


def linux_setup_test(conf):
    port_name = 'linux-functional-test-port'
    conf.set('general', 'applications_list',
             'kernel ovirt-guest-agent xorg-x11-drv-qxl '
             'linux-image xserver-xorg-video-qxl')
    conf.set('general', 'ignored_fs',
             'rootfs tmpfs autofs cgroup selinuxfs udev mqueue '
             'nfds proc sysfs devtmpfs hugetlbfs rpc_pipefs devpts '
             'securityfs debugfs binfmt_misc fuse.gvfsd-fuse '
             'fuse.gvfs-fuse-daemon fusectl usbfs')
    return port_name


class FunctionalTest(GuestAgentTestCase):
    def setUp(self):
        self._config = ConfigParser()
        self._config.add_section('general')
        self._config.add_section('virtio')
        self._vport_name = linux_setup_test(self._config)

        self._validator = MessageValidator(self._vport_name)
        self._vport = self._validator.port()
        test_port.add_test_port(self._vport_name, self._vport)

        self._config.set('general', 'heart_beat_rate', '5')
        self._config.set('general', 'report_user_rate', '10')
        self._config.set('general', 'report_application_rate', '120')
        self._config.set('general', 'report_disk_usage', '300')
        self._config.set('virtio', 'device', self._vport_name)

        if
            from GuestAgentLinux2 import LinuxVdsAgent
            self.vdsAgent = LinuxVdsAgent(self._config)

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

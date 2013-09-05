#! /usr/bin/env python
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

from testrunner import GuestAgentTestCase as TestCaseBase
from VirtIoChannel import _filter_object


class EncodingTest(TestCaseBase):

    def testNonUnicodeKeyInput(self):
        non_unicode_key = {'non-unicode-key': u'unicode value'}
        self.assertRaises(TypeError, _filter_object, (non_unicode_key,))

    def testNonUnicodeValueInput(self):
        non_unicode_value = {u'unicode-key': 'unicode value'}
        self.assertRaises(TypeError, _filter_object, (non_unicode_value,))

    def testIllegalUnicodeInput(self):
        ILLEGAL_DATA = {u'foo': u'\x00data\x00test\uffff\ufffe\udc79\ud800'}
        EXPECTED = {u'foo': u'\ufffddata\ufffdtest\ufffd\ufffd\ufffd\ufffd'}
        self.assertEqual(EXPECTED, _filter_object(ILLEGAL_DATA))

    def testLlegalUnicodeInput(self):
        LEGAL_DATA = {u'foo': u'?data?test\U00010000'}
        self.assertEqual(LEGAL_DATA, _filter_object(LEGAL_DATA))

    def testIllegalUnicodeCharacters(self):
        INVALID = (u'\u0000', u'\ufffe', u'\uffff', u'\ud800', u'\udc79')
        for invchar in INVALID:
            self.assertEqual(u'\ufffd', _filter_object(invchar))

    def testLegalUnicodeCharacters(self):
        LEGAL = (u'\u2122', u'Hello World')
        for legalchar in LEGAL:
            self.assertEqual(legalchar, _filter_object(legalchar))

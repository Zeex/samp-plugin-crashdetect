#!/usr/bin/env python
#
# Copyright (c) 2013-2015 Zeex
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

import argparse
import re
import sys

class Register:
  def __init__(self, name, value):
    self._name = name
    self._value = value

  @property
  def name(self):
    return self._name

  @property
  def value(self):
    return self._value

class Module:
  def __init__(self, filename, start_address, end_address, full_path):
    self._filename = filename
    self._location = (start_address, end_address)
    self._full_path = full_path

  @property
  def filename(self):
    return self._filename

  @property
  def location(self):
    return self._location

  @property
  def path(self):
    return self._full_path

class CrashInfo:
  def __init__(self):
    self._version = None
    self._registers = []
    self._stack = []
    self._modules = []

  def set_version(self, version):
    self._version = version

  def get_version(self):
    return self._version

  def add_register(self, name, value):
    self._registers.append(Register(name, int(value, 16)))

  def get_registers(self):
    return self._registers

  def add_stack(self, value):
    self._stack.append(int(value, 16))

  def get_stack(self):
    return self._stack

  def add_module(self, filename, start_address, end_address, full_path):
    self._modules.append(Module(filename, int(start_address, 16),
                                int(end_address, 16), full_path))

  def get_modules(self):
    return self._modules

  def find_module(self, address):
    for module in self._modules:
      start, end = module.location
      if address >= start and address <= end:
        return module
    return None

  def get_call_stack(self):
    for address in self._stack:
      module = self.find_module(address)
      if module is not None:
        yield address, module

def search_register(name, string):
  match = re.search(r'%s: (?P<value>0x[0-9A-F]{8})' % name, string)
  if match is not None:
    return match.group('value')

def parse_crashinfo(file):
  crashinfo = CrashInfo()
  section = None
  for line in file.readlines():
    if line.startswith('SA-MP Server:'):
      section = None
    elif line.startswith('Registers:'):
      section = 'registers'
      continue
    elif line.startswith('Stack:'):
      section = 'stack'
      continue
    elif line.startswith('Loaded Modules:'):
      section = 'modules'
      continue
    if section is None:
      match = re.match(r'SA-MP Server: (?P<version>[0-9a-zA-Z\-\.]+)', line)
      if match is not None:
        crashinfo.set_version(match.group('version'))
    elif section is 'registers':
      if line.startswith('EAX:'):
        crashinfo.add_register('eax', search_register('EAX', line))
        crashinfo.add_register('ebx', search_register('EBX', line))
        crashinfo.add_register('ecx', search_register('ECX', line))
        crashinfo.add_register('edx', search_register('EDX', line))
      elif line.startswith('ESI:'):
        crashinfo.add_register('esi', search_register('ESI', line))
        crashinfo.add_register('edi', search_register('EDI', line))
        crashinfo.add_register('ebp', search_register('EBP', line))
        crashinfo.add_register('esp', search_register('ESP', line))
      elif line.startswith('EFLAGS'):
        crashinfo.add_register('eflags', search_register('EFLAGS', line))
    elif section is 'stack':
      if re.match(r'\+[0-9a-fA-F]{4}:', line) is not None:
        for word in line[6:].split():
          crashinfo.add_stack(word)
    elif section is 'modules':
      match = re.match(r'(?P<name>.+)\tA: (?P<start>0x[0-9A-F]{8}) - '\
                        '(?P<end>0x[0-9A-F]{8})\t\((?P<path>.+)\)', line)
      if match is not None:
        crashinfo.add_module(*match.groups())
  return crashinfo

def main(argv):
  arg_parser = argparse.ArgumentParser()
  arg_parser.add_argument('-f', '--file', default='crashinfo.txt',
                          help='set input file')
  arg_parser.add_argument('-v', '--version', action='store_true',
                          default=False, help='print server version')
  arg_parser.add_argument('-r', '--registers', action='store_true',
                          default=False, help='print registers')
  arg_parser.add_argument('-s', '--stack', action='store_true',
                          default=False, help='print raw stack')
  arg_parser.add_argument('-m', '--modules', action='store_true',
                          default=False, help='print loaded modules')
  arg_parser.add_argument('-c', '--callstack', action='store_true',
                          default=False, help='print call stack')
  arg_parser.add_argument('-a', '--all', action='store_true',
                          default=False, help='print all')
  args = arg_parser.parse_args(argv[1:])

  with open(args.file, 'r') as file:
    crashinfo = parse_crashinfo(file)
    if args.all or args.version:
      print('Server version: \n  %s' % crashinfo.get_version())
    if args.all or args.registers:
      print('Registers:')
      for reg in crashinfo.get_registers():
        print('  %s = %08x' % (reg.name, reg.value))
    if args.all or args.stack:
      print('Stack:')
      for word in crashinfo.get_stack():
        print('  %08x' % word)
    if args.all or args.modules:
      print('Modules:')
      for module in crashinfo.get_modules():
        start, end = module.location
        print('  %s [%08x, %08x] (%s)' % (module.filename, start, end,
                                          module.path))
    if args.all or args.callstack:
      print 'Call stack:'
      for address, module in crashinfo.get_call_stack():
        print('  %08x in %s' % (address, module.filename))

if __name__ == '__main__':
  main(sys.argv)

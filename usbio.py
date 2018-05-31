#!/usr/bin/env python3

import os
import sys
import hid
import time
import logging

from argparse import ArgumentParser
from IPython import embed

def usage():
  p = os.path.basename(sys.argv[0])
  print("""
{p} - Control Morphy USB-IO and compatible devices
Usage: {p} [options] [new-port-value]
Options:
  -V 0xVID: USB vendor id to look for
  -P 0xPID: USB product id to look for
  -n <N>: Use N-th device found
  -p <N>: operate only on pin #N (keep other pins as-is)
  -f (hex|bin|format-string): output in custom format
  -i: inverted logic mode
  -D [LOGLEVEL]: set verbose level
Example:
  $ sudo {p} 0xabcd
  0xABCD
  $ sudo {p} -f bin
  0b1010101111001101
  $ sudo {p} -p 1
  0
  $ sudo ./usbio.py -i 0x55AA
  0x55AA
  $ sudo ./usbio.py
  0xAA55
""".strip().format(**locals()))
  sys.exit(0)

def get_state(hd):
  ret = hd.read(4)
  val = (ret[3] << 8) | ret[1]
  return val

def set_state(hd, val):
  cmd = bytearray([0x01, 0xFF & val, 0x02, 0xFF & (val >> 8)])
  hd.write(cmd)
  time.sleep(0.3)

def main(opt):
  dev = hid.enumerate(opt.vid, opt.pid)[opt.nth]
  hd = hid.device()
  hd.open_path(dev['path'])

  if len(opt.args) > 0:
    val = int(opt.args[0], 0)

    # inverted mode
    if opt.invert:
      val ^= 0xFFFF

    # preserve current state in single-pin mode
    if opt.pin >= 0:
      curr = get_state(hd)
      mask = ((1 << opt.pin) ^ 0xFFFF) & 0xFFFF
      val = (curr & mask) | ((1 << opt.pin) if val else 0)

    # update state
    set_state(hd, val)

  # readback state
  val = get_state(hd)

  # inverted mode
  if opt.invert:
    val ^= 0xFFFF

  # show current pin/port status
  if opt.pin >= 0:
    print((val >> opt.pin) & 1)
  elif opt.format == 'bin':
    print("0b{0:016b}".format(val))
  elif opt.format == 'hex':
    print("0x{0:04X}".format(val))
  else:
    print(opt.format.format(val))

  hd.close()

def to_int(v):
  return int(v, 0)

if __name__ == '__main__' and '__file__' in globals():
  ap = ArgumentParser()
  ap.print_help = usage
  ap.add_argument('-D', '--debug', nargs='?', default='INFO')
  ap.add_argument('-i', '--invert', action='store_true')
  ap.add_argument('-f', '--format', default='hex')
  ap.add_argument('-p', '--pin', type=int, default=-1)
  ap.add_argument('-n', '--nth', type=int, default=0)
  ap.add_argument('-V', '--vid', type=to_int, default=0x0BFE)
  ap.add_argument('-P', '--pid', type=to_int, default=0x1003)
  ap.add_argument('args', nargs='*')

  opt = ap.parse_args()

  logging.basicConfig(level=eval('logging.' + opt.debug))

  main(opt)




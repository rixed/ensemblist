/*
Copyright (C) 2003 Cedric Cellier, Dominique Lavault

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#define digital_width 8
#define digital_height 384
unsigned char digital_bits[] = {
   0x38, 0x4c, 0x86, 0x86, 0x86, 0xfe, 0x86, 0x00, 0x7e, 0x86, 0x86, 0x7e,
   0x86, 0x86, 0x7e, 0x00, 0x7c, 0x86, 0x06, 0x06, 0x06, 0x86, 0x7c, 0x00,
   0x7e, 0x86, 0x86, 0x86, 0x86, 0x86, 0x7e, 0x00, 0xfe, 0x06, 0x06, 0x3e,
   0x06, 0x06, 0xfe, 0x00, 0xfe, 0x86, 0x06, 0x06, 0x1e, 0x06, 0x06, 0x00,
   0x7c, 0x86, 0x06, 0xe6, 0x86, 0x86, 0x7c, 0x00, 0x86, 0x86, 0x86, 0x86,
   0xfe, 0x86, 0x86, 0x00, 0x78, 0x30, 0x30, 0x30, 0x30, 0x30, 0x78, 0x00,
   0x78, 0x30, 0x30, 0x30, 0x30, 0x34, 0x3c, 0x18, 0x06, 0x26, 0x16, 0x0e,
   0x7e, 0x86, 0x86, 0x00, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0xfe, 0x00,
   0x86, 0xce, 0xb6, 0x86, 0x86, 0x86, 0x86, 0x00, 0x86, 0x8e, 0x96, 0xa6,
   0xc6, 0x86, 0x86, 0x00, 0x7c, 0x86, 0x86, 0x86, 0x86, 0x86, 0x7c, 0x00,
   0x7e, 0x86, 0x86, 0x86, 0x7e, 0x06, 0x06, 0x00, 0x7c, 0x86, 0x86, 0x86,
   0xa6, 0x46, 0xbc, 0x00, 0x7e, 0x86, 0x86, 0x86, 0x7e, 0x86, 0x86, 0x00,
   0x7c, 0x86, 0x06, 0x7c, 0x80, 0x86, 0x7c, 0x00, 0xfe, 0x10, 0x10, 0x10,
   0x10, 0x10, 0x10, 0x00, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x7c, 0x00,
   0x86, 0x86, 0x4c, 0x4c, 0x38, 0x38, 0x10, 0x00, 0x82, 0x82, 0x82, 0x44,
   0x54, 0x54, 0x28, 0x00, 0x82, 0x46, 0x2c, 0x18, 0x2c, 0x46, 0x82, 0x00,
   0x86, 0x86, 0x86, 0x7c, 0x10, 0x10, 0x10, 0x00, 0xfe, 0xc0, 0x60, 0x30,
   0x18, 0x0c, 0xfe, 0x00, 0x7c, 0x86, 0x8e, 0x96, 0xa6, 0xc6, 0x7c, 0x00,
   0x30, 0x38, 0x38, 0x30, 0x30, 0x30, 0x30, 0x00, 0x7c, 0x82, 0x80, 0x7c,
   0x06, 0x86, 0xfe, 0x00, 0x7e, 0xc0, 0xc0, 0x7c, 0xc0, 0xc0, 0x7e, 0x00,
   0x04, 0x04, 0x04, 0x34, 0xfc, 0x30, 0x30, 0x00, 0xfe, 0x86, 0x06, 0x7e,
   0x80, 0x86, 0x7c, 0x00, 0x7c, 0x86, 0x06, 0x7e, 0x86, 0x86, 0x7c, 0x00,
   0xfe, 0x86, 0x40, 0x20, 0x30, 0x30, 0x30, 0x00, 0x7c, 0x86, 0x86, 0x7c,
   0x86, 0x86, 0x7c, 0x00, 0x7c, 0x86, 0x86, 0x7c, 0x40, 0x20, 0x20, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10,
   0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00,
   0x10, 0x10, 0x20, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0,
   0x20, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x07, 0x08, 0x10, 0x10, 0x10,
   0x10, 0x10, 0x08, 0x07, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x88, 0x84, 0xb2,
   0x84, 0x88, 0xf0, 0x00, 0x1e, 0x22, 0x4a, 0x9e, 0x4a, 0x22, 0x1e, 0x00,
   0x10, 0x10, 0x10, 0x10, 0x7c, 0x38, 0x10, 0x00, 0x00, 0x00, 0x10, 0x00,
   0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00};

/*
 * Copyright 1997-2005 Sun Microsystems, Inc.  All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 USA or visit www.sun.com if you need additional information or
 * have any questions.
 *
 */

# include "incls/_precompiled.incl"
# include "incls/_symbolOop.cpp.incl"

bool symbolOopDesc::equals(const char* str, int len) const {
  int l = utf8_length();
  if (l != len) return false;
  while (l-- > 0) {
    if (str[l] != (char) byte_at(l))
      return false;
  }
  assert(l == -1, "we should be at the beginning");
  return true;
}

char* symbolOopDesc::as_C_string(char* buf, int size) const {
  if (size > 0) {
    int len = MIN2(size - 1, utf8_length());
    for (int i = 0; i < len; i++) {
      buf[i] = byte_at(i);
    }
    buf[len] = '\0';
  }
  return buf;
}

char* symbolOopDesc::as_C_string() const {
  int len = utf8_length();
  char* str = NEW_RESOURCE_ARRAY(char, len + 1);
  return as_C_string(str, len + 1);
}

char* symbolOopDesc::as_C_string_flexible_buffer(Thread* t,
                                                 char* buf, int size) const {
  char* str;
  int len = utf8_length();
  int buf_len = len + 1;
  if (size < buf_len) {
    str = NEW_RESOURCE_ARRAY(char, buf_len);
  } else {
    str = buf;
  }
  return as_C_string(str, buf_len);
}

void symbolOopDesc::print_symbol_on(outputStream* st) {
  st = st ? st : tty;
  int length = UTF8::unicode_length((const char*)bytes(), utf8_length());
  const char *ptr = (const char *)bytes();
  jchar value;
  for (int index = 0; index < length; index++) {
    ptr = UTF8::next(ptr, &value);
    if (value >= 32 && value < 127 || value == '\'' || value == '\\') {
      st->put(value);
    } else {
      st->print("\\u%04x", value);
    }
  }
}

jchar* symbolOopDesc::as_unicode(int& length) const {
  symbolOopDesc* this_ptr = (symbolOopDesc*)this;
  length = UTF8::unicode_length((char*)this_ptr->bytes(), utf8_length());
  jchar* result = NEW_RESOURCE_ARRAY(jchar, length);
  if (length > 0) {
    UTF8::convert_to_unicode((char*)this_ptr->bytes(), result, length);
  }
  return result;
}

const char* symbolOopDesc::as_klass_external_name(char* buf, int size) const {
  if (size > 0) {
    char* str    = as_C_string(buf, size);
    int   length = (int)strlen(str);
    // Turn all '/'s into '.'s (also for array klasses)
    for (int index = 0; index < length; index++) {
      if (str[index] == '/') {
        str[index] = '.';
      }
    }
    return str;
  } else {
    return buf;
  }
}

const char* symbolOopDesc::as_klass_external_name() const {
  char* str    = as_C_string();
  int   length = (int)strlen(str);
  // Turn all '/'s into '.'s (also for array klasses)
  for (int index = 0; index < length; index++) {
    if (str[index] == '/') {
      str[index] = '.';
    }
  }
  return str;
}

/*
 * Copyright 2001-2007 Sun Microsystems, Inc.  All Rights Reserved.
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
# include "incls/_numberSeq.cpp.incl"

AbsSeq::AbsSeq(double alpha) :
  _num(0), _sum(0.0), _sum_of_squares(0.0),
  _davg(0.0), _dvariance(0.0), _alpha(alpha) {
}

void AbsSeq::add(double val) {
  if (_num == 0) {
    // if the sequence is empty, the davg is the same as the value
    _davg = val;
    // and the variance is 0
    _dvariance = 0.0;
  } else {
    // otherwise, calculate both
    _davg = (1.0 - _alpha) * val + _alpha * _davg;
    double diff = val - _davg;
    _dvariance = (1.0 - _alpha) * diff * diff + _alpha * _dvariance;
  }
}

double AbsSeq::avg() const {
  if (_num == 0)
    return 0.0;
  else
    return _sum / total();
}

double AbsSeq::variance() const {
  if (_num <= 1)
    return 0.0;

  double x_bar = avg();
  double result = _sum_of_squares / total() - x_bar * x_bar;
  if (result < 0.0) {
    // due to loss-of-precision errors, the variance might be negative
    // by a small bit

    //    guarantee(-0.1 < result && result < 0.0,
    //        "if variance is negative, it should be very small");
    result = 0.0;
  }
  return result;
}

double AbsSeq::sd() const {
  double var = variance();
  guarantee( var >= 0.0, "variance should not be negative" );
  return sqrt(var);
}

double AbsSeq::davg() const {
  return _davg;
}

double AbsSeq::dvariance() const {
  if (_num <= 1)
    return 0.0;

  double result = _dvariance;
  if (result < 0.0) {
    // due to loss-of-precision errors, the variance might be negative
    // by a small bit

    guarantee(-0.1 < result && result < 0.0,
               "if variance is negative, it should be very small");
    result = 0.0;
  }
  return result;
}

double AbsSeq::dsd() const {
  double var = dvariance();
  guarantee( var >= 0.0, "variance should not be negative" );
  return sqrt(var);
}

NumberSeq::NumberSeq(double alpha) :
  AbsSeq(alpha), _maximum(0.0), _last(0.0) {
}

bool NumberSeq::check_nums(NumberSeq *total, int n, NumberSeq **parts) {
  for (int i = 0; i < n; ++i) {
    if (parts[i] != NULL && total->num() != parts[i]->num())
      return false;
  }
  return true;
}

NumberSeq::NumberSeq(NumberSeq *total, int n, NumberSeq **parts) {
  guarantee(check_nums(total, n, parts), "all seq lengths should match");
  double sum = total->sum();
  for (int i = 0; i < n; ++i) {
    if (parts[i] != NULL)
      sum -= parts[i]->sum();
  }

  _num = total->num();
  _sum = sum;

  // we do not calculate these...
  _sum_of_squares = -1.0;
  _maximum = -1.0;
  _davg = -1.0;
  _dvariance = -1.0;
}

void NumberSeq::add(double val) {
  AbsSeq::add(val);

  _last = val;
  if (_num == 0) {
    _maximum = val;
  } else {
    if (val > _maximum)
      _maximum = val;
  }
  _sum += val;
  _sum_of_squares += val * val;
  ++_num;
}


TruncatedSeq::TruncatedSeq(int length, double alpha):
  AbsSeq(alpha), _length(length), _next(0) {
  _sequence = NEW_C_HEAP_ARRAY(double, _length);
  for (int i = 0; i < _length; ++i)
    _sequence[i] = 0.0;
}

void TruncatedSeq::add(double val) {
  AbsSeq::add(val);

  // get the oldest value in the sequence...
  double old_val = _sequence[_next];
  // ...remove it from the sum and sum of squares
  _sum -= old_val;
  _sum_of_squares -= old_val * old_val;

  // ...and update them with the new value
  _sum += val;
  _sum_of_squares += val * val;

  // now replace the old value with the new one
  _sequence[_next] = val;
  _next = (_next + 1) % _length;

  // only increase it if the buffer is not full
  if (_num < _length)
    ++_num;

  guarantee( variance() > -1.0, "variance should be >= 0" );
}

// can't easily keep track of this incrementally...
double TruncatedSeq::maximum() const {
  if (_num == 0)
    return 0.0;
  double ret = _sequence[0];
  for (int i = 1; i < _num; ++i) {
    double val = _sequence[i];
    if (val > ret)
      ret = val;
  }
  return ret;
}

double TruncatedSeq::last() const {
  if (_num == 0)
    return 0.0;
  unsigned last_index = (_next + _length - 1) % _length;
  return _sequence[last_index];
}

double TruncatedSeq::oldest() const {
  if (_num == 0)
    return 0.0;
  else if (_num < _length)
    // index 0 always oldest value until the array is full
    return _sequence[0];
  else {
    // since the array is full, _next is over the oldest value
    return _sequence[_next];
  }
}

double TruncatedSeq::predict_next() const {
  if (_num == 0)
    return 0.0;

  double num           = (double) _num;
  double x_squared_sum = 0.0;
  double x_sum         = 0.0;
  double y_sum         = 0.0;
  double xy_sum        = 0.0;
  double x_avg         = 0.0;
  double y_avg         = 0.0;

  int first = (_next + _length - _num) % _length;
  for (int i = 0; i < _num; ++i) {
    double x = (double) i;
    double y =  _sequence[(first + i) % _length];

    x_squared_sum += x * x;
    x_sum         += x;
    y_sum         += y;
    xy_sum        += x * y;
  }
  x_avg = x_sum / num;
  y_avg = y_sum / num;

  double Sxx = x_squared_sum - x_sum * x_sum / num;
  double Sxy = xy_sum - x_sum * y_sum / num;
  double b1 = Sxy / Sxx;
  double b0 = y_avg - b1 * x_avg;

  return b0 + b1 * num;
}

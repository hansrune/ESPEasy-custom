/*

HLW8012

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <Arduino.h>
#include "HLW8012.h"

#include <GPIO_Direct_Access.h>

  #ifndef CORE_POST_3_0_0
    #ifdef ESP8266
      #define IRAM_ATTR ICACHE_RAM_ATTR
    #endif
  #endif


#define HLW8012_MINIMUM_SAMPLE_DURATION_USEC  1000000   // 1 sec
#define HLW8012_MAXIMUM_SAMPLE_DURATION_USEC  10000000  // 10 sec

void HLW8012_sample::reset() {
    count = 0;
    last_pulse_usec = 0;
    first_pulse_usec = 0;
    start_usec = micros();
}


HLW8012_sample::result_e HLW8012_sample::add() {
    const uint32_t now = micros();
    ++count;
    last_pulse_usec = now;
    if (first_pulse_usec == 0) {
        count = 0;
        first_pulse_usec = now;
        return HLW8012_sample::result_e::NotEnough;
    }

    return enoughData();
}

HLW8012_sample::result_e HLW8012_sample::enoughData() const
{
    const int32_t duration_since_start_usec(timeDiff(start_usec, last_pulse_usec));
    if (duration_since_start_usec >= HLW8012_MAXIMUM_SAMPLE_DURATION_USEC && count == 0) {
      return HLW8012_sample::result_e::Expired;
    } 
    const int32_t duration_usec(timeDiff(first_pulse_usec, last_pulse_usec));
    if (duration_usec >= HLW8012_MINIMUM_SAMPLE_DURATION_USEC && count > 0) {
        return HLW8012_sample::result_e::Enough;
    }
    return HLW8012_sample::result_e::NotEnough;
}


HLW8012_sample::result_e HLW8012_sample::getPulseFreq(float& pulsefreq) const
{
    const result_e res = enoughData();
    if (res == result_e::Enough) {
        const int32_t duration_usec(timeDiff(first_pulse_usec, last_pulse_usec));
        pulsefreq = count;
        pulsefreq /= duration_usec;
    } else {
        pulsefreq = 0.0f;
    }
    return res;
}



void HLW8012::begin(
    unsigned char cf_pin,
    unsigned char cf1_pin,
    unsigned char sel_pin,
    unsigned char currentWhen,
    unsigned long pulse_timeout
    ) {

    _cf_pin = cf_pin;
    _cf1_pin = cf1_pin;
    _sel_pin = sel_pin;
    _current_mode = currentWhen;
    _pulse_timeout = pulse_timeout;

    pinMode(_cf_pin, INPUT_PULLUP);
    pinMode(_cf1_pin, INPUT_PULLUP);
    pinMode(_sel_pin, OUTPUT);

    _calculateDefaultMultipliers();

    _mode = _current_mode;
    _current_sample.reset();
    digitalWrite(_sel_pin, _mode);
}

void HLW8012::setMode(hlw8012_mode_t mode) {
    const unsigned char newMode = (mode == MODE_CURRENT) ? _current_mode : 1 - _current_mode;
    if (_mode == newMode) {
      return;
    }
    if (newMode == _current_mode) {
        _current_sample.reset();
    } else {
        _voltage_sample.reset();
    }
    _mode = newMode;
    DIRECT_pinWrite(_sel_pin, _mode);
}

hlw8012_mode_t HLW8012::getMode() {
    return (_mode == _current_mode) ? MODE_CURRENT : MODE_VOLTAGE;
}

hlw8012_mode_t HLW8012::toggleMode() {
    hlw8012_mode_t new_mode = getMode() == MODE_CURRENT ? MODE_VOLTAGE : MODE_CURRENT;
    setMode(new_mode);
    return new_mode;
}

float HLW8012::getCurrent(bool &valid) {

    // Power measurements are more sensitive to switch offs,
    // so we first check if power is 0 to set _current to 0 too
    getActivePower(valid);
    if (_power == 0) {
        _current = 0;
        return _current;
    }
    if (valid) {
        const float voltage = getVoltage(valid);
        if (valid && voltage > 1) {
            _current = _power / voltage;
            return _current;
        }
    } 
    return getCF1Current(valid);
}

float HLW8012::getCF1Current(bool &valid) {
    _checkCF1Signal();

    float pulsefreq{};
    if (_current_sample.getPulseFreq(pulsefreq) == HLW8012_sample::result_e::Enough) {
      _current = pulsefreq * _current_multiplier / 2.0f;
      valid = true;
    } else {
      _current = 0.0f;
      valid = false;
    }
    return _current;
}


float HLW8012::getVoltage(bool &valid) {
    _checkCF1Signal();
    float pulsefreq{};
    if (_voltage_sample.getPulseFreq(pulsefreq) == HLW8012_sample::result_e::Enough) {
      _voltage = pulsefreq * _voltage_multiplier / 2.0f;
      valid = true;
    } else {
      _voltage = 0.0f;
      valid = false;
    }
    return _voltage;
}

float HLW8012::getActivePower(bool &valid) {
    _checkCFSignal();
    const long count_diff = (long) (_cf_pulse_count_total_prev[0] - _cf_pulse_count_total_prev[1]);
    const long time_diff_usec = (long) (_cf_pulse_count_total_prev_timestamp[0] - _cf_pulse_count_total_prev_timestamp[1]);
    if (count_diff <= 0 || time_diff_usec <= 0) {
        // Either 0 consumption, or volatile values may have been updated.
        _power = 0.0f;
        valid = false;
    } else {
        float energy = count_diff * _power_multiplier / 2.0f;
        _power = energy / static_cast<float>(time_diff_usec);
        valid = true;
    }
    return _power;
}

float HLW8012::getApparentPower(bool &valid) {
    bool valid_cur, valid_volt = false;
    float current = getCF1Current(valid_cur);
    if (!valid_cur) {
        current = getCurrent(valid_cur);
    }
    const float voltage = getVoltage(valid_volt);
    valid = valid_cur && valid_volt;
    return voltage * current;
}

float HLW8012::getReactivePower(bool &valid) {
    bool valid_active, valid_apparent = false;
    const float active = getActivePower(valid_active);
    const float apparent = getApparentPower(valid_apparent);
    valid = valid_active && valid_apparent;
    if (valid && (apparent > active)) {
        return sqrtf((apparent * apparent) - (active * active));
    } else {
        return 0.0f;
    }
}

float HLW8012::getPowerFactor(bool &valid) {
    bool valid_active, valid_apparent = false;
    const float active = getActivePower(valid_active);
    const float apparent = getApparentPower(valid_apparent);
    valid = valid_active && valid_apparent;
    if (!valid) return 0.0f;
    if (active > apparent) return 1.0f;
    if (apparent == 0) return 0.0f;
    return active / apparent;
}

float HLW8012::getEnergy() {
    /*
    Pulse count is directly proportional to energy:
    P = m*f (m=power multiplier, f = Frequency)
    f = N/t (N=pulse count, t = time)
    E = P*t = m*N  (E=energy)
    */
    const float pulse_count = _cf_pulse_count_total;
    return pulse_count * _power_multiplier / 1000000.0f / 2.0f;
}

void HLW8012::resetEnergy() {
    _cf_pulse_count_total = 0;
}

void HLW8012::expectedCurrent(float value) {
    bool valid = false;
    if (static_cast<int>(_current) == 0) getCurrent(valid);
    if (valid && static_cast<int>(_current) > 0) _current_multiplier *= (value / _current);
}

void HLW8012::expectedVoltage(float value) {
    bool valid = false;
    if (static_cast<int>(_voltage) == 0) getVoltage(valid);
    if (valid && static_cast<int>(_voltage) > 0) _voltage_multiplier *= (value / _voltage);
}

void HLW8012::expectedActivePower(float value) {
    bool valid = false;
    if (static_cast<int>(_power) == 0) getActivePower(valid);
    if (valid && static_cast<int>(_power) > 0) _power_multiplier *= (value / _power);
}

void HLW8012::resetMultipliers() {
    _calculateDefaultMultipliers();
}

void HLW8012::setResistors(float current, float voltage_upstream, float voltage_downstream) {
    if (voltage_downstream > 0) {
        if (current > 0.0f) {
          _current_resistor = current;
        }
        _voltage_resistor = (voltage_upstream + voltage_downstream) / voltage_downstream;
        _calculateDefaultMultipliers();
    }
}

unsigned long IRAM_ATTR HLW8012::filter(unsigned long oldvalue, unsigned long newvalue) {
    if (oldvalue == 0) {
        return newvalue;
    }

    oldvalue += 3 * newvalue;
    oldvalue >>= 2;
    return oldvalue;
}


void IRAM_ATTR HLW8012::cf_interrupt() {
  	++_cf_pulse_count_total;

    const unsigned long now = micros();
    const long time_since_cf_switch = (long) (now - _cf_switched);

    // CF pulses correlate with amount of energy used.
    // For power a frequency of 1Hz means around 12W
    // Keep track of last 2 timestamps + total counts covering a roughly constant interval.
    if (time_since_cf_switch > _pulse_timeout) {
        // When power consumption is really low,
        // extend the period upto 10 sec, so we can get as low as 1.2 Watt
        const bool canExtend = (time_since_cf_switch < (HLW8012_MAXIMUM_SAMPLE_DURATION_USEC)) && (_cf_pulse_count_total - _cf_pulse_count_total_prev[0]) < 3;
        if (!canExtend) {
            _cf_switched = now;
            _cf_pulse_count_total_prev[1] = _cf_pulse_count_total_prev[0];
            _cf_pulse_count_total_prev_timestamp[1] = _cf_pulse_count_total_prev_timestamp[0];
        }
        _cf_pulse_count_total_prev[0] = _cf_pulse_count_total;
        _cf_pulse_count_total_prev_timestamp[0] = now;
    }
}

void IRAM_ATTR HLW8012::cf1_interrupt() {
    const unsigned char mode = _mode;

    const auto res = (mode == _current_mode) 
        ? _current_sample.add()
        : _voltage_sample.add();
    
    if (res == HLW8012_sample::result_e::NotEnough) {
        return;
    }

    // switch to other mode.
    const unsigned char newMode = 1 - mode;
    if (newMode == _current_mode) {
        _current_sample.reset();
    } else {
        _voltage_sample.reset();
    }
    DIRECT_pinWrite_ISR(_sel_pin, newMode);
    _mode = newMode;
}

void HLW8012::_checkCFSignal() {
    const unsigned long now = micros();
    const long time_since_cf_switch = (long) (now - _cf_switched);

    // If we conclude here there was no switch triggered via interrupt callback, 
    // then we must conclude there was no new pulse and thus energy consumption was 0.
    if (time_since_cf_switch > (2 * HLW8012_MAXIMUM_SAMPLE_DURATION_USEC)) {
        _cf_pulse_count_total_prev[1] = _cf_pulse_count_total;
        _cf_pulse_count_total_prev[0] = _cf_pulse_count_total;
        _cf_pulse_count_total_prev_timestamp[1] = _cf_switched;
        _cf_pulse_count_total_prev_timestamp[0] = now;
        _cf_switched = now;
    }
}

void HLW8012::_checkCF1Signal() {
    const auto res = (_mode == _current_mode) 
        ? _current_sample.enoughData()
        : _voltage_sample.enoughData();
    if (res == HLW8012_sample::result_e::Expired) {
        toggleMode();
    }
}

// These are the multipliers for current, voltage and power as per datasheet
// These values divided by output period (in useconds) give the actual value
// For power a frequency of 1Hz means around 12W
// For current a frequency of 1Hz means around 15mA
// For voltage a frequency of 1Hz means around 0.5V
void HLW8012::_calculateDefaultMultipliers() {
    constexpr float current_factor = 1000000.0 * 512 * V_REF / 24.0 / F_OSC;
    _current_multiplier = current_factor / _current_resistor;

    constexpr float voltage_factor = 1000000.0 * 512 * V_REF / 2.0 / F_OSC;
    _voltage_multiplier = voltage_factor * _voltage_resistor;

    constexpr float power_factor = 1000000.0 * 128 * V_REF * V_REF / 48.0 / F_OSC;
    _power_multiplier = power_factor * _voltage_resistor / _current_resistor;
}

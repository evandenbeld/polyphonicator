/*
 *    Polyphonicator
 *    Copyright (C) 2020  Erwin van den Beld
 *
 *    Inspired by https://github.com/elkayem/midi2cv/
 *    MIDI2CV
 *    Copyright (C) 2017  Larry McGovern
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License <http://www.gnu.org/licenses/> for more details.
 *
 */

#include <SPI.h>
#include "note.h"

#define ARRAY_LENGTH(array) (sizeof((array))/sizeof((array)[0]))

#define DAC1_PIN  8
#define DAC2_PIN  9
#define DAC_CHANNEL_A 0x1000
#define DAC_CHANNEL_B 0x9000
#define DAC_GAIN 0x2000
#define DAC_DATA_BITS_MASK 0x0FFF
#define DAC_SPEED_LIMIT 8000000

//This value can be tuned if CV output isn't exactly 1V/octave
#define NOTE_2_MILLIVOLTS_FACTOR 47.069f

#define GATE1_PIN A1
#define GATE2_PIN A2
#define GATE3_PIN A3
#define GATE4_PIN A4

Note notes[] = {
    {GATE1_PIN, DAC1_PIN, DAC_CHANNEL_A, 0, false},
    {GATE2_PIN, DAC1_PIN, DAC_CHANNEL_B, 0, false},
    {GATE3_PIN, DAC2_PIN, DAC_CHANNEL_A, 0, false},
    {GATE4_PIN, DAC2_PIN, DAC_CHANNEL_B, 0, false}
};

void setup() {
    for (int i = 0; i < ARRAY_LENGTH(notes); i++) {
        pinMode(notes[i].gatePin, OUTPUT);
        digitalWrite(notes[i].gatePin,LOW);
        pinMode(notes[i].dacPin, OUTPUT);
        digitalWrite(notes[i].dacPin,HIGH);
    }

    SPI.begin();

    //FIXME  MIDI.begin(MIDI_CHANNEL_OMNI);
}

void loop() {
    notes[0].midiNote = 8;
    startNote(notes[0]);
    delay(2000);
    notes[1].midiNote = 40;
    startNote(notes[1]);
    delay(2000);

    notes[0].midiNote = 87;
    stopNote(notes[0]);
    stopNote(notes[1]);
    delay(2000);
}


void startNote(Note note) {
    note.notePlayed = true;

    digitalWrite(note.gatePin, HIGH);
    applyNoteCV(note);
}

inline void applyNoteCV(Note note)
{
    unsigned int noteMilliVolts = (unsigned int) ((float) note.midiNote * NOTE_2_MILLIVOLTS_FACTOR + 0.5);

    unsigned int command = note.dacChannel;
    command |= DAC_GAIN;
    command |= (noteMilliVolts & DAC_DATA_BITS_MASK);

    SPI.beginTransaction(SPISettings(DAC_SPEED_LIMIT, MSBFIRST, SPI_MODE0));
    digitalWrite(note.dacPin,LOW);
    SPI.transfer(command>>8);
    SPI.transfer(command&0xFF);
    digitalWrite(note.dacPin,HIGH);
    SPI.endTransaction();
}

void stopNote(Note note) {
    note.notePlayed = false;

    digitalWrite(note.gatePin, LOW);
}

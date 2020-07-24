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

#define DAC1_PIN  8
#define DAC2_PIN  9
#define DAC_CHANNEL_A 0x1000
#define DAC_CHANNEL_B 0x9000
#define DAC_GAIN 0x2000
#define NOTE_SF 47.069f //FIXME naming: This value can be tuned if CV output isn't exactly 1V/octave

#define GATE1_PIN A1
#define GATE2_PIN A2
#define GATE3_PIN A3
#define GATE4_PIN A4
#define GATE_OR_PIN A5

Note notes[] = {
    {GATE1_PIN, DAC1_PIN, DAC_CHANNEL_A, 0, false},
    {GATE2_PIN, DAC1_PIN, DAC_CHANNEL_B, 0, false},
    {GATE3_PIN, DAC2_PIN, DAC_CHANNEL_A, 0, false},
    {GATE4_PIN, DAC2_PIN, DAC_CHANNEL_B, 0, false}
};

void setup() {
    for (int i = 0; i < 4; i++) {
        pinMode(notes[i].gatePin, OUTPUT);
        digitalWrite(notes[i].gatePin,LOW);
        pinMode(notes[i].dacPin, OUTPUT);
        digitalWrite(notes[i].dacPin,HIGH);
    }

    pinMode(GATE_OR_PIN, OUTPUT);
    digitalWrite(GATE_OR_PIN, LOW);

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
    digitalWrite(GATE_OR_PIN, HIGH);

    applyNoteCV(note);
}

inline void applyNoteCV(Note note)
{
    //FIXME no magic here, but command byte and volts seperate
    unsigned int noteMilliVolts = (unsigned int) ((float) note.midiNote * NOTE_SF + 0.5);

    unsigned int command = note.dacChannel;
    command |= DAC_GAIN;
    command |= (noteMilliVolts & 0x0FFF);

    SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
    digitalWrite(note.dacPin,LOW);
    SPI.transfer(command>>8);
    SPI.transfer(command&0xFF);
    digitalWrite(note.dacPin,HIGH);
    SPI.endTransaction();
}

void stopNote(Note note) {
    note.notePlayed = false;
    digitalWrite(note.gatePin, LOW);
    if(isLastNote())
    {
        digitalWrite(GATE_OR_PIN, LOW);
    }
}

bool isLastNote()
{
    for (int i = 0; i < 4; i++) {
        if(notes[i].notePlayed)
        {
            return false;
        }
    }
    return true;
}

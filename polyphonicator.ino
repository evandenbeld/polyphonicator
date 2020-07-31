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
#include <MIDI.h>
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
// Midi message: A0 = 21, Top Note (88) = 108
#define MIDI_MSG_TO_NOTE_INDEX 21
#define MAXIMUM_NUMBER_OF_KEYS 88

#define GATE1_PIN A1
#define GATE2_PIN A2
#define GATE3_PIN A3
#define GATE4_PIN A4

MIDI_CREATE_DEFAULT_INSTANCE();

Note notes[] = {
    {GATE1_PIN, DAC1_PIN, DAC_CHANNEL_A, 0, false},
    {GATE2_PIN, DAC1_PIN, DAC_CHANNEL_B, 0, false},
    {GATE3_PIN, DAC2_PIN, DAC_CHANNEL_A, 0, false},
    {GATE4_PIN, DAC2_PIN, DAC_CHANNEL_B, 0, false}
};

void setup() {
    setupNotePins();

    SPI.begin();
    
    MIDI.begin(MIDI_CHANNEL_OMNI);
    MIDI.setHandleNoteOn(handleNoteOn);
    MIDI.setHandleNoteOff(handleNoteOff);
}

inline void setupNotePins() {
    for (int i = 0; i < ARRAY_LENGTH(notes); i++) {
        pinMode(notes[i].gatePin, OUTPUT);
        digitalWrite(notes[i].gatePin,LOW);
        pinMode(notes[i].dacPin, OUTPUT);
        digitalWrite(notes[i].dacPin,HIGH);
    }
}

void loop() {
  MIDI.read();
}

void handleNoteOn(byte channel, byte noteMsg, byte velocity) {
    if (correctNotePlayed(noteMsg)) {
        int noteIndex = getFirstAvailableNoteIndex();
        notes[noteIndex].midiNote = noteMsg;
        notes[noteIndex].notePlayed = true;

        digitalWrite(notes[noteIndex].gatePin, HIGH);
        applyNoteCV(notes[noteIndex]);
    }
}

inline bool correctNotePlayed(int noteMsg) {
    return noteMsg > 0 || noteMsg < MAXIMUM_NUMBER_OF_KEYS;
}

int getFirstAvailableNoteIndex()
{
    for(int i=0; i<ARRAY_LENGTH(notes); i++) {
        if(!notes[i].notePlayed)
        {
            return i;
        }
    }
    return 0;
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

void handleNoteOff(byte channel, byte noteMsg, byte velocity) {
    for(int i=0; i<ARRAY_LENGTH(notes); i++) {
        if(notes[i].midiNote == noteMsg && notes[i].notePlayed)
        {
           notes[i].notePlayed = false;
           digitalWrite(notes[i].gatePin, LOW);
        }
    }
}

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

#define DAC1_PIN  9
#define DAC2_PIN  8
#define DAC_CHANNEL_A 0x1000
#define DAC_CHANNEL_B 0x9000
#define DAC_GAIN 0x0000
#define DAC_DATA_BITS_MASK 0x0FFF
#define DAC_SPEED_LIMIT 8000000

//This value can be tuned if CV output isn't exactly 1V/octave
#define NOTE_2_MILLIVOLTS_FACTOR 47.069f
// Midi message: A0 = 21, Top Note (88) = 108
#define MIDI_MSG_TO_NOTE_INDEX 21
#define MAXIMUM_NUMBER_OF_KEYS 88

#define GATE1_PIN 10
#define GATE2_PIN 7
#define GATE3_PIN 6
#define GATE4_PIN 5
#define NOTE_PRIORITY_PIN_1 A0
#define NOTE_PRIORITY_PIN_2 A2

MIDI_CREATE_DEFAULT_INSTANCE();

Note notes[] = {
    {GATE1_PIN, DAC1_PIN, DAC_CHANNEL_A, 0},
    {GATE2_PIN, DAC1_PIN, DAC_CHANNEL_B, 0},
    {GATE3_PIN, DAC2_PIN, DAC_CHANNEL_A, 0},
    {GATE4_PIN, DAC2_PIN, DAC_CHANNEL_B, 0}
};

void setup() {
    setupNotePins();
    setupPriorityPins();

    SPI.begin();
    
    MIDI.begin(MIDI_CHANNEL_OMNI);
    MIDI.setHandleNoteOn(handleNoteOn);
    MIDI.setHandleNoteOff(handleNoteOff);
}

inline void setupNotePins() {
    for (int i = 0; i < 4; i++) {
        pinMode(notes[i].gatePin, OUTPUT);
        digitalWrite(notes[i].gatePin,LOW);
        pinMode(notes[i].dacPin, OUTPUT);
        digitalWrite(notes[i].dacPin,HIGH);
    }
}

inline void setupPriorityPins() {
    pinMode(NOTE_PRIORITY_PIN_1, INPUT_PULLUP);
    pinMode(NOTE_PRIORITY_PIN_2, INPUT_PULLUP);
}

void loop() {
  MIDI.read();
}

void handleNoteOn(byte channel, byte noteMsg, byte velocity) {
    byte note = noteMsg - MIDI_MSG_TO_NOTE_INDEX;
    if (correctNotePlayed(note)) {
        int noteIndex = getNoteIndex();
        if(noteIndex == -1) {
            return;
        }
        notes[noteIndex].midiNote = note;

        digitalWrite(notes[noteIndex].gatePin, HIGH);
        applyNoteCV(notes[noteIndex]);
    }
}

inline bool correctNotePlayed(int noteMsg) {
    return noteMsg > 0 || noteMsg < MAXIMUM_NUMBER_OF_KEYS;
}

int getNoteIndex()
{
    for(int i=0; i<4; i++) {
        if(notes[i].midiNote == 0)
        {
            return i;
        }
    }
    return getUnavailableNoteIndexByPriority();
}

int getUnavailableNoteIndexByPriority()
{
    bool switchUp = digitalRead(NOTE_PRIORITY_PIN_1);
    bool switchDown = digitalRead(NOTE_PRIORITY_PIN_2);

    if(switchUp && switchDown) {
       return -1;
    }
    if(!switchUp && switchDown) {
       return 0;
    }

    return 3;
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
    byte note = noteMsg - MIDI_MSG_TO_NOTE_INDEX;
    for(int i=0; i<4; i++) {
        if(notes[i].midiNote == note)
        {
           notes[i].midiNote = 0;
           digitalWrite(notes[i].gatePin, LOW);
        }
    }
}

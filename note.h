typedef struct Note {
    byte gatePin;
    byte dacPin;
    int dacChannel;
    int midiNote;
    bool notePlayed;
} Note;

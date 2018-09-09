#include <bitset>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>

using namespace std;

union CharUByte {
    char    c;
    uint8_t b;
};

union CharInt {
    char    c[4];
    int32_t i;
};

ifstream inFile;
CharUByte length1;
CharInt length4;

struct IntTextItem {
    int i;
    string text;
};

enum InfoType {Title, Subtitle, Artist, Album, WordsBy, MusicBy, Copyright, Tab, Instructions};
vector<IntTextItem> info;

struct IntIntTextItem {
    int i1;
    int i2;
    string text;
};
vector<IntIntTextItem> lyrics;
vector<IntIntTextItem> vTracks;
int iNoteCount = 0;

string readIntByteText() {
    string result;
    inFile.read(length4.c, 4);
    inFile.read(&length1.c, 1);
    if (length1.b > 0) {
        result.resize(length1.b);
        inFile.read((char *)&result[0], length1.b);
        return result;
    }
    else return "";
}

IntIntTextItem readIntIntText() {
    IntIntTextItem item;
    inFile.read(length4.c, 4); item.i1 = length4.i;
    inFile.read(length4.c, 4); item.i2 = length4.i;

    item.text.resize(item.i2);
    inFile.read((char*)&item.text[0], item.i2);

    return item;
}

string readByteText() {
    string result;
    inFile.read(&length1.c, 1);
    if (length1.b > 0) {
        result.resize(length1.b);
        inFile.read((char *)&result[0], length1.b);
        return result;
    }
    else return "";
}

int main(int argc,      // Number of strings in array argv
          char *argv[]) {  // Array of command-line argument strings
    if (argc < 2)
    {
        cout << "No file specified." << endl;
        return 0;
    }

    cout << "Reading file: " << argv[1] << endl;


    inFile.open(argv[1], ios::binary | ios::in);

    if (inFile.fail()) {
        cerr << "Can't open file: " << strerror(errno);
        return 1;
    }

    string sVersion = readByteText();
    cout << "File version: " << sVersion << endl;
    if (sVersion.compare("FICHIER GUITAR PRO v5.10") != 0)
    {
        cerr << "Unsupported." << endl;
        return 0;
    }
    inFile.seekg(30-length1.b,ios_base::cur);

    string sTitle = readIntByteText();
    string sSubtitle = readIntByteText();
    string sArtist = readIntByteText();
    string sAlbum = readIntByteText();
    string sWordsBy = readIntByteText();
    string sMusicBy = readIntByteText();
    string sCopyright = readIntByteText();
    string sTab = readIntByteText();
    string sInstructions = readIntByteText();

    IntTextItem infoItem;
    infoItem.i = 0; infoItem.text = sTitle; info.push_back(infoItem);
    infoItem.i = 1; infoItem.text = sSubtitle; info.push_back(infoItem);
    infoItem.i = 2; infoItem.text = sArtist; info.push_back(infoItem);
    infoItem.i = 3; infoItem.text = sAlbum; info.push_back(infoItem);
    infoItem.i = 4; infoItem.text = sWordsBy; info.push_back(infoItem);
    infoItem.i = 5; infoItem.text = sMusicBy; info.push_back(infoItem);
    infoItem.i = 6; infoItem.text = sCopyright; info.push_back(infoItem);
    infoItem.i = 7; infoItem.text = sTab; info.push_back(infoItem);
    infoItem.i = 8; infoItem.text = sInstructions; info.push_back(infoItem);

    printf("Artist: %s\nTitle: %s\n",
           info[Artist].text.c_str(),
           info[Title].text.c_str()
           );

    // Read comments
    inFile.read(length4.c, 4);
    int numComments = length4.i;
    string sComment;
    //cout << "Comments:" << endl;
    for (int i = 0; i < numComments; i++) {
        sComment = readIntByteText();
        //cout << sComment << endl;
    }

    //Read lyrics
    inFile.read(length4.c, 4);
    int lyricTrack = length4.i;
    IntIntTextItem lyric;
    //cout << "Lyrics:" << endl;
    for (int i = 0; i < 5; i++) {
        lyric = readIntIntText();
        //cout << lyric.text << endl;
        lyrics.push_back(lyric);
    }

    // skip some bytes (page setup checkboxes?)
    inFile.seekg(49,ios_base::cur);

    // Read page setup
    for (int i = 0; i < 11; i++) {
        inFile.seekg(4,ios_base::cur);
        string sTemp = readByteText();
    }

    inFile.read(length4.c, 4); int tempo = length4.i;
    cout << "Tempo: " << tempo << endl;

    inFile.seekg(1,ios_base::cur);

    inFile.read(length4.c, 4); int key = length4.i;             //cout << "Key: " << key << endl;
    inFile.read(&length1.c, 1); uint8_t octave = length1.b;     //cout << "Octave: " << +octave << endl;

    // Read MIDI data
    for (int i = 0; i < 64; i++) {
        inFile.seekg(4,ios_base::cur); // int - Instrument
        inFile.seekg(6,ios_base::cur); // 6 bytes - Volume Balance Chorus Reverb Phaser Tremolo
        inFile.seekg(2,ios_base::cur); // unknown
    }

    inFile.seekg(42,ios_base::cur);


    inFile.read(length4.c, 4); int measures = length4.i;         cout << "Measures: " << measures << endl;
    inFile.read(length4.c, 4); int tracks = length4.i;           cout << "Tracks: " << tracks << endl;


    // Read measure headers
    for (int i = 0; i < measures; i++) {
        if (i > 0) {
            inFile.seekg(1,ios_base::cur);
        }
        inFile.read(&length1.c, 1); uint8_t flags = length1.b;     //cout << "flags: " << std::bitset<8>(flags) << endl;

        if ((flags & 0x01) != 0) {
            inFile.read(&length1.c, 1); uint8_t numerator = length1.b;
        }
        if ((flags & 0x02) != 0) {
            inFile.read(&length1.c, 1); uint8_t denominator = length1.b;
        }
        if ((flags & 0x08) != 0) {
            inFile.read(&length1.c, 1); uint8_t repeatClose = length1.b;
        }
        if ((flags & 0x20) != 0) {
            string sMarker = readIntByteText();
            cout << "marker at " << i+1 << ": "<< sMarker << endl;
            inFile.seekg(3,ios_base::cur); // 3 bytes - RGB
            inFile.seekg(1,ios_base::cur); // unknown
        }
        if ((flags & 0x10) != 0) {
            inFile.read(&length1.c, 1); uint8_t repeatAlternative = length1.b;
        }
        if ((flags & 0x40) != 0) {
            inFile.seekg(2,ios_base::cur); // unknown
        }
        if ((flags & 0x01) != 0 || (flags & 0x02) != 0) {
            inFile.seekg(4,ios_base::cur); // unknown
        }
        if ((flags & 0x10) == 0) {
            inFile.seekg(1,ios_base::cur); // unknown
        }
        inFile.read(&length1.c, 1); uint8_t tripleFeel = length1.b;
    }

    // Read tracks
    IntIntTextItem track;

    int stringCount, tuning;
    for (int i = 1; i <= tracks; i++) {
        inFile.seekg(1,ios_base::cur); // unknown
        if (i == 1) {
            inFile.seekg(1,ios_base::cur); // unknown
        }

        inFile.read(&length1.c, 1); // track name string length
        track.text.resize(40);
        inFile.read((char*)&track.text[0], 40);
        cout << "  " << i << ". " << track.text << endl;

        inFile.read(length4.c, 4); stringCount = length4.i;
        track.i1 = stringCount;
        vTracks.push_back(track);
        for (int j = 0; j < 7; j++) {
            inFile.read(length4.c, 4); tuning = length4.i;
        }
        inFile.seekg(4,ios_base::cur); // unknown

        //read channel
        inFile.seekg(4,ios_base::cur); // int iIndex;
        inFile.seekg(4,ios_base::cur); // int iEffectChannel;
        inFile.seekg(4,ios_base::cur); // int iInt;
        inFile.seekg(4,ios_base::cur); // int iTrackOffset;
        inFile.seekg(3,ios_base::cur); // 3 bytes - RGB
        inFile.seekg(1,ios_base::cur); // unknown
        inFile.seekg(49,ios_base::cur); // unknown

        readIntByteText();
        readIntByteText();
    }
    inFile.seekg(1,ios_base::cur); // unknown

    //Read measures
    for (int i = 0; i < measures; i++) {
        for (int j = 0; j < tracks; j++) {
            //Read measure
            //printf("Measure: %d\n",i);
            for (int voice = 0; voice < 2; voice++) {
                inFile.read(length4.c, 4); int beats = length4.i;
                for (int k = 0; k < beats; k++) {
                    //Read beat
                    //printf("Beat: %d, voice: %d\n",k,voice);
                    inFile.read(&length1.c, 1); uint8_t flags = length1.b;     //cout << "flags: " << std::bitset<8>(flags) << endl;
                    if ((flags & 0x40) != 0) {
                        inFile.read(&length1.c, 1); uint8_t beatType = length1.b;
                    }
                    //Read duration
                    inFile.read(&length1.c, 1); uint8_t duration = length1.b;
                    if ((flags & 0x20) != 0) {
                        inFile.read(length4.c, 4); int divisionType = length4.i;
                    }
                    if ((flags & 0x02) != 0) {
                        //Read chord
                        inFile.seekg(17,ios_base::cur); // unknown

                        string sChordName; sChordName.resize(21);
                        inFile.read(&length1.c, 1); // chord name string length
                        inFile.read((char*)&sChordName[0], 21);
                        cout << "ChordName: " << sChordName << endl;

                        inFile.seekg(4,ios_base::cur); // unknown
                        inFile.read(length4.c, 4); int firstFret = length4.i;
                        for (int l = 0; l < 7; l++) {
                            inFile.read(length4.c, 4); int fret = length4.i;
                        }
                        inFile.seekg(32,ios_base::cur); // unknown
                    }
                    if ((flags & 0x04) != 0) {
                        //Read text
                        readIntByteText();
                    }
                    if ((flags & 0x08) != 0) {
                        //Read beat effects
                        inFile.read(&length1.c, 1); uint8_t flags1 = length1.b;     //cout << "flags1: " << std::bitset<8>(flags) << endl;
                        inFile.read(&length1.c, 1); uint8_t flags2 = length1.b;     //cout << "flags2: " << std::bitset<8>(flags) << endl;
                        if ((flags1 & 0x20) != 0) {
                            inFile.read(&length1.c, 1); uint8_t effect = length1.b;
                        }
                        if ((flags2 & 0x04) != 0) {
                            //Read tremolo bar
                            inFile.read(length4.c, 4); int numPoints = length4.i;
                            for (int l = 0; l < numPoints; l++) {
                                inFile.read(length4.c, 4); int tremoloPosition = length4.i;
                                inFile.read(length4.c, 4); int tremoloValue = length4.i;
                                inFile.seekg(1,ios_base::cur); // unknown
                            }
                        }
                        if ((flags1 & 0x40) != 0) {
                            inFile.read(&length1.c, 1); uint8_t strokeUp = length1.b;
                            inFile.read(&length1.c, 1); uint8_t strokeDown = length1.b;
                        }
                        if ((flags2 & 0x02) != 0) {
                            inFile.seekg(1,ios_base::cur); // unknown
                        }
                    }
                    if ((flags & 0x10) != 0) {
                        //Read Mix Change
                        inFile.seekg(1,ios_base::cur); // unknown
                        inFile.seekg(16,ios_base::cur); // unknown
                        inFile.read(&length1.c, 1); uint8_t bVolume = length1.b;
                        inFile.read(&length1.c, 1); uint8_t bPan = length1.b;
                        inFile.read(&length1.c, 1); uint8_t bChorus = length1.b;
                        inFile.read(&length1.c, 1); uint8_t bReverb = length1.b;
                        inFile.read(&length1.c, 1); uint8_t bPhaser = length1.b;
                        inFile.read(&length1.c, 1); uint8_t bTremolo = length1.b;

                        // tempoName
                        readIntByteText();
                        inFile.read(length4.c, 4); int tempoValue = length4.i;
                        if ((int8_t)bVolume >= 0) inFile.seekg(1,ios_base::cur); // unknown
                        if ((int8_t)bPan >= 0) inFile.seekg(1,ios_base::cur); // unknown
                        if ((int8_t)bChorus >= 0) inFile.seekg(1,ios_base::cur); // unknown
                        if ((int8_t)bReverb >= 0) inFile.seekg(1,ios_base::cur); // unknown
                        if ((int8_t)bPhaser >= 0) inFile.seekg(1,ios_base::cur); // unknown
                        if ((int8_t)bTremolo >= 0) inFile.seekg(1,ios_base::cur); // unknown

                        if (tempoValue >= 0) inFile.seekg(2,ios_base::cur); // unknown
                        inFile.seekg(2,ios_base::cur); // unknown
                        readIntByteText();
                        readIntByteText();
                    }

                    inFile.read(&length1.c, 1); uint8_t stringFlags = length1.b;
                    for (int m = 6; m >= 0; m--) {
                        if ((stringFlags & (1 << m)) != 0 && (6 - m) < vTracks[j].i1) {
                            //Read note
                            //printf("note on %d string, ",m);
                            iNoteCount++;
                            inFile.read(&length1.c, 1); uint8_t flags = length1.b;     //cout << "flags: " << std::bitset<8>(flags) << endl;
                            if ((flags & 0x20) != 0) {
                                inFile.read(&length1.c, 1); uint8_t noteType = length1.b;
                            }
                            if ((flags & 0x10) != 0) {
                                inFile.read(&length1.c, 1); uint8_t velocity = length1.b;
                            }
                            if ((flags & 0x20) != 0) {
                                inFile.read(&length1.c, 1); uint8_t fret = length1.b;
                                //printf("on %d fret\n",fret);
                            }
                            if ((flags & 0x80) != 0) {
                                inFile.seekg(2,ios_base::cur); // unknown
                            }
                            if ((flags & 0x01) != 0) {
                                inFile.seekg(8,ios_base::cur); // unknown
                            }
                            inFile.seekg(1,ios_base::cur); // unknown
                            if ((flags & 0x08) != 0) {
                                //Read note effects
                                inFile.read(&length1.c, 1); uint8_t flags1 = length1.b;     //cout << "flags1: " << std::bitset<8>(flags) << endl;
                                inFile.read(&length1.c, 1); uint8_t flags2 = length1.b;     //cout << "flags2: " << std::bitset<8>(flags) << endl;
                                if ((flags1 & 0x01) != 0) {
                                    //Read bend
                                    inFile.seekg(5,ios_base::cur); // unknown
                                    inFile.read(length4.c, 4); int numPoints = length4.i;
                                    for (int l = 0; l < numPoints; l++) {
                                        inFile.read(length4.c, 4); int bendPosition = length4.i;
                                        inFile.read(length4.c, 4); int bendValue = length4.i;
                                        inFile.seekg(1,ios_base::cur); // unknown
                                    }
                                }
                                if ((flags1 & 0x10) != 0) {
                                    //Read grace
                                    inFile.seekg(1,ios_base::cur); // ubyte fret;
                                    inFile.seekg(1,ios_base::cur); // ubyte dynamic;
                                    inFile.seekg(1,ios_base::cur); // byte transition;
                                    inFile.seekg(1,ios_base::cur); // ubyte duration;
                                    inFile.seekg(1,ios_base::cur); // ubyte flags;
                                }
                                if ((flags2 & 0x04) != 0) {
                                    //Read tremolo picking
                                    inFile.read(&length1.c, 1); uint8_t tremoloPickingValue = length1.b;
                                }
                                if ((flags2 & 0x08) != 0) {
                                    inFile.seekg(1,ios_base::cur); // unknown
                                }
                                if ((flags2 & 0x10) != 0) {
                                    //Read artificial harmonic
                                    inFile.read(&length1.c, 1); uint8_t artificialHarmonicType = length1.b;
                                    switch (artificialHarmonicType) {
                                    case 1:
                                      //harmonic = HarmonicEffect.NATURAL;
                                      break;
                                    case 2:
                                      inFile.seekg(3,ios_base::cur); // unknown
                                      //harmonic = HarmonicEffect.ARTIFICIAL;
                                      break;
                                    case 3:
                                      inFile.seekg(1,ios_base::cur); // unknown
                                      //harmonic = HarmonicEffect.TAPPED;
                                      break;
                                    case 4:
                                      //harmonic = HarmonicEffect.PINCH;
                                      break;
                                    case 5:
                                      //harmonic = HarmonicEffect.SEMI;
                                      break;
                                    }
                                }
                                if ((flags2 & 0x20) != 0) {
                                    //Read trill
                                    inFile.read(&length1.c, 1); uint8_t trillFret = length1.b;
                                    inFile.read(&length1.c, 1); uint8_t trillPeriod = length1.b;
                                }
                            }
                        }
                    }
                    inFile.seekg(1,ios_base::cur); // unknown
                    inFile.read(&length1.c, 1); uint8_t bRead = length1.b;
                    if ((bRead & 0x08) != 0) {
                        inFile.seekg(1,ios_base::cur); // unknown
                    }
                }
            }
            inFile.seekg(1,ios_base::cur); // unknown
        }
    }
    //done reading file;
    inFile.close();

    cout << "Note count: " << iNoteCount << endl;

    return 0;
}


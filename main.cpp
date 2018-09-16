#include <bitset>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>
#include <wx/wx.h>
#include <wx/dir.h>
#include <wx/busyinfo.h>

using namespace std;

bool UTF8mode = true;
enum ImportFormat {v500, v510};
string formatVersions[2] = {
    "FICHIER GUITAR PRO v5.00",  // import only
    "FICHIER GUITAR PRO v5.10"}; // main format
ImportFormat importFormat = v510;

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
int iNoteCount, iMeasureCount = 0;
int currentMeasure = 1;
int zoom = 4;
int total = 0;

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

// Cyrillic characters support
static void char2utf(char *out, const char *in) {
    static const int table[128] = {
        0x82D0,0x83D0,0x9A80E2,0x93D1,0x9E80E2,0xA680E2,0xA080E2,0xA180E2,
        0xAC82E2,0xB080E2,0x89D0,0xB980E2,0x8AD0,0x8CD0,0x8BD0,0x8FD0,
        0x92D1,0x9880E2,0x9980E2,0x9C80E2,0x9D80E2,0xA280E2,0x9380E2,0x9480E2,
        0,0xA284E2,0x99D1,0xBA80E2,0x9AD1,0x9CD1,0x9BD1,0x9FD1,
        0xA0C2,0x8ED0,0x9ED1,0x88D0,0xA4C2,0x90D2,0xA6C2,0xA7C2,
        0x81D0,0xA9C2,0x84D0,0xABC2,0xACC2,0xADC2,0xAEC2,0x87D0,
        0xB0C2,0xB1C2,0x86D0,0x96D1,0x91D2,0xB5C2,0xB6C2,0xB7C2,
        0x91D1,0x9684E2,0x94D1,0xBBC2,0x98D1,0x85D0,0x95D1,0x97D1,
        0x90D0,0x91D0,0x92D0,0x93D0,0x94D0,0x95D0,0x96D0,0x97D0,
        0x98D0,0x99D0,0x9AD0,0x9BD0,0x9CD0,0x9DD0,0x9ED0,0x9FD0,
        0xA0D0,0xA1D0,0xA2D0,0xA3D0,0xA4D0,0xA5D0,0xA6D0,0xA7D0,
        0xA8D0,0xA9D0,0xAAD0,0xABD0,0xACD0,0xADD0,0xAED0,0xAFD0,
        0xB0D0,0xB1D0,0xB2D0,0xB3D0,0xB4D0,0xB5D0,0xB6D0,0xB7D0,
        0xB8D0,0xB9D0,0xBAD0,0xBBD0,0xBCD0,0xBDD0,0xBED0,0xBFD0,
        0x80D1,0x81D1,0x82D1,0x83D1,0x84D1,0x85D1,0x86D1,0x87D1,
        0x88D1,0x89D1,0x8AD1,0x8BD1,0x8CD1,0x8DD1,0x8ED1,0x8FD1
    };
    while (*in)
        if (*in & 0x80) {
            int v = table[(int)(0x7f & *in++)];
            if (!v)
                continue;
            *out++ = (char)v;
            *out++ = (char)(v >> 8);
            if (v >>= 16)
                *out++ = (char)v;
        }
        else
            *out++ = *in++;
    *out = 0;
}
// Cyrillic characters support
string cp1251_to_utf8(string s) {
    if (!UTF8mode)
        return s;

    int c,i;
    string ns;
    for(i=0; i<s.size(); i++) {
	c=s[i];
        char buf[4], in[2] = {0, 0};
        *in = c;
        char2utf(buf, in);
        ns+=string(buf);
    }
   return ns;
}

class wxCustomButton : public wxPanel
{

    bool pressedDown;
    wxString text;

    static const int buttonWidth = 340;
    static const int buttonHeight = 50;

public:
    wxCustomButton(wxFrame* parent, wxString text);

    void paintEvent(wxPaintEvent & evt);
    void paintNow();
    void sizeEvent(wxSizeEvent & evt);

    void render(wxDC& dc);

    // some useful events
    void mouseMoved(wxMouseEvent& event);
    void mouseDown(wxMouseEvent& event);
    void mouseWheelMoved(wxMouseEvent& event);
    void mouseReleased(wxMouseEvent& event);
    void rightClick(wxMouseEvent& event);
    void mouseLeftWindow(wxMouseEvent& event);
    void keyPressed(wxKeyEvent& event);
    void keyReleased(wxKeyEvent& event);

private:
    wxDECLARE_EVENT_TABLE();
};

wxBEGIN_EVENT_TABLE(wxCustomButton, wxPanel)

    EVT_MOTION(wxCustomButton::mouseMoved)
    EVT_LEFT_DOWN(wxCustomButton::mouseDown)
    EVT_LEFT_UP(wxCustomButton::mouseReleased)
    EVT_RIGHT_DOWN(wxCustomButton::rightClick)
    EVT_LEAVE_WINDOW(wxCustomButton::mouseLeftWindow)
    EVT_KEY_DOWN(wxCustomButton::keyPressed)
    EVT_KEY_UP(wxCustomButton::keyReleased)
    EVT_MOUSEWHEEL(wxCustomButton::mouseWheelMoved)

    // catch paint events
    EVT_PAINT(wxCustomButton::paintEvent)
    EVT_SIZE(wxCustomButton::sizeEvent)

wxEND_EVENT_TABLE()

class MyFrame : public wxFrame
{
public:
    MyFrame();
private:
    void OnExit(wxCommandEvent& event);
    void OnDropFiles(wxDropFilesEvent& event);
};


class MyApp : public wxApp
{
private:
    MyFrame *m_frame;
    wxTextCtrl *txtArtist;
    wxTextCtrl *txtTitle;

    wxCustomButton* measureMap;
public:
    void ReadGP5(string fileName);
    virtual bool OnInit();
};

wxDECLARE_APP(MyApp);
wxIMPLEMENT_APP(MyApp);

bool MyApp::OnInit()
{
    ////do not convert ANSI (1251) codepage to UTF-8
    UTF8mode = false;

    m_frame = new MyFrame();

    wxStaticText *StaticText1 = new wxStaticText(m_frame, wxID_ANY, "Artist:", wxDefaultPosition, wxDefaultSize);
    wxStaticText *StaticText2 = new wxStaticText(m_frame, wxID_ANY, "Title:", wxDefaultPosition, wxDefaultSize);
    wxFont *StaticTextFont = new wxFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
    StaticTextFont->SetPointSize(20);
    StaticText1->SetFont(*StaticTextFont);
    StaticText2->SetFont(*StaticTextFont);
    txtArtist = new wxTextCtrl(m_frame, wxID_ANY, "", wxDefaultPosition, wxSize(350,22));
    txtTitle = new wxTextCtrl(m_frame, wxID_ANY, "", wxDefaultPosition, wxSize(350,22));

    m_frame->SetSize(wxSize(600, 300));
    m_frame->SetMinSize(wxSize(300, 230));
    m_frame->Centre();

    wxBoxSizer *BoxSizer1 = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *BoxSizer2 = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *BoxSizer3 = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *BoxSizer4 = new wxBoxSizer(wxVERTICAL);
    BoxSizer2->Add(StaticText1, 0, wxALL |wxALIGN_LEFT, 10);
    BoxSizer3->Add(StaticText2, 0, wxALL |wxALIGN_LEFT, 10);
    BoxSizer2->Add(txtArtist, 0, wxALL|wxALIGN_RIGHT, 10);
    BoxSizer3->Add(txtTitle, 0, wxALL|wxALIGN_RIGHT, 10);
    BoxSizer1->Add(BoxSizer2);
    BoxSizer1->Add(BoxSizer3);
    BoxSizer4->Add(BoxSizer1);

    measureMap = new wxCustomButton( m_frame, "" );
    BoxSizer4->Add(measureMap, 0, wxALL|wxEXPAND, 5);

    m_frame->SetSizer(BoxSizer4);
    m_frame->Show(true);

    wxString msg;

    if (argc < 2)
    {
        msg = "Hello. Please, drag and drop GP5 file or specify it via command line parameter."; cout << msg << endl; m_frame->SetStatusText(msg);
        return true;
    }

    ReadGP5((string)argv[1]);

    return true;
}

void MyApp::ReadGP5(string fileName)
{
    wxString msg;

    // be sure that previous file is closed before opening new one
    wxASSERT(!inFile.is_open());
    if (inFile.is_open())
        inFile.close();

    msg = wxString::Format("Reading file: %s",fileName); cout << msg << endl; m_frame->SetStatusText(msg);
    inFile.open(fileName.c_str(), ios::binary | ios::in);

    if (inFile.fail()) {
        msg = wxString::Format("Can't open file: %s",strerror(errno));
        cerr << msg << endl; m_frame->SetStatusText(msg);
        return;
    }

    string sVersion = readByteText().substr(0, 24);
    cout << "File version: " << sVersion << endl;
    if (sVersion.compare(formatVersions[v510]) == 0)
        importFormat = v510;
    else
    if (sVersion.compare(formatVersions[v500]) == 0)
        importFormat = v500;
    else {
        inFile.close();
        msg = wxString::Format("Unsupported file format: %s",fileName);
        cerr << msg << endl; m_frame->SetStatusText(msg);
        cerr << "Expected: \t" << formatVersions[v510] << " or " << formatVersions[v500] << endl;
        cerr << "Got: \t\t" << sVersion << endl;
        return;
    }

    // start reading file
    info.clear(); vTracks.clear(); lyrics.clear();
    m_frame->SetTitle(fileName);

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
    infoItem.i = 0; infoItem.text = cp1251_to_utf8(sTitle); info.push_back(infoItem);
    infoItem.i = 1; infoItem.text = cp1251_to_utf8(sSubtitle); info.push_back(infoItem);
    infoItem.i = 2; infoItem.text = cp1251_to_utf8(sArtist); info.push_back(infoItem);
    infoItem.i = 3; infoItem.text = cp1251_to_utf8(sAlbum); info.push_back(infoItem);
    infoItem.i = 4; infoItem.text = cp1251_to_utf8(sWordsBy); info.push_back(infoItem);
    infoItem.i = 5; infoItem.text = cp1251_to_utf8(sMusicBy); info.push_back(infoItem);
    infoItem.i = 6; infoItem.text = cp1251_to_utf8(sCopyright); info.push_back(infoItem);
    infoItem.i = 7; infoItem.text = cp1251_to_utf8(sTab); info.push_back(infoItem);
    infoItem.i = 8; infoItem.text = cp1251_to_utf8(sInstructions); info.push_back(infoItem);

    txtArtist->SetValue(info[Artist].text);
    txtTitle->SetValue(info[Title].text);

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
    if (importFormat == v500)
        inFile.seekg(30,ios_base::cur);
    else // v510
        inFile.seekg(49,ios_base::cur);

    // Read page setup
    for (int i = 0; i < 11; i++) {
        inFile.seekg(4,ios_base::cur);
        string sTemp = readByteText();
    }

    inFile.read(length4.c, 4); int tempo = length4.i;
    cout << "Tempo: " << tempo << endl;

    if (importFormat == v500) {
        // nothing
    } else // v510
        inFile.seekg(1,ios_base::cur); // unknown

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

    iMeasureCount = measures;
    currentMeasure = 0;

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

        if (importFormat == v500) {
            inFile.seekg(1,ios_base::cur);  // unknown
        }
        else { // v510
            if (i == 1)
                inFile.seekg(1,ios_base::cur);  // unknown
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

        if (importFormat == v500) {
            inFile.seekg(44,ios_base::cur); // unknown
        }
        else { // v510
            inFile.seekg(49,ios_base::cur); // unknown
            readIntByteText();
            readIntByteText();
        }
    }
    if (importFormat == v500)
        inFile.seekg(2,ios_base::cur); // unknown
    else // v510
        inFile.seekg(1,ios_base::cur); // unknown


    iNoteCount = 0; // reset note counter from last file

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

                        if (tempoValue >= 0)
                        {
                            if (importFormat == v500)
                                inFile.seekg(1,ios_base::cur); // unknown
                            else // v510
                                inFile.seekg(2,ios_base::cur); // unknown
                        }
                        inFile.seekg(2,ios_base::cur); // unknown
                        if (importFormat == v500) {
                            // nothing
                        } else  // v510
                        {
                            readIntByteText();
                            readIntByteText();
                        }
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
	// done reading file;
	inFile.close();
	msg = wxString::Format("Done. Note count: %d",iNoteCount);
	cout << msg << endl; m_frame->SetStatusText(msg);

	measureMap->paintNow();
}

MyFrame::MyFrame()
        : wxFrame(NULL, wxID_ANY, "GP5 Reader")
{
    DragAcceptFiles(true);
    Connect(wxEVT_DROP_FILES, wxDropFilesEventHandler(MyFrame::OnDropFiles), NULL, this);
    CreateStatusBar();
    SetStatusText("ready");
}

void MyFrame::OnDropFiles(wxDropFilesEvent& event)
{
if (event.GetNumberOfFiles() > 0) {

            wxString* dropped = event.GetFiles();
            wxASSERT(dropped);

            wxBusyCursor busyCursor;
            wxWindowDisabler disabler;
            //wxBusyInfo busyInfo(_("Adding files, wait please..."));

            wxString name;
            wxArrayString files;

            for (int i = 0; i < event.GetNumberOfFiles(); i++) {
                name = dropped[i];
                if (wxFileExists(name))
                    files.push_back(name);
                else if (wxDirExists(name))
                    wxDir::GetAllFiles(name, &files);
            }

            wxGetApp().ReadGP5((string)files[0]);

        }
}

void MyFrame::OnExit(wxCommandEvent& event)
{
    Close(true);
}

wxCustomButton::wxCustomButton(wxFrame* parent, wxString text) :
 wxPanel(parent, wxID_ANY)
{
    SetMinSize( wxSize(buttonWidth, buttonHeight) );
    this->text = text;
    pressedDown = false;
}

/*
 * Called by the system of by wxWidgets when the panel needs
 * to be redrawn. You can also trigger this call by
 * calling Refresh()/Update().
 */

void wxCustomButton::paintEvent(wxPaintEvent & evt)
{
    // depending on your system you may need to look at double-buffered dcs
    wxPaintDC dc(this);
    render(dc);
}

void wxCustomButton::sizeEvent(wxSizeEvent & evt)
{
    paintNow();
}

/*
 * Alternatively, you can use a clientDC to paint on the panel
 * at any time. Using this generally does not free you from
 * catching paint events, since it is possible that e.g. the window
 * manager throws away your drawing when the window comes to the
 * background, and expects you will redraw it when the window comes
 * back (by sending a paint event).
 */
void wxCustomButton::paintNow()
{
    // depending on your system you may need to look at double-buffered dcs
    wxClientDC dc(this);
    render(dc);
}

/*
 * Here we do the actual rendering. I put it in a separate
 * method so that it can work no matter what type of DC
 * (e.g. wxPaintDC or wxClientDC) is used.
 */
void wxCustomButton::render(wxDC&  dc)
{
    //if (pressedDown)
    //    dc.SetBrush( *wxRED_BRUSH );
    //else
    //    dc.SetBrush( *wxGREY_BRUSH );

    int headerHeight = 20;
    int fullWidth = dc.GetSize().GetWidth();
    int fullHeight = dc.GetSize().GetHeight();
    int minSize = 1; // minimum "measure" width

    wxColor color;
    wxBrush brush;
    string caption = "Measure map";


    if (iMeasureCount < 20) {
        zoom = 1;
        color.Set(255, 170 , 170);
    }
    else
    if (iMeasureCount < 100) {
        zoom = 4;
        color.Set(255, 170 , 170);
    } else {
        zoom = 20;
        color = *wxRED;
    }

    caption.append(wxString::Format(" %d:1",zoom));
    total = iMeasureCount / zoom;
    if (iMeasureCount % zoom != 0)
        total++;

    //draw header
    brush.SetColour(233,233,233);
    dc.SetBrush(brush);
    dc.DrawRectangle( 0, 0, dc.GetSize().GetWidth(), headerHeight+1);
    dc.DrawText( caption, 20, 2 );


    dc.SetBrush(*wxGREY_BRUSH);
                    dc.DrawRectangle(
                        0,
                        headerHeight,
                        fullWidth,
                        fullHeight - headerHeight);

    // draw "measures"
    if (total == 0 /*|| currentMeasure == 0*/) {
        dc.SetBrush(*wxGREY_BRUSH);
        dc.DrawRectangle(0,headerHeight,fullWidth,fullHeight - headerHeight);
    } else {
        for (int i = 0; i < total; i++) {

            /*if (m_timer->IsRunning())
                if (i != currentMeasure)
                    continue;*/

            int measureWidth;
            measureWidth = 30;

            bool i_parity = i % 2 == 0;
            bool total_parity = total % 2 == 0;

            int offset = (i+1)/2 * measureWidth;
            if (i_parity)
                offset = -offset;
            int curPos = (fullWidth / 2) - measureWidth/(total_parity?1:2) + offset;

            /*if (i == total) {// last
                if (curPos < fullWidth) { // if empty space left draw gray space
                    dc.SetBrush(*wxGREY_BRUSH);
                    dc.DrawRectangle(
                        curPos-1 + (fullWidth / total),
                        headerHeight,
                        fullWidth - ((fullWidth / total)*i -1),
                        fullHeight - headerHeight);
                }
            }*/

            // draw "measure"
            brush.SetColour(color);
            dc.SetBrush(brush);
            dc.DrawRectangle(
                curPos,
                headerHeight,
                measureWidth+1,
                fullHeight - headerHeight);

            if (i==total-1) // last
            {
                    int n = 1;
                    int curPosAbsolute = (fullWidth / 2) - (measureWidth/(total_parity?1:2) + measureWidth * (total/2-n+(total_parity?0:1)));

                    if (total != 1)
                            dc.DrawText( "1", curPosAbsolute+2, headerHeight );

                    n = total;
                    curPosAbsolute = (fullWidth / 2) - (measureWidth/(total_parity?1:2) + measureWidth * (total/2-n+(total_parity?0:1)));
                    dc.DrawText(wxString::Format("%d",iMeasureCount), curPosAbsolute+2, headerHeight );
            }
            /*if (i==total)
                dc.DrawText( wxString::Format("%d",iMeasureCount),
                            curPos+measureWidth-dc.GetTextExtent(wxString::Format("%d",iMeasureCount)).GetWidth()-2,
                            headerHeight );
            else
                // draw measure number in every 4th square
                dc.DrawText( i%4?"":wxString::Format("%d",i*zoom), curPos+1, headerHeight );
            */
        }
    }
}

void wxCustomButton::mouseDown(wxMouseEvent& event)
{
    pressedDown = true;
    paintNow();
}
void wxCustomButton::mouseReleased(wxMouseEvent& event)
{
    pressedDown = false;
    paintNow();

    // on click code
}
void wxCustomButton::mouseLeftWindow(wxMouseEvent& event)
{
    if (pressedDown)
    {
        pressedDown = false;
        paintNow();
    }
}

// currently unused events
void wxCustomButton::mouseMoved(wxMouseEvent& event) {}
void wxCustomButton::mouseWheelMoved(wxMouseEvent& event) {}
void wxCustomButton::rightClick(wxMouseEvent& event) {}
void wxCustomButton::keyPressed(wxKeyEvent& event) {}
void wxCustomButton::keyReleased(wxKeyEvent& event) {}




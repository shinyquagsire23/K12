#ifndef FMODAPI_H
#define FMODAPI_H

enum FMOD_RESULT
{
    FMOD_OK                     = 0,
    FMOD_ERR_BADCOMMAND         = 1,
    FMOD_ERR_CHANNEL_ALLOC      = 2,
    FMOD_ERR_CHANNEL_STOLEN     = 3,
    FMOD_ERR_DMA                = 4,
    FMOD_ERR_DSP_CONNECTION     = 5,
    FMOD_ERR_DSP_DONTPROCESS    = 6,
    FMOD_ERR_DSP_FORMAT         = 7,
    FMOD_ERR_DSP_INUSE          = 8,
    FMOD_ERR_DSP_NOTFOUND       = 9,
    FMOD_ERR_DSP_RESERVED       = 10,
    FMOD_ERR_DSP_SILENCE        = 11,
    FMOD_ERR_DSP_TYPE           = 12,
    FMOD_ERR_FILE_BAD           = 13,
    FMOD_ERR_FILE_COULDNOTSEEK  = 14,
    FMOD_ERR_FILE_DISKEJECTED   = 15,
    FMOD_ERR_FILE_EOF           = 16,
    FMOD_ERR_FILE_ENDOFDATA     = 17,
    FMOD_ERR_FILE_NOTFOUND      = 18,
    FMOD_ERR_FORMAT             = 19,
    FMOD_ERR_HEADER_MISMATCH    = 20,
    FMOD_ERR_HTTP               = 21,
    FMOD_ERR_HTTP_ACCESS        = 22,
    FMOD_ERR_HTTP_PROXY_AUTH    = 23,
    FMOD_ERR_HTTP_SERVER_ERROR  = 24,
    FMOD_ERR_HTTP_TIMEOUT       = 25,
    FMOD_ERR_INITIALIZATION     = 26,
    FMOD_ERR_INITIALIZED        = 27,
    FMOD_ERR_INTERNAL           = 28,
    FMOD_ERR_INVALID_FLOAT      = 29,
    FMOD_ERR_INVALID_HANDLE     = 30,
    FMOD_ERR_INVALID_PARAM      = 31,
    FMOD_ERR_INVALID_POSITION   = 32,
    FMOD_ERR_INVALID_SPEAKER    = 33,
    FMOD_ERR_INVALID_SYNCDSP    = 34,
    FMOD_ERR_INVALID_VECTOR     = 35,
    FMOD_ERR_ALREADY_LOCKED     = 36,
};

enum FMOD_OUTPUTTYPE
{
    FMOD_OUTPUTTYPE_AUTODETECT   = 0,
    FMOD_OUTPUTTYPE_UNKNOWN      = 1,
    FMOD_OUTPUTTYPE_NOSOUND      = 2,
    FMOD_OUTPUTTYPE_WAVWRITER    = 3,
    FMOD_OUTPUTTYPE_NOSOUND_NRT  = 4,
    FMOD_OUTPUTTYPE_WAVWRITER_NRT= 5,
    FMOD_OUTPUTTYPE_WASAPI       = 6,
    FMOD_OUTPUTTYPE_ASIO         = 7,
    FMOD_OUTPUTTYPE_PULSEAUDIO   = 8,
    FMOD_OUTPUTTYPE_ALSA         = 9,
    FMOD_OUTPUTTYPE_COREAUDIO    = 10,
    FMOD_OUTPUTTYPE_AUDIOTRACK   = 11,
};

enum FMOD_SOUND_TYPE
{
    FMOD_SOUND_TYPE_UNKNOWN   = 0,
    FMOD_SOUND_TYPE_AIFF      = 1,
    FMOD_SOUND_TYPE_ASF       = 2,
    FMOD_SOUND_TYPE_DLS       = 3,
    FMOD_SOUND_TYPE_FLAC      = 4,
    FMOD_SOUND_TYPE_FSB       = 5,
    FMOD_SOUND_TYPE_IT        = 6,
    FMOD_SOUND_TYPE_MIDI      = 7,
    FMOD_SOUND_TYPE_MOD       = 8,
    FMOD_SOUND_TYPE_MPEG      = 9,
    FMOD_SOUND_TYPE_OGGVORBIS = 10,
    FMOD_SOUND_TYPE_PLAYLIST  = 11,
    FMOD_SOUND_TYPE_RAW       = 12,
    FMOD_SOUND_TYPE_S3M       = 13,
    FMOD_SOUND_TYPE_USER      = 14,
    FMOD_SOUND_TYPE_WAV       = 15,
};

enum FMOD_MODE
{
    FMOD_DEFAULT        = 0x00000000,
    FMOD_LOOP_OFF       = 0x00000001,
    FMOD_LOOP_NORMAL    = 0x00000002,
    FMOD_2D             = 0x00000008,
    FMOD_3D             = 0x00000010,
    FMOD_CREATESTREAM   = 0x00000080,
    FMOD_OPENMEMORY     = 0x00000800,
    FMOD_LOWMEM         = 0x08000000,
};

enum FMOD_TIMEUNIT
{
    FMOD_TIMEUNIT_MS          = 0x00000001,
    FMOD_TIMEUNIT_PCM         = 0x00000002,
    FMOD_TIMEUNIT_PCMBYTES    = 0x00000004,
    FMOD_TIMEUNIT_RAWBYTES    = 0x00000008,
    FMOD_TIMEUNIT_SAMPLE      = 0x00010000,
};

enum FMOD_CHANNELCONTROL_TYPE
{
    FMOD_CHANNELCONTROL_CHANNEL      = 0,
    FMOD_CHANNELCONTROL_CHANNELGROUP = 1,
};

enum FMOD_CHANNELCONTROL_CALLBACK_TYPE
{
    FMOD_CHANNELCONTROL_CALLBACK_END          = 0,
    FMOD_CHANNELCONTROL_CALLBACK_VIRTUALVOICE = 1,
    FMOD_CHANNELCONTROL_CALLBACK_SYNCPOINT    = 2,
    FMOD_CHANNELCONTROL_CALLBACK_OCCLUSION    = 3,
};

struct FMOD_VECTOR
{
    float x;
    float y;
    float z;
};

struct FMOD_ASYNCREADINFO;

class FMOD_CHANNELCONTROL;

// bionic's stdio tag, forward-declared rather than included: <stdio.h> pulls in <string.h>, whose
// prototypes lack the exception specifications some callers declare.
struct __sFILE;

// The real FMOD layout, 224 bytes.
struct FMOD_CREATESOUNDEXINFO
{
    int                cbsize;
    unsigned int       length;
    unsigned int       fileoffset;
    int                numchannels;
    int                defaultfrequency;
    int                format;
    unsigned int       decodebuffersize;
    int                initialsubsound;
    int                numsubsounds;
    int*               inclusionlist;
    int                inclusionlistnum;
    void*              pcmreadcallback;
    void*              pcmsetposcallback;
    void*              nonblockcallback;
    const char*        dlsname;
    const char*        encryptionkey;
    int                maxpolyphony;
    void*              userdata;
    int                suggestedsoundtype;
    void*              fileuseropen;
    void*              fileuserclose;
    void*              fileuserread;
    void*              fileuserseek;
    void*              fileuserasyncread;
    void*              fileuserasynccancel;
    void*              fileuserdata;
    int                filebuffersize;
    int                channelorder;
    void*              initialsoundgroup;
    unsigned int       initialseekposition;
    int                initialseekpostype;
    int                ignoresetfilesystem;
    unsigned int       audioqueuepolicy;
    unsigned int       minmidigranularity;
    int                nonblockthreadid;
    void*              fsbguid;
};

namespace FMOD
{

class ChannelControl;
class ChannelGroup;

typedef FMOD_RESULT (*FMOD_FILE_OPENCALLBACK)(const char* name, unsigned int* filesize,
                                              void** handle, void* userdata);
typedef FMOD_RESULT (*FMOD_FILE_CLOSECALLBACK)(void* handle, void* userdata);
typedef FMOD_RESULT (*FMOD_FILE_READCALLBACK)(void* handle, void* buffer, unsigned int sizebytes,
                                              unsigned int* bytesread, void* userdata);
typedef FMOD_RESULT (*FMOD_FILE_SEEKCALLBACK)(void* handle, unsigned int pos, void* userdata);
typedef FMOD_RESULT (*FMOD_FILE_ASYNCREADCALLBACK)(FMOD_ASYNCREADINFO* info, void* userdata);
typedef FMOD_RESULT (*FMOD_FILE_ASYNCCANCELCALLBACK)(FMOD_ASYNCREADINFO* info, void* userdata);
typedef FMOD_RESULT (*FMOD_CHANNELCONTROL_CALLBACK)(FMOD_CHANNELCONTROL* channelcontrol,
                                                    FMOD_CHANNELCONTROL_TYPE controltype,
                                                    FMOD_CHANNELCONTROL_CALLBACK_TYPE callbacktype,
                                                    void* commanddata1, void* commanddata2);

class Sound
{
public:
    FMOD_RESULT setUserData(void* userdata);
    FMOD_RESULT release();
    FMOD_RESULT getLength(unsigned int* length, unsigned int lengthtype);
};

class ChannelControl
{
public:
    FMOD_RESULT stop();
    FMOD_RESULT setPaused(bool paused);
    FMOD_RESULT isPlaying(bool* isplaying);
    FMOD_RESULT setVolume(float volume);
    FMOD_RESULT setPan(float pan);
    FMOD_RESULT set3DAttributes(const FMOD_VECTOR* position, const FMOD_VECTOR* velocity);
    FMOD_RESULT set3DMinMaxDistance(float mindistance, float maxdistance);
    FMOD_RESULT set3DOcclusion(float directocclusion, float reverbocclusion);
    FMOD_RESULT setUserData(void* userdata);
    FMOD_RESULT getUserData(void** userdata);
    FMOD_RESULT setCallback(FMOD_RESULT (*callback)(FMOD_CHANNELCONTROL* channelcontrol,
                                                    FMOD_CHANNELCONTROL_TYPE controltype,
                                                    FMOD_CHANNELCONTROL_CALLBACK_TYPE callbacktype,
                                                    void* commanddata1, void* commanddata2));
};

class Channel : public ChannelControl
{
public:
    FMOD_RESULT setFrequency(float frequency);
    FMOD_RESULT setPriority(int priority);
    FMOD_RESULT setPosition(unsigned int position, unsigned int postype);
    FMOD_RESULT getPosition(unsigned int* position, unsigned int postype);
};

class System
{
public:
    FMOD_RESULT release();
    FMOD_RESULT update();
    FMOD_RESULT init(int maxchannels, unsigned int flags, void* extradriverdata);
    FMOD_RESULT setOutput(FMOD_OUTPUTTYPE output);
    FMOD_RESULT getNumDrivers(int* numdrivers);
    FMOD_RESULT setFileSystem(FMOD_FILE_OPENCALLBACK useropen, FMOD_FILE_CLOSECALLBACK userclose,
                              FMOD_FILE_READCALLBACK userread, FMOD_FILE_SEEKCALLBACK userseek,
                              FMOD_FILE_ASYNCREADCALLBACK userasyncread,
                              FMOD_FILE_ASYNCCANCELCALLBACK userasynccancel, int blockalign);
    FMOD_RESULT setStreamBufferSize(unsigned int filebuffersize, unsigned int filebuffersizetype);
    FMOD_RESULT set3DListenerAttributes(int listener, const FMOD_VECTOR* pos,
                                        const FMOD_VECTOR* vel, const FMOD_VECTOR* forward,
                                        const FMOD_VECTOR* up);
    FMOD_RESULT getChannel(int channelid, Channel** channel);
    FMOD_RESULT playSound(Sound* sound, ChannelGroup* channelgroup, bool paused, Channel** channel);
    FMOD_RESULT createSound(const char* name_or_data, unsigned int mode,
                            FMOD_CREATESOUNDEXINFO* exinfo, Sound** sound);
};

} // namespace FMOD

// The C API's channel handle, a global-namespace type distinct from FMOD::ChannelControl and
// deriving from it.
class FMOD_CHANNELCONTROL : public FMOD::ChannelControl
{
};

extern "C" FMOD_RESULT FMOD_System_Create(FMOD::System** system);

#endif // FMODAPI_H

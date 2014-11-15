#include <map>
#include "RtMidi.h"
#include "maxcpp6.h"

typedef std::map<t_symbol*,int> portmap;
typedef std::vector<unsigned char> midimessage;

const int MAX_STR_SIZE = 512;

const int OUTLET_MIDI     = 0;
const int OUTLET_SYSEX    = 1;
const int OUTLET_INPORTS  = 2;
const int OUTLET_OUTPORTS = 3;

const int SYSEX_START = 0xF0;
const int SYSEX_STOP  = 0xF7;

t_symbol *SYM_APPEND = gensym("append");
t_symbol *SYM_CLEAR  = gensym("clear");
t_symbol *SYM_SET    = gensym("set");
t_symbol *SYM_NONE  = gensym("<none>");

void midiInputCallback(double deltatime, midimessage *message, void *userData);


static inline std::string &trim(std::string &s)
{
    std::string::size_type pos = s.find_last_not_of(' ');
    if(pos != std::string::npos) {
        if (s.length()!=pos+1) {
            // erase trailing whitespace
            s.erase(pos+1);
        }
        pos = s.find_first_not_of(' ');
        if(pos!=0) {
            // erase leading whitespace
            s.erase(0, pos);
        }
    }
    else {
        // all whitespace
        s="";
    }
    return s;
}


class MIDI4L : public MaxCpp6<MIDI4L> {
    
public:
    
    MIDI4L(t_symbol * sym, long ac, t_atom * av) :
        midiin(NULL),
        midiout(NULL),
        numInPorts(-1),
        numOutPorts(-1),
        inPortMap(),
        outPortMap(),
        inPortName(NULL),
        outPortName(NULL),
        message(),
        isSysEx(false)
    {
		setupIO(1, 4); // inlets / outlets
        
        try {
            midiin = new RtMidiIn();
        }
        catch ( RtMidiError &error ) {
            printError("RtMidiIn constructor failure", error);
        }
        
        try {
            midiout = new RtMidiOut();
        }
        catch ( RtMidiError &error ) {
            printError("RtMidiOut consturctor failure", error);
        }
        
        refreshPorts();
        // printPorts();
        
        if(ac > 0) { // first arg is input
            input(0, NULL, ac, av);
        }
        
        if(ac > 1) { // second arg is output
            output(0, NULL, ac-1, av+1);
        }
	}
	
    ~MIDI4L() {
        if(midiin) {
            midiin->cancelCallback();
            midiin->closePort();
            delete midiin;
        }
        if(midiout) {
            midiout->closePort();
            delete midiout;
        }
    }
    
    
    /**
     * Setup inlet and outlet assistance messages.
     */
    void assist(void *b, long io, long index, char *msg) {
        if (io == ASSIST_INLET) {
            strncpy_zero(msg, "(send list) send MIDI to output port, (bang) list ports, (input name) set input port, (output name) set output port", MAX_STR_SIZE);
        }
        else if (io==ASSIST_OUTLET) {
            switch (index) {
                case OUTLET_MIDI:
                    strncpy_zero(msg, "MIDI from input port", MAX_STR_SIZE);
                    break;
                case OUTLET_SYSEX:
                    strncpy_zero(msg, "SysEx from input port", MAX_STR_SIZE);
                    break;
                case OUTLET_INPORTS:
                    strncpy_zero(msg, "input port list", MAX_STR_SIZE);
                    break;
                case OUTLET_OUTPORTS:
                    strncpy_zero(msg, "output port list", MAX_STR_SIZE);
                    break;
            }
        }
    }
    
    
    /**
     * List all input and output ports.
     * Also refreshes the port list in case a device was plugged in or unplugged.
     */
	void bang(long inlet) {
        refreshPorts();
        dumpPorts(m_outlets[OUTLET_INPORTS],  inPortMap,  inPortName);
        dumpPorts(m_outlets[OUTLET_OUTPORTS], outPortMap, outPortName);
    }
    
    
    /**
     * Set the input port by name.
     * If the name is valid, this object will start sending messages out it's outlet when MIDI is received.
     * If the name is invalid, an error is printed to the Max console and nothing else happens.
     * As a special behavior, sending the [inport " "] message will close the port. This plays nice with the way we build the umenu port list.
     */
    void input(long inlet, t_symbol *s, long ac, t_atom *av) {
        t_symbol *portName = _sym_nothing;
        
        // TODO: maybe handle ints (and floats cast to int) and use it to lookup a port by index.
        // Could be a useful for someone with devices with duplicate names.
        // See simplemax_new for an example of how to check the atom type
        
        if( atom_arg_getsym(&portName, 0, ac, av) == MAX_ERR_NONE ) {
            if (midiin) {
                int portIndex = getPortIndex(inPortMap, portName);
                
                if(portIndex >= 0 || portName == SYM_NONE) {
                    midiin->cancelCallback();
                    midiin->closePort();
                    inPortName = NULL;
                }
                
                if(portIndex >= 0) {
                    midiin->openPort( portIndex );
                    midiin->setCallback( &midiInputCallback, this );
                    midiin->ignoreTypes( false, true, true ); // ignore MIDI timing and active sensing messages (but not SysEx)
                    // TODO? midiin->setErrorCallback()
                    inPortName = portName;
                }
                else if(portName != SYM_NONE) {
                    object_error((t_object *)this, "Input port not found: %s", *portName);
                }
            }
            // else we already printed an error in the constructor
        }
        else {
            object_error((t_object *)this, "Invalid input. A portname is required. Or use (input <none>) to close the port.");
        }
    }
    
    
    /**
     * Set the output port by name.
     * If the name is valid, this object will pass messages received to it's first inlet to the MIDI port.
     * If the name is invalid, an error is printed to the Max console and nothing else happens.
     * As a special behavior, sending the [outport " "] message will close the port. This plays nice with the way we build the umenu port list.
     */
    void output(long inlet, t_symbol *s, long ac, t_atom *av) {
        t_symbol *portName = _sym_nothing;
        
        // TODO: maybe handle ints (and floats cast to int) and use it to lookup a port by index.
        // Could be a useful for someone with devices with duplicate names.
        // See simplemax_new for an example of how to check the atom type
        
        if( atom_arg_getsym(&portName, 0, ac, av) == MAX_ERR_NONE ) {
            if (midiout) {
                int portIndex = getPortIndex(outPortMap, portName);
                
                if(portIndex >= 0 || portName == SYM_NONE) {
                    midiout->closePort();
                    outPortName = NULL;
                }
                
                if(portIndex >= 0) {
                    midiout->openPort( portIndex );
                    // TODO? midiout->setErrorCallback()
                    outPortName = portName;
                }
                else if(portName != SYM_NONE) {
                    object_error((t_object *)this, "Output port not found: %s", *portName);
                }
            }
            // else we already printed an error in the constructor
        }
        else {
            object_error((t_object *)this, "Invalid output. A portname is required. Or use (output <none>) to close the port.");
        }
    }
    
    
    /**
     * Receives MIDI message bytes from the Max patch and sends them to the midiout.
     * NOTE: RtMidi needs to send all bytes of a MIDI on message at the same time.
     *       To achieve this, in Max, you should do [midiformat] => [thresh] => [prepend sendmidi] => [midi4l]
     *       I use [thresh 1] to minimize the latency. So far it seems safe to do so.
     */
    void send(long inlet, t_symbol *s, long ac, t_atom *av) {
        message.clear();
        for (int i=0; i<ac; i++) {
            unsigned char value = atom_getlong(av+i);
            message.push_back(value);
        }
        
        if(midiout && midiout->isPortOpen()) {
            midiout->sendMessage( &message );
        }
    }
    
    
    /**
     * Pass a multi-byte MIDI message received from midiin to the outlet.
     * NOTE: This is an internal callback, it is not part of the interface with the Max patch.
     */
    void receive(midimessage *message) {
        if(message) {
            void *midiOutlet = m_outlets[OUTLET_MIDI];
            void *sysexOutlet = m_outlets[OUTLET_SYSEX];
            
            if(midiOutlet && sysexOutlet) {
                
                int byteCount = message->size();                
                for(int i=0; i<byteCount; i++) {
                    int byte = (int)message->at(i);
          
                    if(byte == SYSEX_START) {
                        isSysEx = true;
                    }
          
                    outlet_int(isSysEx ? sysexOutlet : midiOutlet, byte);
          
                    if(byte == SYSEX_STOP) {
                        isSysEx = false;
                    }
                }
            }
        }
    }
    
    
private:
    
    RtMidiIn  *midiin;
    RtMidiOut *midiout;
    int numInPorts;
    int numOutPorts;
    portmap inPortMap;
    portmap outPortMap;
    t_symbol *inPortName;
    t_symbol *outPortName;
    midimessage message;
    bool isSysEx;
    
    
    /**
     * Get the list of available ports. 
     * Can be called repeatedly to regenerate the list if a MIDI device is plugged in or unplugged.
     */
    void refreshPorts() {
        char cPortName[MAX_STR_SIZE];
        
        inPortMap.clear();
        outPortMap.clear();

        if(midiin) {
            numInPorts = midiin->getPortCount();
            for ( int i=0; i<numInPorts; i++ ) {
                try {
                    std::string portName = midiin->getPortName(i);

                    // CME Xkey reports its name as "Xkey  ", which was a hassle to deal with in a Max patch, so auto-trim the names.
                    portName = trim(portName);
                    
                    strncpy(cPortName, portName.c_str(), MAX_STR_SIZE);
                    cPortName[MAX_STR_SIZE - 1] = NULL;
                    
                    inPortMap[gensym(cPortName)] = i;
                }
                catch ( RtMidiError &error ) {
                    printError("Error getting MIDI input port name", error);
                }
            }
        }
        
        if(midiout) {
            numOutPorts = midiout->getPortCount();
            for ( int i=0; i<numOutPorts; i++ ) {
                try {
                    std::string portName = midiout->getPortName(i);
                    
                    // CME Xkey reports its name as "Xkey  ", which was a hassle to deal with in a Max patch, so auto-trim the names.
                    portName = trim(portName);
                    
                    strncpy(cPortName, portName.c_str(), MAX_STR_SIZE);
                    cPortName[MAX_STR_SIZE - 1] = NULL;
                    
                    outPortMap[gensym(cPortName)] = i;
                }
                catch (RtMidiError &error) {
                    printError("Error getting MIDI output port name", error);
                }
            }
        }
    }

    
    /**
     * Lookup a port index given a portMap (input or output port map) and a portName.
     * Returns the port index, or -1 if the port is not found
     */
    int getPortIndex(portmap portMap, t_symbol *portName) {
        portmap::iterator iter = portMap.find(portName);
        if (iter == portMap.end()) {
            return -1;
        }
        else {
            return iter->second;
        }
    }
    
    
    /**
     * Print the port names to the Max console
     */
    void printPorts() {
        object_post((t_object *)this, "MIDI input Port count: %u", numInPorts);
        
        for( portmap::iterator iter=inPortMap.begin(); iter!=inPortMap.end(); iter++ ) {
            t_symbol *portName = (*iter).first;
            object_post((t_object *)this, "input %u: %s", (*iter).second, *portName);
        }
        
        object_post((t_object *)this, " ");
        object_post((t_object *)this, "MIDI output port count: %u", numOutPorts);
        
        for( portmap::iterator iter=outPortMap.begin(); iter!=outPortMap.end(); iter++ ) {
            t_symbol* portName = (*iter).first;
            object_post((t_object *)this, "output %u: %s", (*iter).second, *portName);
        }
    }
    
    
    /**
     * Send messages to build a umenus for the input (2nd outlet) and output (3rd outlet) ports
     */
    void dumpPorts(void *outlet, portmap portMap, t_symbol *selectedPort) {
        t_atom atoms[1];
        
        outlet_anything(outlet, SYM_CLEAR, 0, NULL);
        
        for( portmap::iterator iter=portMap.begin(); iter!=portMap.end(); iter++ ) {
            t_symbol *portName = (*iter).first;
            atom_setsym(&atoms[0], portName);
            outlet_anything(outlet, SYM_APPEND, 1, atoms);
        }
        
        atom_setsym(&atoms[0], SYM_NONE); // start the umenu with a blank item, to indicate no port is selected
        outlet_anything(outlet, SYM_APPEND, 1, atoms);
        
        if(selectedPort) {
            atom_setsym(&atoms[0], selectedPort);
        }
        else {
            atom_setsym(&atoms[0], SYM_NONE);
        }
        outlet_anything(outlet, SYM_SET, 1, atoms);
    }
    
    
    /**
     * Print an RtMidiError to the Max console
     * NOTE: I have not been able to test this code because I don't know how to trigger an RtMidiError. It always "just works" for me.
     */
    void printError(std::string description, RtMidiError &error) {
        char cstr[MAX_STR_SIZE];

        strncpy(cstr, description.c_str(), MAX_STR_SIZE);
        cstr[MAX_STR_SIZE - 1] = NULL;
        object_error((t_object *)this, cstr);
        
        std::string msg = error.getMessage();
        strncpy(cstr, msg.c_str(), MAX_STR_SIZE);
        cstr[MAX_STR_SIZE - 1] = NULL;
        object_error((t_object *)this, cstr);
    }
};


void midiInputCallback(double deltatime, midimessage *message, void *userData) {
    ((MIDI4L*)userData)->receive(message);
}



C74_EXPORT int main(void) {
	MIDI4L::makeMaxClass("midi4l");
    
    REGISTER_METHOD_ASSIST(MIDI4L, assist);
    
    REGISTER_METHOD(MIDI4L, bang);
    REGISTER_METHOD_GIMME(MIDI4L, send);
    REGISTER_METHOD_GIMME(MIDI4L, input);
    REGISTER_METHOD_GIMME(MIDI4L, output);
}
